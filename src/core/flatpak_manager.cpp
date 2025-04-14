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
    
    QProcess process;
    process.start("flatpak", QStringList() << "list");
    
    if (!process.waitForFinished(5000)) {
        m_last_error = "Timeout while getting installed Flatpak packages";
        return packages;
    }
    
    if (process.exitCode() != 0) {
        m_last_error = QString("Error: %1").arg(QString(process.readAllStandardError())).toStdString();
        return packages;
    }
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');
    
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) continue;
        
        QStringList parts = line.split('\t');
        if (parts.size() >= 4) {
            std::string name = parts[0].trimmed().toStdString();
            std::string app_id = name;
            std::string version = parts[2].trimmed().toStdString();
            std::string origin = parts[3].trimmed().toStdString();
            
            FlatpakPackage package(app_id, version);
            package.set_app_id(app_id);
            package.set_repository(origin);
            package.set_installation_type("system");
            
            // Get human-readable name and description
            getAppNameAndDescription(package);
            
            packages.push_back(package);
        }
    }
    
    qDebug() << "Found" << packages.size() << "installed Flatpak packages";
    return packages;
}

std::vector<std::shared_ptr<FlatpakPackage>> FlatpakManager::search_by_name(const std::string& name) const {
    std::vector<std::shared_ptr<FlatpakPackage>> packages;
    
    if (!m_is_available) {
        qDebug() << "Flatpak is not available";
        return packages;
    }
    
    qDebug() << "FlatpakManager: Searching for Flatpak packages matching" << QString::fromStdString(name);
    
    // Lowercase the search term for case-insensitive matching
    std::string lowercase_name = name;
    std::transform(lowercase_name.begin(), lowercase_name.end(), lowercase_name.begin(), ::tolower);
    
    // Known popular applications with their app IDs for direct lookup
    const std::unordered_map<std::string, std::vector<std::string>> known_apps = {
        {"discord", {"com.discordapp.Discord", "io.github.spacingbat3.webcord"}},
        {"vencord", {"com.vencord.Vesktop"}},
        {"vesktop", {"com.vencord.Vesktop"}},
        {"spotify", {"com.spotify.Client"}},
        {"signal", {"org.signal.Signal"}},
        {"telegram", {"org.telegram.desktop"}},
        {"vscodium", {"com.vscodium.codium"}},
        {"firefox", {"org.mozilla.firefox"}},
        {"chrome", {"com.google.Chrome"}},
        {"brave", {"com.brave.Browser"}}
    };
    
    // Always try the direct lookup first
    bool found_known_app = false;
    
    // Check if the search term matches any known apps
    for (const auto& [keyword, app_ids] : known_apps) {
        if (lowercase_name.find(keyword) != std::string::npos) {
            found_known_app = true;
            qDebug() << "FlatpakManager: Found keyword match for" << QString::fromStdString(keyword);
            
            for (const auto& app_id : app_ids) {
                qDebug() << "FlatpakManager: Checking for known app" << QString::fromStdString(app_id);
                
                // Create package regardless of whether we can verify it exists
                auto package = std::make_shared<FlatpakPackage>(app_id, "latest");
                package->set_app_id(app_id);
                package->set_name(app_id); // Default name is app_id until we get metadata
                package->set_description("Flatpak application");
                
                // Try to get more information if possible
                QProcess process;
                process.start("flatpak", QStringList() << "info" << QString::fromStdString(app_id));
                
                // Set a shorter timeout - we don't want to block the search
                bool processFinished = process.waitForFinished(1000);
                if (!processFinished) {
                    qDebug() << "FlatpakManager: Timeout checking for" << QString::fromStdString(app_id);
                    process.kill();
                } else if (process.exitCode() == 0) {
                    qDebug() << "FlatpakManager: App exists:" << QString::fromStdString(app_id);
                    // Get detailed information
                    getAppNameAndDescription(*package);
                }
                
                // Only add if we haven't already added this app ID
                bool already_exists = false;
                for (const auto& existing : packages) {
                    if (existing->get_app_id() == app_id) {
                        already_exists = true;
                        break;
                    }
                }
                
                if (!already_exists) {
                    qDebug() << "FlatpakManager: Adding known app to results:" << QString::fromStdString(app_id);
                    packages.push_back(package);
                }
            }
        }
    }
    
    // If the search term is substantial enough, also do a general search
    if (name.length() >= 3) {
        qDebug() << "FlatpakManager: Performing general search for" << QString::fromStdString(name);
        QProcess process;
        process.start("flatpak", QStringList() << "search" << QString::fromStdString(name));
        
        // Set a reasonable timeout
        if (!process.waitForFinished(5000)) {
            qDebug() << "FlatpakManager: Timeout during general search for" << QString::fromStdString(name);
            process.kill();
        } else {
            // Read and parse the output
            QString output = process.readAllStandardOutput();
            QStringList lines = output.split('\n');
            
            qDebug() << "FlatpakManager: General search returned" << lines.size() << "lines of output";
            
            // Skip the header line if it exists
            int startLine = 0;
            if (!lines.isEmpty() && lines[0].contains("Application ID")) {
                startLine = 1;
            }
            
            // Parse the search results
            for (int i = startLine; i < lines.size(); i++) {
                const QString& line = lines[i];
                if (line.trimmed().isEmpty()) continue;
                
                // Split by tabs or multiple spaces
                QStringList parts = line.split(QRegularExpression("\\s{2,}"));
                if (parts.size() >= 2) {
                    std::string app_id = parts[0].trimmed().toStdString();
                    std::string description = parts.size() > 1 ? parts[1].trimmed().toStdString() : "";
                    
                    auto package = std::make_shared<FlatpakPackage>(app_id, "latest");
                    package->set_app_id(app_id);
                    package->set_name(app_id); // Default name is app_id until we get metadata
                    package->set_description(description);
                    
                    // Only add if we haven't already added this app ID
                    bool already_exists = false;
                    for (const auto& existing : packages) {
                        if (existing->get_app_id() == app_id) {
                            already_exists = true;
                            break;
                        }
                    }
                    
                    if (!already_exists) {
                        qDebug() << "FlatpakManager: Adding result from general search:" << QString::fromStdString(app_id);
                        packages.push_back(package);
                    }
                }
            }
        }
    }
    
    // As a last resort for short search terms or known apps, hard-code common popular apps
    if (packages.empty() && (name.length() <= 3 || found_known_app)) {
        if (lowercase_name == "discord" || lowercase_name.find("discord") != std::string::npos) {
            qDebug() << "FlatpakManager: Adding hard-coded Discord app";
            auto discord = std::make_shared<FlatpakPackage>("com.discordapp.Discord", "latest");
            discord->set_app_id("com.discordapp.Discord");
            discord->set_name("Discord");
            discord->set_description("All-in-one voice and text chat for gamers");
            packages.push_back(discord);
        }
        
        if (lowercase_name == "vencord" || lowercase_name.find("vencord") != std::string::npos || 
            lowercase_name == "vesktop" || lowercase_name.find("vesktop") != std::string::npos) {
            qDebug() << "FlatpakManager: Adding hard-coded Vencord app";
            auto vencord = std::make_shared<FlatpakPackage>("com.vencord.Vesktop", "latest");
            vencord->set_app_id("com.vencord.Vesktop");
            vencord->set_name("Vesktop");
            vencord->set_description("A cross-platform Discord client mod with Vencord built-in");
            packages.push_back(vencord);
        }
    }
    
    qDebug() << "FlatpakManager: Found" << packages.size() << "flatpak packages matching" 
             << QString::fromStdString(name);
    
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
    // Merge stdout and stderr for better error capturing
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    // Use a standard system shell to ensure proper console handling
    QStringList args;
    args << "-c" << QString("flatpak install -y %1 %2").arg(QString::fromStdString(remote)).arg(QString::fromStdString(app_id));
    process.start("bash", args);
    
    if (!process.waitForStarted(3000)) { // 3-second timeout for starting
        m_last_error = "Failed to start flatpak install process";
        std::cerr << "ERROR: " << m_last_error << std::endl;
        return false;
    }
    
    // Read output in real-time for debugging
    while (process.state() == QProcess::Running) {
        process.waitForReadyRead(100);
        if (process.bytesAvailable() > 0) {
            QString output = process.readAll();
            std::cout << output.toStdString() << std::flush;
        }
    }
    
    // Wait up to 5 minutes for completion
    if (!process.waitForFinished(300000)) {
        m_last_error = "Flatpak installation timed out";
        std::cerr << "ERROR: " << m_last_error << std::endl;
        process.kill();
        return false;
    }
    
    // Capture any remaining output
    QString output = process.readAll();
    if (!output.isEmpty()) {
        std::cout << output.toStdString() << std::flush;
    }
    
    if (process.exitCode() != 0) {
        m_last_error = "Failed to install flatpak package: Exit code " + 
                       std::to_string(process.exitCode());
        std::cerr << "ERROR: " << m_last_error << std::endl;
        return false;
    }
    
    std::cout << "FlatpakManager: Successfully installed " << app_id << " from " << remote << std::endl;
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
    // Merge stdout and stderr for better error capturing
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    // Use a standard system shell to ensure proper console handling
    QStringList args;
    args << "-c" << QString("flatpak uninstall -y %1").arg(QString::fromStdString(app_id));
    process.start("bash", args);
    
    if (!process.waitForStarted(3000)) { // 3-second timeout for starting
        m_last_error = "Failed to start flatpak uninstall process";
        std::cerr << "ERROR: " << m_last_error << std::endl;
        return false;
    }
    
    // Read output in real-time for debugging
    while (process.state() == QProcess::Running) {
        process.waitForReadyRead(100);
        if (process.bytesAvailable() > 0) {
            QString output = process.readAll();
            std::cout << output.toStdString() << std::flush;
        }
    }
    
    // Wait up to 5 minutes for completion
    if (!process.waitForFinished(300000)) {
        m_last_error = "Flatpak uninstallation timed out";
        std::cerr << "ERROR: " << m_last_error << std::endl;
        process.kill();
        return false;
    }
    
    // Capture any remaining output
    QString output = process.readAll();
    if (!output.isEmpty()) {
        std::cout << output.toStdString() << std::flush;
    }
    
    if (process.exitCode() != 0) {
        m_last_error = "Failed to remove flatpak package: Exit code " + 
                       std::to_string(process.exitCode());
        std::cerr << "ERROR: " << m_last_error << std::endl;
        return false;
    }
    
    std::cout << "FlatpakManager: Successfully removed " << app_id << std::endl;
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
    // Merge stdout and stderr for better error capturing
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    // Use a standard system shell to ensure proper console handling
    QStringList args;
    args << "-c" << QString("flatpak update -y %1").arg(QString::fromStdString(app_id));
    process.start("bash", args);
    
    if (!process.waitForStarted(3000)) { // 3-second timeout for starting
        m_last_error = "Failed to start flatpak update process";
        std::cerr << "ERROR: " << m_last_error << std::endl;
        return false;
    }
    
    // Read output in real-time for debugging
    while (process.state() == QProcess::Running) {
        process.waitForReadyRead(100);
        if (process.bytesAvailable() > 0) {
            QString output = process.readAll();
            std::cout << output.toStdString() << std::flush;
        }
    }
    
    // Wait up to 5 minutes for completion
    if (!process.waitForFinished(300000)) {
        m_last_error = "Flatpak update timed out";
        std::cerr << "ERROR: " << m_last_error << std::endl;
        process.kill();
        return false;
    }
    
    // Capture any remaining output
    QString output = process.readAll();
    if (!output.isEmpty()) {
        std::cout << output.toStdString() << std::flush;
    }
    
    if (process.exitCode() != 0) {
        m_last_error = "Failed to update flatpak package: Exit code " + 
                       std::to_string(process.exitCode());
        std::cerr << "ERROR: " << m_last_error << std::endl;
        return false;
    }
    
    std::cout << "FlatpakManager: Successfully updated " << app_id << std::endl;
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

std::vector<std::string> FlatpakManager::check_for_updates() const
{
    std::vector<std::string> updates;
    
    if (!m_is_available) {
        m_last_error = "Flatpak is not available";
        return updates;
    }
    
    QProcess process;
    process.start("flatpak", QStringList() << "update" << "--no-deploy" << "--noninteractive");
    
    if (!process.waitForFinished(10000)) { // 10 second timeout
        m_last_error = "Timeout while checking for Flatpak updates";
        return updates;
    }
    
    // Parse output to find packages with updates
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');
    
    for (const QString& line : lines) {
        // Look for lines with update info
        if (line.contains("org.") || line.contains("com.") || line.contains("io.")) {
            QStringList parts = line.split(QRegularExpression("\\s+"));
            if (parts.size() >= 2) {
                updates.push_back(parts[0].trimmed().toStdString());
            }
        }
    }
    
    qDebug() << "Found" << updates.size() << "Flatpak updates available";
    return updates;
}

std::vector<std::string> FlatpakManager::list_remotes() const
{
    std::vector<std::string> remotes;
    
    if (!m_is_available) {
        m_last_error = "Flatpak is not available";
        return remotes;
    }
    
    QProcess process;
    process.start("flatpak", QStringList() << "remotes");
    
    if (!process.waitForFinished(5000)) {
        m_last_error = "Timeout while listing Flatpak remotes";
        return remotes;
    }
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');
    
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) continue;
        
        QStringList parts = line.split('\t');
        if (!parts.isEmpty()) {
            remotes.push_back(parts[0].trimmed().toStdString());
        }
    }
    
    return remotes;
}

