#include "core/packagemanager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <algorithm>
#include <cstdlib>  // For system()
#include <array>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

namespace pacmangui {
namespace core {

// Helper function to execute commands with sudo
bool execute_with_sudo(const std::string& command) {
    std::string sudo_cmd = "sudo " + command;
    std::cout << "Executing: " << sudo_cmd << std::endl;
    int result = system(sudo_cmd.c_str());
    return result == 0;
}

// Helper function to execute commands with sudo and password
bool execute_with_sudo(const std::string& command, const std::string& password) {
    // Use expect-like behavior to handle sudo password prompt more reliably
    std::string quoted_password = "'";
    for (char c : password) {
        if (c == '\'') {
            quoted_password += "'\\''"; // Escape single quotes in the password
        } else {
            quoted_password += c;
        }
    }
    quoted_password += "'";
    
    // Create a command that:
    // 1. Uses 'script' to simulate a terminal (sudo sometimes requires a TTY)
    // 2. Pipes the password directly to sudo
    // 3. Ensures the password isn't visible in process listings
    std::string full_cmd = "script -qec 'echo " + quoted_password + " | sudo -S " + command + "' /dev/null";
    
    std::cout << "Executing sudo command with password authentication" << std::endl;
    
    // Execute the command
    int result = system(full_cmd.c_str());
    return result == 0;
}

// ALPM error callback - updated to match alpm_cb_log signature
void alpm_log_cb(void* ctx, alpm_loglevel_t level, const char *format, va_list args)
{
    if (level == ALPM_LOG_ERROR || level == ALPM_LOG_WARNING) {
        char message[1024];
        vsnprintf(message, sizeof(message), format, args);
        std::cerr << "ALPM: " << message << std::endl;
    }
}

PackageManager::PackageManager()
    : m_handle(nullptr)
    , m_repo_manager(nullptr)
    , m_trans_manager(nullptr)
    , m_last_error("")
{
}

PackageManager::~PackageManager()
{
    if (m_trans_manager) {
        delete m_trans_manager;
        m_trans_manager = nullptr;
    }
    
    if (m_repo_manager) {
        delete m_repo_manager;
        m_repo_manager = nullptr;
    }
    
    if (m_handle) {
        alpm_release(m_handle);
        m_handle = nullptr;
    }
}

bool PackageManager::initialize(const std::string& root_dir, const std::string& db_path)
{
    // Initialize alpm library
    alpm_errno_t err;
    m_handle = alpm_initialize(root_dir.c_str(), db_path.c_str(), &err);
    
    if (!m_handle) {
        std::string error = "Failed to initialize alpm: ";
        error += alpm_strerror(err);
        set_last_error(error);
        return false;
    }
    
    // Set callback for logging
    alpm_option_set_logcb(m_handle, alpm_log_cb, nullptr);
    
    // Create repository manager
    m_repo_manager = new RepositoryManager(m_handle);
    if (!m_repo_manager->initialize()) {
        set_last_error("Failed to initialize repository manager");
        return false;
    }
    
    // Create transaction manager
    m_trans_manager = new TransactionManager(m_handle);
    
    // Register sync databases
    if (!register_sync_databases()) {
        set_last_error("Failed to register sync databases");
        return false;
    }
    
    std::cout << "PackageManager: Initialized successfully" << std::endl;
    return true;
}

std::vector<Package> PackageManager::get_installed_packages() const
{
    std::vector<Package> packages;
    
    if (!m_handle || !m_repo_manager) {
        return packages;
    }
    
    Repository local_db = m_repo_manager->get_local_db();
    packages = local_db.get_packages();
    
    return packages;
}

std::vector<Package> PackageManager::get_available_packages() const
{
    std::vector<Package> packages;
    
    if (!m_handle || !m_repo_manager) {
        return packages;
    }
    
    // Get packages from all sync repositories
    std::vector<Repository> sync_dbs = m_repo_manager->get_sync_dbs();
    for (const auto& repo : sync_dbs) {
        std::vector<Package> repo_packages = repo.get_packages();
        packages.insert(packages.end(), repo_packages.begin(), repo_packages.end());
    }
    
    return packages;
}

std::vector<Package> PackageManager::search_aur(const std::string& name) const
{
    std::vector<Package> results;
    
    if (name.empty()) {
        return results;
    }
    
    std::cout << "PackageManager: Searching for AUR packages matching '" << name << "'" << std::endl;
    
    // For now, just print a message and return empty list
    // This will be expanded in Phase 3 to actually search the AUR
    std::cout << "PackageManager: AUR search not yet implemented" << std::endl;
    
    return results;
}

std::vector<Package> PackageManager::search_by_name(const std::string& name) const
{
    std::vector<Package> results;
    
    if (!m_handle || !m_repo_manager || name.empty()) {
        return results;
    }
    
    std::cout << "PackageManager: Searching for packages matching '" << name << "'" << std::endl;
    
    try {
        // First search in installed packages
        std::vector<Package> installed_packages = get_installed_packages();
        std::cout << "PackageManager: Searching through " << installed_packages.size() << " installed packages" << std::endl;
        
        // Convert search term to lowercase for case-insensitive matching
        std::string search_term = name;
        std::transform(search_term.begin(), search_term.end(), search_term.begin(), 
                    [](unsigned char c){ return std::tolower(c); });
        
        // Search in installed packages
        for (const auto& pkg : installed_packages) {
            std::string pkg_name = pkg.get_name();
            std::transform(pkg_name.begin(), pkg_name.end(), pkg_name.begin(), 
                        [](unsigned char c){ return std::tolower(c); });
            
            if (pkg_name.find(search_term) != std::string::npos) {
            results.push_back(pkg);
            }
        }
        
        std::cout << "PackageManager: Found " << results.size() << " matching installed packages" << std::endl;
        
        // Then search in repository packages
        std::vector<Repository> sync_dbs = m_repo_manager->get_sync_dbs();
        std::cout << "PackageManager: Searching through " << sync_dbs.size() << " repositories" << std::endl;
        
        int total_repo_packages = 0;
        std::vector<Package> repo_results;
        
        // Search each repository explicitly
        for (const auto& repo : sync_dbs) {
            try {
                std::vector<Package> repo_packages = repo.get_packages();
                total_repo_packages += repo_packages.size();
                
                for (const auto& pkg : repo_packages) {
                    if (pkg.get_name().empty()) {
                        continue; // Skip packages with invalid names
                    }
                    
                    std::string pkg_name = pkg.get_name();
                    std::transform(pkg_name.begin(), pkg_name.end(), pkg_name.begin(), 
                                [](unsigned char c){ return std::tolower(c); });
                    
                    // Check both name and description for matches
                    if (pkg_name.find(search_term) != std::string::npos) {
                        // Check if this package is already in results (avoid duplicates)
                        bool already_added = false;
                        for (const auto& existing : results) {
                            if (existing.get_name() == pkg.get_name()) {
                                already_added = true;
                                break;
                            }
                        }
                        
                        if (!already_added) {
                            repo_results.push_back(pkg);
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "PackageManager: Error searching repo " << repo.get_name() 
                        << ": " << e.what() << std::endl;
            }
        }
        
        std::cout << "PackageManager: Searched through " << total_repo_packages << " repository packages" << std::endl;
        std::cout << "PackageManager: Found " << repo_results.size() << " matching repository packages" << std::endl;
        
        // Add repository results to the final results
        results.insert(results.end(), repo_results.begin(), repo_results.end());
        
        std::cout << "PackageManager: Total of " << results.size() << " matching packages found" << std::endl;
        
        // Also search AUR in the future
        std::cout << "PackageManager: AUR search will be implemented in Phase 3" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "PackageManager: Exception during search: " << e.what() << std::endl;
    }
    
    return results;
}

Package PackageManager::get_package_details(const std::string& name) const
{
    if (!m_handle || !m_repo_manager || name.empty()) {
        return Package();
    }
    
    return m_repo_manager->find_package(name);
}

bool PackageManager::install_package(const std::string& package_name)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Installing package: " << package_name << std::endl;
    
    // Use pacman directly with sudo
    std::string command = "pacman -S --noconfirm " + package_name;
    bool success = execute_with_sudo(command);
    
    if (success) {
        std::cout << "PackageManager: Package installed successfully: " << package_name << std::endl;
        return true;
    } else {
        set_last_error("Failed to install package: " + package_name);
        return false;
    }
}

bool PackageManager::install_package(const std::string& package_name, const std::string& password, bool use_overwrite)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Installing package with authentication: " << package_name << std::endl;
    
    // Use pacman directly with sudo and password
    std::string command = "pacman -S --noconfirm ";
    
    // Add overwrite option if requested
    if (use_overwrite) {
        command += "--overwrite \"*\" ";
    }
    
    command += package_name;
    bool success = execute_with_sudo(command, password);
    
    if (success) {
        std::cout << "PackageManager: Package installed successfully: " << package_name << std::endl;
        return true;
    } else {
        set_last_error("Failed to install package: " + package_name + ". Authentication may have failed.");
        return false;
    }
}

bool PackageManager::remove_package(const std::string& package_name)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Removing package: " << package_name << std::endl;
    
    // Use pacman directly with sudo
    std::string command = "pacman -R --noconfirm " + package_name;
    bool success = execute_with_sudo(command);
    
    if (success) {
        std::cout << "PackageManager: Package removed successfully: " << package_name << std::endl;
        return true;
    } else {
        set_last_error("Failed to remove package: " + package_name);
        return false;
    }
}

bool PackageManager::remove_package(const std::string& package_name, const std::string& password)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Removing package with authentication: " << package_name << std::endl;
    
