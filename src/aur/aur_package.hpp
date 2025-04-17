#pragma once

#include <string>
#include <vector>
#include <ctime>

namespace pacmangui {
namespace aur {

/**
 * @brief Class representing an AUR package
 */
class AURPackage {
public:
    /**
     * @brief Default constructor
     */
    AURPackage();

    /**
     * @brief Constructor with basic info
     * @param name Package name
     * @param version Package version
     * @param description Package description
     */
    AURPackage(const std::string& name, const std::string& version, const std::string& description);

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
     * @brief Get package maintainer
     * @return Package maintainer
     */
    std::string get_maintainer() const;

    /**
     * @brief Set package maintainer
     * @param maintainer Package maintainer
     */
    void set_maintainer(const std::string& maintainer);

    /**
     * @brief Get package votes
     * @return Number of votes
     */
    int get_votes() const;

    /**
     * @brief Set package votes
     * @param votes Number of votes
     */
    void set_votes(int votes);

    /**
     * @brief Get package popularity
     * @return Popularity score
     */
    double get_popularity() const;

    /**
     * @brief Set package popularity
     * @param popularity Popularity score
     */
    void set_popularity(double popularity);

    /**
     * @brief Get package dependencies
     * @return List of dependencies
     */
    std::vector<std::string> get_dependencies() const;

    /**
     * @brief Set package dependencies
     * @param dependencies List of dependencies
     */
    void set_dependencies(const std::vector<std::string>& dependencies);

    /**
     * @brief Get package Git URL
     * @return Git URL
     */
    std::string get_git_url() const;

    /**
     * @brief Set package Git URL
     * @param url Git URL
     */
    void set_git_url(const std::string& url);

    /**
     * @brief Get last updated time
     * @return Last updated time
     */
    time_t get_last_updated() const;

    /**
     * @brief Set last updated time
     * @param time Last updated time
     */
    void set_last_updated(time_t time);

    /**
     * @brief Check if package is installed
     * @return True if installed
     */
    bool is_installed() const;

    /**
     * @brief Set installed status
     * @param installed True if installed
     */
    void set_installed(bool installed);

    /**
     * @brief Get installed version
     * @return Installed version
     */
    std::string get_installed_version() const;

    /**
     * @brief Set installed version
     * @param version Installed version
     */
    void set_installed_version(const std::string& version);

    /**
     * @brief Check if package has an update available
     * @return True if update available
     */
    bool has_update() const;

    /**
     * @brief Create from JSON data
     * @param json_data JSON data string
     * @return True if successfully parsed
     */
    bool from_json(const std::string& json_data);

    /**
     * @brief Convert to JSON string
     * @return JSON string representation
     */
    std::string to_json() const;

private:
    std::string m_name;
    std::string m_version;
    std::string m_description;
    std::string m_maintainer;
    int m_votes;
    double m_popularity;
    std::vector<std::string> m_dependencies;
    std::string m_git_url;
    time_t m_last_updated;
    bool m_installed;
    std::string m_installed_version;
};

} // namespace aur
} // namespace pacmangui 