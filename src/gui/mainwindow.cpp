#include "gui/mainwindow.hpp"
#include <QApplication>
#include <QSettings>
#include <QMessageBox>
#include <QHeaderView>
#include <QCloseEvent>
#include <QMenuBar>
#include <QIcon>
#include <QFont>
#include <QStyle>
#include <QPalette>
#include <QStyleFactory>
#include <QScreen>
#include <QInputDialog>

namespace pacmangui {
namespace gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_searchBox(nullptr)
    , m_searchButton(nullptr)
    , m_packagesView(nullptr)
    , m_installedView(nullptr)
    , m_updatesView(nullptr)
    , m_detailsWidget(nullptr)
    , m_packageNameLabel(nullptr)
    , m_packageVersionLabel(nullptr)
    , m_packageDescLabel(nullptr)
    , m_actionButton(nullptr)
    , m_packagesModel(nullptr)
    , m_installedModel(nullptr)
    , m_updatesModel(nullptr)
    , m_darkTheme(false)
{
    // Initialize package manager
    if (!m_packageManager.initialize("/", "/var/lib/pacman")) {
        QMessageBox::critical(this, "Error", "Failed to initialize package manager");
    }
    
    // Load user settings
    loadSettings();
    
    // Set up the UI
    setupUi();
    setupActions();
    setupMenus();
    setupConnections();
    
    // Apply theme based on settings
    applyTheme(m_darkTheme);
    
    // Set window properties
    setWindowTitle("PacmanGUI - Package Manager");
    setMinimumSize(800, 600);
    
    // Center on screen
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    // Refresh the package lists
    refreshInstalledPackages();
    refreshUpdatesList();
}

MainWindow::~MainWindow()
{
    // Clean up
    delete m_packagesModel;
    delete m_installedModel;
    delete m_updatesModel;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Save settings before closing
    saveSettings();
    event->accept();
}

void MainWindow::setupUi()
{
    // Create central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Create search bar at the top
    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLayout->setSpacing(8);
    
    QLabel* searchLabel = new QLabel("Search:", this);
    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText("Search for packages...");
    m_searchButton = new QPushButton("Search", this);
    m_searchButton->setDefault(true);
    
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchBox, 1); // Make search box expandable
    searchLayout->addWidget(m_searchButton);
    
    mainLayout->addLayout(searchLayout);
    
    // Create tab widget for different views
    m_tabWidget = new QTabWidget(this);
    
    // Create tab pages
    QWidget* packagesTab = new QWidget(this);
    QWidget* installedTab = new QWidget(this);
    QWidget* updatesTab = new QWidget(this);
    
    // Set up package list view
    QVBoxLayout* packagesLayout = new QVBoxLayout(packagesTab);
    packagesLayout->setContentsMargins(0, 0, 0, 0);
    
    m_packagesView = new QTableView(packagesTab);
    m_packagesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_packagesView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_packagesView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_packagesView->setAlternatingRowColors(true);
    m_packagesView->horizontalHeader()->setStretchLastSection(true);
    m_packagesView->verticalHeader()->setVisible(false);
    m_packagesView->setSortingEnabled(true);
    
    m_packagesModel = new QStandardItemModel(0, 3, this);
    m_packagesModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Version" << "Description");
    m_packagesView->setModel(m_packagesModel);
    
    packagesLayout->addWidget(m_packagesView);
    
    // Set up installed packages view
    QVBoxLayout* installedLayout = new QVBoxLayout(installedTab);
    installedLayout->setContentsMargins(0, 0, 0, 0);
    
    m_installedView = new QTableView(installedTab);
    m_installedView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_installedView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_installedView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_installedView->setAlternatingRowColors(true);
    m_installedView->horizontalHeader()->setStretchLastSection(true);
    m_installedView->verticalHeader()->setVisible(false);
    m_installedView->setSortingEnabled(true);
    
    m_installedModel = new QStandardItemModel(0, 3, this);
    m_installedModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Version" << "Description");
    m_installedView->setModel(m_installedModel);
    