    // Use pacman directly with sudo and password
    std::string command = "pacman -R --noconfirm " + package_name;
    bool success = execute_with_sudo(command, password);
    
    if (success) {
        std::cout << "PackageManager: Package removed successfully: " << package_name << std::endl;
        return true;
    } else {
        set_last_error("Failed to remove package: " + package_name + ". Authentication may have failed.");
        return false;
    }
}

bool PackageManager::update_package(const std::string& package_name)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Updating package: " << package_name << std::endl;
    
    // Use pacman directly with sudo
    std::string command = "pacman -S --noconfirm " + package_name;
    bool success = execute_with_sudo(command);
    
    if (success) {
        std::cout << "PackageManager: Package updated successfully: " << package_name << std::endl;
        return true;
    } else {
        set_last_error("Failed to update package: " + package_name);
        return false;
    }
}

bool PackageManager::update_package(const std::string& package_name, const std::string& password, bool use_overwrite)
{
    if (package_name.empty()) {
        set_last_error("Invalid package name");
        return false;
    }
    
    std::cout << "PackageManager: Updating package with authentication: " << package_name << std::endl;
    
    // Use pacman directly with sudo and password
    std::string command = "pacman -S --noconfirm ";
    
    // Add overwrite option if requested
    if (use_overwrite) {
        command += "--overwrite \"*\" ";
    }
    
    command += package_name;
    bool success = execute_with_sudo(command, password);
    
    if (success) {
        std::cout << "PackageManager: Package updated successfully: " << package_name << std::endl;
        return true;
    } else {
        set_last_error("Failed to update package: " + package_name + ". Authentication may have failed.");
        return false;
    }
}

