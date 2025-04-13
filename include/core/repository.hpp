#pragma once

#include <string>
#include <vector>
#include <memory>
#include <alpm.h>
#include "core/package.hpp"

namespace pacmangui {
namespace core {

/**
 * @brief Class representing a pacman repository
 */
class Repository {
public:
    /**
     * @brief Constructor with repository name
     * 
     * @param name Repository name
     */
    explicit Repository(const std::string& name);
    
    /**
     * @brief Destructor
     */
    ~Repository();
    
    /**
     * @brief Create repository from alpm database
     * 
     * @param db The alpm database
     * @return Repository A new repository object
     */
    static Repository create_from_alpm(alpm_db_t* db);
    
    /**
     * @brief Get repository name
     * 
     * @return std::string The repository name
     */
    std::string get_name() const;
    
    /**
     * @brief Check if repository is sync database
     * 
     * @return bool True if sync database
     */
    bool is_sync() const;
    
    /**
     * @brief Get all packages in the repository
     * 
     * @return std::vector<Package> List of packages
     */
    std::vector<Package> get_packages() const;
    
    /**
     * @brief Find a package by name
     * 
     * @param name Package name to find
     * @return Package The found package or empty if not found
     */
    Package find_package(const std::string& name) const;
    
    /**
     * @brief Set the internal alpm database pointer
     * 
     * @param db The alpm database
     */
    void set_alpm_db(alpm_db_t* db);
    
    /**
     * @brief Get the internal alpm database pointer
     * 
     * @return alpm_db_t* The alpm database
     */
    alpm_db_t* get_alpm_db() const;

private:
    std::string m_name;   ///< Repository name
    bool m_is_sync;       ///< Whether this is a sync database
    alpm_db_t* m_db;      ///< Pointer to the alpm database
};

/**
 * @brief Manager class for repositories
 */
class RepositoryManager {
public:
    /**
     * @brief Constructor with alpm handle
     * 
     * @param handle The alpm handle
     */
    explicit RepositoryManager(alpm_handle_t* handle);
    
    /**
     * @brief Destructor
     */
    ~RepositoryManager();
    
    /**
     * @brief Initialize the repository manager
     * 
     * @return bool True if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Get local database repository
     * 
     * @return Repository The local database
     */
    Repository get_local_db() const;
    
    /**
     * @brief Get all sync repositories
     * 
     * @return std::vector<Repository> List of sync repositories
     */
    std::vector<Repository> get_sync_dbs() const;
    
    /**
     * @brief Find a package across all repositories
     * 
     * @param name Package name to find
     * @return Package The found package or empty if not found
     */
    Package find_package(const std::string& name) const;
    
    /**
     * @brief Get all packages from all repositories
     * 
     * @return std::vector<Package> List of all packages
     */
    std::vector<Package> get_all_packages() const;

private:
    alpm_handle_t* m_handle;            ///< The alpm handle
    Repository m_local_db;              ///< The local database
    std::vector<Repository> m_sync_dbs; ///< List of sync databases
};

} // namespace core
} // namespace pacmangui 