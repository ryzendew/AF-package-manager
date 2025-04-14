#include "core/packagemanager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <algorithm>
#include <cstdlib>  // For system()
#include <array>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <QSettings>

namespace pacmangui {
namespace core {

// Helper function to execute commands with sudo
bool execute_with_sudo(const std::string& command) {
    std::string sudo_cmd = "sudo " + command;
    std::cout << "Executing: " << sudo_cmd << std::endl;
    int result = system(sudo_cmd.c_str());
    return result == 0;
}

// Helper function to execute commands with sudo and password
bool execute_with_sudo(const std::string& command, const std::string& password) {
    // Use expect-like behavior to handle sudo password prompt more reliably
    std::string quoted_password = "'";
    for (char c : password) {
        if (c == '\'') {
            quoted_password += "'\\''"; // Escape single quotes in the password
        } else {
            quoted_password += c;
        }
    }
    quoted_password += "'";
    
    // Create a command that:
    // 1. Uses 'script' to simulate a terminal (sudo sometimes requires a TTY)
    // 2. Pipes the password directly to sudo
    // 3. Ensures the password isn't visible in process listings
    std::string full_cmd = "script -qec 'echo " + quoted_password + " | sudo -S " + command + "' /dev/null";
    
    std::cout << "Executing sudo command with password authentication" << std::endl;
    
    // Execute the command
    int result = system(full_cmd.c_str());
    return result == 0;
}

// ALPM error callback - updated to match alpm_cb_log signature
void alpm_log_cb(void* ctx, alpm_loglevel_t level, const char *format, va_list args)
{
    if (level == ALPM_LOG_ERROR || level == ALPM_LOG_WARNING) {
        char message[1024];
        vsnprintf(message, sizeof(message), format, args);
        std::cerr << "ALPM: " << message << std::endl;
    }
}

PackageManager::PackageManager()
    : m_handle(nullptr)
    , m_repo_manager(nullptr)
    , m_trans_manager(nullptr)
    , m_last_error("")
{
}

PackageManager::~PackageManager()
{
    if (m_trans_manager) {
        delete m_trans_manager;
        m_trans_manager = nullptr;
    }
    
    if (m_repo_manager) {
        delete m_repo_manager;
        m_repo_manager = nullptr;
    }
    
    if (m_handle) {
        alpm_release(m_handle);
        m_handle = nullptr;
    }
}

bool PackageManager::initialize(const std::string& root_dir, const std::string& db_path)
{
    std::cout << "PackageManager: Initializing with root path '" << root_dir << "' and DB path '" << db_path << "'" << std::endl;
    
    // Initialize alpm library
    alpm_errno_t err;
    m_handle = alpm_initialize(root_dir.c_str(), db_path.c_str(), &err);
    
    if (!m_handle) {
        std::string error = "Failed to initialize alpm: ";
        error += alpm_strerror(err);
        set_last_error(error);
        return false;
    }
    
    // Set callback for logging
    alpm_option_set_logcb(m_handle, alpm_log_cb, nullptr);
    
    // Create repository manager
    m_repo_manager = new RepositoryManager(m_handle);
    if (!m_repo_manager->initialize()) {
        set_last_error("Failed to initialize repository manager");
        return false;
    }
    
    // Create transaction manager
    m_trans_manager = new TransactionManager(m_handle);
    
    // Register sync databases
    if (!register_sync_databases()) {
        set_last_error("Failed to register sync databases");
        return false;
    }
    
    // Initialize Flatpak manager
    m_flatpak_manager.initialize();
    
    std::cout << "PackageManager: Initialized successfully" << std::endl;
    return true;
}

std::vector<Package> PackageManager::get_installed_packages() const
{
    std::vector<Package> packages;
    
    if (!m_handle || !m_repo_manager) {
        return packages;
    }
    
    Repository local_db = m_repo_manager->get_local_db();
    packages = local_db.get_packages();
    
    return packages;
}

std::vector<Package> PackageManager::get_available_packages() const
{
    std::vector<Package> packages;
    
    if (!m_handle || !m_repo_manager) {
        return packages;
    }
    
    // Get packages from all sync repositories
    std::vector<Repository> sync_dbs = m_repo_manager->get_sync_dbs();
    for (const auto& repo : sync_dbs) {
        std::vector<Package> repo_packages = repo.get_packages();
        packages.insert(packages.end(), repo_packages.begin(), repo_packages.end());
    }
    
    return packages;
}

std::vector<Package> PackageManager::search_aur(const std::string& name) const
{
    std::vector<Package> results;
    
    if (name.empty()) {
        return results;
    }
    
    std::cout << "PackageManager: Searching for AUR packages matching '" << name << "'" << std::endl;
    
    // Check if AUR is enabled in settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    bool aurEnabled = settings.value("aur/enabled", false).toBool();
    
    if (!aurEnabled) {
        std::cout << "PackageManager: AUR search is disabled in settings" << std::endl;
        return results;
    }
    
    try {
        // Directly call the AUR RPC API using curl
        // This is inspired by Octopi's approach but simplified
        std::string tempFile = "/tmp/aur_search_results.json";
        std::string escapedSearch = name;
        
        // Escape special characters in the search term
        for (size_t i = 0; i < escapedSearch.length(); i++) {
            if (escapedSearch[i] == ' ') {
                escapedSearch.replace(i, 1, "%20");
            }
        }
        
        // Construct the curl command to query the AUR RPC API
        std::string command = "curl -s 'https://aur.archlinux.org/rpc/?v=5&type=search&arg=" + 
                             escapedSearch + "' -o " + tempFile;
        
        // Execute the curl command
        std::cout << "PackageManager: Executing AUR API query" << std::endl;
        int result = system(command.c_str());
        
        if (result != 0) {
            std::cerr << "PackageManager: Failed to execute curl command" << std::endl;
            return results;
        }
        
        // Parse the JSON response
        std::ifstream file(tempFile);
        if (!file.is_open()) {
            std::cerr << "PackageManager: Failed to open temporary file" << std::endl;
            return results;
        }
        
        std::string jsonContent((std::istreambuf_iterator<char>(file)), 
                               std::istreambuf_iterator<char>());
        file.close();
        
        // Very simple JSON parsing (in a real app, use a proper JSON library)
        if (jsonContent.find("\"resultcount\":0") != std::string::npos) {
            std::cout << "PackageManager: No AUR packages found matching '" << name << "'" << std::endl;
            return results;
        }
        
        size_t pos = 0;
        std::string packageNameKey = "\"Name\":\"";
        std::string versionKey = "\"Version\":\"";
        std::string descriptionKey = "\"Description\":\"";
        
        while ((pos = jsonContent.find(packageNameKey, pos)) != std::string::npos) {
            Package aurPackage;
            
            // Extract package name
            size_t nameStart = pos + packageNameKey.length();
            size_t nameEnd = jsonContent.find("\"", nameStart);
            if (nameEnd != std::string::npos) {
                std::string packageName = jsonContent.substr(nameStart, nameEnd - nameStart);
                aurPackage.set_name(packageName);
                aurPackage.set_repository("aur");
                
                // Extract version (search after the package name)
                size_t versionPos = jsonContent.find(versionKey, nameEnd);
                if (versionPos != std::string::npos && versionPos < jsonContent.find(packageNameKey, nameEnd + 1)) {
                    size_t versionStart = versionPos + versionKey.length();
                    size_t versionEnd = jsonContent.find("\"", versionStart);
                    if (versionEnd != std::string::npos) {
                        std::string version = jsonContent.substr(versionStart, versionEnd - versionStart);
                        aurPackage.set_version(version);
                    }
                }
                
                // Extract description (search after the package name)
                size_t descPos = jsonContent.find(descriptionKey, nameEnd);
                if (descPos != std::string::npos && descPos < jsonContent.find(packageNameKey, nameEnd + 1)) {
                    size_t descStart = descPos + descriptionKey.length();
                    size_t descEnd = jsonContent.find("\"", descStart);
                    if (descEnd != std::string::npos) {
                        std::string description = jsonContent.substr(descStart, descEnd - descStart);
                        aurPackage.set_description(description);
                    }
                }
                
                // Set AUR-specific information
                aurPackage.set_aur_info("AUR Package");
                
                // Add to results
                results.push_back(aurPackage);
            }
            
            // Move to the next package
            pos = nameEnd + 1;
        }
        
        // Clean up the temporary file
        remove(tempFile.c_str());
        
        std::cout << "PackageManager: Found " << results.size() << " AUR packages matching '" << name << "'" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "PackageManager: Exception during AUR search: " << e.what() << std::endl;
    }
    
    return results;
}

std::vector<Package> PackageManager::search_by_name(const std::string& name) const
{
    std::vector<Package> results;
    
    if (!m_handle || !m_repo_manager || name.empty()) {
        return results;
    }
    
    std::cout << "PackageManager: Searching for packages matching '" << name << "'" << std::endl;
    
    try {
        // First search in installed packages
        std::vector<Package> installed_packages = get_installed_packages();
        std::cout << "PackageManager: Searching through " << installed_packages.size() << " installed packages" << std::endl;
        
        // Convert search term to lowercase for case-insensitive matching
        std::string search_term = name;
        std::transform(search_term.begin(), search_term.end(), search_term.begin(), 
                    [](unsigned char c){ return std::tolower(c); });
        
        // Search in installed packages
        for (const auto& pkg : installed_packages) {
            std::string pkg_name = pkg.get_name();
            std::transform(pkg_name.begin(), pkg_name.end(), pkg_name.begin(), 
                        [](unsigned char c){ return std::tolower(c); });
            
            if (pkg_name.find(search_term) != std::string::npos) {
            results.push_back(pkg);
        }
        }
        
        std::cout << "PackageManager: Found " << results.size() << " matching installed packages" << std::endl;
        
        // Then search in repository packages
        std::vector<Repository> sync_dbs = m_repo_manager->get_sync_dbs();
        std::cout << "PackageManager: Searching through " << sync_dbs.size() << " repositories" << std::endl;
        
        int total_repo_packages = 0;
        std::vector<Package> repo_results;
        
        // Search each repository explicitly
        for (const auto& repo : sync_dbs) {
            try {
                std::vector<Package> repo_packages = repo.get_packages();
                total_repo_packages += repo_packages.size();
                
                for (const auto& pkg : repo_packages) {
                    if (pkg.get_name().empty()) {
                        continue; // Skip packages with invalid names
                    }
                    
                    std::string pkg_name = pkg.get_name();
                    std::transform(pkg_name.begin(), pkg_name.end(), pkg_name.begin(), 
                                [](unsigned char c){ return std::tolower(c); });
                    
                    // Check both name and description for matches
                    if (pkg_name.find(search_term) != std::string::npos) {
                        // Check if this package is already in results (avoid duplicates)
                        bool already_added = false;
                        for (const auto& existing : results) {
                            if (existing.get_name() == pkg.get_name()) {
                                already_added = true;
                                break;
                            }
                        }
                        
                        if (!already_added) {
                            repo_results.push_back(pkg);
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "PackageManager: Error searching repo " << repo.get_name() 
                        << ": " << e.what() << std::endl;
            }
        }
        
        std::cout << "PackageManager: Searched through " << total_repo_packages << " repository packages" << std::endl;
        std::cout << "PackageManager: Found " << repo_results.size() << " matching repository packages" << std::endl;
        
        // Add repository results to the final results
        results.insert(results.end(), repo_results.begin(), repo_results.end());
        
        std::cout << "PackageManager: Total of " << results.size() << " matching packages found" << std::endl;
        
        // Check if AUR is enabled and search AUR packages
        QSettings settings("PacmanGUI", "PacmanGUI");
        bool aurEnabled = settings.value("aur/enabled", false).toBool();
        
        if (aurEnabled) {
            std::cout << "PackageManager: AUR search is enabled, searching AUR packages" << std::endl;
            
            // Search AUR packages
            std::vector<Package> aur_results = search_aur(name);
            
            // Filter out duplicates (packages already in results)
            std::vector<Package> filtered_aur_results;
            for (const auto& pkg : aur_results) {
                bool already_added = false;
                for (const auto& existing : results) {
                    if (existing.get_name() == pkg.get_name()) {
                        already_added = true;
                        break;
                    }
                }
                
                if (!already_added) {
                    filtered_aur_results.push_back(pkg);
                }
            }
            
            std::cout << "PackageManager: Found " << filtered_aur_results.size() << " unique AUR packages" << std::endl;
            
            // Add AUR results to the final results
            results.insert(results.end(), filtered_aur_results.begin(), filtered_aur_results.end());
            
            std::cout << "PackageManager: Total of " << results.size() << " matching packages found (including AUR)" << std::endl;
        } else {
            std::cout << "PackageManager: AUR search is disabled" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "PackageManager: Exception during search: " << e.what() << std::endl;
    }
    
    return results;
}

Package PackageManager::get_package_details(const std::string& name) const
{
    if (!m_handle || !m_repo_manager || name.empty()) {
        return Package();
    }
    
    return m_repo_manager->find_package(name);
}

bool PackageManager::install_package(const std::string& package_name)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Installing package: " << package_name << std::endl;
    
    // Use pacman directly with sudo
    std::string command = "pacman -S --noconfirm " + package_name;
    bool success = execute_with_sudo(command);
    
    if (success) {
        std::cout << "PackageManager: Package installed successfully: " << package_name << std::endl;
        return true;
    } else {
        set_last_error("Failed to install package: " + package_name);
        return false;
    }
}

bool PackageManager::install_package(const std::string& package_name, const std::string& password, bool use_overwrite)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Installing package with authentication: " << package_name << std::endl;
    
    // Use pacman directly with sudo and password
    std::string command = "pacman -S --noconfirm ";
    
    // Add overwrite option if requested
    if (use_overwrite) {
        command += "--overwrite \"*\" ";
    }
    
    command += package_name;
    bool success = execute_with_sudo(command, password);
    
    if (success) {
        std::cout << "PackageManager: Package installed successfully: " << package_name << std::endl;
        return true;
    } else {
        set_last_error("Failed to install package: " + package_name + ". Authentication may have failed.");
        return false;
    }
}

bool PackageManager::remove_package(const std::string& package_name)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Removing package: " << package_name << std::endl;
    
    // Use pacman directly with sudo
    std::string command = "pacman -R --noconfirm " + package_name;
    bool success = execute_with_sudo(command);
    
    if (success) {
        std::cout << "PackageManager: Package removed successfully: " << package_name << std::endl;
        return true;
    } else {
        set_last_error("Failed to remove package: " + package_name);
        return false;
    }
}

bool PackageManager::remove_package(const std::string& package_name, const std::string& password)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Removing package with authentication: " << package_name << std::endl;
    
    // Use pacman directly with sudo and password
    std::string command = "pacman -R --noconfirm " + package_name;
    bool success = execute_with_sudo(command, password);
    
    if (success) {
        std::cout << "PackageManager: Package removed successfully: " << package_name << std::endl;
    return true;
    } else {
        set_last_error("Failed to remove package: " + package_name + ". Authentication may have failed.");
        return false;
    }
}

bool PackageManager::update_package(const std::string& package_name)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Updating package: " << package_name << std::endl;
    