    installedLayout->addWidget(m_installedView);
    
    // Set up updates view
    QVBoxLayout* updatesLayout = new QVBoxLayout(updatesTab);
    updatesLayout->setContentsMargins(0, 0, 0, 0);
    
    m_updatesView = new QTableView(updatesTab);
    m_updatesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_updatesView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_updatesView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_updatesView->setAlternatingRowColors(true);
    m_updatesView->horizontalHeader()->setStretchLastSection(true);
    m_updatesView->verticalHeader()->setVisible(false);
    m_updatesView->setSortingEnabled(true);
    
    m_updatesModel = new QStandardItemModel(0, 3, this);
    m_updatesModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Current Version" << "New Version");
    m_updatesView->setModel(m_updatesModel);
    
    updatesLayout->addWidget(m_updatesView);
    
    // Add tabs to tab widget
    m_tabWidget->addTab(packagesTab, "All Packages");
    m_tabWidget->addTab(installedTab, "Installed");
    m_tabWidget->addTab(updatesTab, "Updates");
    
    // Add tab widget to main layout
    mainLayout->addWidget(m_tabWidget, 1); // Make tab widget expandable
    
    // Create package details view at the bottom
    m_detailsWidget = new QWidget(this);
    QVBoxLayout* detailsLayout = new QVBoxLayout(m_detailsWidget);
    
    // Package details
    m_packageNameLabel = new QLabel(this);
    QFont nameFont = m_packageNameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 2);
    m_packageNameLabel->setFont(nameFont);
    
    m_packageVersionLabel = new QLabel(this);
    m_packageDescLabel = new QLabel(this);
    m_packageDescLabel->setWordWrap(true);
    
    // Action button
    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->addStretch(1);
    
    m_actionButton = new QPushButton(this);
    m_actionButton->setMinimumWidth(120);
    actionLayout->addWidget(m_actionButton);
    
    // Add widgets to details layout
    detailsLayout->addWidget(m_packageNameLabel);
    detailsLayout->addWidget(m_packageVersionLabel);
    detailsLayout->addWidget(m_packageDescLabel);
    detailsLayout->addStretch(1);
    detailsLayout->addLayout(actionLayout);
    
    // Add details widget to main layout
    mainLayout->addWidget(m_detailsWidget);
    
    // Initially hide details widget until a package is selected
    m_detailsWidget->setVisible(false);
    
    // Create status bar
    statusBar()->showMessage("Ready");
}

void MainWindow::setupActions()
{
    // Create actions
    QAction* refreshAction = new QAction("Refresh", this);
    refreshAction->setShortcut(QKeySequence::Refresh);
    refreshAction->setStatusTip("Refresh package lists");
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onSyncAll);
    
    QAction* installAction = new QAction("Install", this);
    installAction->setStatusTip("Install selected package");
    connect(installAction, &QAction::triggered, this, &MainWindow::onInstallPackage);
    
    QAction* removeAction = new QAction("Remove", this);
    removeAction->setStatusTip("Remove selected package");
    connect(removeAction, &QAction::triggered, this, &MainWindow::onRemovePackage);
    
    QAction* updateAction = new QAction("Update", this);
    updateAction->setStatusTip("Update selected package");
    connect(updateAction, &QAction::triggered, this, &MainWindow::onUpdatePackage);
    
    QAction* themeAction = new QAction("Toggle Theme", this);
    themeAction->setStatusTip("Switch between light and dark theme");
    connect(themeAction, &QAction::triggered, this, &MainWindow::toggleTheme);
    
    QAction* exitAction = new QAction("Exit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setStatusTip("Exit the application");
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    // Add actions to toolbar
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->addAction(refreshAction);
    toolbar->addSeparator();
    toolbar->addAction(installAction);
    toolbar->addAction(removeAction);
    toolbar->addAction(updateAction);
    toolbar->addSeparator();
    toolbar->addAction(themeAction);
}

