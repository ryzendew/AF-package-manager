#include <iostream>
#include <QApplication>
#include <QGuiApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QSettings>
#include <QFile>
#include <QFont>
#include <QDir>
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
        // Enable High DPI scaling
        QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
        
        // Set high DPI scale factor rounding policy to PassThrough for fractional scaling
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
            Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
        
        // Initialize Qt application
        QApplication app(argc, argv);
        
        // Set application information
        app.setApplicationName("PacmanGUI");
        app.setApplicationVersion("0.1.0");
        app.setOrganizationName("PacmanGUI");
        app.setOrganizationDomain("pacmangui.org");
        
        // Apply custom scaling factor from settings if available
        QSettings settings("PacmanGUI", "PacmanGUI");
        double scalingFactor = settings.value("appearance/scalingFactor", 1.0).toDouble();
        if (scalingFactor != 1.0) {
            qputenv("QT_SCALE_FACTOR", QByteArray::number(scalingFactor));
            
            // Adjust default font size based on scaling factor
            QFont defaultFont = QApplication::font();
            int scaledFontSize = qRound(defaultFont.pointSize() * scalingFactor);
            defaultFont.setPointSize(scaledFontSize);
            QApplication::setFont(defaultFont);
        }
        
        // Get selected theme
        QString selectedTheme = settings.value("appearance/theme", "dark_colorful").toString();
        std::cout << "main.cpp - Selected theme from settings: " << selectedTheme.toStdString() << std::endl;
        
        // Determine which stylesheet file to load
        QString qssFileName;
        if (selectedTheme == "dark_colorful") {
            qssFileName = "dark_colorful.qss";
        } else if (selectedTheme == "dark") {
            qssFileName = "dark.qss";
        } else if (selectedTheme == "light_colorful") {
            qssFileName = "light_colorful.qss";
        } else {
            qssFileName = "light.qss";
        }
        
        std::cout << "main.cpp - Will attempt to load stylesheet: " << qssFileName.toStdString() << std::endl;
        
        // Load stylesheet from various paths
        bool styleLoaded = false;
        
        // Try resource path
        QFile styleFile(":/styles/" + qssFileName);
        std::cout << "main.cpp - Attempting to load from resource path: :/styles/" << qssFileName.toStdString() << std::endl;
        if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
            QString style = styleFile.readAll();
            app.setStyleSheet(style);
            styleFile.close();
            std::cout << "Successfully loaded " << qssFileName.toStdString() << " stylesheet from resource" << std::endl;
            styleLoaded = true;
        } else {
            std::cerr << "main.cpp - Failed to load from resource: " << styleFile.errorString().toStdString() << std::endl;
            
            // Try alternative resource path
            QFile alternativeStyleFile(":/resources/styles/" + qssFileName);
            std::cout << "main.cpp - Attempting to load from alternative resource path: :/resources/styles/" << qssFileName.toStdString() << std::endl;
            if (alternativeStyleFile.open(QFile::ReadOnly | QFile::Text)) {
                QString style = alternativeStyleFile.readAll();
                app.setStyleSheet(style);
                alternativeStyleFile.close();
                std::cout << "Successfully loaded " << qssFileName.toStdString() << " stylesheet from alternative resource path" << std::endl;
                styleLoaded = true;
            } else {
                std::cerr << "main.cpp - Failed to load from alternative resource: " << alternativeStyleFile.errorString().toStdString() << std::endl;
                
                // Try file system relative to executable
                QFile localStyleFile("resources/styles/" + qssFileName);
                std::cout << "main.cpp - Attempting to load from local file: resources/styles/" << qssFileName.toStdString() << std::endl;
                if (localStyleFile.open(QFile::ReadOnly | QFile::Text)) {
                    QString style = localStyleFile.readAll();
                    app.setStyleSheet(style);
                    localStyleFile.close();
                    std::cout << "Successfully loaded " << qssFileName.toStdString() << " stylesheet from local file" << std::endl;
                    styleLoaded = true;
                } else {
                    std::cerr << "main.cpp - Failed to load from local file: " << localStyleFile.errorString().toStdString() << std::endl;
                    
                    // Try absolute path
                    QString sourcePath = QDir::currentPath() + "/../resources/styles/" + qssFileName;
                    std::cout << "main.cpp - Attempting to load from source path: " << sourcePath.toStdString() << std::endl;
                    QFile sourceStyleFile(sourcePath);
                    if (sourceStyleFile.open(QFile::ReadOnly | QFile::Text)) {
                        QString style = sourceStyleFile.readAll();
                        app.setStyleSheet(style);
                        sourceStyleFile.close();
                        std::cout << "Successfully loaded " << qssFileName.toStdString() << " stylesheet from source path: " << sourcePath.toStdString() << std::endl;
                        styleLoaded = true;
                    } else {
                        std::cerr << "main.cpp - Failed to load from source path: " << sourceStyleFile.errorString().toStdString() << std::endl;
                    }
                }
            }
        }
        
        // If all paths failed, try dark_colorful.qss as fallback
        if (!styleLoaded && qssFileName != "dark_colorful.qss") {
            std::cerr << "Failed to load " << qssFileName.toStdString() << " stylesheet, trying dark_colorful.qss as fallback" << std::endl;
            
            // Try absolute path for dark_colorful.qss
            QString sourcePath = QDir::currentPath() + "/../resources/styles/dark_colorful.qss";
            QFile sourceStyleFile(sourcePath);
            if (sourceStyleFile.open(QFile::ReadOnly | QFile::Text)) {
                QString style = sourceStyleFile.readAll();
                app.setStyleSheet(style);
                sourceStyleFile.close();
                std::cout << "Successfully loaded dark_colorful.qss stylesheet from source path as fallback" << std::endl;
            } else {
                std::cerr << "Failed to load any stylesheet" << std::endl;
            }
        }
        
        // Set application style
        app.setStyle("Fusion");
        
        // Create main window
        MainWindow mainWindow;
        
        // Set window to maximize by default
        mainWindow.showMaximized();
        
        // Run the application
    return app.exec();
    }
} 