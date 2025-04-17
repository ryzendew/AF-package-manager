#include "aur/aur_package.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

namespace pacmangui {
namespace aur {

using json = nlohmann::json;

AURPackage::AURPackage()
    : m_name("")
    , m_version("")
    , m_description("")
    , m_maintainer("")
    , m_votes(0)
    , m_popularity(0.0)
    , m_git_url("")
    , m_last_updated(0)
    , m_installed(false)
    , m_installed_version("")
{
}

AURPackage::AURPackage(const std::string& name, const std::string& version, const std::string& description)
    : m_name(name)
    , m_version(version)
    , m_description(description)
    , m_maintainer("")
    , m_votes(0)
    , m_popularity(0.0)
    , m_git_url("")
    , m_last_updated(0)
    , m_installed(false)
    , m_installed_version("")
{
}

std::string AURPackage::get_name() const
{
    return m_name;
}

void AURPackage::set_name(const std::string& name)
{
    m_name = name;
}

std::string AURPackage::get_version() const
{
    return m_version;
}

void AURPackage::set_version(const std::string& version)
{
    m_version = version;
}

std::string AURPackage::get_description() const
{
    return m_description;
}

void AURPackage::set_description(const std::string& description)
{
    m_description = description;
}

std::string AURPackage::get_maintainer() const
{
    return m_maintainer;
}

void AURPackage::set_maintainer(const std::string& maintainer)
{
    m_maintainer = maintainer;
}

int AURPackage::get_votes() const
{
    return m_votes;
}

void AURPackage::set_votes(int votes)
{
    m_votes = votes;
}

double AURPackage::get_popularity() const
{
    return m_popularity;
}

void AURPackage::set_popularity(double popularity)
{
    m_popularity = popularity;
}

std::vector<std::string> AURPackage::get_dependencies() const
{
    return m_dependencies;
}

void AURPackage::set_dependencies(const std::vector<std::string>& dependencies)
{
    m_dependencies = dependencies;
}

std::string AURPackage::get_git_url() const
{
    return m_git_url;
}

void AURPackage::set_git_url(const std::string& url)
{
    m_git_url = url;
}

time_t AURPackage::get_last_updated() const
{
    return m_last_updated;
}

void AURPackage::set_last_updated(time_t time)
{
    m_last_updated = time;
}

bool AURPackage::is_installed() const
{
    return m_installed;
}

void AURPackage::set_installed(bool installed)
{
    m_installed = installed;
}

std::string AURPackage::get_installed_version() const
{
    return m_installed_version;
}

void AURPackage::set_installed_version(const std::string& version)
{
    m_installed_version = version;
}

bool AURPackage::has_update() const
{
    return m_installed && m_installed_version != m_version;
}

bool AURPackage::from_json(const std::string& json_data)
{
    try {
        json j = json::parse(json_data);
        
        // Required fields
        if (!j.contains("Name") || !j.contains("Version") || !j.contains("Description")) {
            return false;
        }
        
        m_name = j["Name"].get<std::string>();
        m_version = j["Version"].get<std::string>();
        m_description = j["Description"].get<std::string>();
        
        // Optional fields
        if (j.contains("Maintainer")) {
            m_maintainer = j["Maintainer"].get<std::string>();
        }
        
        if (j.contains("NumVotes")) {
            m_votes = j["NumVotes"].get<int>();
        }
        
        if (j.contains("Popularity")) {
            m_popularity = j["Popularity"].get<double>();
        }
        
        if (j.contains("Depends")) {
            m_dependencies = j["Depends"].get<std::vector<std::string>>();
        }
        
        if (j.contains("URL")) {
            m_git_url = j["URL"].get<std::string>();
        }
        
        if (j.contains("LastModified")) {
            m_last_updated = j["LastModified"].get<time_t>();
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string AURPackage::to_json() const
{
    json j;
    
    // Required fields
    j["Name"] = m_name;
    j["Version"] = m_version;
    j["Description"] = m_description;
    
    // Optional fields
    if (!m_maintainer.empty()) {
        j["Maintainer"] = m_maintainer;
    }
    
    if (m_votes > 0) {
        j["NumVotes"] = m_votes;
    }
    
    if (m_popularity > 0.0) {
        j["Popularity"] = m_popularity;
    }
    
    if (!m_dependencies.empty()) {
        j["Depends"] = m_dependencies;
    }
    
    if (!m_git_url.empty()) {
        j["URL"] = m_git_url;
    }
    
    if (m_last_updated > 0) {
        j["LastModified"] = m_last_updated;
    }
    
    if (m_installed) {
        j["Installed"] = true;
        j["InstalledVersion"] = m_installed_version;
    }
    
    return j.dump();
}

} // namespace aur
} // namespace pacmangui 