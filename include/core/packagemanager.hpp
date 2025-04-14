#pragma once

#include <string>
#include <vector>
#include <memory>
#include <alpm.h>
#include "core/package.hpp"
#include "core/repository.hpp"
#include "core/transaction.hpp"
#include "core/flatpak_manager.hpp"
#include "core/flatpak_package.hpp"
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
     * @brief Install a package from AUR
     * 
     * @param package_name Name of the AUR package to install
     * @return bool True if installation successful
     */
    bool install_aur_package(const std::string& package_name);
    
    /**
     * @brief Install a package from AUR with authentication
     * 
     * @param package_name Name of the AUR package to install
     * @param password The password to use for sudo authentication
     * @param aur_helper AUR helper to use (yay, paru, etc.)
     * @return bool True if installation successful
     */
    bool install_aur_package(const std::string& package_name, const std::string& password, 
                            const std::string& aur_helper = "yay");
    
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
    
    /**
     * @brief Check for available AUR updates (without installing)
     * 
     * @param aur_helper The AUR helper to use (e.g., "yay", "paru")
     * @return std::vector<std::pair<std::string, std::string>> List of AUR packages with updates (name, new_version)
     */
    std::vector<std::pair<std::string, std::string>> check_aur_updates(const std::string& aur_helper = "") const;
    
    /**
     * @brief Update all AUR packages
     * 
     * @param aur_helper The AUR helper to use (e.g., "yay", "paru")
     * @return bool True if update successful
     */
    bool update_aur_packages(const std::string& aur_helper = "");
    
    /**
     * @brief Update all AUR packages with authentication
     * 
     * @param password The password to use for sudo authentication
     * @param aur_helper The AUR helper to use (e.g., "yay", "paru")
     * @param output_callback Callback function to receive real-time output
     * @return bool True if update successful
     */
    bool update_aur_packages(const std::string& password, 
                           const std::string& aur_helper = "",
                           std::function<void(const std::string&)> output_callback = nullptr);
    
    /**
     * @brief Execute a command with sudo authentication
     * 
     * @param command The command to execute
     * @param password The password to use for sudo authentication
     * @return bool True if command executed successfully
     */
    bool execute_with_sudo(const std::string& command, const std::string& password);
    
    /**
     * @brief Execute a command with sudo (no password)
     * 
     * @param command The command to execute
     * @return bool True if command executed successfully
     */
    bool execute_with_sudo(const std::string& command);
    
    /**
     * @brief Clear the package cache (pacman -Sc)
     * 
     * @param clean_all Whether to remove all packages (true) or just uninstalled packages (false)
     * @param password The password to use for sudo authentication
     * @param output_callback Callback function to receive real-time output
     * @return bool True if operation was successful
     */
    bool clear_package_cache(bool clean_all, const std::string& password, 
                           std::function<void(const std::string&)> output_callback = nullptr);
    
    /**
     * @brief Remove orphaned packages (not required by any other package)
     * 
     * @param password The password to use for sudo authentication
     * @param output_callback Callback function to receive real-time output
     * @return bool True if operation was successful
     */
    bool remove_orphaned_packages(const std::string& password,
                                std::function<void(const std::string&)> output_callback = nullptr);
    
    /**
     * @brief Get a list of orphaned packages (not required by any other package)
     * 
     * @return std::vector<std::string> List of orphaned package names
     */
    std::vector<std::string> get_orphaned_packages() const;
    
    /**
     * @brief Check pacman database for errors
     * 
     * @param check_sync_dbs Whether to check sync databases as well (true) or just local database (false)
     * @param output_callback Callback function to receive real-time output
     * @return bool True if no errors were found
     */
    bool check_database(bool check_sync_dbs = false,
                      std::function<void(const std::string&)> output_callback = nullptr);
    
    /**
     * @brief Find pacnew/pacsave files
     * 
     * @return std::vector<std::string> List of paths to pacnew/pacsave files
     */
    std::vector<std::string> find_pacnew_files() const;
    
    /**
     * @brief Backup pacman database
     * 
     * @param backup_path Path where the backup will be saved
     * @param output_callback Callback function to receive real-time output
     * @return bool True if backup was successful
     */
    bool backup_database(const std::string& backup_path,
                       std::function<void(const std::string&)> output_callback = nullptr);
    
    /**
     * @brief Restore pacman database from backup
     * 
     * @param backup_path Path to the backup file
     * @param password The password to use for sudo authentication
     * @param output_callback Callback function to receive real-time output
     * @return bool True if restore was successful
     */
    bool restore_database(const std::string& backup_path, const std::string& password,
                        std::function<void(const std::string&)> output_callback = nullptr);
    
    /**
     * @brief Get all installed Flatpak packages
     * 
     * @return std::vector<FlatpakPackage> List of installed Flatpak packages
     */
    std::vector<FlatpakPackage> get_installed_flatpak_packages() const;
    
    /**
     * @brief Search for Flatpak packages by name
     * 
     * @param name Package name to search for
     * @return std::vector<FlatpakPackage> List of matching Flatpak packages
     */
    std::vector<FlatpakPackage> search_flatpak_by_name(const std::string& name) const;
    
    /**
     * @brief Install a Flatpak package
     * 
     * @param app_id Application ID of the Flatpak package to install
     * @param remote Remote name (e.g., "flathub")
     * @return bool True if installation successful
     */
    bool install_flatpak_package(const std::string& app_id, const std::string& remote = "flathub");
    
    /**
     * @brief Remove a Flatpak package
     * 
     * @param app_id Application ID of the Flatpak package to remove
     * @return bool True if removal successful
     */
    bool remove_flatpak_package(const std::string& app_id);
    
    /**
     * @brief Update a Flatpak package
     * 
     * @param app_id Application ID of the Flatpak package to update
     * @return bool True if update successful
     */
    bool update_flatpak_package(const std::string& app_id);
    
    /**
     * @brief Update all Flatpak packages
     * 
     * @return bool True if update successful
     */
    bool update_all_flatpak_packages();
    
    /**
     * @brief Check if Flatpak is available on the system
     * 
     * @return bool True if Flatpak is available
     */
    bool is_flatpak_available() const;
    
    /**
     * @brief Get available Flatpak remotes
     * 
     * @return std::vector<std::string> List of available remotes
     */
    std::vector<std::string> get_flatpak_remotes() const;

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
    
    // Flatpak manager
    FlatpakManager m_flatpak_manager;
};

} // namespace core
} // namespace pacmangui 