void MainWindow::setupMenus()
{
    // Create menus
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Refresh", this, &MainWindow::onSyncAll);
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &QMainWindow::close);
    
    QMenu* packageMenu = menuBar()->addMenu("Package");
    packageMenu->addAction("Install", this, &MainWindow::onInstallPackage);
    packageMenu->addAction("Remove", this, &MainWindow::onRemovePackage);
    packageMenu->addAction("Update", this, &MainWindow::onUpdatePackage);
    
    QMenu* viewMenu = menuBar()->addMenu("View");
    viewMenu->addAction("Toggle Theme", this, &MainWindow::toggleTheme);
    
    QMenu* helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction("About", [this]() {
        QMessageBox::about(this, "About PacmanGUI",
                          "PacmanGUI - A modern GUI frontend for the Arch Linux package manager.\n\n"
                          "Version: 0.1.0\n"
                          "License: GPL-3.0");
    });
}

void MainWindow::setupConnections()
{
    // Connect search box and button
    connect(m_searchBox, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked);
    connect(m_searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    
    // Connect tab changed signal
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    
    // Connect package selection signals
    connect(m_packagesView->selectionModel(), &QItemSelectionModel::selectionChanged, [this]() {
        // Show details for the selected package
        QModelIndexList indexes = m_packagesView->selectionModel()->selectedRows();
        if (!indexes.isEmpty()) {
            QModelIndex index = indexes.first();
            QString packageName = m_packagesModel->data(m_packagesModel->index(index.row(), 0)).toString();
            QString version = m_packagesModel->data(m_packagesModel->index(index.row(), 1)).toString();
            QString description = m_packagesModel->data(m_packagesModel->index(index.row(), 2)).toString();
            
            m_packageNameLabel->setText(packageName);
            m_packageVersionLabel->setText("Version: " + version);
            m_packageDescLabel->setText(description);
            
            // Set action button based on installation status
            bool isInstalled = m_packageManager.is_package_installed(packageName.toStdString());
            if (isInstalled) {
                m_actionButton->setText("Remove");
                connect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onRemovePackage, Qt::UniqueConnection);
            } else {
                m_actionButton->setText("Install");
                connect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onInstallPackage, Qt::UniqueConnection);
            }
            
            m_detailsWidget->setVisible(true);
        } else {
            m_detailsWidget->setVisible(false);
        }
    });
    
    // Similar connections for installed and updates views
    // (Implementation omitted for brevity)
}

void MainWindow::loadSettings()
{
    QSettings settings("PacmanGUI", "PacmanGUI");
    m_darkTheme = settings.value("appearance/darkTheme", false).toBool();
    
    // Restore window geometry
    if (settings.contains("window/geometry")) {
        restoreGeometry(settings.value("window/geometry").toByteArray());
    }
    
    // Restore window state
    if (settings.contains("window/state")) {
        restoreState(settings.value("window/state").toByteArray());
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("PacmanGUI", "PacmanGUI");
    settings.setValue("appearance/darkTheme", m_darkTheme);
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());
}

void MainWindow::applyTheme(bool isDark)
{
    m_darkTheme = isDark;
    
    // Set up application-wide palette based on theme
    QPalette palette;
    
    if (isDark) {
        // Dark theme colors
        palette.setColor(QPalette::Window, QColor(53, 53, 53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(25, 25, 25));
        palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::black);
        
        // Set dark style sheets for specific widgets
        m_packagesView->setStyleSheet("QTableView { border: 1px solid #3c3c3c; selection-background-color: #2a82da; }");
        m_installedView->setStyleSheet("QTableView { border: 1px solid #3c3c3c; selection-background-color: #2a82da; }");
        m_updatesView->setStyleSheet("QTableView { border: 1px solid #3c3c3c; selection-background-color: #2a82da; }");
        m_searchBox->setStyleSheet("QLineEdit { border: 1px solid #3c3c3c; border-radius: 3px; padding: 2px; }");
        m_actionButton->setStyleSheet("QPushButton { background-color: #2a82da; color: white; border-radius: 3px; padding: 6px; }");
    } else {
        // Light theme colors - use system default with some customizations
        palette = QApplication::style()->standardPalette();
        
        // Set light style sheets for specific widgets
        m_packagesView->setStyleSheet("QTableView { border: 1px solid #d0d0d0; selection-background-color: #308cc6; }");
        m_installedView->setStyleSheet("QTableView { border: 1px solid #d0d0d0; selection-background-color: #308cc6; }");
        m_updatesView->setStyleSheet("QTableView { border: 1px solid #d0d0d0; selection-background-color: #308cc6; }");
        m_searchBox->setStyleSheet("QLineEdit { border: 1px solid #d0d0d0; border-radius: 3px; padding: 2px; }");
        m_actionButton->setStyleSheet("QPushButton { background-color: #308cc6; color: white; border-radius: 3px; padding: 6px; }");
    }
    
    // Apply the palette to the application
    QApplication::setPalette(palette);
    
    // Update status bar
    statusBar()->showMessage(isDark ? "Dark theme applied" : "Light theme applied", 3000);
}