    // Use pacman directly with sudo
    std::string command = "pacman -S --noconfirm " + package_name;
    bool success = execute_with_sudo(command);
    
    if (success) {
        std::cout << "PackageManager: Package updated successfully: " << package_name << std::endl;
        return true;
    } else {
        set_last_error("Failed to update package: " + package_name);
        return false;
    }
}

bool PackageManager::update_package(const std::string& package_name, const std::string& password, bool use_overwrite)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Updating package with authentication: " << package_name << std::endl;
    
    // Use pacman directly with sudo and password
    std::string command = "pacman -S --noconfirm ";
    
    // Add overwrite option if requested
    if (use_overwrite) {
        command += "--overwrite \"*\" ";
    }
    
    command += package_name;
    bool success = execute_with_sudo(command, password);
    
    if (success) {
        std::cout << "PackageManager: Package updated successfully: " << package_name << std::endl;
        return true;
    } else {
        set_last_error("Failed to update package: " + package_name + ". Authentication may have failed.");
        return false;
    }
}

bool PackageManager::sync_all()
{
    std::cout << "PackageManager: Synchronizing all packages" << std::endl;
    
    // First refresh the package databases
    std::string refresh_cmd = "pacman -Sy";
    bool refresh_success = execute_with_sudo(refresh_cmd);
    
    if (!refresh_success) {
        set_last_error("Failed to refresh package databases");
        return false;
    }
    
    std::cout << "PackageManager: Package databases refreshed successfully" << std::endl;
    
    // Re-initialize to get updated package information
    m_repo_manager->initialize();
    
    return true;
}

