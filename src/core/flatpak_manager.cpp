#include "core/flatpak_manager.hpp"
#include <iostream>
#include <sstream>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpression>
#include <set>
#include <tuple>
#include <unordered_map>

namespace pacmangui {
namespace core {

FlatpakManager::FlatpakManager()
    : m_is_available(false), m_last_error("")
{
}

FlatpakManager::~FlatpakManager()
{
}

bool FlatpakManager::initialize()
{
    std::cout << "FlatpakManager: Initializing..." << std::endl;
    
    // Check if flatpak is installed
    QProcess process;
    process.start("which", QStringList() << "flatpak");
    process.waitForFinished();
    
    m_is_available = (process.exitCode() == 0);
    
    if (m_is_available) {
        std::cout << "FlatpakManager: Flatpak is available on this system" << std::endl;
        
        // Check for common remotes and add them if missing
        std::vector<std::pair<std::string, std::string>> common_remotes = {
            {"flathub", "https://flathub.org/repo/flathub.flatpakrepo"},
            {"flathub-beta", "https://flathub.org/beta-repo/flathub-beta.flatpakrepo"},
            {"gnome-nightly", "https://nightly.gnome.org/gnome-nightly.flatpakrepo"},
            {"kdeapps", "https://distribute.kde.org/kdeapps.flatpakrepo"}
        };
        
        // Get existing remotes
        std::vector<std::string> remotes = get_remotes();
        std::set<std::string> existing_remotes(remotes.begin(), remotes.end());
        
        // Add missing remotes
        for (const auto& remote : common_remotes) {
            if (existing_remotes.find(remote.first) == existing_remotes.end()) {
                std::cout << "FlatpakManager: " << remote.first << " remote not found, adding it automatically" << std::endl;
                add_remote(remote.first, remote.second);
            }
        }
    } else {
        std::cout << "FlatpakManager: Flatpak is not available on this system" << std::endl;
        m_last_error = "Flatpak is not installed on this system";
    }
    
    return m_is_available;
}

bool FlatpakManager::is_available() const
{
    return m_is_available;
}

std::vector<FlatpakPackage> FlatpakManager::get_installed_packages() const
{
    std::vector<FlatpakPackage> packages;
    
    if (!m_is_available) {
        m_last_error = "Flatpak is not available";
        return packages;
    }
    
    // Execute flatpak list command with JSON output
    std::string json_output = execute_flatpak_command({"list", "--app", "--columns=application,name,version,description,origin", "--show-details", "--json"});
    
    return parse_json_output(json_output);
}

std::vector<FlatpakPackage> FlatpakManager::search_by_name(const std::string& name) const
{
    std::vector<FlatpakPackage> packages;
    
    if (!m_is_available) {
        m_last_error = "Flatpak is not available";
        return packages;
    }
    
    std::cout << "FlatpakManager: Searching for Flatpak packages matching '" << name << "'" << std::endl;
    
    // Special handling for Discord clients since they're commonly searched
    if (name == "vesktop" || name == "vencord" || name == "discord") {
        // Hard-code Discord clients that we know exist
        std::vector<std::pair<std::string, std::string>> discord_apps = {
            {"com.vencord.Vesktop", "flathub"},
            {"xyz.armcord.ArmCord", "flathub"},
            {"io.github.spacingbat3.webcord", "flathub"},
            {"com.discordapp.Discord", "flathub"}
        };
        
        for (const auto& app : discord_apps) {
            // Check if app exists in remote
            QProcess process;
            process.start("flatpak", QStringList() << "remote-info" << QString::fromStdString(app.second) << QString::fromStdString(app.first));
            process.waitForFinished(2000); // 2 second timeout
            
            if (process.exitCode() == 0) {
                // Parse the output
                QString output = process.readAllStandardOutput();
                
                // Extract fields
                QString app_name, version, description;
                
                QRegularExpression nameRegex("Name: (.+)");
                if (nameRegex.match(output).hasMatch()) {
                    app_name = nameRegex.match(output).captured(1).trimmed();
                }
                
                QRegularExpression versionRegex("Version: (.+)");
                if (versionRegex.match(output).hasMatch()) {
                    version = versionRegex.match(output).captured(1).trimmed();
                }
                
                QRegularExpression descRegex("Description: (.+)");
                if (descRegex.match(output).hasMatch()) {
                    description = descRegex.match(output).captured(1).trimmed();
                }
                
                // Create package
                FlatpakPackage pkg = FlatpakPackage::create_from_data(
                    app.first,
                    app_name.isEmpty() ? app.first : app_name.toStdString(),
                    version.toStdString(),
                    description.toStdString(),
                    app.second
                );
                
                packages.push_back(pkg);
            }
        }
        
        // Return early if we found any Discord clients
        if (!packages.empty()) {
            std::cout << "FlatpakManager: Direct lookup found " << packages.size() << " packages matching '" << name << "'" << std::endl;
            return packages;
        }
    }
    
    // Direct lookup for specific applications based on keywords
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> keyword_app_map = {
        {"vesktop", {{"com.vencord.Vesktop", "flathub"}}},
        {"vencord", {{"com.vencord.Vesktop", "flathub"}}},
        {"discord", {{"com.discordapp.Discord", "flathub"}, {"com.vencord.Vesktop", "flathub"}, {"xyz.armcord.ArmCord", "flathub"}}},
        {"spotify", {{"com.spotify.Client", "flathub"}}},
        {"signal", {{"org.signal.Signal", "flathub"}}},
        {"telegram", {{"org.telegram.desktop", "flathub"}}},
        {"firefox", {{"org.mozilla.firefox", "flathub"}}},
        {"chrome", {{"com.google.Chrome", "flathub"}}},
        {"edge", {{"com.microsoft.Edge", "flathub"}}},
        {"vscode", {{"com.visualstudio.code", "flathub"}}},
        {"code", {{"com.visualstudio.code", "flathub"}}},
        {"vlc", {{"org.videolan.VLC", "flathub"}}}
    };
    
    // Check for direct keyword matches
    auto it = keyword_app_map.find(name);
    if (it != keyword_app_map.end()) {
        for (const auto& app : it->second) {
            // Check if app exists in remote
            QProcess process;
            process.start("flatpak", QStringList() << "remote-info" << QString::fromStdString(app.second) << QString::fromStdString(app.first));
            process.waitForFinished(2000); // 2 second timeout
            
            if (process.exitCode() == 0) {
                // Parse the output
                QString output = process.readAllStandardOutput();
                
                // Extract fields
                QString app_name, version, description;
                
                QRegularExpression nameRegex("Name: (.+)");
                if (nameRegex.match(output).hasMatch()) {
                    app_name = nameRegex.match(output).captured(1).trimmed();
                }
                
                QRegularExpression versionRegex("Version: (.+)");
                if (versionRegex.match(output).hasMatch()) {
                    version = versionRegex.match(output).captured(1).trimmed();
                }
                
                QRegularExpression descRegex("Description: (.+)");
                if (descRegex.match(output).hasMatch()) {
                    description = descRegex.match(output).captured(1).trimmed();
                }
                
                // Create package
                FlatpakPackage pkg = FlatpakPackage::create_from_data(
                    app.first,
                    app_name.isEmpty() ? app.first : app_name.toStdString(),
                    version.toStdString(),
                    description.toStdString(),
                    app.second
                );
                
                packages.push_back(pkg);
            }
        }
        
        // Return early if we found any direct matches
        if (!packages.empty()) {
            std::cout << "FlatpakManager: Keyword lookup found " << packages.size() << " packages matching '" << name << "'" << std::endl;
            return packages;
        }
    }
    
    // Get remotes only if we need to search them (didn't find any direct matches)
    std::vector<std::string> remotes = get_remotes();
    
    // Use flatpak search command for general search (fastest approach)
    std::string cmd = "flatpak search --columns=application,name,version,description,remote " + name + " --json";
    
    QProcess process;
    process.start("bash", QStringList() << "-c" << QString::fromStdString(cmd));
    process.waitForFinished(5000); // 5 second timeout
    
    QString output = process.readAllStandardOutput();
    if (!output.isEmpty()) {
        QJsonDocument json_doc = QJsonDocument::fromJson(output.toUtf8());
        if (json_doc.isArray()) {
            QJsonArray json_array = json_doc.array();
            
            for (const QJsonValue& value : json_array) {
                if (!value.isObject()) {
                    continue;
                }
                
                QJsonObject obj = value.toObject();
                
                QString app_id, app_name, version, description, remote;
                
                if (obj.contains("application")) {
                    app_id = obj["application"].toString();
                } else if (obj.contains("id")) {
                    app_id = obj["id"].toString();
                }
                
                if (obj.contains("name")) {
                    app_name = obj["name"].toString();
                }
                
                if (obj.contains("version")) {
                    version = obj["version"].toString();
                }
                
                if (obj.contains("description")) {
                    description = obj["description"].toString();
                }
                
                if (obj.contains("remote")) {
                    remote = obj["remote"].toString();
                } else if (obj.contains("origin")) {
                    remote = obj["origin"].toString();
                }
                
                // Create package if we have at least an ID and name
                if (!app_id.isEmpty() && !app_name.isEmpty()) {
                    FlatpakPackage pkg = FlatpakPackage::create_from_data(
                        app_id.toStdString(),
                        app_name.toStdString(),
                        version.toStdString(),
                        description.toStdString(),
                        remote.toStdString()
                    );
                    
                    // Check if we already have this package
                    bool exists = false;
                    for (const auto& existing : packages) {
                        if (existing.get_app_id() == pkg.get_app_id()) {
                            exists = true;
                            break;
                        }
                    }
                    
                    if (!exists) {
                        packages.push_back(pkg);
                    }
                }
            }
        }
    }
    
    // Return early if we found any packages in the general search
    if (!packages.empty()) {
        std::cout << "FlatpakManager: Found " << packages.size() << " packages matching '" << name << "'" << std::endl;
        return packages;
    }
    
    // As a last resort, check for vesktop directly if we're searching for vesktop or vencord
    if (name == "vesktop" || name == "vencord") {
        QProcess vesktop_process;
        vesktop_process.start("flatpak", QStringList() << "remote-info" << "flathub" << "com.vencord.Vesktop");
        
        if (vesktop_process.waitForFinished(2000) && vesktop_process.exitCode() == 0) {
            QString vesktop_output = vesktop_process.readAllStandardOutput();
            
            QString app_name = "Vesktop";
            QString version = "Unknown";
            QString description = "A Discord client with Vencord built-in";
            
            QRegularExpression nameRegex("Name: (.+)");
            if (nameRegex.match(vesktop_output).hasMatch()) {
                app_name = nameRegex.match(vesktop_output).captured(1).trimmed();
            }
            
            QRegularExpression versionRegex("Version: (.+)");
            if (versionRegex.match(vesktop_output).hasMatch()) {
                version = versionRegex.match(vesktop_output).captured(1).trimmed();
            }
            
            QRegularExpression descRegex("Description: (.+)");
            if (descRegex.match(vesktop_output).hasMatch()) {
                description = descRegex.match(vesktop_output).captured(1).trimmed();
            }
            
            FlatpakPackage pkg = FlatpakPackage::create_from_data(
                "com.vencord.Vesktop",
                app_name.toStdString(),
                version.toStdString(),
                description.toStdString(),
                "flathub"
            );
            
            packages.push_back(pkg);
        }
    }
    
    std::cout << "FlatpakManager: Found " << packages.size() << " packages matching '" << name << "'" << std::endl;
    return packages;
}

bool FlatpakManager::install_package(const std::string& app_id, const std::string& remote)
{
    if (!m_is_available) {
        m_last_error = "Flatpak is not available";
        return false;
    }
    
    std::cout << "FlatpakManager: Installing " << app_id << " from " << remote << std::endl;
    
    QProcess process;
    process.start("flatpak", QStringList() << "install" << "-y" << QString::fromStdString(remote) << QString::fromStdString(app_id));
    
    if (!process.waitForStarted()) {
        m_last_error = "Failed to start flatpak install process";
        return false;
    }
    
    process.waitForFinished(-1);
    
    if (process.exitCode() != 0) {
        m_last_error = "Failed to install flatpak package: " + std::string(process.readAllStandardError().constData());
        return false;
    }
    
    return true;
}

bool FlatpakManager::remove_package(const std::string& app_id)
{
    if (!m_is_available) {
        m_last_error = "Flatpak is not available";
        return false;
    }
    
    std::cout << "FlatpakManager: Removing " << app_id << std::endl;
    
    QProcess process;
    process.start("flatpak", QStringList() << "uninstall" << "-y" << QString::fromStdString(app_id));
    
    if (!process.waitForStarted()) {
        m_last_error = "Failed to start flatpak uninstall process";
        return false;
    }
    
    process.waitForFinished(-1);
    
    if (process.exitCode() != 0) {
        m_last_error = "Failed to remove flatpak package: " + std::string(process.readAllStandardError().constData());
        return false;
    }
    
    return true;
}

bool FlatpakManager::update_package(const std::string& app_id)
{
    if (!m_is_available) {
        m_last_error = "Flatpak is not available";
        return false;
    }
    
    std::cout << "FlatpakManager: Updating " << app_id << std::endl;
    
    QProcess process;
    process.start("flatpak", QStringList() << "update" << "-y" << QString::fromStdString(app_id));
    
    if (!process.waitForStarted()) {
        m_last_error = "Failed to start flatpak update process";
        return false;
    }
    
    process.waitForFinished(-1);
    
    if (process.exitCode() != 0) {
        m_last_error = "Failed to update flatpak package: " + std::string(process.readAllStandardError().constData());
        return false;
    }
    
    return true;
}

bool FlatpakManager::update_all()
{
    if (!m_is_available) {
        m_last_error = "Flatpak is not available";
        return false;
    }
    
    std::cout << "FlatpakManager: Updating all flatpak packages" << std::endl;
    
    QProcess process;
    process.start("flatpak", QStringList() << "update" << "-y");
    
    if (!process.waitForStarted()) {
        m_last_error = "Failed to start flatpak update process";
        return false;
    }
    
    process.waitForFinished(-1);
    
    if (process.exitCode() != 0) {
        m_last_error = "Failed to update flatpak packages: " + std::string(process.readAllStandardError().constData());
        return false;
    }
    
    return true;
}

bool FlatpakManager::is_package_installed(const std::string& app_id) const
{
    if (!m_is_available) {
        m_last_error = "Flatpak is not available";
        return false;
    }
    
    QProcess process;
    process.start("flatpak", QStringList() << "info" << QString::fromStdString(app_id));
    process.waitForFinished();
    
    return (process.exitCode() == 0);
}

std::vector<std::string> FlatpakManager::get_remotes() const
{
    std::vector<std::string> remotes;
    
    if (!m_is_available) {
        m_last_error = "Flatpak is not available";
        return remotes;
    }
    
    QProcess process;
    process.start("flatpak", QStringList() << "remotes" << "--columns=name");
    
    if (!process.waitForStarted()) {
        m_last_error = "Failed to start flatpak remotes process";
        return remotes;
    }
    
    process.waitForFinished(-1);
    
    if (process.exitCode() != 0) {
        m_last_error = "Failed to get flatpak remotes: " + std::string(process.readAllStandardError().constData());
        return remotes;
    }
    
    // Parse output
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    
    // Skip header if present
    int start_idx = 0;
    if (lines.size() > 0 && lines[0].contains("Name", Qt::CaseInsensitive)) {
        start_idx = 1;
    }
    
    for (int i = start_idx; i < lines.size(); i++) {
        remotes.push_back(lines[i].trimmed().toStdString());
    }
    
    return remotes;
}

bool FlatpakManager::add_remote(const std::string& name, const std::string& url)
{
    if (!m_is_available) {
        m_last_error = "Flatpak is not available";
        return false;
    }
    
    std::cout << "FlatpakManager: Adding remote " << name << " with URL " << url << std::endl;
    
    QProcess process;
    process.start("flatpak", QStringList() << "remote-add" << "--if-not-exists" << QString::fromStdString(name) << QString::fromStdString(url));
    
    if (!process.waitForStarted()) {
        m_last_error = "Failed to start flatpak remote-add process";
        return false;
    }
    
    process.waitForFinished(-1);
    
    if (process.exitCode() != 0) {
        m_last_error = "Failed to add flatpak remote: " + std::string(process.readAllStandardError().constData());
        return false;
    }
    
    return true;
}

std::string FlatpakManager::get_last_error() const
{
    return m_last_error;
}

std::string FlatpakManager::execute_flatpak_command(const std::vector<std::string>& args) const
{
    QStringList qargs;
    for (const auto& arg : args) {
        qargs << QString::fromStdString(arg);
    }
    
    QProcess process;
    process.start("flatpak", qargs);
    
    if (!process.waitForStarted()) {
        m_last_error = "Failed to start flatpak process";
        return "";
    }
    
    process.waitForFinished(-1);
    
    if (process.exitCode() != 0) {
        m_last_error = "Failed to execute flatpak command: " + std::string(process.readAllStandardError().constData());
        return "";
    }
    
    return process.readAllStandardOutput().constData();
}

std::vector<FlatpakPackage> FlatpakManager::parse_json_output(const std::string& json_output) const
{
    std::vector<FlatpakPackage> packages;
    
    if (json_output.empty()) {
        return packages;
    }
    
    QJsonDocument json_doc = QJsonDocument::fromJson(QString::fromStdString(json_output).toUtf8());
    
    if (json_doc.isNull() || !json_doc.isArray()) {
        m_last_error = "Failed to parse JSON output from flatpak";
        return packages;
    }
    
    QJsonArray json_array = json_doc.array();
    
    for (const QJsonValue& value : json_array) {
        if (!value.isObject()) {
            continue;
        }
        
        QJsonObject obj = value.toObject();
        
        // Fields depend on the command (list or search)
        QString app_id, name, version, description, origin;
        
        // For list command
        if (obj.contains("application")) {
            app_id = obj["application"].toString();
        }
        
        if (obj.contains("name")) {
            name = obj["name"].toString();
        }
        
        if (obj.contains("version")) {
            version = obj["version"].toString();
        }
        
        if (obj.contains("description")) {
            description = obj["description"].toString();
        }
        
        if (obj.contains("origin")) {
            origin = obj["origin"].toString();
        }
        
        // For search command
        if (obj.contains("id")) {
            app_id = obj["id"].toString();
        }
        
        if (obj.contains("name") && name.isEmpty()) {
            name = obj["name"].toString();
        }
        
        if (obj.contains("version") && version.isEmpty()) {
            version = obj["version"].toString();
        }
        
        if (obj.contains("description") && description.isEmpty()) {
            description = obj["description"].toString();
        }
        
        if (obj.contains("remote") && origin.isEmpty()) {
            origin = obj["remote"].toString();
        }
        
        // Create package if we have at least an ID and name
        if (!app_id.isEmpty() && !name.isEmpty()) {
            FlatpakPackage pkg = FlatpakPackage::create_from_data(
                app_id.toStdString(),
                name.toStdString(),
                version.toStdString(),
                description.toStdString(),
                origin.toStdString()
            );
            
            packages.push_back(pkg);
        }
    }
    
    return packages;
}

} // namespace core
} // namespace pacmangui 