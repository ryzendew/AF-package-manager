#include "aur/aur_manager.hpp"
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>

namespace pacmangui {
namespace aur {

AURManager::AURManager()
    : m_helper()
    , m_last_error("")
    , m_initialized(false)
{
}

AURManager::~AURManager()
{
}

bool AURManager::initialize()
{
    if (m_initialized) {
        return true;
    }

    // Initialize AUR helper
    if (!m_helper.detect_helpers()) {
        set_last_error("No AUR helpers found");
        return false;
    }

    // Load installed AUR packages
    refresh_installed_packages();

    m_initialized = true;
    return true;
}

std::vector<AURPackage> AURManager::search(const std::string& search_term)
{
    if (!m_initialized) {
        set_last_error("AUR manager not initialized");
        return {};
    }

    // Query AUR RPC API
    std::string json_data = query_aur_rpc("search", search_term);
    if (json_data.empty()) {
        return {};
    }

    // Parse results
    return parse_package_list(json_data);
}

AURPackage AURManager::get_package_info(const std::string& package_name)
{
    if (!m_initialized) {
        set_last_error("AUR manager not initialized");
        return AURPackage();
    }

    // Query AUR RPC API
    std::string json_data = query_aur_rpc("info", package_name);
    if (json_data.empty()) {
        return AURPackage();
    }

    // Parse result
    std::vector<AURPackage> packages = parse_package_list(json_data);
    if (packages.empty()) {
        return AURPackage();
    }

    return packages[0];
}

bool AURManager::install_package(const std::string& package_name,
                               const std::string& password,
                               std::function<void(const std::string&)> output_callback)
{
    if (!m_initialized) {
        set_last_error("AUR manager not initialized");
        return false;
    }

    // Get install command from helper
    std::string cmd = m_helper.get_install_command(package_name);
    if (cmd.empty()) {
        set_last_error("Failed to get install command");
        return false;
    }

    // Create temporary file for output
    std::string temp_output_file = "/tmp/pacmangui_aur_install_output.txt";
    cmd += " 2>&1 | tee " + temp_output_file;

    // Execute command with sudo
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);

    // Start the process
    process.start("bash", QStringList() << "-c" << QString::fromStdString(cmd));

    // Read output in real-time
    while (process.state() != QProcess::NotRunning) {
        process.waitForReadyRead();
        if (output_callback) {
            QString output = process.readAll();
            output_callback(output.toStdString());
        }
    }

    process.waitForFinished(-1);
    bool success = (process.exitCode() == 0);

    if (!success) {
        set_last_error("Failed to install AUR package: " + package_name);
    } else {
        // Update installed packages list
        refresh_installed_packages();
    }

    return success;
}

bool AURManager::remove_package(const std::string& package_name,
                              const std::string& password,
                              std::function<void(const std::string&)> output_callback)
{
    if (!m_initialized) {
        set_last_error("AUR manager not initialized");
        return false;
    }

    // Get remove command from helper
    std::string cmd = m_helper.get_remove_command(package_name);
    if (cmd.empty()) {
        set_last_error("Failed to get remove command");
        return false;
    }

    // Create temporary file for output
    std::string temp_output_file = "/tmp/pacmangui_aur_remove_output.txt";
    cmd += " 2>&1 | tee " + temp_output_file;

    // Execute command with sudo
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);

    // Start the process
    process.start("bash", QStringList() << "-c" << QString::fromStdString(cmd));

    // Read output in real-time
    while (process.state() != QProcess::NotRunning) {
        process.waitForReadyRead();
        if (output_callback) {
            QString output = process.readAll();
            output_callback(output.toStdString());
        }
    }

    process.waitForFinished(-1);
    bool success = (process.exitCode() == 0);

    if (!success) {
        set_last_error("Failed to remove AUR package: " + package_name);
    } else {
        // Update installed packages list
        refresh_installed_packages();
    }

    return success;
}

bool AURManager::update_all(const std::string& password,
                          std::function<void(const std::string&)> output_callback)
{
    if (!m_initialized) {
        set_last_error("AUR manager not initialized");
        return false;
    }

    // Get update command from helper
    std::string cmd = m_helper.get_update_command();
    if (cmd.empty()) {
        set_last_error("Failed to get update command");
        return false;
    }

    // Create temporary file for output
    std::string temp_output_file = "/tmp/pacmangui_aur_update_output.txt";
    cmd += " 2>&1 | tee " + temp_output_file;

    // Execute command with sudo
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);

    // Start the process
    process.start("bash", QStringList() << "-c" << QString::fromStdString(cmd));

    // Read output in real-time
    while (process.state() != QProcess::NotRunning) {
        process.waitForReadyRead();
        if (output_callback) {
            QString output = process.readAll();
            output_callback(output.toStdString());
        }
    }

    process.waitForFinished(-1);
    bool success = (process.exitCode() == 0);

    if (!success) {
        set_last_error("Failed to update AUR packages");
    } else {
        // Update installed packages list
        refresh_installed_packages();
    }

    return success;
}

