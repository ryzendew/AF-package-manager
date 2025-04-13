#include "package.hpp"
#include <alpm.h>
#include <iostream>

namespace pacmangui {
namespace core {

Package::Package()
    : m_name("")
    , m_version("")
    , m_description("")
    , m_installed(false)
{
}

Package::Package(const std::string& name, const std::string& version)
    : m_name(name)
    , m_version(version)
    , m_description("")
    , m_installed(false)
{
}

Package Package::create_from_alpm(alpm_pkg_t* pkg)
{
    if (!pkg) {
        return Package();
    }
    
    Package result(alpm_pkg_get_name(pkg), alpm_pkg_get_version(pkg));
    result.set_description(alpm_pkg_get_desc(pkg));
    
    // Check if package is installed by checking the origin
    // If it's from a sync db, it's not installed
    result.set_installed(alpm_pkg_get_origin(pkg) == ALPM_PKG_FROM_LOCALDB);
    
    return result;
}

std::string Package::get_name() const
{
    return m_name;
}

void Package::set_name(const std::string& name)
{
    m_name = name;
}

std::string Package::get_version() const
{
    return m_version;
}

void Package::set_version(const std::string& version)
{
    m_version = version;
}

std::string Package::get_description() const
{
    return m_description;
}

void Package::set_description(const std::string& description)
{
    m_description = description;
}

bool Package::is_installed() const
{
    return m_installed;
}

void Package::set_installed(bool installed)
{
    m_installed = installed;
}

bool Package::operator==(const Package& other) const
{
    return m_name == other.m_name;
}

bool Package::operator!=(const Package& other) const
{
    return !(*this == other);
}

} // namespace core
} // namespace pacmangui 