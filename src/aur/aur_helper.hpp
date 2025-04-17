#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace pacmangui {
namespace aur {

/**
 * @brief Class to manage AUR helper detection and command execution
 */
class AURHelper {
public:
    /**
     * @brief Default constructor
     */
    AURHelper();

    /**
     * @brief Destructor
     */
    ~AURHelper();

    /**
     * @brief Auto-detect installed AUR helpers
     * @return True if any helper was found
     */
    bool detect_helpers();

    /**
     * @brief Get the currently selected AUR helper
     * @return Name of the current helper
     */
    std::string get_current_helper() const;

    /**
     * @brief Set the AUR helper to use
     * @param helper_name Name of the helper to use
     * @return True if helper is valid and available
     */
    bool set_helper(const std::string& helper_name);

    /**
     * @brief Get list of available AUR helpers
     * @return Vector of available helper names
     */
    std::vector<std::string> get_available_helpers() const;

    /**
     * @brief Check if a specific helper is available
     * @param helper_name Name of the helper to check
     * @return True if helper is available
     */
    bool is_helper_available(const std::string& helper_name) const;

    /**
     * @brief Get the install command for the current helper
     * @param package_name Name of the package to install
     * @param options Additional options (optional)
     * @return The complete install command
     */
    std::string get_install_command(const std::string& package_name, 
                                  const std::vector<std::string>& options = {}) const;

    /**
     * @brief Get the remove command for the current helper
     * @param package_name Name of the package to remove
     * @param options Additional options (optional)
     * @return The complete remove command
     */
    std::string get_remove_command(const std::string& package_name,
                                 const std::vector<std::string>& options = {}) const;

    /**
     * @brief Get the update command for the current helper
     * @param options Additional options (optional)
     * @return The complete update command
     */
    std::string get_update_command(const std::vector<std::string>& options = {}) const;

    /**
     * @brief Get the search command for the current helper
     * @param search_term Term to search for
     * @param options Additional options (optional)
     * @return The complete search command
     */
    std::string get_search_command(const std::string& search_term,
                                 const std::vector<std::string>& options = {}) const;

    /**
     * @brief Validate the current helper's functionality
     * @return True if helper is working correctly
     */
    bool validate_helper() const;

private:
    struct HelperCommands {
        std::string install;
        std::string remove;
        std::string update;
        std::string search;
        std::string check_updates;
    };

    std::string m_current_helper;
    std::map<std::string, HelperCommands> m_helper_commands;
    std::vector<std::string> m_available_helpers;

    /**
     * @brief Initialize command templates for known helpers
     */
    void init_helper_commands();

    /**
     * @brief Check if a helper is installed in the system
     * @param helper_name Name of the helper to check
     * @return True if helper is installed
     */
    bool check_helper_installed(const std::string& helper_name) const;
};

} // namespace aur
} // namespace pacmangui 