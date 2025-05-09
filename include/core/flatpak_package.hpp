#pragma once

#include "core/package.hpp"

namespace pacmangui {
namespace core {

/**
 * @brief Class representing a Flatpak package
 */
class FlatpakPackage : public Package {
public:
    /**
     * @brief Default constructor
     */
    FlatpakPackage();

    /**
     * @brief Constructor with name and version
     * @param name Package name
     * @param version Package version
     */
    FlatpakPackage(const std::string& name, const std::string& version);

    /**
     * @brief Create a FlatpakPackage from JSON data
     * @param app_id Application ID
     * @param name Display name
     * @param version Version
     * @param description Description
     * @param origin Origin/Repository
     * @return FlatpakPackage object
     */
    static FlatpakPackage create_from_data(
        const std::string& app_id,
        const std::string& name,
        const std::string& version,
        const std::string& description,
        const std::string& origin
    );

    /**
     * @brief Get the Flatpak application ID (org.example.App)
     * @return Application ID
     */
    std::string get_app_id() const;

    /**
     * @brief Set the Flatpak application ID
     * @param app_id Application ID
     */
    void set_app_id(const std::string& app_id);

    /**
     * @brief Get the Flatpak runtime
     * @return Runtime
     */
    std::string get_runtime() const;

    /**
     * @brief Set the Flatpak runtime
     * @param runtime Runtime
     */
    void set_runtime(const std::string& runtime);

    /**
     * @brief Get the Flatpak installation type (system/user)
     * @return Installation type
     */
    std::string get_installation_type() const;

    /**
     * @brief Set the Flatpak installation type
     * @param type Installation type
     */
    void set_installation_type(const std::string& type);

    /**
     * @brief Get the Flatpak branch
     * @return Branch name
     */
    std::string get_branch() const;

    /**
     * @brief Set the Flatpak branch
     * @param branch Branch name
     */
    void set_branch(const std::string& branch);

    /**
     * @brief Check if package is installed system-wide
     * @return true if system-wide, false if user installation
     */
    bool is_system_wide() const;

    /**
     * @brief Get the installed size of the package
     * @return Size in bytes
     */
    std::string get_size() const;

    /**
     * @brief Set the installed size of the package
     * @param size Size in bytes
     */
    void set_size(const std::string& size);

    /**
     * @brief Get the search score for this package
     * @return double The search score
     */
    double get_search_score() const { return m_search_score; }

    /**
     * @brief Set the search score for this package
     * @param score The search score to set
     */
    void set_search_score(double score) { m_search_score = score; }

private:
    std::string m_app_id;              ///< Flatpak application ID
    std::string m_runtime;             ///< Flatpak runtime
    std::string m_installation_type;   ///< Flatpak installation type (system/user)
    std::string m_branch;              ///< Flatpak branch
    std::string m_size;                ///< Installed size
    double m_search_score = 0.0;  ///< Search score for ranking results
};

} // namespace core
} // namespace pacmangui 