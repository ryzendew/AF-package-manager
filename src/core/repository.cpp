#include "repository.hpp"
#include <iostream>

namespace pacmangui {
namespace core {

Repository::Repository(const std::string& name)
    : m_name(name)
    , m_is_sync(false)
    , m_db(nullptr)
{
}

Repository::~Repository()
{
    m_db = nullptr; // We don't own this pointer
}

Repository Repository::create_from_alpm(alpm_db_t* db)
{
    if (!db) {
        return Repository("unknown");
    }
    
    const char* db_name = alpm_db_get_name(db);
    if (!db_name) {
        return Repository("unnamed");
    }
    
    Repository result(db_name);
    result.m_db = db;
    
    // Check if this is a sync db
    int usage = 0;
    alpm_db_get_usage(db, &usage);
    result.m_is_sync = (usage & ALPM_DB_USAGE_SYNC);
    
    return result;
}

std::string Repository::get_name() const
{
    return m_name;
}

bool Repository::is_sync() const
{
    return m_is_sync;
}

alpm_db_t* Repository::get_alpm_db() const
{
    return m_db;
}

std::vector<pacmangui::core::Package> Repository::get_packages() const
{
    std::vector<pacmangui::core::Package> packages;
    
    if (!m_db) {
        std::cerr << "Repository: No database available for " << m_name << std::endl;
        return packages;
    }
    
    // Get the package cache from the database
    alpm_list_t* pkg_list = alpm_db_get_pkgcache(m_db);
    if (!pkg_list) {
        std::cerr << "Repository: No package cache available for " << m_name << std::endl;
        return packages;
    }
    
    // Count packages for logging
    int count = 0;
    
    // Convert ALPM packages to our Package objects
    for (alpm_list_t* i = pkg_list; i; i = alpm_list_next(i)) {
        alpm_pkg_t* pkg = static_cast<alpm_pkg_t*>(i->data);
        if (!pkg) {
            continue;
        }
        
        const char* name = alpm_pkg_get_name(pkg);
        const char* version = alpm_pkg_get_version(pkg);
        const char* desc = alpm_pkg_get_desc(pkg);
        
        if (!name || !version) {
            std::cerr << "Repository: Invalid package data in " << m_name << std::endl;
            continue;
        }
        
        pacmangui::core::Package package;
        package.set_name(name);
        package.set_version(version);
        package.set_repository(m_name);
        
        // Description is optional
        if (desc) {
            package.set_description(desc);
        } else {
            package.set_description("No description available");
        }
        
        // Set installed flag based on whether this is the local db
        if (!m_is_sync) {
            package.set_installed(true);
        }
        
        packages.push_back(package);
        count++;
    }
    
    // Log how many packages were found for debugging
    if (count == 0 && m_is_sync) {
        std::cerr << "Repository: Warning - No packages found in sync repository " << m_name << std::endl;
    }
    
    return packages;
}

pacmangui::core::Package Repository::find_package(const std::string& name) const
{
    pacmangui::core::Package result;
    if (!m_db || name.empty()) {
        return result;
    }
    
    alpm_pkg_t* pkg = alpm_db_get_pkg(m_db, name.c_str());
    if (!pkg) {
        return result;
    }
    
    const char* pkg_name = alpm_pkg_get_name(pkg);
    const char* pkg_version = alpm_pkg_get_version(pkg);
    const char* pkg_desc = alpm_pkg_get_desc(pkg);
    
    if (!pkg_name || !pkg_version) {
        return result;
    }
    
    result.set_name(pkg_name);
    result.set_version(pkg_version);
    result.set_repository(m_name);
    
    // Description is optional
    if (pkg_desc) {
        result.set_description(pkg_desc);
    } else {
        result.set_description("No description available");
    }
    
    return result;
}

void Repository::set_alpm_db(alpm_db_t* db)
{
    if (db) {
        m_db = db;
        m_name = alpm_db_get_name(db);
        
        // Check if this is a sync db
        int usage = 0;
        alpm_db_get_usage(db, &usage);
        m_is_sync = (usage & ALPM_DB_USAGE_SYNC);
    }
}

// RepositoryManager implementation

RepositoryManager::RepositoryManager(alpm_handle_t* handle)
    : m_handle(handle)
    , m_local_db("local")
    , m_sync_dbs()
{
}

RepositoryManager::~RepositoryManager()
{
    // We don't own the alpm handle, so don't free it here
    m_handle = nullptr;
}

bool RepositoryManager::initialize()
{
    if (!m_handle) {
        std::cerr << "RepositoryManager: No ALPM handle provided" << std::endl;
        return false;
    }
    
    // Set local database
    alpm_db_t* local_db = alpm_get_localdb(m_handle);
    if (!local_db) {
        std::cerr << "RepositoryManager: Failed to get local database" << std::endl;
        return false;
    }
    
    m_local_db = Repository::create_from_alpm(local_db);
    std::cout << "RepositoryManager: Loaded local database with " 
              << m_local_db.get_packages().size() << " packages" << std::endl;
    
    // Get sync databases
    alpm_list_t* sync_dbs = alpm_get_syncdbs(m_handle);
    if (!sync_dbs) {
        std::cerr << "RepositoryManager: No sync databases found" << std::endl;
        return true; // Still return true since we have the local db
    }
    
    int repo_count = 0;
    for (alpm_list_t* item = sync_dbs; item; item = alpm_list_next(item)) {
        alpm_db_t* db = static_cast<alpm_db_t*>(item->data);
        if (db) {
            Repository repo = Repository::create_from_alpm(db);
            m_sync_dbs.push_back(repo);
            repo_count++;
            
            // Display repository info
            std::cout << "RepositoryManager: Loaded " << repo.get_name() 
                     << " repository with " << repo.get_packages().size() 
                     << " packages" << std::endl;
        }
    }
    
    std::cout << "RepositoryManager: Successfully initialized with " 
              << repo_count << " sync repositories" << std::endl;
    
    return true;
}

Repository RepositoryManager::get_local_db() const
{
    return m_local_db;
}

std::vector<Repository> RepositoryManager::get_sync_dbs() const
{
    return m_sync_dbs;
}

pacmangui::core::Package RepositoryManager::find_package(const std::string& name) const
{
    // First check in local database
    pacmangui::core::Package pkg = m_local_db.find_package(name);
    if (!pkg.get_name().empty()) {
        return pkg;
    }
    
    // Then check in sync databases
    for (const auto& repo : m_sync_dbs) {
        pkg = repo.find_package(name);
        if (!pkg.get_name().empty()) {
            return pkg;
        }
    }
    
    return pacmangui::core::Package();
}

std::vector<pacmangui::core::Package> RepositoryManager::get_all_packages() const
{
    std::vector<pacmangui::core::Package> all_packages;
    
    // Get packages from local database
    std::vector<pacmangui::core::Package> local_packages = m_local_db.get_packages();
    all_packages.insert(all_packages.end(), local_packages.begin(), local_packages.end());
    
    // Get packages from sync databases
    for (const auto& repo : m_sync_dbs) {
        std::vector<pacmangui::core::Package> repo_packages = repo.get_packages();
        all_packages.insert(all_packages.end(), repo_packages.begin(), repo_packages.end());
    }
    
    return all_packages;
}

} // namespace core
} // namespace pacmangui 