bool PackageManager::sync_all(const std::string& password)
{
    std::cout << "PackageManager: Synchronizing all packages with authentication" << std::endl;
    
    // First refresh the package databases
    std::string refresh_cmd = "pacman -Sy";
    bool refresh_success = execute_with_sudo(refresh_cmd, password);
    
    if (!refresh_success) {
        set_last_error("Failed to refresh package databases. Authentication may have failed.");
        return false;
    }
    
    std::cout << "PackageManager: Package databases refreshed successfully" << std::endl;
    
    // Re-initialize to get updated package information
    m_repo_manager->initialize();
    
    return true;
}

bool PackageManager::install_aur_package(const std::string& package_name)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    // Check if AUR is enabled in settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    bool aurEnabled = settings.value("aur/enabled", false).toBool();
    
    if (!aurEnabled) {
        set_last_error("AUR support is disabled in settings");
        return false;
    }
    
    // Get configured AUR helper
    std::string aurHelper = settings.value("aur/helper", "yay").toString().toStdString();
    if (aurHelper.empty()) {
        set_last_error("No AUR helper configured");
        return false;
    }
    
    std::cout << "PackageManager: Installing AUR package: " << package_name << " using " << aurHelper << std::endl;
    
    // Execute the AUR helper command
    std::string command = aurHelper + " -S --noconfirm " + package_name;
    int result = system(command.c_str());
    
    if (result == 0) {
        std::cout << "PackageManager: AUR Package installed successfully: " << package_name << std::endl;
    return true;
    } else {
        set_last_error("Failed to install AUR package: " + package_name);
        return false;
    }
}

