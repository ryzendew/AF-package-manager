#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Forward declaration
struct _alpm_pkg_t;
typedef struct _alpm_pkg_t alpm_pkg_t;

namespace pacmangui {
namespace core {

/**
 * @brief Class representing a package
 */
class Package {
public:
    /**
     * @brief Default constructor
     */
    Package();

    /**
     * @brief Constructor with name and version
     * @param name Package name
     * @param version Package version
     */
    Package(const std::string& name, const std::string& version);

    /**
     * @brief Create a package from alpm package
     * @param pkg ALPM package
     * @return Package object
     */
    static Package create_from_alpm(alpm_pkg_t* pkg);

    /**
     * @brief Get package name
     * @return Package name
     */
    std::string get_name() const;

    /**
     * @brief Set package name
     * @param name Package name
     */
    void set_name(const std::string& name);

    /**
     * @brief Get package version
     * @return Package version
     */
    std::string get_version() const;

    /**
     * @brief Set package version
     * @param version Package version
     */
    void set_version(const std::string& version);

    /**
     * @brief Get package description
     * @return Package description
     */
    std::string get_description() const;

    /**
     * @brief Set package description
     * @param description Package description
     */
    void set_description(const std::string& description);

    /**
     * @brief Check if package is installed
     * @return True if package is installed
     */
    bool is_installed() const;

    /**
     * @brief Set installed status
     * @param installed Installation status
     */
    void set_installed(bool installed);

    /**
     * @brief Get package repository
     * @return Repository name
     */
    std::string get_repository() const;

    /**
     * @brief Set package repository
     * @param repository Repository name
     */
    void set_repository(const std::string& repository);

    /**
     * @brief Get AUR package information
     * @return AUR information string
     */
    std::string get_aur_info() const;

    /**
     * @brief Set AUR package information
     * @param aur_info AUR information string
     */
    void set_aur_info(const std::string& aur_info);

    /**
     * @brief Equality operator
     * @param other Package to compare to
     * @return True if packages are equal
     */
    bool operator==(const Package& other) const;

    /**
     * @brief Inequality operator
     * @param other Package to compare to
     * @return True if packages are not equal
     */
    bool operator!=(const Package& other) const;

private:
    std::string m_name;                ///< Package name
    std::string m_version;             ///< Package version
    std::string m_description;         ///< Package description
    std::string m_repository;          ///< Repository name
    std::string m_aur_info;            ///< AUR information (if from AUR)
    bool m_installed;                  ///< True if package is installed
};

} // namespace core
} // namespace pacmangui 