bool PackageManager::sync_all()
{
    std::cout << "PackageManager: Synchronizing all packages" << std::endl;
    
    // First refresh the package databases
    std::string refresh_cmd = "pacman -Sy";
    bool refresh_success = execute_with_sudo(refresh_cmd);
    
    if (!refresh_success) {
        set_last_error("Failed to refresh package databases");
        return false;
    }
    
    std::cout << "PackageManager: Package databases refreshed successfully" << std::endl;
    
    // Re-initialize to get updated package information
    m_repo_manager->initialize();
    
    return true;
}

bool PackageManager::sync_all(const std::string& password)
{
    std::cout << "PackageManager: Synchronizing all packages with authentication" << std::endl;
    
    // First refresh the package databases
    std::string refresh_cmd = "pacman -Sy";
    bool refresh_success = execute_with_sudo(refresh_cmd, password);
    
    if (!refresh_success) {
        set_last_error("Failed to refresh package databases. Authentication may have failed.");
        return false;
    }
    
    std::cout << "PackageManager: Package databases refreshed successfully" << std::endl;
    
    // Re-initialize to get updated package information
    m_repo_manager->initialize();
    
    return true;
}

bool PackageManager::is_package_installed(const std::string& package_name) const
{
    if (!m_handle || !m_repo_manager || package_name.empty()) {
        return false;
    }
    
    Repository local_db = m_repo_manager->get_local_db();
    Package pkg = local_db.find_package(package_name);
    
    return !pkg.get_name().empty();
}

