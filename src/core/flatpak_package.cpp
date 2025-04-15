#include "core/flatpak_package.hpp"

namespace pacmangui {
namespace core {

FlatpakPackage::FlatpakPackage()
    : Package(), m_app_id(""), m_runtime(""), m_installation_type("") {
}

FlatpakPackage::FlatpakPackage(const std::string& name, const std::string& version)
    : Package(name, version), m_app_id(""), m_runtime(""), m_installation_type("") {
}

FlatpakPackage FlatpakPackage::create_from_data(
    const std::string& app_id,
    const std::string& name,
    const std::string& version,
    const std::string& description,
    const std::string& origin)
{
    FlatpakPackage pkg(name, version);
    pkg.set_app_id(app_id);
    pkg.set_description(description);
    pkg.set_repository(origin);
    return pkg;
}

std::string FlatpakPackage::get_app_id() const {
    return m_app_id;
}

void FlatpakPackage::set_app_id(const std::string& app_id) {
    m_app_id = app_id;
}

std::string FlatpakPackage::get_runtime() const {
    return m_runtime;
}

void FlatpakPackage::set_runtime(const std::string& runtime) {
    m_runtime = runtime;
}

std::string FlatpakPackage::get_installation_type() const {
    return m_installation_type;
}

void FlatpakPackage::set_installation_type(const std::string& type) {
    m_installation_type = type;
}

std::string FlatpakPackage::get_branch() const {
    return m_branch;
}

void FlatpakPackage::set_branch(const std::string& branch) {
    m_branch = branch;
}

bool FlatpakPackage::is_system_wide() const {
    return m_installation_type == "system";
}

std::string FlatpakPackage::get_size() const {
    return m_size;
}

void FlatpakPackage::set_size(const std::string& size) {
    m_size = size;
}

} // namespace core
} // namespace pacmangui 