void MainWindow::toggleTheme()
{
    applyTheme(!m_darkTheme);
}

void MainWindow::onTabChanged(int index)
{
    // Handle tab changes
    switch (index) {
        case 0: // All Packages
            statusBar()->showMessage("Viewing all available packages");
            break;
        case 1: // Installed
            statusBar()->showMessage("Viewing installed packages");
            refreshInstalledPackages();
            break;
        case 2: // Updates
            statusBar()->showMessage("Viewing available updates");
            refreshUpdatesList();
            break;
    }
}

void MainWindow::onSearchTextChanged(const QString& text)
{
    // Enable/disable search button based on text
    m_searchButton->setEnabled(!text.isEmpty());
}

void MainWindow::onSearchClicked()
{
    QString searchTerm = m_searchBox->text().trimmed();
    if (searchTerm.isEmpty()) {
        return;
    }
    
    searchPackages(searchTerm);
}

void MainWindow::searchPackages(const QString& searchTerm)
{
    statusBar()->showMessage("Searching for: " + searchTerm);
    
    // Clear previous search results
    m_packagesModel->removeRows(0, m_packagesModel->rowCount());
    
    // Search for packages
    std::vector<core::Package> results = m_packageManager.search_by_name(searchTerm.toStdString());
    
    // Display results
    for (const auto& pkg : results) {
        QList<QStandardItem*> row;
        row << new QStandardItem(QString::fromStdString(pkg.get_name()));
        row << new QStandardItem(QString::fromStdString(pkg.get_version()));
        row << new QStandardItem(QString::fromStdString(pkg.get_description()));
        
        m_packagesModel->appendRow(row);
    }
    
    // Switch to the All Packages tab
    m_tabWidget->setCurrentIndex(0);
    
    // Update status bar
    statusBar()->showMessage("Found " + QString::number(results.size()) + " packages matching: " + searchTerm);
    
    // Auto-select first result if available
    if (m_packagesModel->rowCount() > 0) {
        m_packagesView->selectRow(0);
    }
}

void MainWindow::refreshInstalledPackages()
{
    statusBar()->showMessage("Refreshing installed packages...");
    
    // Clear previous list
    m_installedModel->removeRows(0, m_installedModel->rowCount());
    
    // Get installed packages
    std::vector<core::Package> installed = m_packageManager.get_installed_packages();
    
    // Display packages
    for (const auto& pkg : installed) {
        QList<QStandardItem*> row;
        row << new QStandardItem(QString::fromStdString(pkg.get_name()));
        row << new QStandardItem(QString::fromStdString(pkg.get_version()));
        row << new QStandardItem(QString::fromStdString(pkg.get_description()));
        
        m_installedModel->appendRow(row);
    }
    
    // Update status bar
    statusBar()->showMessage("Found " + QString::number(installed.size()) + " installed packages");
}

void MainWindow::refreshUpdatesList()
{
    statusBar()->showMessage("Checking for updates...");
    
    // Clear previous list
    m_updatesModel->removeRows(0, m_updatesModel->rowCount());
    
    // TODO: Implement update check using pacman
    // For now, just show a message
    statusBar()->showMessage("Update check will be implemented in future version");
}