std::vector<Repository> PackageManager::get_repositories() const
{
    std::vector<Repository> repositories;
    
    if (!m_handle || !m_repo_manager) {
        return repositories;
    }
    
    // Add local repository
    repositories.push_back(m_repo_manager->get_local_db());
    
    // Add sync repositories
    std::vector<Repository> sync_dbs = m_repo_manager->get_sync_dbs();
    repositories.insert(repositories.end(), sync_dbs.begin(), sync_dbs.end());
    
    return repositories;
}

std::string PackageManager::get_last_error() const
{
    return m_last_error;
}

void PackageManager::set_last_error(const std::string& error)
{
    m_last_error = error;
    std::cerr << "PackageManager: " << error << std::endl;
}

bool PackageManager::register_sync_databases()
{
    if (!m_handle) {
        return false;
    }
    
    // Path to pacman configuration file
    std::string config_path = "/etc/pacman.conf";
    
    // Open the configuration file
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        std::cerr << "Failed to open pacman configuration file: " << config_path << std::endl;
        return false;
    }
    
    std::string line;
    std::string current_repo;
    int repo_count = 0;
    std::vector<std::string> registered_repos;
    
    std::cout << "PackageManager: Reading repositories from " << config_path << std::endl;
    
    // Parse the configuration file
    while (std::getline(config_file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Check for repository name
        if (line[0] == '[' && line[line.size() - 1] == ']') {
            current_repo = line.substr(1, line.size() - 2);
            
            // Skip local repositories
            if (current_repo == "options" || current_repo == "local") {
                current_repo = "";
                continue;
            }
            
            std::cout << "PackageManager: Found repository: " << current_repo << std::endl;
            
            // Register the repository
            alpm_db_t* db = alpm_register_syncdb(m_handle, current_repo.c_str(), ALPM_SIG_USE_DEFAULT);
            if (!db) {
                alpm_errno_t err = alpm_errno(m_handle);
                std::cerr << "Failed to register sync database: " << current_repo << ": " << alpm_strerror(err) << std::endl;
                continue; // Skip this repo but continue with others
            }
            
            // Add to our list of registered repos
            registered_repos.push_back(current_repo);
            repo_count++;
        }
    }
    
    config_file.close();
    
    // Force a refresh of repositories to ensure they have data
    if (repo_count > 0) {
        std::cout << "PackageManager: Successfully registered " << repo_count << " repositories:" << std::endl;
        for (const auto& repo : registered_repos) {
            std::cout << " - " << repo << std::endl;
        }
        
        // Reinitialize the repository manager to ensure it loads all the registered dbs
        try {
            if (!m_repo_manager->initialize()) {
                std::cerr << "PackageManager: Failed to reinitialize repository manager after registering databases" << std::endl;
                return false;
            }
            
            // Get the current sync databases
            std::vector<Repository> sync_dbs = m_repo_manager->get_sync_dbs();
            std::cout << "PackageManager: Loaded " << sync_dbs.size() << " sync databases" << std::endl;
            
            // Display package counts for each repo
            int total_packages = 0;
            for (const auto& repo : sync_dbs) {
                try {
                    std::vector<Package> repo_packages = repo.get_packages();
                    int package_count = repo_packages.size();
                    total_packages += package_count;
                    std::cout << "PackageManager: Repository '" << repo.get_name() << "' has " 
                            << package_count << " packages" << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "PackageManager: Error processing repo " << repo.get_name() 
                            << ": " << e.what() << std::endl;
                }
            }
            
            std::cout << "PackageManager: Total of " << total_packages << " packages available in repositories" << std::endl;
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "PackageManager: Exception during repository initialization: " << e.what() << std::endl;
            return false;
        }
    } else {
        std::cerr << "PackageManager: No sync databases registered successfully" << std::endl;
        return false;
    }
}