std::vector<AURPackage> AURManager::check_updates()
{
    std::vector<AURPackage> updates;

    if (!m_initialized) {
        set_last_error("AUR manager not initialized");
        return updates;
    }

    // Get list of installed AUR packages
    for (const auto& pkg : m_installed_packages) {
        // Get latest version from AUR
        AURPackage aur_pkg = get_package_info(pkg.get_name());
        if (!aur_pkg.get_name().empty() && aur_pkg.get_version() != pkg.get_version()) {
            updates.push_back(aur_pkg);
        }
    }

    return updates;
}

std::vector<AURPackage> AURManager::get_installed_packages() const
{
    return m_installed_packages;
}

std::string AURManager::get_current_helper() const
{
    return m_helper.get_current_helper();
}

bool AURManager::set_helper(const std::string& helper_name)
{
    return m_helper.set_helper(helper_name);
}

std::vector<std::string> AURManager::get_available_helpers() const
{
    return m_helper.get_available_helpers();
}

std::string AURManager::get_last_error() const
{
    return m_last_error;
}

void AURManager::set_last_error(const std::string& error)
{
    m_last_error = error;
    std::cerr << "AURManager: " << error << std::endl;
}

std::string AURManager::query_aur_rpc(const std::string& type, const std::string& arg) const
{
    QNetworkAccessManager manager;
    QEventLoop loop;

    // Construct URL
    QUrl url("https://aur.archlinux.org/rpc/");
    QUrlQuery query;
    query.addQueryItem("v", "5");
    query.addQueryItem("type", QString::fromStdString(type));
    query.addQueryItem("arg", QString::fromStdString(arg));
    url.setQuery(query);

    // Create request
    QNetworkRequest request(url);
    QNetworkReply* reply = manager.get(request);

    // Wait for response
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        const_cast<AURManager*>(this)->set_last_error("Network error: " + reply->errorString().toStdString());
        reply->deleteLater();
        return "";
    }

    // Read response
    QString response = reply->readAll();
    reply->deleteLater();

    return response.toStdString();
}

std::vector<AURPackage> AURManager::parse_package_list(const std::string& json_data) const
{
    std::vector<AURPackage> packages;

    try {
        QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(json_data).toUtf8());
        if (doc.isNull()) {
            const_cast<AURManager*>(this)->set_last_error("Invalid JSON response");
            return packages;
        }

        QJsonObject root = doc.object();
        if (root["type"].toString() != "search" && root["type"].toString() != "info") {
            const_cast<AURManager*>(this)->set_last_error("Unexpected response type");
            return packages;
        }

        QJsonArray results = root["results"].toArray();
        for (const QJsonValue& val : results) {
            QJsonObject obj = val.toObject();
            
            AURPackage pkg;
            pkg.set_name(obj["Name"].toString().toStdString());
            pkg.set_version(obj["Version"].toString().toStdString());
            pkg.set_description(obj["Description"].toString().toStdString());
            pkg.set_maintainer(obj["Maintainer"].toString().toStdString());
            pkg.set_votes(obj["NumVotes"].toInt());
            pkg.set_popularity(obj["Popularity"].toDouble());
            pkg.set_git_url(obj["URL"].toString().toStdString());
            pkg.set_last_updated(obj["LastModified"].toInteger());

            if (obj.contains("Depends")) {
                QJsonArray deps = obj["Depends"].toArray();
                std::vector<std::string> dependencies;
                for (const QJsonValue& dep : deps) {
                    dependencies.push_back(dep.toString().toStdString());
                }
                pkg.set_dependencies(dependencies);
            }

            // Check if package is installed
            for (const auto& installed : m_installed_packages) {
                if (installed.get_name() == pkg.get_name()) {
                    pkg.set_installed(true);
                    pkg.set_installed_version(installed.get_version());
                    break;
                }
            }

            packages.push_back(pkg);
        }
    } catch (const std::exception& e) {
        const_cast<AURManager*>(this)->set_last_error("Failed to parse JSON: " + std::string(e.what()));
        return std::vector<AURPackage>();
    }

    return packages;
}

void AURManager::refresh_installed_packages()
{
    m_installed_packages.clear();

    // Get list of installed packages that are from AUR
    QProcess process;
    process.start("pacman", QStringList() << "-Qm");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QStringList parts = line.split(" ");
        if (parts.size() >= 2) {
            AURPackage pkg;
            pkg.set_name(parts[0].toStdString());
            pkg.set_version(parts[1].toStdString());
            pkg.set_installed(true);
            pkg.set_installed_version(parts[1].toStdString());
            m_installed_packages.push_back(pkg);
        }
    }
}

} // namespace aur
} // namespace pacmangui 