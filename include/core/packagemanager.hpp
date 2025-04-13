#pragma once

#include <string>
#include <vector>
#include <memory>
#include <alpm.h>
#include "core/package.hpp"
#include "core/repository.hpp"
#include "core/transaction.hpp"
#include <functional>

namespace pacmangui {
namespace core {

/**
 * @brief Main class for managing packages
 */
class PackageManager {
public:
    /**
     * @brief Default constructor
     */
    PackageManager();
    
    /**
     * @brief Destructor
     */
    ~PackageManager();
    
    /**
     * @brief Initialize the package manager
     * 
     * @param root_dir Root directory (e.g., "/")
     * @param db_path Database path (e.g., "/var/lib/pacman")
     * @return bool True if initialization successful
     */
    bool initialize(const std::string& root_dir, const std::string& db_path);
    
    /**
     * @brief Get all installed packages
     * 
     * @return std::vector<Package> List of installed packages
     */
    std::vector<Package> get_installed_packages() const;
    
    /**
     * @brief Get all available packages from sync databases
     * 
     * @return std::vector<Package> List of available packages
     */
    std::vector<Package> get_available_packages() const;
    
    /**
     * @brief Search for packages by name
     * 
     * @param name Package name to search for
     * @return std::vector<Package> List of matching packages
     */
    std::vector<Package> search_by_name(const std::string& name) const;
    
    /**
     * @brief Search for packages in the AUR
     * 
     * @param name Package name to search for
     * @return std::vector<Package> List of matching AUR packages
     */
    std::vector<Package> search_aur(const std::string& name) const;
    
    /**
     * @brief Get detailed information for a package
     * 
     * @param name Package name
     * @return Package The package details
     */
    Package get_package_details(const std::string& name) const;
    
    /**
     * @brief Install a package
     * 
     * @param package_name Name of the package to install
     * @return bool True if installation successful
     */
    bool install_package(const std::string& package_name);
    
    /**
     * @brief Install a package with authentication
     * 
     * @param package_name Name of the package to install
     * @param password The password to use for sudo authentication
     * @param use_overwrite Whether to use the --overwrite="*" option
     * @return bool True if installation successful
     */
    bool install_package(const std::string& package_name, const std::string& password, bool use_overwrite = false);
    
    /**
     * @brief Remove a package
     * 
     * @param package_name Name of the package to remove
     * @return bool True if removal successful
     */
    bool remove_package(const std::string& package_name);
    
    /**
     * @brief Remove a package with authentication
     * 
     * @param package_name Name of the package to remove
     * @param password The password to use for sudo authentication
     * @return bool True if removal successful
     */
    bool remove_package(const std::string& package_name, const std::string& password);
    
    /**
     * @brief Update a package
     * 
     * @param package_name Name of the package to update
     * @return bool True if update successful
     */
    bool update_package(const std::string& package_name);
    
    /**
     * @brief Update a package with authentication
     * 
     * @param package_name Name of the package to update
     * @param password The password to use for sudo authentication
     * @param use_overwrite Whether to use the --overwrite="*" option
     * @return bool True if update successful
     */
    bool update_package(const std::string& package_name, const std::string& password, bool use_overwrite = false);
    
    /**
     * @brief Synchronize all packages (update all)
     * 
     * @return bool True if synchronization successful
     */
    bool sync_all();
    
    /**
     * @brief Synchronize all packages (update all) with authentication
     * 
     * @param password The password to use for sudo authentication
     * @return bool True if synchronization successful
     */
    bool sync_all(const std::string& password);
    
    /**
     * @brief Check if a package is installed
     * 
     * @param package_name Package name to check
     * @return bool True if package is installed
     */
    bool is_package_installed(const std::string& package_name) const;
    
    /**
     * @brief Get all repositories
     * 
     * @return std::vector<Repository> List of repositories
     */
    std::vector<Repository> get_repositories() const;
    
    /**
     * @brief Get last error message
     * 
     * @return std::string The last error message
     */
    std::string get_last_error() const;
    
    /**
     * @brief Perform full system update (pacman -Syu)
     * 
     * @param password The password to use for sudo authentication
     * @param use_overwrite Whether to use the --overwrite="*" option
     * @return bool True if update successful
     */
    bool update_system(const std::string& password, bool use_overwrite = false);
    
    /**
     * @brief Perform full system update (pacman -Syu) with real-time output
     * 
     * @param password The password to use for sudo authentication
     * @param output_callback Callback function to receive real-time output from pacman
     * @param use_overwrite Whether to use the --overwrite="*" option
     * @return bool True if update successful
     */
    bool update_system(const std::string& password, std::function<void(const std::string&)> output_callback, bool use_overwrite = false);
    
    /**
     * @brief Check for available system updates (without installing)
     * 
     * @return std::vector<std::pair<std::string, std::string>> List of packages with updates (name, new_version)
     */
    std::vector<std::pair<std::string, std::string>> check_updates() const;

private:
    alpm_handle_t* m_handle;                          ///< ALPM handle
    RepositoryManager* m_repo_manager;                ///< Repository manager
    TransactionManager* m_trans_manager;              ///< Transaction manager
    std::string m_last_error;                         ///< Last error message
    
    /**
     * @brief Set the last error message
     * 
     * @param error Error message
     */
    void set_last_error(const std::string& error);
    
    /**
     * @brief Register all available sync databases
     * 
     * @return bool True if registration successful
     */
    bool register_sync_databases();
};

} // namespace core
} // namespace pacmangui 