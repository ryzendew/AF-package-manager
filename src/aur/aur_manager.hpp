#pragma once

#include "aur/aur_package.hpp"
#include "aur/aur_helper.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace pacmangui {
namespace aur {

/**
 * @brief Core manager class for AUR operations
 */
class AURManager {
public:
    /**
     * @brief Default constructor
     */
    AURManager();

    /**
     * @brief Destructor
     */
    ~AURManager();

    /**
     * @brief Initialize the AUR manager
     * @return True if initialization successful
     */
    bool initialize();

    /**
     * @brief Search for AUR packages
     * @param search_term Term to search for
     * @return Vector of matching packages
     */
    std::vector<AURPackage> search(const std::string& search_term);

    /**
     * @brief Get package details from AUR
     * @param package_name Package name
     * @return Package details
     */
    AURPackage get_package_info(const std::string& package_name);

    /**
     * @brief Install an AUR package
     * @param package_name Package name
     * @param password User password for sudo
     * @param output_callback Callback for installation output
     * @return True if installation successful
     */
    bool install_package(const std::string& package_name, 
                        const std::string& password,
                        std::function<void(const std::string&)> output_callback = nullptr);

    /**
     * @brief Remove an AUR package
     * @param package_name Package name
     * @param password User password for sudo
     * @param output_callback Callback for removal output
     * @return True if removal successful
     */
    bool remove_package(const std::string& package_name,
                       const std::string& password,
                       std::function<void(const std::string&)> output_callback = nullptr);

    /**
     * @brief Update all AUR packages
     * @param password User password for sudo
     * @param output_callback Callback for update output
     * @return True if update successful
     */
    bool update_all(const std::string& password,
                   std::function<void(const std::string&)> output_callback = nullptr);

    /**
     * @brief Check for AUR package updates
     * @return Vector of packages with updates
     */
    std::vector<AURPackage> check_updates();

    /**
     * @brief Get list of installed AUR packages
     * @return Vector of installed packages
     */
    std::vector<AURPackage> get_installed_packages() const;

    /**
     * @brief Get the currently used AUR helper
     * @return Name of current AUR helper
     */
    std::string get_current_helper() const;

    /**
     * @brief Set the AUR helper to use
     * @param helper_name Name of helper to use
     * @return True if helper set successfully
     */
    bool set_helper(const std::string& helper_name);

    /**
     * @brief Get list of available AUR helpers
     * @return Vector of available helper names
     */
    std::vector<std::string> get_available_helpers() const;

    /**
     * @brief Get the last error message
     * @return Error message
     */
    std::string get_last_error() const;

private:
    /**
     * @brief Query the AUR RPC API
     * @param type Query type (search, info, etc.)
     * @param arg Query argument
     * @return JSON response string
     */
    std::string query_aur_rpc(const std::string& type, const std::string& arg) const;

    /**
     * @brief Parse package list from JSON response
     * @param json_data JSON response data
     * @return Vector of packages
     */
    std::vector<AURPackage> parse_package_list(const std::string& json_data) const;

    /**
     * @brief Set the last error message
     * @param error Error message
     */
    void set_last_error(const std::string& error);

    AURHelper m_helper;
    std::string m_last_error;
    std::vector<AURPackage> m_installed_packages;
    bool m_initialized;
};

} // namespace aur
} // namespace pacmangui 