bool PackageManager::install_aur_package(const std::string& package_name, const std::string& password,
                                        const std::string& aur_helper)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    // Check if AUR is enabled in settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    bool aurEnabled = settings.value("aur/enabled", false).toBool();
    
    if (!aurEnabled) {
        set_last_error("AUR support is disabled in settings");
        return false;
    }
    
    // Use provided AUR helper or fall back to configured one
    std::string aurHelper = aur_helper;
    if (aurHelper.empty()) {
        aurHelper = settings.value("aur/helper", "yay").toString().toStdString();
    }
    
    if (aurHelper.empty()) {
        set_last_error("No AUR helper configured");
        return false;
    }
    
    std::cout << "PackageManager: Installing AUR package with authentication: " 
             << package_name << " using " << aurHelper << std::endl;
    
    // Execute the AUR helper command with sudo
    std::string command = aurHelper + " -S --noconfirm " + package_name;
    bool success = execute_with_sudo(command, password);
    
    if (success) {
        std::cout << "PackageManager: AUR Package installed successfully: " << package_name << std::endl;
    return true;
    } else {
        set_last_error("Failed to install AUR package: " + package_name + ". Authentication may have failed.");
        return false;
    }
}

bool PackageManager::is_package_installed(const std::string& package_name) const
{
    if (!m_handle || !m_repo_manager || package_name.empty()) {
        return false;
    }
    
    Repository local_db = m_repo_manager->get_local_db();
    Package pkg = local_db.find_package(package_name);
    
    return !pkg.get_name().empty();
}

std::vector<Repository> PackageManager::get_repositories() const
{
    std::vector<Repository> repositories;
    
    if (!m_handle || !m_repo_manager) {
        return repositories;
    }
    
    // Add local repository
    repositories.push_back(m_repo_manager->get_local_db());
    
    // Add sync repositories
    std::vector<Repository> sync_dbs = m_repo_manager->get_sync_dbs();
    repositories.insert(repositories.end(), sync_dbs.begin(), sync_dbs.end());
    
    return repositories;
}

std::string PackageManager::get_last_error() const
{
    return m_last_error;
}

void PackageManager::set_last_error(const std::string& error)
{
    m_last_error = error;
    std::cerr << "PackageManager: " << error << std::endl;
}

bool PackageManager::register_sync_databases()
{
    if (!m_handle) {
        return false;
    }
    
    // Path to pacman configuration file
    std::string config_path = "/etc/pacman.conf";
    
    // Open the configuration file
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        std::cerr << "Failed to open pacman configuration file: " << config_path << std::endl;
        return false;
    }
    
    std::string line;
    std::string current_repo;
    int repo_count = 0;
    std::vector<std::string> registered_repos;
    
    std::cout << "PackageManager: Reading repositories from " << config_path << std::endl;
    
    // Parse the configuration file
    while (std::getline(config_file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Check for repository name
        if (line[0] == '[' && line[line.size() - 1] == ']') {
            current_repo = line.substr(1, line.size() - 2);
            
            // Skip local repositories
            if (current_repo == "options" || current_repo == "local") {
                current_repo = "";
                continue;
            }
            
            std::cout << "PackageManager: Found repository: " << current_repo << std::endl;
            
            // Register the repository
            alpm_db_t* db = alpm_register_syncdb(m_handle, current_repo.c_str(), ALPM_SIG_USE_DEFAULT);
            if (!db) {
                alpm_errno_t err = alpm_errno(m_handle);
                std::cerr << "Failed to register sync database: " << current_repo << ": " << alpm_strerror(err) << std::endl;
                continue; // Skip this repo but continue with others
            }
            
            // Add to our list of registered repos
            registered_repos.push_back(current_repo);
            repo_count++;
        }
    }
    
    config_file.close();
    
    // Force a refresh of repositories to ensure they have data
    if (repo_count > 0) {
        std::cout << "PackageManager: Successfully registered " << repo_count << " repositories:" << std::endl;
        for (const auto& repo : registered_repos) {
            std::cout << " - " << repo << std::endl;
        }
        
        // Reinitialize the repository manager to ensure it loads all the registered dbs
        try {
            if (!m_repo_manager->initialize()) {
                std::cerr << "PackageManager: Failed to reinitialize repository manager after registering databases" << std::endl;
                return false;
            }
            
            // Get the current sync databases
            std::vector<Repository> sync_dbs = m_repo_manager->get_sync_dbs();
            std::cout << "PackageManager: Loaded " << sync_dbs.size() << " sync databases" << std::endl;
            
            // Display package counts for each repo
            int total_packages = 0;
            for (const auto& repo : sync_dbs) {
                try {
                    std::vector<Package> repo_packages = repo.get_packages();
                    int package_count = repo_packages.size();
                    total_packages += package_count;
                    std::cout << "PackageManager: Repository '" << repo.get_name() << "' has " 
                            << package_count << " packages" << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "PackageManager: Error processing repo " << repo.get_name() 
                            << ": " << e.what() << std::endl;
                }
            }
            
            std::cout << "PackageManager: Total of " << total_packages << " packages available in repositories" << std::endl;
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "PackageManager: Exception during repository initialization: " << e.what() << std::endl;
            return false;
        }
    } else {
        std::cerr << "PackageManager: No sync databases registered successfully" << std::endl;
        return false;
    }
}