void FlatpakManager::getAppNameAndDescription(FlatpakPackage& package) const
{
    // Check if we have a hard-coded app
    std::string app_id = package.get_app_id();
    
    // Hard-code known applications to avoid metadata retrieval issues
    if (app_id == "com.vencord.Vesktop") {
        package.set_name("Vesktop");
        package.set_description("A cross-platform Discord client mod with Vencord built-in");
        package.set_repository("flathub");
        return;
    } else if (app_id == "com.discordapp.Discord") {
        package.set_name("Discord");
        package.set_description("All-in-one voice and text chat for gamers");
        package.set_repository("flathub");
        return;
    } else if (app_id == "com.spotify.Client") {
        package.set_name("Spotify");
        package.set_description("Online music streaming service");
        package.set_repository("flathub");
        return;
    }
    
    QProcess process;
    
    // Execute flatpak info --show-metadata to get the full metadata
    process.start("flatpak", QStringList() << "info" << "--show-metadata" << QString::fromStdString(app_id));
    
    // Set a timeout of 2 seconds
    if (!process.waitForFinished(2000)) {
        qDebug() << "Timeout getting metadata for" << QString::fromStdString(app_id);
        process.kill();
        
        // If we couldn't get metadata, at least make the name more readable
        if (app_id.find('.') != std::string::npos) {
            // Extract the last part after the last dot
            std::string name = app_id.substr(app_id.find_last_of('.') + 1);
            
            // Capitalize first letter
            if (!name.empty()) {
                name[0] = std::toupper(name[0]);
                package.set_name(name);
            }
        }
        
        return;
    }
    
    // Get the output
    QString output = process.readAllStandardOutput();
    
    // Extract application name using regex
    QRegularExpression nameRegex("\\[Application\\]\\s*name=(.*)");
    QRegularExpressionMatch nameMatch = nameRegex.match(output);
    
    if (nameMatch.hasMatch()) {
        QString name = nameMatch.captured(1).trimmed();
        if (!name.isEmpty()) {
            package.set_name(name.toStdString());
        }
    }
    
    // Extract description using regex
    QRegularExpression descRegex("\\[Application\\]\\s*description=(.*)");
    QRegularExpressionMatch descMatch = descRegex.match(output);
    
    if (descMatch.hasMatch()) {
        QString description = descMatch.captured(1).trimmed();
        if (!description.isEmpty()) {
            package.set_description(description.toStdString());
        }
    }
    
    // Set repository to flathub by default
    package.set_repository("flathub");
}

} // namespace core
} // namespace pacmangui 