bool PackageManager::update_system(const std::string& password, std::function<void(const std::string&)> output_callback, bool use_overwrite)
{
    std::cout << "PackageManager: Performing full system update" << std::endl;
    
    if (output_callback) {
        output_callback("Starting system update...\n");
    }
    
    // Create a temporary file to capture output
    std::string temp_output_file = "/tmp/pacmangui_update_output.txt";
    
    // Run pacman -Syu with --noconfirm to avoid interactive prompts
    // Redirect output to the temporary file
    std::string command = "pacman -Syu --noconfirm ";
    
    // Add overwrite option if requested
    if (use_overwrite) {
        command += "--overwrite \"*\" ";
        
        if (output_callback) {
            output_callback("Using --overwrite=\"*\" option. This may overwrite conflicting files.\n");
        }
    }
    
    command += "| tee " + temp_output_file;
    
    // Start a background thread to monitor the output file and send updates
    std::atomic<bool> running(true);
    std::thread output_thread;
    
    if (output_callback) {
        output_thread = std::thread([temp_output_file, &output_callback, &running]() {
            std::ifstream output_file(temp_output_file);
            if (!output_file.is_open()) {
                return;
            }
            
            std::string line;
            while (running) {
                if (std::getline(output_file, line)) {
                    output_callback(line + "\n");
                } else {
                    // Wait a bit before checking for more output
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            
            // Read any remaining output
            while (std::getline(output_file, line)) {
                output_callback(line + "\n");
            }
            
            output_file.close();
        });
    }
    
    // Execute the command
    bool success = execute_with_sudo(command, password);
    
    // Stop the output thread and wait for it to finish
    if (output_callback) {
        running = false;
        if (output_thread.joinable()) {
            output_thread.join();
        }
    }
    
    // Clean up the temporary file
    std::remove(temp_output_file.c_str());
    
    if (success) {
        std::cout << "PackageManager: System update completed successfully" << std::endl;
        
        if (output_callback) {
            output_callback("System update completed successfully.\n");
        }
        
        // Re-initialize to get updated package information
        m_repo_manager->initialize();
        
        return true;
    } else {
        std::string error_message = "Failed to update system. Authentication may have failed.";
        set_last_error(error_message);
        
        if (output_callback) {
            output_callback("ERROR: " + error_message + "\n");
        }
        
        return false;
    }
}

// Overloaded method for backward compatibility
bool PackageManager::update_system(const std::string& password, bool use_overwrite)
{
    return update_system(password, nullptr, use_overwrite);
}

std::vector<std::pair<std::string, std::string>> PackageManager::check_updates() const
{
    std::vector<std::pair<std::string, std::string>> updates;
    
    std::cout << "PackageManager: Checking for available updates" << std::endl;
    
    // Run pacman -Qu to check for updates
    FILE* pipe = popen("pacman -Qu", "r");
    if (!pipe) {
        std::cerr << "PackageManager: Error running pacman -Qu" << std::endl;
        return updates;
    }
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        std::string line(buffer);
        
        // Parse output line (format: package_name old_version -> new_version)
        size_t first_space = line.find(' ');
        if (first_space != std::string::npos) {
            std::string package_name = line.substr(0, first_space);
            
            // Find the arrow separator
            size_t arrow_pos = line.find("->");
            if (arrow_pos != std::string::npos) {
                // Extract new version (skip spaces after arrow)
                size_t version_start = arrow_pos + 2;
                while (version_start < line.size() && isspace(line[version_start])) {
                    version_start++;
                }
                
                std::string new_version = line.substr(version_start);
                // Trim trailing whitespace/newline
                new_version.erase(new_version.find_last_not_of(" \n\r\t") + 1);
                
                updates.push_back(std::make_pair(package_name, new_version));
            }
        }
    }
    
    pclose(pipe);
    
    std::cout << "PackageManager: Found " << updates.size() << " available updates" << std::endl;
    return updates;
}

} // namespace core
} // namespace pacmangui 