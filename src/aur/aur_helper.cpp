#include "aur/aur_helper.hpp"
#include <QProcess>
#include <QSettings>
#include <iostream>

namespace pacmangui {
namespace aur {

AURHelper::AURHelper()
    : m_current_helper("")
{
    init_helper_commands();
    detect_helpers();
}

AURHelper::~AURHelper()
{
}

void AURHelper::init_helper_commands()
{
    // yay commands
    HelperCommands yay;
    yay.install = "yay -S --noconfirm";
    yay.remove = "yay -R --noconfirm";
    yay.update = "yay -Sua --noconfirm";
    yay.search = "yay -Ss";
    yay.check_updates = "yay -Qua";
    m_helper_commands["yay"] = yay;

    // paru commands
    HelperCommands paru;
    paru.install = "paru -S --noconfirm";
    paru.remove = "paru -R --noconfirm";
    paru.update = "paru -Sua --noconfirm";
    paru.search = "paru -Ss";
    paru.check_updates = "paru -Qua";
    m_helper_commands["paru"] = paru;

    // aurman commands
    HelperCommands aurman;
    aurman.install = "aurman -S --noconfirm";
    aurman.remove = "aurman -R --noconfirm";
    aurman.update = "aurman -Sua --noconfirm";
    aurman.search = "aurman -Ss";
    aurman.check_updates = "aurman -Qua";
    m_helper_commands["aurman"] = aurman;

    // trizen commands
    HelperCommands trizen;
    trizen.install = "trizen -S --noconfirm";
    trizen.remove = "trizen -R --noconfirm";
    trizen.update = "trizen -Sua --noconfirm";
    trizen.search = "trizen -Ss";
    trizen.check_updates = "trizen -Qua";
    m_helper_commands["trizen"] = trizen;

    // pamac commands
    HelperCommands pamac;
    pamac.install = "pamac build --no-confirm";
    pamac.remove = "pamac remove --no-confirm";
    pamac.update = "pamac upgrade --no-confirm";
    pamac.search = "pamac search -a";
    pamac.check_updates = "pamac checkupdates -a";
    m_helper_commands["pamac"] = pamac;
}

bool AURHelper::detect_helpers()
{
    m_available_helpers.clear();
    
    // Priority order of helpers to check
    std::vector<std::string> helpers = {"yay", "paru", "aurman", "trizen", "pamac"};
    
    for (const auto& helper : helpers) {
        if (check_helper_installed(helper)) {
            m_available_helpers.push_back(helper);
            
            // If we don't have a current helper set, use the first one found
            if (m_current_helper.empty()) {
                m_current_helper = helper;
            }
        }
    }
    
    // Load preferred helper from settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    std::string preferred = settings.value("aur/helper", "").toString().toStdString();
    
    if (!preferred.empty() && is_helper_available(preferred)) {
        m_current_helper = preferred;
    }
    
    return !m_available_helpers.empty();
}

bool AURHelper::check_helper_installed(const std::string& helper_name) const
{
    QProcess process;
    process.start("which", QStringList() << QString::fromStdString(helper_name));
    process.waitForFinished();
    return process.exitCode() == 0;
}

std::string AURHelper::get_current_helper() const
{
    return m_current_helper;
}

bool AURHelper::set_helper(const std::string& helper_name)
{
    if (!is_helper_available(helper_name)) {
        return false;
    }
    
    m_current_helper = helper_name;
    
    // Save to settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    settings.setValue("aur/helper", QString::fromStdString(helper_name));
    
    return true;
}

std::vector<std::string> AURHelper::get_available_helpers() const
{
    return m_available_helpers;
}

bool AURHelper::is_helper_available(const std::string& helper_name) const
{
    return std::find(m_available_helpers.begin(), m_available_helpers.end(), helper_name) 
           != m_available_helpers.end();
}

std::string AURHelper::get_install_command(const std::string& package_name,
                                         const std::vector<std::string>& options) const
{
    if (m_current_helper.empty() || !m_helper_commands.count(m_current_helper)) {
        return "";
    }
    
    std::string cmd = m_helper_commands.at(m_current_helper).install;
    
    // Add options
    for (const auto& opt : options) {
        cmd += " " + opt;
    }
    
    // Add package name
    cmd += " " + package_name;
    
    return cmd;
}

std::string AURHelper::get_remove_command(const std::string& package_name,
                                        const std::vector<std::string>& options) const
{
    if (m_current_helper.empty() || !m_helper_commands.count(m_current_helper)) {
        return "";
    }
    
    std::string cmd = m_helper_commands.at(m_current_helper).remove;
    
    // Add options
    for (const auto& opt : options) {
        cmd += " " + opt;
    }
    
    // Add package name
    cmd += " " + package_name;
    
    return cmd;
}

std::string AURHelper::get_update_command(const std::vector<std::string>& options) const
{
    if (m_current_helper.empty() || !m_helper_commands.count(m_current_helper)) {
        return "";
    }
    
    std::string cmd = m_helper_commands.at(m_current_helper).update;
    
    // Add options
    for (const auto& opt : options) {
        cmd += " " + opt;
    }
    
    return cmd;
}

std::string AURHelper::get_search_command(const std::string& search_term,
                                        const std::vector<std::string>& options) const
{
    if (m_current_helper.empty() || !m_helper_commands.count(m_current_helper)) {
        return "";
    }
    
    std::string cmd = m_helper_commands.at(m_current_helper).search;
    
    // Add options
    for (const auto& opt : options) {
        cmd += " " + opt;
    }
    
    // Add search term
    cmd += " " + search_term;
    
    return cmd;
}

bool AURHelper::validate_helper() const
{
    if (m_current_helper.empty()) {
        return false;
    }
    
    // Try to run a simple command with the helper
    QProcess process;
    process.start(QString::fromStdString(m_current_helper), QStringList() << "--version");
    process.waitForFinished();
    
    return process.exitCode() == 0;
}

} // namespace aur
} // namespace pacmangui 