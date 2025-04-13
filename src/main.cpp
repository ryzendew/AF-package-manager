#include <iostream>
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "gui/mainwindow.hpp"
#include "core/packagemanager.hpp"

using namespace pacmangui::core;
using namespace pacmangui::gui;

void print_help() {
    std::cout << "PacmanGui - Package Manager\n";
    std::cout << "Available commands:\n";
    std::cout << "  search <term>   - Search for packages by name in repositories\n";
    std::cout << "  info <package>  - Show detailed information about a package\n";
    std::cout << "  list-installed  - List all installed packages\n";
    std::cout << "  refresh         - Refresh package databases\n";
    std::cout << "  install <pkg>   - Install a package\n";
    std::cout << "  remove <pkg>    - Remove a package\n";
    std::cout << "  update <pkg>    - Update a package\n";
    std::cout << "  sync-all        - Update all packages\n";
    std::cout << "  quit            - Exit the program\n";
    std::cout << "  help            - Show this help message\n";
    std::cout << "\nNote: AUR support will be added in Phase 3\n";
}

void show_common_commands() {
    std::cout << "\nCommon commands:\n";
    std::cout << "  search <term>   - Search for packages by name in repositories\n";
    std::cout << "  info <pkg>      - Show detailed information about a package\n";
    std::cout << "  install <pkg>   - Install a package\n";
    std::cout << "  remove <pkg>    - Remove a package\n";
    std::cout << "  update <pkg>    - Update a package\n";
    std::cout << "  refresh         - Refresh package databases\n";
    std::cout << "  help            - Show all available commands\n";
}

void print_package_details(const Package& pkg) {
    std::cout << "Name: " << pkg.get_name() << "\n";
    std::cout << "Version: " << pkg.get_version() << "\n";
    std::cout << "Description: " << pkg.get_description() << "\n";
    std::cout << "Installed: " << (pkg.is_installed() ? "Yes" : "No") << "\n";
}

void print_package_list(const std::vector<Package>& packages) {
    for (const auto& pkg : packages) {
        std::cout << pkg.get_name() << " (" << pkg.get_version() << "): " << pkg.get_description() << "\n";
    }
    std::cout << "Total: " << packages.size() << " packages\n";
}

int startCli(int argc, char *argv[]) {
    // Initialize package manager
    PackageManager pm;
    
    std::cout << "Initializing package manager...\n";
    if (!pm.initialize("/", "/var/lib/pacman")) {
        std::cerr << "Failed to initialize package manager: " << pm.get_last_error() << std::endl;
        return 1;
    }
    std::cout << "Package manager initialized successfully.\n";
    
    // Print welcome message and help
    std::cout << "Welcome to PacmanGui CLI\n";
    print_help();
    
    // Main CLI loop
    bool running = true;
    std::string command;
    
    while (running) {
        std::cout << "\n> ";
        std::string input;
        std::getline(std::cin, input);
        
        // Parse command
        std::string::size_type pos = input.find(' ');
        if (pos != std::string::npos) {
            command = input.substr(0, pos);
            input = input.substr(pos + 1);
        } else {
            command = input;
            input = "";
        }
        
        // Execute command
        if (command == "quit" || command == "exit") {
            running = false;
        }
        else if (command == "help") {
            print_help();
        }
        else if (command == "search" && !input.empty()) {
            std::cout << "Searching for packages matching '" << input << "'...\n";
            std::vector<Package> results = pm.search_by_name(input);
            print_package_list(results);
            
            // Show common commands
            show_common_commands();
        }
        else if (command == "info" && !input.empty()) {
            std::cout << "Getting information for package '" << input << "'...\n";
            Package pkg = pm.get_package_details(input);
            if (pkg.get_name().empty()) {
                std::cout << "Package not found.\n";
            } else {
                print_package_details(pkg);
            }
            
            // Show common commands
            show_common_commands();
        }
        else if (command == "list-installed") {
            std::cout << "Getting installed packages...\n";
            std::vector<Package> packages = pm.get_installed_packages();
            print_package_list(packages);
            
            // Show common commands
            show_common_commands();
        }
        else if (command == "install" && !input.empty()) {
            std::cout << "Installing package '" << input << "'...\n";
            if (pm.install_package(input)) {
                std::cout << "Package installed successfully.\n";
            } else {
                std::cerr << "Failed to install package: " << pm.get_last_error() << std::endl;
            }
            
            // Show common commands
            show_common_commands();
        }
        else if (command == "remove" && !input.empty()) {
            std::cout << "Removing package '" << input << "'...\n";
            if (pm.remove_package(input)) {
                std::cout << "Package removed successfully.\n";
            } else {
                std::cerr << "Failed to remove package: " << pm.get_last_error() << std::endl;
            }
            
            // Show common commands
            show_common_commands();
        }
        else if (command == "update" && !input.empty()) {
            std::cout << "Updating package '" << input << "'...\n";
            if (pm.update_package(input)) {
                std::cout << "Package updated successfully.\n";
            } else {
                std::cerr << "Failed to update package: " << pm.get_last_error() << std::endl;
            }
            
            // Show common commands
            show_common_commands();
        }
        else if (command == "sync-all") {
            std::cout << "Synchronizing all packages...\n";
            if (pm.sync_all()) {
                std::cout << "All packages synchronized successfully.\n";
            } else {
                std::cerr << "Failed to synchronize packages: " << pm.get_last_error() << std::endl;
            }
            
            // Show common commands
            show_common_commands();
        }
        else if (command == "refresh") {
            std::cout << "Refreshing package databases...\n";
            if (pm.sync_all()) {
                std::cout << "Package databases refreshed successfully.\n";
            } else {
                std::cerr << "Failed to refresh package databases: " << pm.get_last_error() << std::endl;
            }
            
            // Show common commands
            show_common_commands();
        }
        else {
            std::cout << "Unknown command. Type 'help' for available commands.\n";
        }
    }
    
    std::cout << "Exiting PacmanGui CLI.\n";
    return 0;
}

int main(int argc, char *argv[])
{
    // Parse command line arguments
    bool useCliMode = false;
    
    if (argc > 1 && std::string(argv[1]) == "--cli") {
        useCliMode = true;
    }
    
    if (useCliMode) {
        return startCli(argc, argv);
    } else {
        // Initialize Qt application
        QApplication app(argc, argv);
        
        // Set application information
        app.setApplicationName("PacmanGUI");
        app.setApplicationVersion("0.1.0");
        app.setOrganizationName("PacmanGUI");
        app.setOrganizationDomain("pacmangui.org");
        
        // Set application style
        app.setStyle("Fusion");
        
        // Create main window
        MainWindow mainWindow;
        mainWindow.show();
        
        // Run the application
        return app.exec();
    }
} 