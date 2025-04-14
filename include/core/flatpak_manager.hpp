#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "core/flatpak_package.hpp"

namespace pacmangui {
namespace core {

/**
 * @brief Class for managing Flatpak packages
 */
class FlatpakManager {
public:
    /**
     * @brief Default constructor
     */
    FlatpakManager();
    
    /**
     * @brief Destructor
     */
    ~FlatpakManager();
    
    /**
     * @brief Initialize the Flatpak manager
     * @return bool True if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Check if Flatpak is installed on the system
     * @return bool True if Flatpak is available
     */
    bool is_available() const;
    
    /**
     * @brief Get all installed Flatpak packages
     * @return std::vector<FlatpakPackage> List of installed Flatpak packages
     */
    std::vector<FlatpakPackage> get_installed_packages() const;
    
    /**
     * @brief Search for Flatpak packages by name
     * @param name Package name to search for
     * @return std::vector<FlatpakPackage> List of matching Flatpak packages
     */
    std::vector<FlatpakPackage> search_by_name(const std::string& name) const;
    
    /**
     * @brief Install a Flatpak package
     * @param app_id Flatpak application ID to install
     * @param remote Flatpak remote (flathub, etc.)
     * @return bool True if installation successful
     */
    bool install_package(const std::string& app_id, const std::string& remote = "flathub");
    
    /**
     * @brief Remove a Flatpak package
     * @param app_id Flatpak application ID to remove
     * @return bool True if removal successful
     */
    bool remove_package(const std::string& app_id);
    
    /**
     * @brief Update a Flatpak package
     * @param app_id Flatpak application ID to update
     * @return bool True if update successful
     */
    bool update_package(const std::string& app_id);
    
    /**
     * @brief Update all Flatpak packages
     * @return bool True if update successful
     */
    bool update_all();
    
    /**
     * @brief Check if a Flatpak package is installed
     * @param app_id Flatpak application ID to check
     * @return bool True if package is installed
     */
    bool is_package_installed(const std::string& app_id) const;
    
    /**
     * @brief Get available Flatpak remotes
     * @return std::vector<std::string> List of available remotes
     */
    std::vector<std::string> get_remotes() const;
    
    /**
     * @brief Add a Flatpak remote
     * @param name Remote name
     * @param url Remote URL
     * @return bool True if remote added successfully
     */
    bool add_remote(const std::string& name, const std::string& url);
    
    /**
     * @brief Get last error message
     * @return std::string The last error message
     */
    std::string get_last_error() const;

private:
    /**
     * @brief Execute a Flatpak command and parse the JSON output
     * @param args Command arguments
     * @return std::string Command output (JSON)
     */
    std::string execute_flatpak_command(const std::vector<std::string>& args) const;
    
    /**
     * @brief Parse JSON output from Flatpak command into FlatpakPackage objects
     * @param json_output JSON output from Flatpak command
     * @return std::vector<FlatpakPackage> List of Flatpak packages
     */
    std::vector<FlatpakPackage> parse_json_output(const std::string& json_output) const;
    
    bool m_is_available;              ///< Flag indicating if Flatpak is available
    mutable std::string m_last_error; ///< Last error message
};

} // namespace core
} // namespace pacmangui 