bool PackageManager::update_system(const std::string& password, std::function<void(const std::string&)> output_callback, bool use_overwrite)
{
    std::cout << "PackageManager: Performing full system update" << std::endl;
    
    if (output_callback) {
        output_callback("Starting system update...\n");
    }
    
    // Create a temporary file to capture output
    std::string temp_output_file = "/tmp/pacmangui_update_output.txt";
    
    // Run pacman -Syu with --noconfirm to avoid interactive prompts
    // Redirect output to the temporary file
    std::string command = "pacman -Syu --noconfirm ";
    
    // Add overwrite option if requested
    if (use_overwrite) {
        command += "--overwrite \"*\" ";
        
        if (output_callback) {
            output_callback("Using --overwrite=\"*\" option. This may overwrite conflicting files.\n");
        }
    }
    
    command += "| tee " + temp_output_file;
    
    // Start a background thread to monitor the output file and send updates
    std::atomic<bool> running(true);
    std::thread output_thread;
    
    if (output_callback) {
        output_thread = std::thread([temp_output_file, &output_callback, &running]() {
            std::ifstream output_file(temp_output_file);
            if (!output_file.is_open()) {
                return;
            }
            
            std::string line;
            while (running) {
                if (std::getline(output_file, line)) {
                    output_callback(line + "\n");
                } else {
                    // Wait a bit before checking for more output
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            
            // Read any remaining output
            while (std::getline(output_file, line)) {
                output_callback(line + "\n");
            }
            
            output_file.close();
        });
    }
    
    // Execute the command
    bool success = execute_with_sudo(command, password);
    
    // Stop the output thread and wait for it to finish
    if (output_callback) {
        running = false;
        if (output_thread.joinable()) {
            output_thread.join();
        }
    }
    
    // Clean up the temporary file
    std::remove(temp_output_file.c_str());
    
    if (success) {
        std::cout << "PackageManager: System update completed successfully" << std::endl;
        
        if (output_callback) {
            output_callback("System update completed successfully.\n");
        }
        
        // Re-initialize to get updated package information
        m_repo_manager->initialize();
        
        return true;
    } else {
        std::string error_message = "Failed to update system. Authentication may have failed.";
        set_last_error(error_message);
        
        if (output_callback) {
            output_callback("ERROR: " + error_message + "\n");
        }
        
        return false;
    }
}

// Overloaded method for backward compatibility
bool PackageManager::update_system(const std::string& password, bool use_overwrite)
{
    return update_system(password, nullptr, use_overwrite);
}

std::vector<std::pair<std::string, std::string>> PackageManager::check_updates() const
{
    std::vector<std::pair<std::string, std::string>> updates;
    
    std::cout << "PackageManager: Checking for available updates" << std::endl;
    
    // Run pacman -Qu to check for updates
    FILE* pipe = popen("pacman -Qu", "r");
    if (!pipe) {
        std::cerr << "PackageManager: Error running pacman -Qu" << std::endl;
        return updates;
    }
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        std::string line(buffer);
        
        // Parse output line (format: package_name old_version -> new_version)
        size_t first_space = line.find(' ');
        if (first_space != std::string::npos) {
            std::string package_name = line.substr(0, first_space);
            
            // Find the arrow separator
            size_t arrow_pos = line.find("->");
            if (arrow_pos != std::string::npos) {
                // Extract new version (skip spaces after arrow)
                size_t version_start = arrow_pos + 2;
                while (version_start < line.size() && isspace(line[version_start])) {
                    version_start++;
                }
                
                std::string new_version = line.substr(version_start);
                // Trim trailing whitespace/newline
                new_version.erase(new_version.find_last_not_of(" \n\r\t") + 1);
                
                updates.push_back(std::make_pair(package_name, new_version));
            }
        }
    }
    
    pclose(pipe);
    
    std::cout << "PackageManager: Found " << updates.size() << " available updates" << std::endl;
    return updates;
}

std::vector<std::pair<std::string, std::string>> PackageManager::check_aur_updates(const std::string& aur_helper) const
{
    std::vector<std::pair<std::string, std::string>> updates;
    
    // Check if AUR is enabled in settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    bool aurEnabled = settings.value("aur/enabled", false).toBool();
    
    if (!aurEnabled) {
        std::cout << "PackageManager: AUR support is disabled in settings" << std::endl;
        return updates;
    }
    
    // Get the AUR helper to use
    std::string helper = aur_helper;
    if (helper.empty()) {
        helper = settings.value("aur/helper", "yay").toString().toStdString();
    }
    
    if (helper.empty()) {
        std::cerr << "PackageManager: No AUR helper configured" << std::endl;
        return updates;
    }
    
    std::cout << "PackageManager: Checking for available AUR updates using " << helper << std::endl;
    
    // Different command formats based on helper
    std::string update_check_cmd;
    if (helper == "yay") {
        update_check_cmd = "yay -Qua";
    } else if (helper == "paru") {
        update_check_cmd = "paru -Qua";
    } else {
        // Generic format that should work with most helpers
        update_check_cmd = helper + " -Qua";
    }
    
    // Run the AUR helper to check for updates
    FILE* pipe = popen(update_check_cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "PackageManager: Error running AUR update check" << std::endl;
        return updates;
    }
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        std::string line(buffer);
        
        // Parse output line (format depends on the helper but usually: package_name old_version -> new_version)
        size_t first_space = line.find(' ');
        if (first_space != std::string::npos) {
            std::string package_name = line.substr(0, first_space);
            
            // Find the arrow separator (common in pacman-compatible output)
            size_t arrow_pos = line.find("->");
            if (arrow_pos != std::string::npos) {
                // Extract new version (skip spaces after arrow)
                size_t version_start = arrow_pos + 2;
                while (version_start < line.size() && isspace(line[version_start])) {
                    version_start++;
                }
                
                std::string new_version = line.substr(version_start);
                // Trim trailing whitespace/newline
                new_version.erase(new_version.find_last_not_of(" \n\r\t") + 1);
                
                updates.push_back(std::make_pair(package_name, new_version));
            }
        }
    }
    
    pclose(pipe);
    
    std::cout << "PackageManager: Found " << updates.size() << " available AUR updates" << std::endl;
    return updates;
}

bool PackageManager::update_aur_packages(const std::string& aur_helper)
{
    // Check if AUR is enabled in settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    bool aurEnabled = settings.value("aur/enabled", false).toBool();
    
    if (!aurEnabled) {
        set_last_error("AUR support is disabled in settings");
        return false;
    }
    
    // Get the AUR helper to use
    std::string helper = aur_helper;
    if (helper.empty()) {
        helper = settings.value("aur/helper", "yay").toString().toStdString();
    }
    
    if (helper.empty()) {
        set_last_error("No AUR helper configured");
        return false;
    }
    
    std::cout << "PackageManager: Updating AUR packages using " << helper << std::endl;
    
    // Different command formats based on helper
    std::string update_cmd;
    if (helper == "yay") {
        update_cmd = "yay -Sua --noconfirm";
    } else if (helper == "paru") {
        update_cmd = "paru -Sua --noconfirm";
    } else {
        // Generic format that should work with most helpers
        update_cmd = helper + " -Sua --noconfirm";
    }
    
    // Run the AUR helper to update packages
    int result = system(update_cmd.c_str());
    
    if (result == 0) {
        std::cout << "PackageManager: AUR packages updated successfully" << std::endl;
        return true;
    } else {
        set_last_error("Failed to update AUR packages using " + helper);
        return false;
    }
}