void MainWindow::onInstallPackage()
{
    // Get selected package
    QModelIndexList indexes = m_packagesView->selectionModel()->selectedRows();
    if (indexes.isEmpty()) {
        QMessageBox::warning(this, "No Package Selected", "Please select a package to install.");
        return;
    }
    
    QModelIndex index = indexes.first();
    QString packageName = m_packagesModel->data(m_packagesModel->index(index.row(), 0)).toString();
    
    // Confirm installation
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Install Package",
                                 "Do you want to install the package '" + packageName + "'?",
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Add password dialog
        bool ok;
        QString password = QInputDialog::getText(this, "Authentication Required",
                                               "Enter your password to install package:",
                                               QLineEdit::Password, "", &ok);
        
        if (!ok || password.isEmpty()) {
            statusBar()->showMessage("Package installation cancelled");
            return;
        }
        
        setCursor(Qt::WaitCursor);
        statusBar()->showMessage("Installing package: " + packageName);
        
        bool success = m_packageManager.install_package(packageName.toStdString(), password.toStdString());
        setCursor(Qt::ArrowCursor);
        
        if (success) {
            QMessageBox::information(this, "Success", "Package '" + packageName + "' installed successfully.");
            statusBar()->showMessage("Package '" + packageName + "' installed successfully.");
            
            // Refresh the installed packages list
            refreshInstalledPackages();
        } else {
            QMessageBox::critical(this, "Installation Failed", 
                                 "Failed to install package '" + packageName + "'.\n" + 
                                 QString::fromStdString(m_packageManager.get_last_error()));
            statusBar()->showMessage("Installation failed: " + QString::fromStdString(m_packageManager.get_last_error()));
        }
    }
}

void MainWindow::onRemovePackage()
{
    // Get selected package
    QModelIndexList indexes;
    QString packageName;
    
    // Check which tab is active and get the selection
    int currentTab = m_tabWidget->currentIndex();
    if (currentTab == 0) { // All Packages
        indexes = m_packagesView->selectionModel()->selectedRows();
        if (!indexes.isEmpty()) {
            QModelIndex index = indexes.first();
            packageName = m_packagesModel->data(m_packagesModel->index(index.row(), 0)).toString();
        }
    } else if (currentTab == 1) { // Installed
        indexes = m_installedView->selectionModel()->selectedRows();
        if (!indexes.isEmpty()) {
            QModelIndex index = indexes.first();
            packageName = m_installedModel->data(m_installedModel->index(index.row(), 0)).toString();
        }
    }
    
    if (packageName.isEmpty()) {
        QMessageBox::warning(this, "No Package Selected", "Please select a package to remove.");
        return;
    }
    
    // Confirm removal
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Remove Package",
                                 "Do you want to remove the package '" + packageName + "'?",
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Add password dialog
        bool ok;
        QString password = QInputDialog::getText(this, "Authentication Required",
                                               "Enter your password to remove package:",
                                               QLineEdit::Password, "", &ok);
        
        if (!ok || password.isEmpty()) {
            statusBar()->showMessage("Package removal cancelled");
            return;
        }
        
        setCursor(Qt::WaitCursor);
        statusBar()->showMessage("Removing package: " + packageName);
        
        bool success = m_packageManager.remove_package(packageName.toStdString(), password.toStdString());
        setCursor(Qt::ArrowCursor);
        
        if (success) {
            QMessageBox::information(this, "Success", "Package '" + packageName + "' removed successfully.");
            statusBar()->showMessage("Package '" + packageName + "' removed successfully.");
            
            // Refresh the installed packages list
            refreshInstalledPackages();
        } else {
            QMessageBox::critical(this, "Removal Failed", 
                                 "Failed to remove package '" + packageName + "'.\n" + 
                                 QString::fromStdString(m_packageManager.get_last_error()));
            statusBar()->showMessage("Removal failed: " + QString::fromStdString(m_packageManager.get_last_error()));
        }
    }
}