bool PackageManager::update_aur_packages(const std::string& password, 
                                       const std::string& aur_helper,
                                       std::function<void(const std::string&)> output_callback)
{
    // Check if AUR is enabled in settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    bool aurEnabled = settings.value("aur/enabled", false).toBool();
    
    if (!aurEnabled) {
        set_last_error("AUR support is disabled in settings");
        if (output_callback) {
            output_callback("ERROR: AUR support is disabled in settings\n");
        }
        return false;
    }
    
    // Get the AUR helper to use
    std::string helper = aur_helper;
    if (helper.empty()) {
        helper = settings.value("aur/helper", "yay").toString().toStdString();
    }
    
    if (helper.empty()) {
        set_last_error("No AUR helper configured");
        if (output_callback) {
            output_callback("ERROR: No AUR helper configured\n");
        }
        return false;
    }
    
    std::cout << "PackageManager: Updating AUR packages with authentication using " << helper << std::endl;
    if (output_callback) {
        output_callback("Updating AUR packages using " + helper + "...\n");
    }
    
    // Create a temporary file to capture output
    std::string temp_output_file = "/tmp/pacmangui_aur_update_output.txt";
    
    // Different command formats based on helper
    std::string update_cmd;
    if (helper == "yay") {
        update_cmd = "yay -Sua --noconfirm";
    } else if (helper == "paru") {
        update_cmd = "paru -Sua --noconfirm";
    } else {
        // Generic format that should work with most helpers
        update_cmd = helper + " -Sua --noconfirm";
    }
    
    // Add output redirection
    update_cmd += " | tee " + temp_output_file;
    
    // Start a background thread to monitor the output file and send updates
    std::atomic<bool> running(true);
    std::thread output_thread;
    
    if (output_callback) {
        output_thread = std::thread([temp_output_file, &output_callback, &running]() {
            std::ifstream output_file(temp_output_file);
            if (!output_file.is_open()) {
                return;
            }
            
            std::string line;
            while (running) {
                if (std::getline(output_file, line)) {
                    output_callback(line + "\n");
                } else {
                    // Wait a bit before checking for more output
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            
            // Read any remaining output
            while (std::getline(output_file, line)) {
                output_callback(line + "\n");
            }
            
            output_file.close();
        });
    }
    
    // Execute the command
    bool success = execute_with_sudo(update_cmd, password);
    
    // Stop the output thread and wait for it to finish
    if (output_callback && output_thread.joinable()) {
        running = false;
        output_thread.join();
    }
    
    // Clean up the temporary file
    std::remove(temp_output_file.c_str());
    
    if (success) {
        std::cout << "PackageManager: AUR packages updated successfully" << std::endl;
        if (output_callback) {
            output_callback("AUR packages updated successfully.\n");
        }
        return true;
    } else {
        std::string error_message = "Failed to update AUR packages using " + helper + ". Authentication may have failed.";
        set_last_error(error_message);
        if (output_callback) {
            output_callback("ERROR: " + error_message + "\n");
        }
        return false;
    }
}

bool PackageManager::execute_with_sudo(const std::string& command, const std::string& password)
{
    // Call the free function
    return ::pacmangui::core::execute_with_sudo(command, password);
}

bool PackageManager::execute_with_sudo(const std::string& command)
{
    // Call the free function
    return ::pacmangui::core::execute_with_sudo(command);
}

bool PackageManager::clear_package_cache(bool clean_all, const std::string& password, 
                                      std::function<void(const std::string&)> output_callback)
{
    std::cout << "PackageManager: Clearing package cache" << std::endl;
    if (output_callback) {
        output_callback("Starting package cache cleanup...\n");
    }
    
    // Create the command based on the clean_all flag
    std::string cmd = "pacman -S" + std::string(clean_all ? "cc" : "c") + " --noconfirm";
    
    // Create a temporary file to capture output
    std::string temp_output_file = "/tmp/pacmangui_cache_cleanup_output.txt";
    cmd += " | tee " + temp_output_file;
    
    // Start a background thread to monitor the output file and send updates
    std::atomic<bool> running(true);
    std::thread output_thread;
    
    if (output_callback) {
        output_thread = std::thread([temp_output_file, &output_callback, &running]() {
            std::ifstream output_file(temp_output_file);
            if (!output_file.is_open()) {
                return;
            }
            
            std::string line;
            while (running) {
                if (std::getline(output_file, line)) {
                    output_callback(line + "\n");
                } else {
                    // Wait a bit before checking for more output
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            
            // Read any remaining output
            while (std::getline(output_file, line)) {
                output_callback(line + "\n");
            }
            
            output_file.close();
        });
    }
    
    // Execute the command
    bool success = execute_with_sudo(cmd, password);
    
    // Stop the output thread and wait for it to finish
    if (output_callback && output_thread.joinable()) {
        running = false;
        output_thread.join();
    }
    
    // Clean up the temporary file
    std::remove(temp_output_file.c_str());
    
    if (success) {
        std::string msg = "Package cache cleanup " + std::string(clean_all ? "(all packages)" : "(unused packages)") + " completed successfully";
        std::cout << "PackageManager: " << msg << std::endl;
        if (output_callback) {
            output_callback(msg + "\n");
        }
        return true;
    } else {
        std::string error_message = "Failed to clean package cache. Authentication may have failed.";
        set_last_error(error_message);
        if (output_callback) {
            output_callback("ERROR: " + error_message + "\n");
        }
        return false;
    }
}

std::vector<std::string> PackageManager::get_orphaned_packages() const
{
    std::vector<std::string> orphaned_packages;
    
    std::cout << "PackageManager: Finding orphaned packages" << std::endl;
    
    // Use popen to capture the output of the command
    std::string cmd = "pacman -Qtdq";
    FILE* pipe = popen(cmd.c_str(), "r");
    
    if (!pipe) {
        std::cerr << "PackageManager: Error executing command: " << cmd << std::endl;
        return orphaned_packages;
    }
    
    // Read the output line by line
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        // Remove newline characters
        std::string line(buffer);
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        
        // Add the package name to the list
        if (!line.empty()) {
            orphaned_packages.push_back(line);
        }
    }
    
    pclose(pipe);
    
    std::cout << "PackageManager: Found " << orphaned_packages.size() << " orphaned packages" << std::endl;
    return orphaned_packages;
}

bool PackageManager::remove_orphaned_packages(const std::string& password,
                                           std::function<void(const std::string&)> output_callback)
{
    std::cout << "PackageManager: Removing orphaned packages" << std::endl;
    if (output_callback) {
        output_callback("Finding and removing orphaned packages...\n");
    }
    
    // First, get the list of orphaned packages
    std::vector<std::string> orphaned = get_orphaned_packages();
    
    if (orphaned.empty()) {
        std::string msg = "No orphaned packages found.";
        std::cout << "PackageManager: " << msg << std::endl;
        if (output_callback) {
            output_callback(msg + "\n");
        }
        return true;
    }
    
    if (output_callback) {
        output_callback("Found " + std::to_string(orphaned.size()) + " orphaned packages.\n");
    }
    
    // Create a temporary file to list orphaned packages
    std::string temp_file = "/tmp/pacmangui_orphaned_packages.txt";
    std::ofstream ofs(temp_file);
    if (!ofs.is_open()) {
        std::string error_message = "Failed to create temporary file for orphaned packages list";
        set_last_error(error_message);
        if (output_callback) {
            output_callback("ERROR: " + error_message + "\n");
        }
        return false;
    }
    
    for (const auto& pkg : orphaned) {
        ofs << pkg << std::endl;
    }
    ofs.close();
    
    // Create the command to remove orphaned packages
    std::string cmd = "cat " + temp_file + " | pacman -Rns - --noconfirm";
    
    // Create a temporary file to capture output
    std::string temp_output_file = "/tmp/pacmangui_remove_orphans_output.txt";
    cmd += " | tee " + temp_output_file;
    
    // Start a background thread to monitor the output file and send updates
    std::atomic<bool> running(true);
    std::thread output_thread;
    
    if (output_callback) {
        output_thread = std::thread([temp_output_file, &output_callback, &running]() {
            std::ifstream output_file(temp_output_file);
            if (!output_file.is_open()) {
                return;
            }
            
            std::string line;
            while (running) {
                if (std::getline(output_file, line)) {
                    output_callback(line + "\n");
                } else {
                    // Wait a bit before checking for more output
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            
            // Read any remaining output
            while (std::getline(output_file, line)) {
                output_callback(line + "\n");
            }
            
            output_file.close();
        });
    }
    
    // Execute the command
    bool success = execute_with_sudo(cmd, password);
    
    // Stop the output thread and wait for it to finish
    if (output_callback && output_thread.joinable()) {
        running = false;
        output_thread.join();
    }
    
    // Clean up temporary files
    std::remove(temp_file.c_str());
    std::remove(temp_output_file.c_str());
    
    if (success) {
        std::string msg = "Successfully removed " + std::to_string(orphaned.size()) + " orphaned packages";
        std::cout << "PackageManager: " << msg << std::endl;
        if (output_callback) {
            output_callback(msg + "\n");
        }
        return true;
    } else {
        std::string error_message = "Failed to remove orphaned packages. Authentication may have failed.";
        set_last_error(error_message);
        if (output_callback) {
            output_callback("ERROR: " + error_message + "\n");
        }
        return false;
    }
}

bool PackageManager::check_database(bool check_sync_dbs,
                                 std::function<void(const std::string&)> output_callback)
{
    std::cout << "PackageManager: Checking database for errors" << std::endl;
    if (output_callback) {
        output_callback("Checking pacman database for errors...\n");
    }
    
    // Create the command based on the check_sync_dbs flag
    std::string cmd = "pacman -D" + std::string(check_sync_dbs ? "kk" : "k");
    
    // Create a temporary file to capture output
    std::string temp_output_file = "/tmp/pacmangui_db_check_output.txt";
    
    // Use popen to execute the command and capture the output
    std::string full_cmd = cmd + " > " + temp_output_file + " 2>&1";
    int result = system(full_cmd.c_str());
    bool success = (result == 0);
    
    // Read the output file and send updates
    if (output_callback) {
        std::ifstream output_file(temp_output_file);
        if (output_file.is_open()) {
            std::string line;
            while (std::getline(output_file, line)) {
                output_callback(line + "\n");
            }
            output_file.close();
        }
    }
    
    // Clean up the temporary file
    std::remove(temp_output_file.c_str());
    
    if (success) {
        std::string msg = "Database check " + std::string(check_sync_dbs ? "(including sync databases)" : "") + " completed without errors";
        std::cout << "PackageManager: " << msg << std::endl;
        if (output_callback) {
            output_callback(msg + "\n");
        }
        return true;
    } else {
        std::string error_message = "Database check found errors. Please check the output for details.";
        set_last_error(error_message);
        if (output_callback) {
            output_callback("ERROR: " + error_message + "\n");
        }
        return false;
    }
}

std::vector<std::string> PackageManager::find_pacnew_files() const
{
    std::vector<std::string> pacnew_files;
    
    std::cout << "PackageManager: Finding .pacnew and .pacsave files" << std::endl;
    
    // Check if pacdiff is available
    FILE* which_pipe = popen("which pacdiff 2>/dev/null", "r");
    if (!which_pipe || pclose(which_pipe) != 0) {
        std::cerr << "PackageManager: pacdiff not found, using find command instead" << std::endl;
        
        // If pacdiff is not available, use find directly
        std::string cmd = "find /etc -name \"*.pacnew\" -o -name \"*.pacsave\" 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        
        if (!pipe) {
            std::cerr << "PackageManager: Error executing command: " << cmd << std::endl;
            return pacnew_files;
        }
        
        // Read the output line by line
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            // Remove newline characters
            std::string line(buffer);
            line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
            
            // Add the file path to the list
            if (!line.empty()) {
                pacnew_files.push_back(line);
            }
        }
        
        pclose(pipe);
    } else {
        // If pacdiff is available, use it
        std::string cmd = "pacdiff -o";
        FILE* pipe = popen(cmd.c_str(), "r");
        
        if (!pipe) {
            std::cerr << "PackageManager: Error executing command: " << cmd << std::endl;
            return pacnew_files;
        }
        
        // Read the output line by line
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            // Remove newline characters
            std::string line(buffer);
            line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
            
            // Process lines to extract file paths
            if (line.find("pacnew") != std::string::npos || line.find("pacsave") != std::string::npos) {
                // Parse the file path from the pacdiff output
                size_t pos = line.find('/');
                if (pos != std::string::npos) {
                    std::string file_path = line.substr(pos);
                    
                    // Extract path until space or end of line
                    size_t end_pos = file_path.find(' ');
                    if (end_pos != std::string::npos) {
                        file_path = file_path.substr(0, end_pos);
                    }
                    
                    if (!file_path.empty()) {
                        pacnew_files.push_back(file_path);
                    }
                }
            }
        }
        
        pclose(pipe);
    }
    
    std::cout << "PackageManager: Found " << pacnew_files.size() << " .pacnew/.pacsave files" << std::endl;
    return pacnew_files;
}

bool PackageManager::backup_database(const std::string& backup_path,
                                  std::function<void(const std::string&)> output_callback)
{
    std::cout << "PackageManager: Backing up pacman database to " << backup_path << std::endl;
    if (output_callback) {
        output_callback("Starting pacman database backup...\n");
    }
    
    // Create the backup command
    std::string cmd = "tar -czf " + backup_path + " /var/lib/pacman/local";
    
    // Create a temporary file to capture output
    std::string temp_output_file = "/tmp/pacmangui_backup_output.txt";
    cmd += " 2>&1 | tee " + temp_output_file;
    
    // Execute the command
    int result = system(cmd.c_str());
    bool success = (result == 0);
    
    // Read the output file and send updates
    if (output_callback) {
        std::ifstream output_file(temp_output_file);
        if (output_file.is_open()) {
            std::string line;
            while (std::getline(output_file, line)) {
                output_callback(line + "\n");
            }
            output_file.close();
        }
    }
    
    // Clean up the temporary file
    std::remove(temp_output_file.c_str());
    
    if (success) {
        std::string msg = "Pacman database backup completed successfully to " + backup_path;
        std::cout << "PackageManager: " << msg << std::endl;
        if (output_callback) {
            output_callback(msg + "\n");
        }
        return true;
    } else {
        std::string error_message = "Failed to backup pacman database";
        set_last_error(error_message);
        if (output_callback) {
            output_callback("ERROR: " + error_message + "\n");
        }
        return false;
    }
}

bool PackageManager::restore_database(const std::string& backup_path, const std::string& password,
                                   std::function<void(const std::string&)> output_callback)
{
    std::cout << "PackageManager: Restoring pacman database from " << backup_path << std::endl;
    if (output_callback) {
        output_callback("Starting pacman database restore from backup...\n");
    }
    
    // Check if the backup file exists
    std::ifstream backup_file(backup_path);
    if (!backup_file.good()) {
        std::string error_message = "Backup file does not exist or is not accessible: " + backup_path;
        set_last_error(error_message);
        if (output_callback) {
            output_callback("ERROR: " + error_message + "\n");
        }
        return false;
    }
    backup_file.close();
    
    // Create a temporary directory
    std::string temp_dir = "/tmp/pacmangui_restore_temp";
    std::string mkdir_cmd = "mkdir -p " + temp_dir;
    system(mkdir_cmd.c_str());
    
    // Command to restore the database
    std::string cmd = "rm -rf " + temp_dir + "/* && "
                     "tar -xzf " + backup_path + " -C " + temp_dir + " && "
                     "rm -rf /var/lib/pacman/local && "
                     "cp -a " + temp_dir + "/var/lib/pacman/local /var/lib/pacman/ && "
                     "rm -rf " + temp_dir;
    
    // Create a temporary file to capture output
    std::string temp_output_file = "/tmp/pacmangui_restore_output.txt";
    cmd += " 2>&1 | tee " + temp_output_file;
    
    // Start a background thread to monitor the output file and send updates
    std::atomic<bool> running(true);
    std::thread output_thread;
    
    if (output_callback) {
        output_thread = std::thread([temp_output_file, &output_callback, &running]() {
            std::ifstream output_file(temp_output_file);
            if (!output_file.is_open()) {
                return;
            }
            
            std::string line;
            while (running) {
                if (std::getline(output_file, line)) {
                    output_callback(line + "\n");
                } else {
                    // Wait a bit before checking for more output
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            
            // Read any remaining output
            while (std::getline(output_file, line)) {
                output_callback(line + "\n");
            }
            
            output_file.close();
        });
    }
    
    // Execute the command
    bool success = execute_with_sudo(cmd, password);
    
    // Stop the output thread and wait for it to finish
    if (output_callback && output_thread.joinable()) {
        running = false;
        output_thread.join();
    }
    
    // Clean up temporary files
    std::remove(temp_output_file.c_str());
    
    if (success) {
        std::string msg = "Pacman database restored successfully from " + backup_path;
        std::cout << "PackageManager: " << msg << std::endl;
        if (output_callback) {
            output_callback(msg + "\n");
        }
        return true;
    } else {
        std::string error_message = "Failed to restore pacman database. Authentication may have failed.";
        set_last_error(error_message);
        if (output_callback) {
            output_callback("ERROR: " + error_message + "\n");
        }
        return false;
    }
}

std::vector<FlatpakPackage> PackageManager::get_installed_flatpak_packages() const
{
    return m_flatpak_manager.get_installed_packages();
}

std::vector<FlatpakPackage> PackageManager::search_flatpak_by_name(const std::string& name) const
{
    return m_flatpak_manager.search_by_name(name);
}

bool PackageManager::install_flatpak_package(const std::string& app_id, const std::string& remote)
{
    return m_flatpak_manager.install_package(app_id, remote);
}

bool PackageManager::remove_flatpak_package(const std::string& app_id)
{
    return m_flatpak_manager.remove_package(app_id);
}

bool PackageManager::update_flatpak_package(const std::string& app_id)
{
    return m_flatpak_manager.update_package(app_id);
}

bool PackageManager::update_all_flatpak_packages()
{
    return m_flatpak_manager.update_all();
}

bool PackageManager::is_flatpak_available() const
{
    return m_flatpak_manager.is_available();
}

std::vector<std::string> PackageManager::get_flatpak_remotes() const
{
    return m_flatpak_manager.get_remotes();
}

} // namespace core
} // namespace pacmangui 