void MainWindow::onUpdatePackage()
{
    // Get selected package
    QModelIndexList indexes;
    QString packageName;
    
    // Check which tab is active and get the selection
    int currentTab = m_tabWidget->currentIndex();
    if (currentTab == 0) { // All Packages
        indexes = m_packagesView->selectionModel()->selectedRows();
        if (!indexes.isEmpty()) {
            QModelIndex index = indexes.first();
            packageName = m_packagesModel->data(m_packagesModel->index(index.row(), 0)).toString();
        }
    } else if (currentTab == 1) { // Installed
        indexes = m_installedView->selectionModel()->selectedRows();
        if (!indexes.isEmpty()) {
            QModelIndex index = indexes.first();
            packageName = m_installedModel->data(m_installedModel->index(index.row(), 0)).toString();
        }
    } else if (currentTab == 2) { // Updates
        indexes = m_updatesView->selectionModel()->selectedRows();
        if (!indexes.isEmpty()) {
            QModelIndex index = indexes.first();
            packageName = m_updatesModel->data(m_updatesModel->index(index.row(), 0)).toString();
        }
    }
    
    if (packageName.isEmpty()) {
        QMessageBox::warning(this, "No Package Selected", "Please select a package to update.");
        return;
    }
    
    // Confirm update
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Update Package",
                                 "Do you want to update the package '" + packageName + "'?",
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Add password dialog
        bool ok;
        QString password = QInputDialog::getText(this, "Authentication Required",
                                               "Enter your password to update package:",
                                               QLineEdit::Password, "", &ok);
        
        if (!ok || password.isEmpty()) {
            statusBar()->showMessage("Package update cancelled");
            return;
        }
        
        setCursor(Qt::WaitCursor);
        statusBar()->showMessage("Updating package: " + packageName);
        
        bool success = m_packageManager.update_package(packageName.toStdString(), password.toStdString());
        setCursor(Qt::ArrowCursor);
        
        if (success) {
            QMessageBox::information(this, "Success", "Package '" + packageName + "' updated successfully.");
            statusBar()->showMessage("Package '" + packageName + "' updated successfully.");
            
            // Refresh the installed packages list
            refreshInstalledPackages();
            refreshUpdatesList();
        } else {
            QMessageBox::critical(this, "Update Failed", 
                                 "Failed to update package '" + packageName + "'.\n" + 
                                 QString::fromStdString(m_packageManager.get_last_error()));
            statusBar()->showMessage("Update failed: " + QString::fromStdString(m_packageManager.get_last_error()));
        }
    }
}

void MainWindow::onSyncAll()
{
    statusBar()->showMessage("Refreshing package databases...");
    
    // Confirm sync
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Refresh Databases",
                                 "Do you want to refresh all package databases?",
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Add password dialog
        bool ok;
        QString password = QInputDialog::getText(this, "Authentication Required",
                                               "Enter your password to perform system update:",
                                               QLineEdit::Password, "", &ok);
        
        if (!ok || password.isEmpty()) {
            statusBar()->showMessage("Database refresh cancelled");
            return;
        }
        
        setCursor(Qt::WaitCursor);
        bool success = m_packageManager.sync_all(password.toStdString());
        setCursor(Qt::ArrowCursor);
        
        if (success) {
            QMessageBox::information(this, "Success", "Package databases refreshed successfully.");
            statusBar()->showMessage("Package databases refreshed successfully.");
            
            // Refresh all package lists
            refreshInstalledPackages();
            refreshUpdatesList();
            
            // If there's a search term, re-run the search
            if (!m_searchBox->text().isEmpty()) {
                searchPackages(m_searchBox->text());
            }
        } else {
            QMessageBox::critical(this, "Refresh Failed", 
                                "Failed to refresh package databases.\n" + 
                                QString::fromStdString(m_packageManager.get_last_error()));
            statusBar()->showMessage("Refresh failed: " + QString::fromStdString(m_packageManager.get_last_error()));
        }
    }
}

} // namespace gui
} // namespace pacmangui 