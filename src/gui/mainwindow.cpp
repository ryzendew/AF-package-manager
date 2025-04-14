#include "gui/mainwindow.hpp"
#include "gui/settingsdialog.hpp"
#include "gui/flatpak_manager_tab.hpp"
#include "wayland/wayland_backend.hpp"
#include "wayland/wayland_protocols.hpp"
#include "wayland/wayland_security.hpp"
#include "wayland/wayland_optimization.hpp"

#include <QMenuBar>
#include <QStatusBar>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QProcess>
#include <QSettings>
#include <QCloseEvent>
#include <QTemporaryDir>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollBar>
#include <QThread>
#include <QStyleFactory>
#include <QStyle>
#include <QPalette>
#include <QTimer>
#include <QProgressDialog>
#include <QProgressBar>
#include <QScreen>
#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <QShortcut>
#include <QGridLayout>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QCheckBox>

#include <iostream>
#include <functional>
#include <QDebug>

namespace pacmangui {
namespace gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    m_packageManager(),
    m_settingsDialog(nullptr),
    m_waylandSupported(false),
    m_slideAnimation(nullptr),
    m_packagesModel(nullptr),
    m_installedModel(nullptr),
    m_systemUpdatesModel(nullptr),
    m_updatesModel(nullptr),
    m_searchWatcher(nullptr),
    m_updateButton(nullptr),
    m_installAurButton(nullptr),
    m_updateInstalledButton(nullptr),
    m_flatpakModel(nullptr),
    m_installedFlatpakModel(nullptr),
    m_flatpakSearchWatcher(nullptr),
    m_flatpakSearchCheckbox(nullptr),
    m_installFlatpakButton(nullptr),
    m_removeFlatpakButton(nullptr),
    m_flatpakSearchEnabled(false)
{
    setWindowTitle(tr("PacmanGUI"));
    setMinimumSize(800, 600);

    // Initialize package manager
    m_packageManager.initialize("/", "/var/lib/pacman");

    // Initialize models before we use them
    // Add a checkbox column at the beginning for multi-selection
    m_packagesModel = new QStandardItemModel(0, 5, this);
    m_packagesModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Version") << tr("Repository") << tr("Description"));
        
    m_installedModel = new QStandardItemModel(0, 5, this);
    m_installedModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Version") << tr("Repository") << tr("Description"));
        
    m_updatesModel = new QStandardItemModel(0, 5, this);
    m_updatesModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Current Version") << tr("New Version") << tr("Repository"));
        
    // Initialize Flatpak models
    m_flatpakModel = new QStandardItemModel(0, 5, this);
    m_flatpakModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Version") << tr("Repository") << tr("Description"));
        
    m_installedFlatpakModel = new QStandardItemModel(0, 5, this);
    m_installedFlatpakModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Version") << tr("Repository") << tr("Description"));
    
    // Set up UI components in the correct order
    setupUi();
    setupActions();
    setupMenus();
    setupSystemUpdateTab(); // Create the tab and its buttons before connecting signals
    setupMaintenanceTab();  // Create maintenance buttons before connecting them
    setupFlatpakSupport(); // Initialize Flatpak support
    
    // Initialize Wayland support, if available and enabled
#if defined(ENABLE_WAYLAND_SUPPORT) && ENABLE_WAYLAND_SUPPORT == 1
    setupWaylandSupport();
#else
    qDebug() << "DEBUG: Wayland support is disabled in build configuration";
    m_waylandSupported = false;
    m_waylandSecurityAction = nullptr;
    m_waylandOptimizationsAction = nullptr;
#endif

    // Now connect signals after all buttons have been created
    setupConnections();
    setupDetailPanel();

    // Load settings
    loadSettings();
    applyTheme(isDarkThemeEnabled());

    // Populate tables - enable these to load actual data
    refreshInstalledPackages();
    // Don't search packages on startup - let user initiate search
    // searchPackages(""); // Start with empty search to show all packages
    
    // Don't check for updates automatically as it might be slow
    // We'll let the user trigger this manually

    // Check for AUR helper
    checkAurHelper();

    // Show welcome message
    showStatusMessage(tr("Welcome to PacmanGUI"));
}

MainWindow::~MainWindow()
{
    // Save settings before closing
    saveSettings();
    
    // Clean up allocated resources
    if (m_slideAnimation) {
        m_slideAnimation->stop();
        delete m_slideAnimation;
        m_slideAnimation = nullptr;
    }
    
    // Clean up async search resources
    if (m_searchWatcher) {
        if (m_searchWatcher->isRunning()) {
            m_searchWatcher->cancel();
            m_searchWatcher->waitForFinished();
        }
        delete m_searchWatcher;
        m_searchWatcher = nullptr;
    }
    
    delete m_packagesModel;
    delete m_installedModel;
    delete m_systemUpdatesModel;
    delete m_updatesModel;
    delete m_settingsDialog;
    
    // Clean up Flatpak resources
    if (m_flatpakSearchWatcher) {
        if (m_flatpakSearchWatcher->isRunning()) {
            m_flatpakSearchWatcher->cancel();
            m_flatpakSearchWatcher->waitForFinished();
        }
        delete m_flatpakSearchWatcher;
        m_flatpakSearchWatcher = nullptr;
    }
    
    delete m_flatpakModel;
    delete m_installedFlatpakModel;
}

void MainWindow::setupWaylandSupport()
{
    qDebug() << "DEBUG: Entering setupWaylandSupport()";
#if defined(ENABLE_WAYLAND_SUPPORT) && ENABLE_WAYLAND_SUPPORT == 1
    qDebug() << "DEBUG: ENABLE_WAYLAND_SUPPORT is defined and enabled";
    // Check if Wayland is available
    m_waylandSupported = pacmangui::wayland::WaylandBackend::isWaylandAvailable();
    qDebug() << "DEBUG: m_waylandSupported =" << m_waylandSupported;
    
    if (m_waylandSupported) {
        qDebug() << "DEBUG: Wayland is supported, initializing components";
        // Initialize static Wayland support
        pacmangui::wayland::WaylandBackend::initialize();
        qDebug() << "DEBUG: WaylandBackend initialized";
        pacmangui::wayland::WaylandProtocols::initialize();
        qDebug() << "DEBUG: WaylandProtocols initialized";
        pacmangui::wayland::WaylandSecurity::initialize();
        qDebug() << "DEBUG: WaylandSecurity initialized";
        pacmangui::wayland::WaylandOptimization::initialize();
        qDebug() << "DEBUG: WaylandOptimization initialized";
        
        qDebug() << "DEBUG: Creating Wayland actions";
        // Create actions
        m_waylandSecurityAction = new QAction("Enable Wayland Security Features", this);
        m_waylandSecurityAction->setCheckable(true);
        m_waylandSecurityAction->setChecked(true);
        
        m_waylandOptimizationsAction = new QAction("Enable Wayland Optimizations", this);
        m_waylandOptimizationsAction->setCheckable(true);
        m_waylandOptimizationsAction->setChecked(true);
        
        qDebug() << "DEBUG: Connecting Wayland action signals";
        // Connect signals and slots
        connect(m_waylandSecurityAction, &QAction::toggled, this, [this](bool checked) {
            pacmangui::wayland::WaylandSecurity::enableSecurityFeatures(checked);
            statusBar()->showMessage(
                checked ? "Wayland security features enabled" 
                        : "Wayland security features disabled", 
                3000);
        });
        
        connect(m_waylandOptimizationsAction, &QAction::toggled, this, [this](bool checked) {
            pacmangui::wayland::WaylandOptimization::enableOptimizations(checked);
            statusBar()->showMessage(
                checked ? "Wayland optimizations enabled" 
                        : "Wayland optimizations disabled", 
                3000);
        });
        
        qDebug() << "DEBUG: Getting Wayland display info";
        // Display Wayland info in status bar
        QString displayInfo = pacmangui::wayland::WaylandBackend::getDisplayInfo();
        if (!displayInfo.isEmpty()) {
            statusBar()->showMessage("Wayland support enabled: " + displayInfo, 5000);
        }
    }
#else
    qDebug() << "DEBUG: ENABLE_WAYLAND_SUPPORT is not enabled";
    m_waylandSupported = false;
    m_waylandSecurityAction = nullptr;
    m_waylandOptimizationsAction = nullptr;
#endif
    qDebug() << "DEBUG: Exiting setupWaylandSupport()";
}

// Empty implementations for Wayland callbacks
void MainWindow::onWaylandBackendAvailabilityChanged(bool available) {
    Q_UNUSED(available);
}

void MainWindow::onWaylandOutputChanged() {
}

void MainWindow::onWaylandPermissionChanged(const QString& featureName, bool granted) {
    Q_UNUSED(featureName);
    Q_UNUSED(granted);
}

void MainWindow::onWaylandSecurityEvent(const QString& eventType, const QString& details) {
    Q_UNUSED(eventType);
    Q_UNUSED(details);
}

void MainWindow::onWaylandHardwareAccelerationStatusChanged(bool available) {
    Q_UNUSED(available);
}

void MainWindow::onWaylandPerformanceMetricsUpdated(const QVariantMap& metrics) {
    Q_UNUSED(metrics);
}

// Here the rest of the implementation would be placed, but we're focusing only on the Wayland part

// Add implementation for setupUi
void MainWindow::setupUi() {
    qDebug() << "DEBUG: Entering setupUi()";
    
    // Create the main tab widget
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Create tabs
    m_tabWidget = new QTabWidget(m_centralWidget);
    qDebug() << "DEBUG: Creating tab widget";
    m_mainLayout->addWidget(m_tabWidget);
    
    // Create Search tab
    m_searchTab = new QWidget();
    qDebug() << "DEBUG: Creating search tab";
    m_searchLayout = new QVBoxLayout(m_searchTab);
    m_searchLayout->setContentsMargins(10, 10, 10, 10);
    m_searchLayout->setSpacing(10);
    
    // Create search controls layout
    qDebug() << "DEBUG: Creating search controls";
    m_searchControlsLayout = new QHBoxLayout();
    m_searchInput = new QLineEdit(m_searchTab);
    m_searchInput->setPlaceholderText(tr("Search packages..."));
    m_searchButton = new QPushButton(tr("Search"), m_searchTab);
    m_searchButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    m_searchControlsLayout->addWidget(m_searchInput);
    m_searchControlsLayout->addWidget(m_searchButton);
    
    m_searchLayout->addLayout(m_searchControlsLayout);
    
    // Create package actions layout
    qDebug() << "DEBUG: Creating package action buttons";
    m_packageActionsLayout = new QHBoxLayout();
    m_installButton = new QPushButton(tr("Install"), m_searchTab);
    m_removeButton = new QPushButton(tr("Remove"), m_searchTab);
    
    m_packageActionsLayout->addWidget(m_installButton);
    m_packageActionsLayout->addWidget(m_removeButton);
    m_packageActionsLayout->addStretch();
    
    m_searchLayout->addLayout(m_packageActionsLayout);
    
    // Create packages table
    qDebug() << "DEBUG: Creating packages table";
    m_packagesTable = new QTreeView(m_searchTab);
    m_packagesTable->setAlternatingRowColors(true);
    m_packagesTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_packagesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    applyTableStyle(m_packagesTable);
    
    // Set model for packages table
    m_packagesTable->setModel(m_packagesModel);
    
    // Set column sizes for packages table
    m_packagesTable->header()->setSectionResizeMode(0, QHeaderView::Fixed); // Checkbox column - fixed width
    m_packagesTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Name column - resize to contents
    m_packagesTable->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Version column - resize to contents
    m_packagesTable->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Repository column - resize to contents
    m_packagesTable->header()->setSectionResizeMode(4, QHeaderView::Stretch); // Description column - stretch
    
    // Set minimum sizes - special handling for checkbox column
    m_packagesTable->header()->setMinimumSectionSize(2); // Allow extremely small sections
    m_packagesTable->setColumnWidth(0, 30); // Checkbox column - wider for better visibility
    
    m_searchLayout->addWidget(m_packagesTable);
    
    // Add tab
    qDebug() << "DEBUG: Adding search tab to main tab widget";
    m_tabWidget->addTab(m_searchTab, tr("Search"));
    
    // Create Installed tab
    m_installedTab = new QWidget();
    qDebug() << "DEBUG: Creating installed packages tab";
    m_installedLayout = new QVBoxLayout(m_installedTab);
    m_installedLayout->setContentsMargins(10, 10, 10, 10);
    m_installedLayout->setSpacing(10);
    
    // Create installed packages actions layout
    qDebug() << "DEBUG: Creating installed packages action buttons";
    m_installedActionsLayout = new QHBoxLayout();
    m_removeInstalledButton = new QPushButton(tr("Remove"), m_installedTab);
    
    m_installedActionsLayout->addWidget(m_removeInstalledButton);
    m_installedActionsLayout->addStretch();
    
    m_installedLayout->addLayout(m_installedActionsLayout);
    
    // Create installed packages table
    qDebug() << "DEBUG: Creating installed packages table";
    m_installedTable = new QTreeView(m_installedTab);
    m_installedTable->setAlternatingRowColors(true);
    m_installedTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_installedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    applyTableStyle(m_installedTable);
    
    // Set model for installed packages table
    m_installedTable->setModel(m_installedModel);
    
    // Set column sizes for installed packages table
    m_installedTable->header()->setSectionResizeMode(0, QHeaderView::Fixed); // Checkbox column - fixed width
    m_installedTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Name column - resize to contents
    m_installedTable->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Version column - resize to contents
    m_installedTable->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Repository column - resize to contents
    m_installedTable->header()->setSectionResizeMode(4, QHeaderView::Stretch); // Description column - stretch
    
    // Set minimum sizes - special handling for checkbox column
    m_installedTable->header()->setMinimumSectionSize(2); // Allow extremely small sections
    m_installedTable->setColumnWidth(0, 30); // Checkbox column - wider for better visibility
    
    m_installedLayout->addWidget(m_installedTable);
    
    // Add tab
    qDebug() << "DEBUG: Adding installed packages tab to main tab widget";
    m_tabWidget->addTab(m_installedTab, tr("Installed Packages"));
    
    // Allow the tabs to be closable if needed
    m_tabWidget->setTabsClosable(false);
    
    // Create a status bar
    statusBar()->showMessage(tr("Ready"));
    
    // Add flatpak management tab
    setupFlatpakTab();
    
    qDebug() << "DEBUG: Exiting setupUi()";
}

// Helper method to apply consistent styling to tables
void MainWindow::applyTableStyle(QTreeView* tableView)
{
    tableView->setStyleSheet(
        "QTreeView { "
        "  background-color: #1e1e2e; "
        "  alternate-background-color: #2a2a3a; "
        "  color: #ffffff; "
        "  selection-background-color: #3daee9; "
        "  selection-color: #ffffff; "
        "  border: none; "
        "} "
        "QTreeView::item { "
        "  padding: 4px; "
        "  border: none; "
        "} "
        "QTreeView::item:selected { "
        "  background-color: #3daee9; "
        "} "
        "QTreeView::indicator { "
        "  width: 12px; "
        "  height: 12px; "
        "  background-color: #2a2a3a; "
        "  border: 1px solid #4a4a5a; "
        "  border-radius: 2px; "
        "} "
        "QTreeView::indicator:checked { "
        "  background-color: #3daee9; "
        "  image: url(:/icons/check.png); "
        "}"
    );
}

// Add implementation for setupActions
void MainWindow::setupActions() {
    // Create file actions
    m_exitAction = new QAction(tr("Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    
    // Create edit actions
    m_settingsAction = new QAction(tr("Settings"), this);
    
    // Create package actions
    m_syncAllAction = new QAction(tr("Sync All"), this);
    m_checkForUpdatesAction = new QAction(tr("Check for Updates"), this);
    // Removed batch install action as per plan.md
    
    // Create maintenance actions
    m_clearPackageCacheAction = new QAction(tr("Clear Package Cache"), this);
    m_removeOrphansAction = new QAction(tr("Remove Orphaned Packages"), this);
    m_checkDatabaseAction = new QAction(tr("Check Database"), this);
    m_findPacnewFilesAction = new QAction(tr("Find .pacnew Files"), this);
    m_backupDatabaseAction = new QAction(tr("Backup Database"), this);
    m_restoreDatabaseAction = new QAction(tr("Restore Database"), this);
    
    // Create theme action (used only in settings now)
    m_toggleThemeAction = new QAction(tr("Toggle Dark/Light Theme"), this);
    
    // Create help actions
    m_aboutAction = new QAction(tr("About"), this);
}

// Add implementation for setupMenus
void MainWindow::setupMenus() {
    // Create file menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_exitAction);
    
    // Create edit menu
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_settingsAction);
    
    // Create package menu
    m_packageMenu = menuBar()->addMenu(tr("&Packages"));
    m_packageMenu->addAction(m_syncAllAction);
    m_packageMenu->addAction(m_checkForUpdatesAction);
    // Remove batch install as requested in plan.md
    
    // Create maintenance menu
    m_maintenanceMenu = menuBar()->addMenu(tr("&Maintenance"));
    m_maintenanceMenu->addAction(m_clearPackageCacheAction);
    m_maintenanceMenu->addAction(m_removeOrphansAction);
    m_maintenanceMenu->addSeparator();
    m_maintenanceMenu->addAction(m_checkDatabaseAction);
    m_maintenanceMenu->addAction(m_findPacnewFilesAction);
    m_maintenanceMenu->addSeparator();
    m_maintenanceMenu->addAction(m_backupDatabaseAction);
    m_maintenanceMenu->addAction(m_restoreDatabaseAction);
    
    // Remove view menu since themes are managed in settings
    
    // Create help menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
    
    // Add Wayland menu items if supported
    if (m_waylandSupported) {
        m_waylandMenu = menuBar()->addMenu(tr("&Wayland"));
        m_waylandMenu->addAction(m_waylandSecurityAction);
        m_waylandMenu->addAction(m_waylandOptimizationsAction);
    }
}

// Add implementation for setupConnections
void MainWindow::setupConnections() {
    qDebug() << "DEBUG: Entering setupConnections()";
    
    // Connect tab changed signal
    qDebug() << "DEBUG: Connecting tab changed signal";
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    
    // Connect search signals
    qDebug() << "DEBUG: Connecting search signals";
    // Remove the automatic search on text change which causes UI freezing
    // connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    
    // Only search when button is clicked or Enter is pressed
    connect(m_searchInput, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked);
    connect(m_searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    
    // Connect package table signals
    qDebug() << "DEBUG: Connecting package table signals";
    connect(m_packagesTable, &QTreeView::clicked, this, &MainWindow::onPackageSelected);
    
    // Connect selection model changes
    qDebug() << "DEBUG: Connecting selection model changes";
    connect(m_packagesTable->selectionModel(), &QItemSelectionModel::selectionChanged, 
            this, [this]() { updateInstallButtonText(); });
            
    // Connect package action buttons
    qDebug() << "DEBUG: Connecting package action buttons";
    qDebug() << "DEBUG: m_installButton = " << (m_installButton ? "valid" : "NULL");
    if (m_installButton) {
        connect(m_installButton, &QPushButton::clicked, this, &MainWindow::onInstallPackage);
    }
    
    qDebug() << "DEBUG: m_updateButton = " << (m_updateButton ? "valid" : "NULL");
    if (m_updateButton) {
        connect(m_updateButton, &QPushButton::clicked, this, &MainWindow::onUpdatePackage);
    }
    
    qDebug() << "DEBUG: m_removeButton = " << (m_removeButton ? "valid" : "NULL");
    if (m_removeButton) {
        connect(m_removeButton, &QPushButton::clicked, this, &MainWindow::onRemovePackage);
    }
    
    qDebug() << "DEBUG: m_installAurButton = " << (m_installAurButton ? "valid" : "NULL");
    if (m_installAurButton) {
        connect(m_installAurButton, &QPushButton::clicked, this, &MainWindow::onInstallAurPackage);
    }
    
    // Connect installed package action buttons
    qDebug() << "DEBUG: Connecting installed package action buttons";
    qDebug() << "DEBUG: m_updateInstalledButton = " << (m_updateInstalledButton ? "valid" : "NULL");
    if (m_updateInstalledButton) {
        connect(m_updateInstalledButton, &QPushButton::clicked, this, &MainWindow::onUpdatePackage);
    }
    
    qDebug() << "DEBUG: m_removeInstalledButton = " << (m_removeInstalledButton ? "valid" : "NULL");
    if (m_removeInstalledButton) {
        connect(m_removeInstalledButton, &QPushButton::clicked, this, &MainWindow::onRemovePackage);
    }
    
    // Connect menu actions
    qDebug() << "DEBUG: Connecting menu actions";
    qDebug() << "DEBUG: m_exitAction = " << (m_exitAction ? "valid" : "NULL");
    if (m_exitAction) {
        connect(m_exitAction, &QAction::triggered, this, [this]() { close(); });
    }
    
    qDebug() << "DEBUG: m_settingsAction = " << (m_settingsAction ? "valid" : "NULL");
    if (m_settingsAction) {
        connect(m_settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    }
    
    qDebug() << "DEBUG: m_syncAllAction = " << (m_syncAllAction ? "valid" : "NULL");
    if (m_syncAllAction) {
        connect(m_syncAllAction, &QAction::triggered, this, &MainWindow::onSyncAll);
    }
    
    qDebug() << "DEBUG: m_checkForUpdatesAction = " << (m_checkForUpdatesAction ? "valid" : "NULL");
    if (m_checkForUpdatesAction) {
        connect(m_checkForUpdatesAction, &QAction::triggered, this, &MainWindow::onCheckForUpdates);
    }
    
    // Connect system update buttons
    qDebug() << "DEBUG: Connecting system update buttons";
    qDebug() << "DEBUG: m_systemUpdateButton = " << (m_systemUpdateButton ? "valid" : "NULL");
    if (m_systemUpdateButton) {
        connect(m_systemUpdateButton, &QPushButton::clicked, this, &MainWindow::onSystemUpdate);
    }
    
    qDebug() << "DEBUG: m_checkUpdatesButton = " << (m_checkUpdatesButton ? "valid" : "NULL");
    if (m_checkUpdatesButton) {
        connect(m_checkUpdatesButton, &QPushButton::clicked, this, &MainWindow::onCheckForUpdates);
    }
    
    // Connect maintenance buttons
    qDebug() << "DEBUG: Connecting maintenance buttons";
    qDebug() << "DEBUG: m_clearCacheButton = " << (m_clearCacheButton ? "valid" : "NULL");
    if (m_clearCacheButton) {
        connect(m_clearCacheButton, &QPushButton::clicked, this, &MainWindow::onClearPackageCache);
    }
    
    qDebug() << "DEBUG: m_removeOrphansButton = " << (m_removeOrphansButton ? "valid" : "NULL");
    if (m_removeOrphansButton) {
        connect(m_removeOrphansButton, &QPushButton::clicked, this, &MainWindow::onRemoveOrphans);
    }
    
    qDebug() << "DEBUG: Exiting setupConnections()";
}

// Add implementation for setupSystemUpdateTab
void MainWindow::setupSystemUpdateTab() {
    // Create system update tab
    m_systemUpdateTab = new QWidget();
    m_systemUpdateLayout = new QVBoxLayout(m_systemUpdateTab);
    
    // Create system update action buttons at the top
    m_systemUpdateActionsLayout = new QHBoxLayout();
    m_checkUpdatesButton = new QPushButton(tr("Check for Updates"), m_systemUpdateTab);
    m_systemUpdateButton = new QPushButton(tr("Update System"), m_systemUpdateTab);
    
    // Make buttons consistent with the rest of the UI
    QString buttonStyle = "QPushButton {"
                         "background-color: #0078d7; "
                         "color: white;"
                         "border: none;"
                         "border-radius: 3px;"
                         "padding: 6px 12px;"
                         "margin: 2px 4px;"
                         "}"
                         "QPushButton:hover {"
                         "background-color: #0063b1;"
                         "}"
                         "QPushButton:pressed {"
                         "background-color: #004b8d;"
                         "}";
    
    // Place Check for Updates first, then Update System with consistent styling
    m_checkUpdatesButton->setStyleSheet(buttonStyle);
    m_checkUpdatesButton->setMinimumWidth(130);
    m_checkUpdatesButton->setMinimumHeight(26);
    m_checkUpdatesButton->setMaximumHeight(30);
    m_checkUpdatesButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    m_systemUpdateButton->setStyleSheet(buttonStyle);
    m_systemUpdateButton->setMinimumWidth(110);
    m_systemUpdateButton->setMinimumHeight(26);
    m_systemUpdateButton->setMaximumHeight(30);
    m_systemUpdateButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    m_systemUpdateActionsLayout->addWidget(m_checkUpdatesButton);
    m_systemUpdateActionsLayout->addWidget(m_systemUpdateButton);
    m_systemUpdateActionsLayout->addStretch(1); // Push buttons to the left
    
    m_systemUpdateLayout->addLayout(m_systemUpdateActionsLayout);
    
    // Create status label
    m_systemUpdateInfoLabel = new QLabel(tr("Check for updates to see available package upgrades."), m_systemUpdateTab);
    m_systemUpdateInfoLabel->setStyleSheet("color: #666666; font-size: 11px;");
    m_systemUpdateInfoLabel->setWordWrap(true);
    m_systemUpdateLayout->addWidget(m_systemUpdateInfoLabel);
    
    // Create system update table
    m_systemUpdatesTable = new QTreeView(m_systemUpdateTab);
    m_systemUpdatesTable->setSelectionMode(QAbstractItemView::ExtendedSelection); // Allow multiple selection
    m_systemUpdatesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_systemUpdatesTable->setSortingEnabled(true);
    m_systemUpdatesTable->setAlternatingRowColors(true);
    
    m_systemUpdatesModel = new QStandardItemModel(0, 5, this);
    m_systemUpdatesModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Current") << tr("New") << tr("Repository"));
    m_systemUpdatesTable->setModel(m_systemUpdatesModel);
    
    // Update column sizing to account for checkbox column
    m_systemUpdatesTable->header()->setSectionResizeMode(0, QHeaderView::Fixed);  // Checkbox column
    m_systemUpdatesTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Name
    m_systemUpdatesTable->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Current version
    m_systemUpdatesTable->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // New version
    m_systemUpdatesTable->header()->setSectionResizeMode(4, QHeaderView::Stretch); // Repository
    
    // Set checkbox column width for system updates table
    m_systemUpdatesTable->header()->setMinimumSectionSize(2); // Allow extremely small sections
    m_systemUpdatesTable->setColumnWidth(0, 2); // Set to absolute minimum 2px
    
    m_systemUpdateLayout->addWidget(m_systemUpdatesTable);
    
    // Create update log view
    m_systemUpdateLogView = new QTextEdit(m_systemUpdateTab);
    m_systemUpdateLogView->setReadOnly(true);
    m_systemUpdateLogView->setMaximumHeight(120);
    m_systemUpdateLogView->setPlaceholderText(tr("Update log will appear here..."));
    m_systemUpdateLogView->setStyleSheet(
        "QTextEdit {"
        "    background-color: #1e1e2e;"
        "    color: #ffffff;"
        "    border: 1px solid #3daee9;"
        "    border-radius: 2px;"
        "    padding: 2px;"
        "    font-family: monospace;"
        "}"
    );
    m_systemUpdateLayout->addWidget(m_systemUpdateLogView);
    
    
    // Add system update tab to tabwidget
    m_tabWidget->addTab(m_systemUpdateTab, tr("System Update"));
}

// Add implementation for setupMaintenanceTab
void MainWindow::setupMaintenanceTab() {
    // Create maintenance tab
    m_maintenanceTab = new QWidget();
    
    // Use grid layout with better spacing
    QGridLayout* gridLayout = new QGridLayout(m_maintenanceTab);
    gridLayout->setSpacing(10);  // Add spacing between elements
    gridLayout->setContentsMargins(15, 15, 15, 15);  // Add margins
    
    m_maintenanceLayout = new QVBoxLayout(); // Keep this for backward compatibility
    
    // Create maintenance action descriptions with better styling
    struct MaintenanceAction {
        QString buttonText;
        QString description;
        QPushButton** buttonPtr;
        QString tooltip;
    };
    
    MaintenanceAction actions[] = {
        {tr("Clear Package Cache"), 
         tr("Remove downloaded package files to free disk space."), 
         &m_clearCacheButton,
         tr("Removes downloaded package files from the cache.")},
         
        {tr("Remove Orphaned Packages"), 
         tr("Remove packages that are no longer required by any other package."), 
         &m_removeOrphansButton,
         tr("Finds and removes packages that are not dependencies of any installed package.")},
         
        {tr("Check Database"), 
         tr("Verify integrity of the package database."), 
         &m_checkDatabaseButton,
         tr("Checks the package database for inconsistencies and errors.")},
         
        {tr("Find .pacnew Files"), 
         tr("Find configuration files that need to be merged after updates."), 
         &m_findPacnewButton,
         tr("Locates .pacnew and .pacsave files that need attention after package updates.")},
         
        {tr("Backup Database"), 
         tr("Create a backup of the package database."), 
         &m_backupDatabaseButton,
         tr("Saves a copy of the package database to restore later if needed.")},
         
        {tr("Restore Database"), 
         tr("Restore a previous backup of the package database."), 
         &m_restoreDatabaseButton,
         tr("Replaces the current package database with a previously created backup.")}
    };
    
    // Common button style
    QString buttonStyle = 
        "QPushButton {"
        "    background-color: #0078d7;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 3px;"
        "    padding: 5px 10px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2980b9;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1c6ea4;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #bdc3c7;"
        "}";
    
    int row = 0;
    for (const auto& action : actions) {
        // Create button with reduced size and better styling
        *(action.buttonPtr) = new QPushButton(action.buttonText, m_maintenanceTab);
        (*(action.buttonPtr))->setMaximumHeight(32); // Reduced from 40
        (*(action.buttonPtr))->setMinimumWidth(180);
        (*(action.buttonPtr))->setMaximumWidth(200);
        (*(action.buttonPtr))->setToolTip(action.tooltip);
        (*(action.buttonPtr))->setStyleSheet(buttonStyle);
        
        // Create description label with better styling
        QLabel* description = new QLabel(action.description, m_maintenanceTab);
        description->setWordWrap(true);
        description->setStyleSheet("color: #666666; font-size: 11px;");
        
        // Add to grid layout
        gridLayout->addWidget(*(action.buttonPtr), row, 0);
        gridLayout->addWidget(description, row, 1);
        
        // Add a separator line after each item except the last one
        if (row < 5) {
            QFrame* line = new QFrame(m_maintenanceTab);
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            line->setStyleSheet("color: #e0e0e0;");
            gridLayout->addWidget(line, row + 1, 0, 1, 2);
            row += 2; // Skip a row for the separator
        } else {
            row++;
        }
    }
    
    // Add spacer item at the bottom with less space
    gridLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding), 
                       row, 0, 1, 2);
    
    // Create log view with better styling
    m_maintenanceLogView = new QTextEdit(m_maintenanceTab);
    m_maintenanceLogView->setReadOnly(true);
    m_maintenanceLogView->setMinimumHeight(150); // Reduced from 200
    m_maintenanceLogView->setPlaceholderText(tr("Maintenance operations log will appear here..."));
    m_maintenanceLogView->setStyleSheet(
        "QTextEdit {"
        "    background-color: #1e1e2e;"
        "    color: #ffffff;"
        "    border: 1px solid #3daee9;"
        "    border-radius: 2px;"
        "    padding: 2px;"
        "    font-family: monospace;"
        "}"
    );
    
    // Create progress bar with better styling
    m_maintenanceProgressBar = new QProgressBar(m_maintenanceTab);
    m_maintenanceProgressBar->setRange(0, 100);
    m_maintenanceProgressBar->setValue(0);
    m_maintenanceProgressBar->setVisible(false);
    m_maintenanceProgressBar->setStyleSheet("QProgressBar {border: 1px solid #e0e0e0; border-radius: 3px; text-align: center;} "
                                           "QProgressBar::chunk {background-color: #0078d7;}");
    
    // Add log view and progress bar to the grid layout
    gridLayout->addWidget(m_maintenanceLogView, ++row, 0, 1, 2);
    gridLayout->addWidget(m_maintenanceProgressBar, ++row, 0, 1, 2);
    
    // Add maintenance tab to tabwidget
    m_tabWidget->addTab(m_maintenanceTab, tr("Maintenance"));
}

// Add implementation for setupDetailPanel
void MainWindow::setupDetailPanel() {
    // Create a simple detail panel
    m_detailPanel = new QWidget(this);
    m_detailPanel->setFixedWidth(300);
    m_detailPanel->setMaximumWidth(300);
    m_detailPanel->hide();
    
    m_detailLayout = new QVBoxLayout(m_detailPanel);
    
    QLabel* placeholderLabel = new QLabel(tr("Detail panel functionality to be implemented"), m_detailPanel);
    m_detailLayout->addWidget(placeholderLabel);
}

// Add implementation for loadSettings
void MainWindow::loadSettings() {
    QSettings settings("PacmanGUI", "PacmanGUI");
    
    // Load window geometry
    QByteArray geometry = settings.value("window/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    
    // Load theme settings
    m_darkTheme = settings.value("appearance/darkTheme", true).toBool();
    
    // Load Flatpak settings
    if (m_packageManager.is_flatpak_available()) {
        m_flatpakSearchEnabled = settings.value("flatpak/enabled", false).toBool();
    } else {
        m_flatpakSearchEnabled = false;
    }
}

// Add implementation for isDarkThemeEnabled
bool MainWindow::isDarkThemeEnabled() const {
    return m_darkTheme;
}

// Add implementation for applyTheme
void MainWindow::applyTheme(bool isDark) {
    // Save the setting
    QSettings settings("PacmanGUI", "PacmanGUI");
    settings.setValue("appearance/darkTheme", isDark);
    
    // Update the action state if it exists
    if (m_toggleThemeAction) {
        m_toggleThemeAction->setChecked(isDark);
    }
    
    // Get the theme name
    QString themeName = isDark ? "dark" : "light";
    
    // Try to load the stylesheet from multiple locations
    QString styleContent;
    bool loaded = false;
    
    // Paths to try in order
    QStringList stylePaths = {
        QApplication::applicationDirPath() + "/../resources/styles/" + themeName + ".qss",         // From build directory
        QApplication::applicationDirPath() + "/resources/styles/" + themeName + ".qss",            // From install directory
        QDir::currentPath() + "/resources/styles/" + themeName + ".qss",                           // From current directory
        QDir::currentPath() + "/../resources/styles/" + themeName + ".qss"                         // From parent directory
    };
    
    for (const QString& stylePath : stylePaths) {
        QFile file(stylePath);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            styleContent = file.readAll();
            file.close();
            qDebug() << "Successfully loaded" << themeName << "theme from" << stylePath;
            loaded = true;
            break;
        }
    }
    
    if (!loaded) {
        qDebug() << "Failed to load" << themeName << "theme from any location. Tried:";
        for (const QString& path : stylePaths) {
            qDebug() << "  -" << path;
        }
        
        // Fallback to built-in minimal theme
        if (isDark) {
            styleContent = "QWidget { background-color: #2D2D30; color: #E0E0E0; }"
                           "QMainWindow { background-color: #1E1E1E; }"
                           "QTreeView, QTableView { background-color: #252526; alternate-background-color: #2D2D30; color: #E0E0E0; }"
                           "QLineEdit { background-color: #333337; color: #E0E0E0; border: 1px solid #3F3F46; }";
                } else {
            styleContent = "QWidget { background-color: #F0F0F0; color: #202020; }"
                           "QMainWindow { background-color: #FFFFFF; }"
                           "QTreeView, QTableView { background-color: #FFFFFF; alternate-background-color: #F5F5F5; color: #202020; }"
                           "QLineEdit { background-color: #FFFFFF; color: #202020; border: 1px solid #C0C0C0; }";
        }
        qDebug() << "Using fallback built-in theme";
    }
    
    // Apply the theme
    qApp->setStyleSheet(styleContent);
}

// Add implementation for showStatusMessage
void MainWindow::showStatusMessage(const QString& message, int timeout) {
    statusBar()->showMessage(message, timeout);
}

// Add implementation for refreshInstalledPackages
void MainWindow::refreshInstalledPackages() {
    qDebug() << "Refreshing installed packages";
    m_installedModel->clear();
    m_installedModel->setHorizontalHeaderLabels(QStringList() << tr("") << tr("Name") << tr("Version") << tr("Repository") << tr("Description"));

    std::vector<pacmangui::core::Package> installedPackages = m_packageManager.get_installed_packages();
    for (const auto& pkg : installedPackages) {
        QList<QStandardItem*> row;
        
        // Add checkbox item
        QStandardItem* checkItem = new QStandardItem();
        checkItem->setCheckable(true);
        checkItem->setCheckState(Qt::Unchecked);
        checkItem->setData(Qt::AlignCenter, Qt::TextAlignmentRole);
        
        QStandardItem* nameItem = new QStandardItem(QString::fromStdString(pkg.get_name()));
        QStandardItem* versionItem = new QStandardItem(QString::fromStdString(pkg.get_version()));
        QStandardItem* repoItem = new QStandardItem(QString::fromStdString(pkg.get_repository()));
        QStandardItem* descItem = new QStandardItem(QString::fromStdString(pkg.get_description()));
        
        row << checkItem << nameItem << versionItem << repoItem << descItem;
        m_installedModel->appendRow(row);
    }

    // Set column widths after populating data
    m_installedTable->setColumnWidth(0, 30);  // Checkbox column
    m_installedTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // Name column resize to content
    m_installedTable->setColumnWidth(2, 100);  // Version column
    m_installedTable->setColumnWidth(3, 100);  // Repository column
    m_installedTable->header()->setSectionResizeMode(4, QHeaderView::Stretch);  // Description column takes remaining space

    showStatusMessage(tr("Loaded %1 installed packages").arg(installedPackages.size()), 3000);
}

// Add implementation for searchPackages
void MainWindow::searchPackages(const QString& searchTerm) {
    // This is now a wrapper around performAsyncSearch
    performAsyncSearch(searchTerm);
}

// Implementation of async search
void MainWindow::performAsyncSearch(const QString& searchTerm) {
    qDebug() << "Performing async search for:" << searchTerm;
    
    // Initialize model
    m_packagesModel->clear();
    m_packagesModel->setHorizontalHeaderLabels(QStringList() << tr("") << tr("Name") << tr("Version") << tr("Repository") << tr("Description"));
    
    if (searchTerm.isEmpty()) {
        showStatusMessage(tr("Please enter a search term"), 3000);
        return;
    }
    
    showStatusMessage(tr("Searching for packages matching '%1'...").arg(searchTerm), 0);
    
    QFuture<std::vector<pacmangui::core::Package>> future = QtConcurrent::run(
        [this, searchTerm]() {
            return m_packageManager.search_by_name(searchTerm.toStdString());
        }
    );
    
    QFutureWatcher<std::vector<pacmangui::core::Package>>* watcher = new QFutureWatcher<std::vector<pacmangui::core::Package>>();
    
    connect(watcher, &QFutureWatcher<std::vector<pacmangui::core::Package>>::finished, this, [this, watcher, searchTerm]() {
        std::vector<pacmangui::core::Package> results = watcher->result();
        
        // Add packages to model
        for (const auto& pkg : results) {
            QList<QStandardItem*> row;
            
            // Add checkbox item
            QStandardItem* checkItem = new QStandardItem();
            checkItem->setCheckable(true);
            checkItem->setCheckState(Qt::Unchecked);
            checkItem->setData(Qt::AlignCenter, Qt::TextAlignmentRole);
            
            QStandardItem* nameItem = new QStandardItem(QString::fromStdString(pkg.get_name()));
            QStandardItem* versionItem = new QStandardItem(QString::fromStdString(pkg.get_version()));
            
            // Get repository name
            QString repo = QString::fromStdString(pkg.get_repository());
            
            // Handle repository display
            if (repo.toLower() == "aur") {
                // Mark AUR packages
                checkItem->setData("aur", Qt::UserRole + 1);
                nameItem->setData("aur", Qt::UserRole + 1);
                repo = "AUR";
            } else if (repo.toLower().contains("cachyos")) {
                // Handle CachyOS repositories
                repo = repo.toUpper(); // Convert to uppercase for consistency
            } else if (repo.toLower() == "chaotic-aur") {
                // Handle Chaotic AUR
                repo = "Chaotic-AUR";
            }
            
            QStandardItem* repoItem = new QStandardItem(repo);
            QStandardItem* descItem = new QStandardItem(QString::fromStdString(pkg.get_description()));
            
            row << checkItem << nameItem << versionItem << repoItem << descItem;
            m_packagesModel->appendRow(row);
        }
        
        // Set column widths after populating data
        m_packagesTable->setColumnWidth(0, 30);  // Checkbox column
        m_packagesTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // Name column resize to content
        m_packagesTable->setColumnWidth(2, 100);  // Version column
        m_packagesTable->setColumnWidth(3, 120);  // Repository column - slightly wider for readability
        m_packagesTable->header()->setSectionResizeMode(4, QHeaderView::Stretch);  // Description column takes remaining space
        
        showStatusMessage(tr("Found %1 packages matching '%2'").arg(results.size()).arg(searchTerm), 3000);
        
        watcher->deleteLater();
    });
    
    watcher->setFuture(future);
}

// Add implementation for checkAurHelper
void MainWindow::checkAurHelper() {
    // Simple check for common AUR helpers
        QProcess process;
    QStringList helpers = {"yay", "paru", "trizen", "aurman", "pamac"};
    
    for (const QString& helper : helpers) {
        process.start("which", QStringList() << helper);
        process.waitForFinished();
        
        if (process.exitCode() == 0) {
            m_aurHelper = helper;
            if (m_installAurButton)
                m_installAurButton->setEnabled(true);
            showStatusMessage(tr("Found AUR helper: %1").arg(helper), 3000);
                return;
            }
    }
    
    // No AUR helper found
    m_aurHelper = "";
    if (m_installAurButton)
        m_installAurButton->setEnabled(false);
    showStatusMessage(tr("No AUR helper found. AUR functions will be disabled."), 5000);
}

// Add implementation for onTabChanged
void MainWindow::onTabChanged(int index) {
    Q_UNUSED(index);
    // Implement onTabChanged
}

// Add implementation for onSearchTextChanged
void MainWindow::onSearchTextChanged(const QString& text) {
    // Auto-search after a short delay
    if (text.length() >= 2) {
        // Don't immediately search on each keystroke, let the user finish typing
        // We can add a timer for debouncing if needed
        performAsyncSearch(text);
        
        // If Flatpak search is enabled, also search for Flatpak packages
        if (m_flatpakSearchEnabled && m_packageManager.is_flatpak_available()) {
            performAsyncFlatpakSearch(text);
        }
    } else if (text.isEmpty()) {
        // Clear the search results
        m_packagesModel->clear();
        m_packagesModel->setHorizontalHeaderLabels(
            QStringList() << tr("") << tr("Name") << tr("Version") << tr("Repository") << tr("Description"));
    }
}

// Add implementation for onSearchClicked
void MainWindow::onSearchClicked()
{
    QString text = m_searchInput->text();
    if (text.isEmpty()) {
        showStatusMessage(tr("Please enter a search term"), 3000);
                return;
            }
            
    // Clear the model
    m_packagesModel->removeRows(0, m_packagesModel->rowCount());
    
    // Update status bar
    showStatusMessage(tr("Searching for packages matching '%1'...").arg(text), 0);
    
    // Perform the search
    qDebug() << "Performing async search for:" << text;
    performAsyncSearch(text);
    
    // If Flatpak support is enabled, also search for Flatpak packages
    if (m_packageManager.is_flatpak_available() && m_flatpakSearchEnabled) {
        qDebug() << "Flatpak search is enabled, searching for Flatpak packages";
        performAsyncFlatpakSearch(text);
    } else {
        qDebug() << "Flatpak search is disabled or not available";
    }
}

// Add implementation for onInstallPackage
void MainWindow::onInstallPackage() {
    // Get selected packages
    QModelIndexList selected = m_packagesTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        showStatusMessage(tr("No packages selected for installation"), 3000);
        return;
    }
    
    // Collect package names and separate Flatpak packages
    QStringList packageNames;
    QStringList flatpakPackages;
    QStringList packageDetails;
    
    for (const QModelIndex& index : selected) {
        QModelIndex nameIndex = m_packagesModel->index(index.row(), 1); // Name column
        QModelIndex versionIndex = m_packagesModel->index(index.row(), 2); // Version column
        QModelIndex repoIndex = m_packagesModel->index(index.row(), 3); // Repository column
        
        QString name = m_packagesModel->data(nameIndex).toString();
        QString version = m_packagesModel->data(versionIndex).toString();
        QString repo = m_packagesModel->data(repoIndex).toString();
        
        // Check if this is a Flatpak package
        QVariant packageType = nameIndex.data(Qt::UserRole + 1);
        bool isFlatpak = (packageType.toString() == "flatpak");
        
        // Get app ID for Flatpak packages
        QString appId = nameIndex.data(Qt::UserRole).toString();
        if (isFlatpak && appId.isEmpty()) {
            // If no app ID is stored, use the name
            appId = name;
        }
        
        if (isFlatpak) {
            flatpakPackages.append(appId);
            packageDetails.append(tr("%1 (%2) [Flatpak]").arg(name).arg(version));
        } else {
            packageNames.append(name);
            packageDetails.append(tr("%1 (%2) from %3").arg(name).arg(version).arg(repo));
        }
    }
    
    // Check if we have any packages to install
    if (packageNames.isEmpty() && flatpakPackages.isEmpty()) {
        showStatusMessage(tr("No valid packages selected for installation"), 3000);
        return;
    }
    
    // Confirm installation
    QString message;
    if (packageNames.size() + flatpakPackages.size() == 1) {
        message = tr("Are you sure you want to install %1?\n\nThis operation will require your password for authentication.").arg(packageDetails.first());
                } else {
        message = tr("Are you sure you want to install the following %1 packages?\n\n%2\n\nThis operation will require your password for authentication.")
            .arg(packageNames.size() + flatpakPackages.size())
            .arg(packageDetails.join("\n"));
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
            this,
        tr("Confirm Installation"),
        message,
        QMessageBox::Yes|QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
                return;
            }
            
    // Show status message
    showStatusMessage(tr("Installing packages..."), 0);
    
    // Install regular packages if any
    if (!packageNames.isEmpty()) {
        // Find available terminal emulator
        QString terminal = findTerminalEmulator();
        QStringList args;
        QString packagesStr = packageNames.join(" ");
        
        // Set up the command based on terminal type
        if (terminal == "konsole" || terminal == "gnome-terminal" || terminal == "xfce4-terminal" || terminal == "mate-terminal") {
            args << "-e" << QString("sudo pacman -S --noconfirm %1; read -p 'Press Enter to close'").arg(packagesStr);
        } else if (terminal == "kitty" || terminal == "alacritty") {
            args << "-e" << "sudo" << "pacman" << "-S" << "--noconfirm";
            args.append(packageNames);
                    } else {
            // Default for xterm and others
            args << "-e" << QString("sudo pacman -S --noconfirm %1 && echo 'Press ENTER to close this window' && read").arg(packagesStr);
        }
        
        // Run the command in the terminal
        QProcess::startDetached(terminal, args);
    }
    
    // Install Flatpak packages if any
    if (!flatpakPackages.isEmpty()) {
        // Find available terminal emulator
        QString terminal = findTerminalEmulator();
        QStringList args;
        
        // Build the command to install all Flatpak packages
        QString flatpakInstallCmd = "flatpak install -y flathub " + flatpakPackages.join(" ");
        
        // Set up the command based on terminal type
        if (terminal == "konsole" || terminal == "gnome-terminal" || terminal == "xfce4-terminal" || terminal == "mate-terminal") {
            args << "-e" << QString("%1; read -p 'Press Enter to close'").arg(flatpakInstallCmd);
        } else if (terminal == "kitty" || terminal == "alacritty") {
            args << "-e" << "sh" << "-c" << QString("%1; read -p 'Press Enter to close'").arg(flatpakInstallCmd);
                } else {
            // Default for xterm and others
            args << "-e" << QString("%1 && echo 'Press ENTER to close this window' && read").arg(flatpakInstallCmd);
        }
        
        // Run the command in the terminal
        QProcess::startDetached(terminal, args);
    }
    
    // Schedule a refresh of installed packages after some time
    QTimer::singleShot(10000, this, &MainWindow::refreshInstalledPackages);
    
    // Also refresh Flatpak list if we installed any Flatpak packages
    if (!flatpakPackages.isEmpty()) {
        QTimer::singleShot(10000, this, &MainWindow::refreshFlatpakList);
    }
}

// Add implementation for onRemovePackage
void MainWindow::onRemovePackage() {
    // Get selected packages
    QModelIndexList selected;
    
    // Get selection from the active tab
    if (m_tabWidget->currentIndex() == 0) { // Search tab
        selected = m_packagesTable->selectionModel()->selectedRows();
    } else if (m_tabWidget->currentIndex() == 1) { // Installed tab
        selected = m_installedTable->selectionModel()->selectedRows();
    }
    
    if (selected.isEmpty()) {
        showStatusMessage(tr("No packages selected for removal"), 3000);
        return;
    }
    
    // Collect package names
    QStringList packageNames;
    QStringList flatpakPackages;
    QStringList packageDetails;
    
    QStandardItemModel* model = m_tabWidget->currentIndex() == 0 ? m_packagesModel : m_installedModel;
    
    for (const QModelIndex& index : selected) {
        QModelIndex nameIndex = model->index(index.row(), 1); // Name column
        QModelIndex versionIndex = model->index(index.row(), 2); // Version column
        
        QString name = model->data(nameIndex).toString();
        QString version = model->data(versionIndex).toString();
        
        // Check if this is a Flatpak package
        QVariant packageType = nameIndex.data(Qt::UserRole + 1);
        bool isFlatpak = (packageType.toString() == "flatpak");
        
        // Get app ID for Flatpak packages
        QString appId = nameIndex.data(Qt::UserRole).toString();
        if (isFlatpak && appId.isEmpty()) {
            // If no app ID is stored, use the name
            appId = name;
        }
        
        if (isFlatpak) {
            flatpakPackages.append(appId);
            packageDetails.append(tr("%1 (%2) [Flatpak]").arg(name).arg(version));
        } else {
            packageNames.append(name);
            packageDetails.append(tr("%1 (%2)").arg(name).arg(version));
        }
    }
    
    // Check if we have any packages to remove
    if (packageNames.isEmpty() && flatpakPackages.isEmpty()) {
        showStatusMessage(tr("No valid packages selected for removal"), 3000);
        return;
    }
    
    // Confirm removal
    QString message;
    if (packageNames.size() + flatpakPackages.size() == 1) {
        message = tr("Are you sure you want to remove %1?\n\nThis operation will require your password for authentication.").arg(packageDetails.first());
    } else {
        message = tr("Are you sure you want to remove the following %1 packages?\n\n%2\n\nThis operation will require your password for authentication.")
            .arg(packageNames.size() + flatpakPackages.size())
            .arg(packageDetails.join("\n"));
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        tr("Confirm Removal"),
        message,
        QMessageBox::Yes|QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Show status message
    showStatusMessage(tr("Removing packages..."), 0);
    
    // Remove regular packages if any
    if (!packageNames.isEmpty()) {
        // Find available terminal emulator
        QString terminal = findTerminalEmulator();
        QStringList args;
        QString packagesStr = packageNames.join(" ");
        
        // Set up the command based on terminal type
        if (terminal == "konsole" || terminal == "gnome-terminal" || terminal == "xfce4-terminal" || terminal == "mate-terminal") {
            args << "-e" << QString("sudo pacman -R --noconfirm %1; read -p 'Press Enter to close'").arg(packagesStr);
        } else if (terminal == "kitty" || terminal == "alacritty") {
            args << "-e" << "sudo" << "pacman" << "-R" << "--noconfirm";
            args.append(packageNames);
        } else {
            // Default for xterm and others
            args << "-e" << QString("sudo pacman -R --noconfirm %1 && echo 'Press ENTER to close this window' && read").arg(packagesStr);
        }
        
        // Run the command in the terminal
        QProcess::startDetached(terminal, args);
    }
    
    // Remove Flatpak packages if any
    if (!flatpakPackages.isEmpty()) {
        // Find available terminal emulator
        QString terminal = findTerminalEmulator();
        QStringList args;
        
        // Build the command to remove all Flatpak packages
        QString flatpakRemoveCmd = "flatpak uninstall -y " + flatpakPackages.join(" ");
        
        // Set up the command based on terminal type
        if (terminal == "konsole" || terminal == "gnome-terminal" || terminal == "xfce4-terminal" || terminal == "mate-terminal") {
            args << "-e" << QString("%1; read -p 'Press Enter to close'").arg(flatpakRemoveCmd);
        } else if (terminal == "kitty" || terminal == "alacritty") {
            args << "-e" << "sh" << "-c" << QString("%1; read -p 'Press Enter to close'").arg(flatpakRemoveCmd);
        } else {
            // Default for xterm and others
            args << "-e" << QString("%1 && echo 'Press ENTER to close this window' && read").arg(flatpakRemoveCmd);
        }
        
        // Run the command in the terminal
        QProcess::startDetached(terminal, args);
    }
    
    // Schedule a refresh of installed packages after some time
    QTimer::singleShot(10000, this, &MainWindow::refreshInstalledPackages);
    
    // Also refresh Flatpak list if we removed any Flatpak packages
    if (!flatpakPackages.isEmpty()) {
        QTimer::singleShot(10000, this, &MainWindow::refreshFlatpakList);
    }
}

// Add implementation for onUpdatePackage
void MainWindow::onUpdatePackage() {
    // Implement onUpdatePackage
}

// Add implementation for onSyncAll
void MainWindow::onSyncAll() {
    // Implement onSyncAll
}

// Add implementation for onBatchInstall
void MainWindow::onBatchInstall() {
    // Implement onBatchInstall
}

// Add implementation for onPackageSelected
void MainWindow::onPackageSelected(const QModelIndex& index) {
    Q_UNUSED(index);
    updateInstallButtonText();
    
    // Enable/disable buttons based on selection
    bool hasSelection = m_packagesTable->selectionModel()->hasSelection();
    m_installButton->setEnabled(hasSelection);
    m_removeButton->setEnabled(hasSelection);
}

// Add implementation for onInstalledPackageSelected
void MainWindow::onInstalledPackageSelected(const QModelIndex& index) {
    Q_UNUSED(index);
    // Implement onInstalledPackageSelected
}

// Add implementation for onPackageItemChanged
void MainWindow::onPackageItemChanged(QStandardItem* item) {
    Q_UNUSED(item);
    // Implement onPackageItemChanged
}

// Add implementation for onInstallAurPackage
void MainWindow::onInstallAurPackage() {
    // Implement onInstallAurPackage
}

// Add implementation for onUpdateAurPackages
void MainWindow::onUpdateAurPackages() {
    // Implement onUpdateAurPackages
}

// Add implementation for onSystemUpdate
void MainWindow::onSystemUpdate() {
    // First, confirm with the user
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this, 
        tr("Confirm System Update"), 
        tr("Are you sure you want to update all packages?\n\nThis operation will require your password for authentication."),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (confirm != QMessageBox::Yes) {
        return;
    }
    
    // Clear the update log
    m_systemUpdateLogView->clear();
    m_systemUpdateLogView->append(tr("Starting system update..."));
    m_systemUpdateLogView->append(tr("Opening terminal to handle password prompt..."));
    
    // Find available terminal emulator
    QString terminal = findTerminalEmulator();
    QStringList args;
    
    // Set up the command based on terminal type
    if (terminal == "konsole" || terminal == "gnome-terminal" || terminal == "xfce4-terminal" || terminal == "mate-terminal") {
        args << "-e" << "sudo pacman -Syu --noconfirm; read -p 'Press Enter to close'";
    } else if (terminal == "kitty" || terminal == "alacritty") {
        args << "-e" << "sudo" << "pacman" << "-Syu" << "--noconfirm";
    } else {
        // Default for xterm and others
        args << "-e" << "sudo pacman -Syu --noconfirm && echo 'Press ENTER to close this window' && read";
    }
    
    // Run the command in the terminal
    QProcess::startDetached(terminal, args);
    
    // Show status message
    showStatusMessage(tr("Updating system packages..."), 0);
    
    // Schedule a refresh after some time to update the UI with new package info
    QTimer::singleShot(10000, this, &MainWindow::refreshInstalledPackages);
}

// Add implementation for onCheckForUpdates
void MainWindow::onCheckForUpdates() {
    // First, confirm with the user
    QMessageBox::StandardButton confirm = QMessageBox::question(
            this, 
        tr("Confirm Check for Updates"), 
        tr("Are you sure you want to check for system updates?\n\nThis operation will sync the package database and may require your password for authentication."),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (confirm != QMessageBox::Yes) {
        return;
    }
    
    // Show status message and update UI
    showStatusMessage(tr("Checking for updates..."), 0);
    m_systemUpdateInfoLabel->setText(tr("Checking for updates..."));
    
    // Clear the log view
    m_systemUpdateLogView->clear();
    m_systemUpdateLogView->append(tr("Starting update check..."));
    m_systemUpdateLogView->append(tr("Opening terminal to handle password prompt..."));
    
    // Ensure we're using the correct tab for updates
    if (m_tabWidget->indexOf(m_systemUpdateTab) >= 0) {
        m_tabWidget->setCurrentIndex(m_tabWidget->indexOf(m_systemUpdateTab));
    }
    
    // Clear the updates model
    if (m_systemUpdatesModel) {
        m_systemUpdatesModel->clear();
        m_systemUpdatesModel->setHorizontalHeaderLabels(
            QStringList() << tr("Name") << tr("Current Version") << tr("New Version") << tr("Repository"));
    }
    
    // Find available terminal emulator
    QString terminal = findTerminalEmulator();
    QStringList args;
    
    // Set up the command based on terminal type
    if (terminal == "konsole" || terminal == "gnome-terminal" || terminal == "xfce4-terminal" || terminal == "mate-terminal") {
        args << "-e" << "sudo pacman -Sy; read -p 'Press Enter to close'";
    } else if (terminal == "kitty" || terminal == "alacritty") {
        args << "-e" << "sudo" << "pacman" << "-Sy";
        } else {
        // Default for xterm and others
        args << "-e" << "sudo pacman -Sy && echo 'Press ENTER to close this window' && read";
    }
    
    // Run the command in the terminal
    QProcess::startDetached(terminal, args);
    
    // Schedule a check for updates after some time
    QTimer::singleShot(8000, this, &MainWindow::checkForUpdatesAfterSync);
}

// New method to check for updates after sync has completed
void MainWindow::checkForUpdatesAfterSync() {
    try {
        std::vector<std::pair<std::string, std::string>> updates = m_packageManager.check_updates();
        
        // Add updates to the model
        for (const auto& update : updates) {
            QList<QStandardItem*> row;
            
            // Get package details to find the repository and current version
            QString pkgName = QString::fromStdString(update.first);
            QString newVersion = QString::fromStdString(update.second);
            
            // Try to get the current version from installed packages
            QString currentVersion = "Unknown";
            QString repoName = "Unknown";
            
            // Find the package in the installed packages model
            for (int i = 0; i < m_installedModel->rowCount(); ++i) {
                QString name = m_installedModel->item(i, 1)->text(); // Name column (was incorrectly 0)
                if (name == pkgName) {
                    currentVersion = m_installedModel->item(i, 2)->text(); // Version column (was incorrectly 1)
                    repoName = m_installedModel->item(i, 3)->text(); // Repository column
                    break;
                }
            }
            
            QStandardItem* nameItem = new QStandardItem(pkgName);
            QStandardItem* currentVersionItem = new QStandardItem(currentVersion);
            QStandardItem* newVersionItem = new QStandardItem(newVersion);
            QStandardItem* repoItem = new QStandardItem(repoName);
            
            row << nameItem << currentVersionItem << newVersionItem << repoItem;
            m_systemUpdatesModel->appendRow(row);
        }
        
        // Update status bar and info label
        if (updates.empty()) {
            m_systemUpdateInfoLabel->setText(tr("Your system is up to date."));
            showStatusMessage(tr("Your system is up to date"), 5000);
            m_systemUpdateLogView->append(tr("No updates available."));
    } else {
            m_systemUpdateInfoLabel->setText(tr("Found %1 updates available.").arg(updates.size()));
            showStatusMessage(tr("Found %1 updates").arg(updates.size()), 5000);
            m_systemUpdateLogView->append(tr("Found %1 updates available.").arg(updates.size()));
        }
        
        // Auto-size columns
        for (int i = 0; i < m_systemUpdatesTable->model()->columnCount(); ++i) {
            m_systemUpdatesTable->resizeColumnToContents(i);
        }
        
    } catch (const std::exception& e) {
        // Handle any exceptions
        m_systemUpdateInfoLabel->setText(tr("Error checking for updates."));
        showStatusMessage(tr("Error checking for updates: %1").arg(e.what()), 5000);
        m_systemUpdateLogView->append(tr("Error: %1").arg(e.what()));
    }
}

// Add implementation for onClearPackageCache
void MainWindow::onClearPackageCache() {
    // Show confirmation dialog
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        tr("Confirm Package Cache Cleanup"),
        tr("Are you sure you want to clear the package cache? This will remove downloaded package files to free disk space."),
        QMessageBox::Yes|QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Show status message
    showStatusMessage(tr("Clearing package cache..."), 0);
    m_maintenanceLogView->append(tr("Starting package cache cleanup..."));
    
    // Execute pacman clean command
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    // Run the command with sudo
    process.start("pkexec", QStringList() << "pacman" << "-Sc" << "--noconfirm");
    
    if (!process.waitForStarted()) {
        showStatusMessage(tr("Failed to start package cache cleanup"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to start package cache cleanup."));
        return;
    }
    
    // Read output as it becomes available
    while (process.state() != QProcess::NotRunning) {
        process.waitForReadyRead();
        QString output = process.readAllStandardOutput();
        if (!output.isEmpty()) {
            m_maintenanceLogView->append(output);
        }
        QApplication::processEvents();
    }
    
    // Check exit code
    if (process.exitCode() == 0) {
        showStatusMessage(tr("Package cache cleared successfully"), 5000);
        m_maintenanceLogView->append(tr("Package cache cleared successfully."));
        
        // Run df to show disk space
        QProcess dfProcess;
        dfProcess.start("df", QStringList() << "-h" << "/");
        dfProcess.waitForFinished();
        QString dfOutput = dfProcess.readAllStandardOutput();
        m_maintenanceLogView->append(tr("\nDisk space after cleanup:"));
        m_maintenanceLogView->append(dfOutput);
    } else {
        showStatusMessage(tr("Error clearing package cache"), 5000);
        m_maintenanceLogView->append(tr("Error: Package cache cleanup failed with exit code %1.").arg(process.exitCode()));
        m_maintenanceLogView->append(process.readAllStandardError());
    }
}

// Add implementation for onRemoveOrphans
void MainWindow::onRemoveOrphans() {
    // First find orphaned packages
    showStatusMessage(tr("Finding orphaned packages..."), 0);
    m_maintenanceLogView->append(tr("Searching for orphaned packages..."));
    
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    // Run pacman to find orphans (packages installed as deps but no longer required)
    process.start("pacman", QStringList() << "-Qdt");
    
    if (!process.waitForStarted()) {
        showStatusMessage(tr("Failed to start orphan search"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to search for orphaned packages."));
        return;
    }
    
    process.waitForFinished(-1);
    QString output = process.readAllStandardOutput();
    
    // Process the output
    QStringList orphans = output.split("\n", Qt::SkipEmptyParts);
    
    if (orphans.isEmpty()) {
        showStatusMessage(tr("No orphaned packages found"), 5000);
        m_maintenanceLogView->append(tr("No orphaned packages found on the system."));
        return;
    }
    
    // Extract just the package names
    QStringList packageNames;
    for (const QString& orphan : orphans) {
        packageNames.append(orphan.split(" ").first());
    }
    
    // Display the found orphans
    m_maintenanceLogView->append(tr("Found %1 orphaned packages:").arg(orphans.size()));
    for (const QString& orphan : orphans) {
        m_maintenanceLogView->append(orphan);
    }
    
    // Ask for confirmation before removing
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Remove Orphaned Packages"),
                                  tr("Do you want to remove %1 orphaned packages?").arg(orphans.size()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::No) {
        showStatusMessage(tr("Orphan removal canceled"), 5000);
        m_maintenanceLogView->append(tr("Orphan removal canceled by user."));
        return;
    }
    
    // Remove the orphaned packages
    showStatusMessage(tr("Removing orphaned packages..."), 0);
    m_maintenanceLogView->append(tr("Removing orphaned packages. This may take some time..."));
    
    QStringList args;
    args << "-Rs";
    args.append(packageNames);
    
    QProcess removeProcess;
    removeProcess.setProcessChannelMode(QProcess::MergedChannels);
    
    // Use pkexec to get elevated privileges
    QStringList pkexecArgs;
    pkexecArgs << "pacman";
    pkexecArgs.append(args);
    
    removeProcess.start("pkexec", pkexecArgs);
    
    if (!removeProcess.waitForStarted()) {
        showStatusMessage(tr("Failed to start orphan removal"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to start orphaned package removal."));
        return;
    }
    
    removeProcess.waitForFinished(-1);
    output = removeProcess.readAllStandardOutput();
    
    int exitCode = removeProcess.exitCode();
    if (exitCode == 0) {
        showStatusMessage(tr("Orphaned packages removed successfully"), 5000);
        m_maintenanceLogView->append(tr("Orphaned packages removed successfully."));
        } else {
        showStatusMessage(tr("Failed to remove orphaned packages"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to remove orphaned packages."));
    }
    
    // Display the command output
    m_maintenanceLogView->append(tr("\nCommand output:"));
    m_maintenanceLogView->append(output);
}

// Add implementation for onCheckDatabase
void MainWindow::onCheckDatabase() {
    // Show confirmation dialog
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        tr("Confirm Database Check"),
        tr("Are you sure you want to check the package database? This will verify the integrity of all installed packages."),
        QMessageBox::Yes|QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Show status message
    showStatusMessage(tr("Checking package database..."), 0);
    m_maintenanceLogView->append(tr("Starting package database check..."));
    
    // Execute pacman database check
        QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
        
        // Run the command with sudo
    process.start("pkexec", QStringList() << "pacman" << "-Qk");
    
    if (!process.waitForStarted()) {
        showStatusMessage(tr("Failed to start database check"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to start database check."));
        return;
    }
    
    // Read output as it becomes available
    while (process.state() != QProcess::NotRunning) {
        process.waitForReadyRead();
        QString output = process.readAllStandardOutput();
                if (!output.isEmpty()) {
            m_maintenanceLogView->append(output);
                    }
        QApplication::processEvents();
                }
    
    // Check exit code
    if (process.exitCode() == 0) {
        showStatusMessage(tr("Database check completed successfully"), 5000);
        m_maintenanceLogView->append(tr("Database check completed successfully."));
            } else {
        showStatusMessage(tr("Database check found issues"), 5000);
        m_maintenanceLogView->append(tr("Database check found issues. See above log for details."));
    }
}

// Add implementation for onFindPacnewFiles
void MainWindow::onFindPacnewFiles() {
    // Show status message
    showStatusMessage(tr("Finding .pacnew files..."), 0);
    m_maintenanceLogView->append(tr("Searching for .pacnew and .pacsave files..."));
    
    // Execute find command to locate .pacnew files
        QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    // Run the find command
    process.start("find", QStringList() << "/" << "-type" << "f" << "(" << "-name" << "*.pacnew" << "-o" << "-name" << "*.pacsave" << ")" << "2>/dev/null");
    
    if (!process.waitForStarted()) {
        showStatusMessage(tr("Failed to start file search"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to start .pacnew/.pacsave file search."));
        return;
    }
    
    process.waitForFinished(-1);
    QString output = process.readAllStandardOutput();
    
    // Process the output
    QStringList files = output.split("\n", Qt::SkipEmptyParts);
    
    if (files.isEmpty()) {
        showStatusMessage(tr("No .pacnew or .pacsave files found"), 5000);
        m_maintenanceLogView->append(tr("No .pacnew or .pacsave files found on the system."));
        return;
    }
    
    // Display the found files
    m_maintenanceLogView->append(tr("Found %1 .pacnew/.pacsave files:").arg(files.size()));
    for (const QString& file : files) {
        m_maintenanceLogView->append(file);
    }
    
    // Suggest tools for merging
    m_maintenanceLogView->append(tr("\nYou can use tools like 'pacdiff' (from pacman-contrib package) to merge these files."));
    m_maintenanceLogView->append(tr("To install pacman-contrib if not installed:"));
    m_maintenanceLogView->append(tr("  sudo pacman -S pacman-contrib"));
    m_maintenanceLogView->append(tr("To merge using pacdiff:"));
    m_maintenanceLogView->append(tr("  sudo pacdiff"));
    
    showStatusMessage(tr("Found %1 .pacnew/.pacsave files").arg(files.size()), 5000);
}

// Add implementation for onBackupDatabase
void MainWindow::onBackupDatabase() {
    // Implement onBackupDatabase
}

// Add implementation for onRestoreDatabase
void MainWindow::onRestoreDatabase() {
    // Implement onRestoreDatabase
}

// Add implementation for onMaintenanceTaskFinished
void MainWindow::onMaintenanceTaskFinished(bool success, const QString& message) {
    Q_UNUSED(success);
    Q_UNUSED(message);
    // Implement onMaintenanceTaskFinished
}

// Add implementation for toggleTheme
void MainWindow::toggleTheme() {
    // Toggle the theme
    bool isDark = !isDarkThemeEnabled();
    toggleTheme(isDark);
}

// Add implementation for toggleTheme with bool parameter
void MainWindow::toggleTheme(bool isDark) {
    // Save the setting
    QSettings settings("PacmanGUI", "PacmanGUI");
    settings.setValue("appearance/darkTheme", isDark);
    
    // Apply the theme
    applyTheme(isDark);
    
    // Show status message
    showStatusMessage(isDark ? tr("Dark theme applied") : tr("Light theme applied"), 2000);
}

// Add implementation for openSettings
void MainWindow::openSettings() {
    // Create settings dialog if it doesn't exist
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
        
        // Connect theme changed signal
        connect(m_settingsDialog, &SettingsDialog::themeChanged, 
                [this](bool isDark) { this->toggleTheme(isDark); });
        
        // Connect AUR status changed signal
        connect(m_settingsDialog, &SettingsDialog::aurStatusChanged, [this](bool enabled) {
            // If AUR is enabled, check if we have a helper
            if (enabled) {
                checkAurHelper();
                } else {
                // If disabled, disable AUR button
                if (m_installAurButton) {
                    m_installAurButton->setEnabled(false);
                }
            }
        });
        
        // Connect Flatpak status changed signal
        connect(m_settingsDialog, &SettingsDialog::flatpakStatusChanged, [this](bool enabled) {
            // Update Flatpak search enabled flag
            m_flatpakSearchEnabled = enabled;
            
            // If Flatpak is enabled and we have it available, refresh packages
            if (enabled && m_packageManager.is_flatpak_available()) {
                refreshInstalledFlatpakPackages();
                
                // If we have a search term, search for Flatpak packages
                QString searchText = m_searchInput->text();
                if (!searchText.isEmpty()) {
                    performAsyncFlatpakSearch(searchText);
                }
            }
        });
    }
    
    // Show the dialog
    m_settingsDialog->exec();
}

// Add implementation for onAbout
void MainWindow::onAbout() {
    // Implement onAbout
}

// Add implementation for onDetailPanelAnimationFinished
void MainWindow::onDetailPanelAnimationFinished() {
    // Implement onDetailPanelAnimationFinished
}

// Add implementation for closeDetailPanel
void MainWindow::closeDetailPanel() {
    // Implement closeDetailPanel
}

// Add implementation for eventFilter
bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    Q_UNUSED(watched);
    Q_UNUSED(event);
    return false; // Default implementation
}

// Add implementation for resizeEvent
void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event); // Call base implementation
}

// Add implementation for closeEvent
void MainWindow::closeEvent(QCloseEvent* event)
{
    // Save settings before closing
    saveSettings();
    
    // Accept the close event
    event->accept();
}

// Add implementation for updateInstallButtonText
void MainWindow::updateInstallButtonText() {
    QItemSelectionModel* selectionModel = m_packagesTable->selectionModel();
    if (selectionModel) {
        QModelIndexList selected = selectionModel->selectedRows();
        if (selected.size() > 1) {
            m_installButton->setText(tr("Install Selected Packages"));
            } else {
            m_installButton->setText(tr("Install"));
        }
    }
}

// Add implementation for onCleanCache
void MainWindow::onCleanCache() {
    // Ask user what type of cache cleaning they want
    QStringList options;
    options << tr("Remove all packages from cache")
            << tr("Keep only the latest version of each package")
            << tr("Keep currently installed packages only");
    
    bool ok;
    QString selected = QInputDialog::getItem(this, tr("Clean Package Cache"),
                                            tr("Select cache cleaning option:"), options, 
                                            1, false, &ok);
    
    if (!ok || selected.isEmpty()) {
        showStatusMessage(tr("Cache cleaning canceled"), 5000);
        m_maintenanceLogView->append(tr("Package cache cleaning was canceled."));
        return;
    }
    
    // Prepare the command based on selection
    QStringList args;
    
    if (selected == options[0]) {
        // Remove all packages from cache
        args << "-Scc" << "--noconfirm";
        m_maintenanceLogView->append(tr("Removing all packages from cache..."));
    } else if (selected == options[1]) {
        // Keep only the latest version of each package
        args << "-Sc" << "--noconfirm";
        m_maintenanceLogView->append(tr("Removing old versions of packages from cache..."));
    } else if (selected == options[2]) {
        // Keep currently installed packages only
        args << "-Sc" << "--noconfirm";
        m_maintenanceLogView->append(tr("Removing packages not currently installed from cache..."));
    }
    
    // Show status message
    showStatusMessage(tr("Cleaning package cache..."), 0);
    
    // Execute pacman clean command with elevated privileges
        QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    // Use pkexec to get root privileges
    QStringList pkexecArgs;
    pkexecArgs << "pacman";
    pkexecArgs.append(args);
    
    process.start("pkexec", pkexecArgs);
    
    if (!process.waitForStarted()) {
        showStatusMessage(tr("Failed to start cache cleaning"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to start package cache cleaning."));
        return;
    }
    
    process.waitForFinished(-1);
    QString output = process.readAllStandardOutput();
    
    int exitCode = process.exitCode();
            if (exitCode == 0) {
        // Get the cache size after cleaning
        QProcess sizeProcess;
        sizeProcess.start("du", QStringList() << "-sh" << "/var/cache/pacman/pkg/");
        sizeProcess.waitForFinished();
        QString sizeOutput = sizeProcess.readAllStandardOutput();
        QString currentSize = sizeOutput.trimmed().split("\t").first();
        
        showStatusMessage(tr("Package cache cleaned successfully"), 5000);
        m_maintenanceLogView->append(tr("Package cache cleaned successfully."));
        m_maintenanceLogView->append(tr("Current cache size: %1").arg(currentSize));
            } else {
        showStatusMessage(tr("Failed to clean package cache"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to clean package cache."));
    }
    
    // Display the command output
    m_maintenanceLogView->append(tr("\nCommand output:"));
    m_maintenanceLogView->append(output);
}

// Add implementation for onClearPackageLock
void MainWindow::onClearPackageLock() {
    // Show confirmation dialog
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Clear Package Lock"),
                                 tr("Are you sure you want to remove the package manager lock?\n\n"
                                    "This should only be done if you're certain no package operations "
                                    "are in progress. Removing the lock while an operation is in progress "
                                    "could damage your system."),
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        showStatusMessage(tr("Package lock removal canceled"), 5000);
        m_maintenanceLogView->append(tr("Package lock removal was canceled."));
        return;
    }
    
    // Show status message
    showStatusMessage(tr("Removing package lock..."), 0);
    m_maintenanceLogView->append(tr("Attempting to remove package manager lock..."));
    
    // Execute command to remove the lock file with elevated privileges
        QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    // Use pkexec to get root privileges
        QStringList args;
    args << "rm" << "-f" << "/var/lib/pacman/db.lck";
    
    process.start("pkexec", args);
    
    if (!process.waitForStarted()) {
        showStatusMessage(tr("Failed to start lock removal"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to start package lock removal."));
        return;
    }
    
    process.waitForFinished(-1);
    QString output = process.readAllStandardOutput();
    
        int exitCode = process.exitCode();
            if (exitCode == 0) {
        showStatusMessage(tr("Package lock removed successfully"), 5000);
        m_maintenanceLogView->append(tr("Package manager lock was successfully removed."));
                } else {
        showStatusMessage(tr("Failed to remove package lock"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to remove package manager lock."));
        m_maintenanceLogView->append(tr("Exit code: %1").arg(exitCode));
    }
    
    // Display the command output
    if (!output.isEmpty()) {
        m_maintenanceLogView->append(tr("\nCommand output:"));
        m_maintenanceLogView->append(output);
    }
    
    // Check if lock file exists after attempted removal
    QProcess checkProcess;
    checkProcess.start("ls", QStringList() << "-l" << "/var/lib/pacman/db.lck");
    checkProcess.waitForFinished();
    int checkExitCode = checkProcess.exitCode();
    
    if (checkExitCode != 0) {
        m_maintenanceLogView->append(tr("Confirmed: Lock file no longer exists."));
            } else {
        m_maintenanceLogView->append(tr("Warning: Lock file may still exist despite removal attempt."));
    }
}

// Add implementation for onCheckIntegrityAllPackages
void MainWindow::onCheckIntegrityAllPackages() {
    // Confirm operation with user
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Check Package Integrity"),
                                 tr("This will check the integrity of all installed packages.\n"
                                    "The process may take some time. Do you want to continue?"),
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        showStatusMessage(tr("Package integrity check canceled"), 5000);
        m_maintenanceLogView->append(tr("Package integrity check was canceled."));
        return;
    }
    
    // Show status message
    showStatusMessage(tr("Checking package integrity..."), 0);
    m_maintenanceLogView->append(tr("Starting integrity check for all installed packages..."));
    
    // Execute pacman command to check integrity
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    // Use paccheck from pacutils or pacman to verify packages
    QStringList args;
    
    // First, check if paccheck is available
    QProcess checkPaccheck;
    checkPaccheck.start("which", QStringList() << "paccheck");
    checkPaccheck.waitForFinished();
    
    if (checkPaccheck.exitCode() == 0) {
        // Use paccheck which is more thorough
        args << "paccheck" << "--md5sum" << "--files" << "--backup" << "--quiet";
        process.start("pkexec", args);
        } else {
        // Fallback to pacman's verification
        args << "pacman" << "-Qk";
        process.start("pkexec", args);
    }
    
    if (!process.waitForStarted()) {
        showStatusMessage(tr("Failed to start integrity check"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to start package integrity check."));
        return;
    }
    
    // Set cursor to wait cursor
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    process.waitForFinished(-1);
    QString output = process.readAllStandardOutput();
    
    // Reset cursor
    QApplication::restoreOverrideCursor();
    
    int exitCode = process.exitCode();
    if (exitCode == 0) {
        showStatusMessage(tr("Package integrity check completed"), 5000);
        
        // Check if there were any issues reported in the output
        if (output.contains("warning") || output.contains("error") || 
            output.contains("corrupted") || output.contains("missing")) {
            m_maintenanceLogView->append(tr("Package integrity check completed with issues detected."));
        } else {
            m_maintenanceLogView->append(tr("Package integrity check completed successfully. No issues found."));
        }
    } else {
        showStatusMessage(tr("Package integrity check failed"), 5000);
        m_maintenanceLogView->append(tr("Error: Package integrity check failed."));
        m_maintenanceLogView->append(tr("Exit code: %1").arg(exitCode));
    }
    
    // Display the command output with formatting
    if (!output.isEmpty()) {
        m_maintenanceLogView->append(tr("\nIntegrity check results:"));
        
        // Highlight warnings and errors
        QStringList lines = output.split("\n");
        for (const QString& line : lines) {
            QString formattedLine = line;
            
            // Simple HTML formatting for warnings and errors
            if (line.contains("warning", Qt::CaseInsensitive) || 
                line.contains("missing", Qt::CaseInsensitive)) {
                formattedLine = "<span style='color:orange'>" + line + "</span>";
                m_maintenanceLogView->append(formattedLine);
            } else if (line.contains("error", Qt::CaseInsensitive) || 
                       line.contains("corrupted", Qt::CaseInsensitive) || 
                       line.contains("failed", Qt::CaseInsensitive)) {
                formattedLine = "<span style='color:red'>" + line + "</span>";
                m_maintenanceLogView->append(formattedLine);
    } else {
                m_maintenanceLogView->append(line);
            }
        }
    }
    
    // Provide a summary if issues were found
    if (output.contains("warning") || output.contains("error") || 
        output.contains("corrupted") || output.contains("missing")) {
        m_maintenanceLogView->append(tr("\nSome packages have integrity issues. "
                                     "Consider reinstalling the affected packages."));
    }
}

void MainWindow::onRefreshMirrorList() {
    // Confirm operation with user
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Refresh Mirror List"),
                                 tr("This will update your pacman mirror list with faster mirrors.\n"
                                    "Do you want to continue?"),
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        showStatusMessage(tr("Mirror list refresh canceled"), 5000);
        m_maintenanceLogView->append(tr("Mirror list refresh was canceled."));
        return;
    }
    
    // Show status message
    showStatusMessage(tr("Refreshing mirror list..."), 0);
    m_maintenanceLogView->append(tr("Starting mirror list refresh..."));
    
    // Execute reflector command to update mirrors
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    // Set up the reflector command with appropriate parameters
    QStringList args;
    args << "reflector";
    
    // Default options - customize as needed
    args << "--verbose";
    args << "--latest" << "20";
    args << "--sort" << "rate";
    args << "--save" << "/etc/pacman.d/mirrorlist";
    
    // Check if reflector is installed
    QProcess checkReflector;
    checkReflector.start("which", QStringList() << "reflector");
    checkReflector.waitForFinished();
    
    if (checkReflector.exitCode() != 0) {
        // Reflector not found
        showStatusMessage(tr("Reflector not found"), 5000);
        
        // Ask if the user wants to install reflector
        QMessageBox::StandardButton installReply;
        installReply = QMessageBox::question(this, tr("Install Reflector"),
                                          tr("The reflector tool is not installed. It is needed to refresh mirrors.\n"
                                             "Do you want to install it now?"),
                                          QMessageBox::Yes | QMessageBox::No);
        
        if (installReply == QMessageBox::Yes) {
            m_maintenanceLogView->append(tr("Installing reflector package..."));
            
            // Install reflector
            QProcess installProcess;
            installProcess.setProcessChannelMode(QProcess::MergedChannels);
            
            QStringList installArgs;
            installArgs << "pacman" << "-S" << "--noconfirm" << "reflector";
            
            installProcess.start("pkexec", installArgs);
            
            if (!installProcess.waitForStarted()) {
                showStatusMessage(tr("Failed to start reflector installation"), 5000);
                m_maintenanceLogView->append(tr("Error: Failed to start reflector installation."));
        return;
    }
    
            installProcess.waitForFinished(-1);
            QString installOutput = installProcess.readAllStandardOutput();
            
            if (installProcess.exitCode() == 0) {
                m_maintenanceLogView->append(tr("Reflector installation completed successfully."));
                m_maintenanceLogView->append(tr("Continuing with mirror list refresh..."));
    } else {
                showStatusMessage(tr("Failed to install reflector"), 5000);
                m_maintenanceLogView->append(tr("Error: Failed to install reflector."));
                m_maintenanceLogView->append(installOutput);
        return;
            }
    } else {
            m_maintenanceLogView->append(tr("Mirror list refresh canceled - reflector not installed."));
            return;
        }
    }
    
    // Start the reflector process
    process.start("pkexec", args);
    
    if (!process.waitForStarted()) {
        showStatusMessage(tr("Failed to start mirror refresh"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to start mirror list refresh."));
        return;
    }
    
    // Set cursor to wait cursor
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    // Wait for process to finish
    process.waitForFinished(-1);
    QString output = process.readAllStandardOutput();
    
    // Reset cursor
    QApplication::restoreOverrideCursor();
    
    int exitCode = process.exitCode();
    if (exitCode == 0) {
        showStatusMessage(tr("Mirror list refreshed successfully"), 5000);
        m_maintenanceLogView->append(tr("Mirror list has been updated successfully."));
    } else {
        showStatusMessage(tr("Failed to refresh mirror list"), 5000);
        m_maintenanceLogView->append(tr("Error: Failed to refresh mirror list."));
        m_maintenanceLogView->append(tr("Exit code: %1").arg(exitCode));
    }
    
    // Display the command output
    if (!output.isEmpty()) {
        m_maintenanceLogView->append(tr("\nOutput from reflector:"));
        m_maintenanceLogView->append(output);
    }
    
    // Verify the mirror list was updated
    QFile mirrorlistFile("/etc/pacman.d/mirrorlist");
    if (mirrorlistFile.exists()) {
        QDateTime lastModified = QFileInfo(mirrorlistFile).lastModified();
        QString formattedTime = lastModified.toString("yyyy-MM-dd hh:mm:ss");
        m_maintenanceLogView->append(tr("\nMirror list last modified: %1").arg(formattedTime));
    }
}
        
// Helper to find an available terminal emulator
QString MainWindow::findTerminalEmulator() {
    // List of common terminal emulators in order of preference
    QStringList terminals = {
        "konsole",
        "gnome-terminal",
        "xterm",
        "terminator",
        "kitty",
        "alacritty",
        "xfce4-terminal",
        "mate-terminal",
        "lxterminal"
    };
    
    for (const QString& terminal : terminals) {
        // Check if the terminal is available in the path
        QProcess which;
        which.start("which", QStringList() << terminal);
        which.waitForFinished();
        
        if (which.exitCode() == 0) {
            return terminal;
        }
    }
    
    // If no terminal is found, return a default
    return "xterm";
}

// At the end of the file, add the Flatpak methods implementation
void MainWindow::setupFlatpakSupport()
{
    qDebug() << "DEBUG: Entering setupFlatpakSupport()";
    
    // Check if Flatpak is available
    bool flatpakAvailable = m_packageManager.is_flatpak_available();
    
    // Initialize Flatpak related variables
    m_flatpakSearchEnabled = false;
    
    if (flatpakAvailable) {
        qDebug() << "DEBUG: Flatpak is available on this system";
        
        // Create Flatpak install button
        m_installFlatpakButton = new QPushButton(tr("Install (Flatpak)"), m_searchTab);
        m_installFlatpakButton->setEnabled(false);
        m_packageActionsLayout->addWidget(m_installFlatpakButton);
        
        // Connect signal for installation
        connect(m_installFlatpakButton, &QPushButton::clicked, this, &MainWindow::onInstallFlatpakPackage);
        
        // Also add Flatpak removal for installed packages tab
        m_removeFlatpakButton = new QPushButton(tr("Remove (Flatpak)"), m_installedTab);
        m_removeFlatpakButton->setEnabled(false);
        m_installedActionsLayout->addWidget(m_removeFlatpakButton);
        
        // Connect signal for removal
        connect(m_removeFlatpakButton, &QPushButton::clicked, this, &MainWindow::onRemoveFlatpakPackage);
     
        // Load settings to check if Flatpak is enabled
        QSettings settings("PacmanGUI", "PacmanGUI");
        m_flatpakSearchEnabled = settings.value("flatpak/enabled", false).toBool();
        
        // If Flatpak is enabled, load installed Flatpak packages
        if (m_flatpakSearchEnabled) {
            refreshInstalledFlatpakPackages();
        }
    } else {
        qDebug() << "DEBUG: Flatpak is not available on this system";
    }
    
    qDebug() << "DEBUG: Exiting setupFlatpakSupport()";
}

void MainWindow::onToggleFlatpakSearch(bool enabled)
{
    qDebug() << "DEBUG: Flatpak search toggled:" << enabled;
    m_flatpakSearchEnabled = enabled;
    
    if (enabled) {
        // If search is enabled and we have search text, perform search
        QString searchText = m_searchInput->text();
        if (!searchText.isEmpty()) {
            performAsyncFlatpakSearch(searchText);
        }
                } else {
        // If search is disabled, clear any Flatpak results
        m_flatpakModel->clear();
        m_flatpakModel->setHorizontalHeaderLabels(
            QStringList() << tr("") << tr("Name") << tr("Version") << tr("Repository") << tr("Description"));
        
        // Disable Flatpak install button
        if (m_installFlatpakButton) {
            m_installFlatpakButton->setEnabled(false);
        }
    }
}

void MainWindow::refreshInstalledFlatpakPackages()
{
    if (!m_packageManager.is_flatpak_available()) {
        return;
    }
    
    // Clear the model
    m_installedFlatpakModel->clear();
    m_installedFlatpakModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Version") << tr("Repository") << tr("Description"));
    
    // Get installed packages
    std::vector<pacmangui::core::FlatpakPackage> installedPackages = m_packageManager.get_installed_flatpak_packages();
    
    // Add packages to the model
    for (const auto& pkg : installedPackages) {
        QList<QStandardItem*> row;
        
        // Add checkbox item
        QStandardItem* checkItem = new QStandardItem();
        checkItem->setCheckable(true);
        checkItem->setCheckState(Qt::Unchecked);
        checkItem->setData(Qt::AlignCenter, Qt::TextAlignmentRole);
        
        QStandardItem* nameItem = new QStandardItem(QString::fromStdString(pkg.get_name()));
        nameItem->setData(QString::fromStdString(pkg.get_app_id()), Qt::UserRole); // Store app_id for later use
        
        QStandardItem* versionItem = new QStandardItem(QString::fromStdString(pkg.get_version()));
        QStandardItem* repoItem = new QStandardItem(QString::fromStdString(pkg.get_repository()));
        QStandardItem* descItem = new QStandardItem(QString::fromStdString(pkg.get_description()));
        
        row << checkItem << nameItem << versionItem << repoItem << descItem;
        m_installedFlatpakModel->appendRow(row);
    }
    
    // Update status bar
    showStatusMessage(tr("Loaded %1 installed Flatpak packages").arg(installedPackages.size()), 3000);
    
    // Enable/disable remove button based on package count
    if (m_removeFlatpakButton) {
        m_removeFlatpakButton->setEnabled(!installedPackages.empty());
    }
}

void MainWindow::searchFlatpakPackages(const QString& searchTerm)
{
    // This is now a wrapper around performAsyncFlatpakSearch
    performAsyncFlatpakSearch(searchTerm);
}

void MainWindow::performAsyncFlatpakSearch(const QString& searchTerm) {
    if (!m_packageManager.is_flatpak_available()) {
        showStatusMessage("Flatpak support is not enabled", 3000);
        return;
    }

    // Show that we're searching
    showStatusMessage("Searching for Flatpak packages...", 0);

    // Create a QFutureWatcher to handle the async search
    auto watcher = new QFutureWatcher<std::vector<pacmangui::core::FlatpakPackage>>();

    // Connect signals
    connect(watcher, &QFutureWatcher<std::vector<pacmangui::core::FlatpakPackage>>::finished, this, [=]() {
        // Get the search results
        auto results = watcher->result();
        
        qDebug() << "Found" << results.size() << "Flatpak packages matching" << searchTerm;
        
        // Update the model with the results
        for (const auto& package : results) {
            // Create checkbox item
            auto checkbox = new QStandardItem();
            checkbox->setCheckable(true);
            checkbox->setCheckState(Qt::Unchecked);
            
            // Create items for other columns
            auto nameItem = new QStandardItem(QString::fromStdString(package.get_name()));
            auto versionItem = new QStandardItem(QString::fromStdString(package.get_version()));
            auto repositoryItem = new QStandardItem("Flatpak");
            auto descriptionItem = new QStandardItem(QString::fromStdString(package.get_description()));
            
            // Store the package in the first column
            QVariant v;
            v.setValue(package);
            checkbox->setData(v, Qt::UserRole);
            
            // Mark as Flatpak for later identification
            checkbox->setData("flatpak", Qt::UserRole + 1);
            nameItem->setData("flatpak", Qt::UserRole + 1);
            
            // Add the row to the main model
            QList<QStandardItem*> items;
            items << checkbox << nameItem << versionItem << repositoryItem << descriptionItem;
            m_packagesModel->appendRow(items);
        }
        
        // Update status
        if (results.empty()) {
            showStatusMessage("No Flatpak packages found matching '" + searchTerm + "'", 3000);
        } else {
            showStatusMessage("Found " + QString::number(results.size()) + " Flatpak package(s) matching '" + searchTerm + "'", 3000);
        }
        
        // Clean up the watcher
        watcher->deleteLater();
    });

    // Start the search
    auto future = QtConcurrent::run([=]() {
        return m_packageManager.search_flatpak_by_name(searchTerm.toStdString());
    });
    
    watcher->setFuture(future);
}

void MainWindow::onInstallFlatpakPackage()
{
    // Get selected packages
    QModelIndexList selected = m_packagesTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        showStatusMessage(tr("No Flatpak packages selected for installation"), 3000);
        return;
    }
    
    // Collect package names and app IDs
    QStringList packageNames;
    QStringList packageAppIds;
    QStringList packageDetails;
    QStringList packageRemotes;
    
    for (const QModelIndex& index : selected) {
        // Check if the selected row is a Flatpak package
        QModelIndex typeIndex = m_packagesModel->index(index.row(), 1); // Name column contains type data
        QVariant packageType = m_packagesModel->data(typeIndex, Qt::UserRole + 1);
        
        // Skip if not a Flatpak package
        if (packageType.toString() != "flatpak") {
            continue;
        }
        
        // Get app ID from the name column's user role
        QModelIndex nameIndex = m_packagesModel->index(index.row(), 1); // Name column
        QString name = m_packagesModel->data(nameIndex).toString();
        QString appId = m_packagesModel->data(nameIndex, Qt::UserRole).toString();
        
        if (appId.isEmpty()) {
            // If no app ID stored, fall back to using the name
            appId = name;
        }
        
        QModelIndex versionIndex = m_packagesModel->index(index.row(), 2); // Version column
        QModelIndex repoIndex = m_packagesModel->index(index.row(), 3); // Repository column
        
        QString version = m_packagesModel->data(versionIndex).toString();
        QString repo = m_packagesModel->data(repoIndex).toString();
        
        // Remove " (Flatpak)" suffix from repository name
        repo.remove(" (Flatpak)");
        
        packageNames.append(name);
        packageAppIds.append(appId);
        packageRemotes.append(repo);
        packageDetails.append(tr("%1 (%2) [%3]").arg(name).arg(version).arg(repo));
    }
    
    // Check if we found any Flatpak packages
    if (packageNames.isEmpty()) {
        showStatusMessage(tr("No Flatpak packages selected for installation"), 3000);
        return;
    }
    
    // Confirm installation
    QString message;
    if (packageNames.size() == 1) {
        message = tr("Do you want to install the following Flatpak package?\n\n%1").arg(packageDetails.first());
    } else {
        message = tr("Do you want to install the following %1 Flatpak packages?\n\n%2")
            .arg(packageNames.size())
            .arg(packageDetails.join("\n"));
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        tr("Confirm Flatpak Installation"),
        message,
        QMessageBox::Yes|QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Show progress dialog
    QProgressDialog progress(tr("Installing Flatpak packages..."), tr("Cancel"), 0, packageAppIds.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);
    
    // Install packages one by one
    bool allSuccessful = true;
    for (int i = 0; i < packageAppIds.size(); i++) {
        QString appId = packageAppIds[i];
        QString name = packageNames[i];
        QString remote = packageRemotes[i];
        
        progress.setLabelText(tr("Installing %1...").arg(name));
        progress.setValue(i);
        
        if (progress.wasCanceled()) {
            break;
        }
        
        // Install the package
        bool success = m_packageManager.install_flatpak_package(appId.toStdString(), remote.toStdString());
        
        if (!success) {
            allSuccessful = false;
            QMessageBox::warning(
                this,
                tr("Installation Failed"),
                tr("Failed to install %1: %2")
                    .arg(name)
                    .arg(QString::fromStdString(m_packageManager.get_last_error())),
                QMessageBox::Ok
            );
        }
    }
    
    progress.setValue(packageAppIds.size());
    
    // Show status message
    if (allSuccessful) {
        showStatusMessage(tr("Successfully installed Flatpak packages"), 3000);
        } else {
        showStatusMessage(tr("Some Flatpak packages failed to install"), 3000);
    }
    
    // Refresh installed packages list
    refreshInstalledFlatpakPackages();
}

void MainWindow::onRemoveFlatpakPackage()
{
    // Get selected packages from installed table
    QModelIndexList selected = m_installedTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        showStatusMessage(tr("No Flatpak packages selected for removal"), 3000);
        return;
    }
    
    // Collect package names and app IDs
    QStringList packageNames;
    QStringList packageAppIds;
    QStringList packageDetails;
    
    for (const QModelIndex& index : selected) {
        // Get the repository column to check if it's a Flatpak
        QModelIndex repoIndex = m_installedModel->index(index.row(), 3); // Repository column
        QString repo = m_installedModel->data(repoIndex).toString();
        
        // Skip if not a Flatpak package (should contain "Flatpak" in the repository name)
        if (!repo.contains("Flatpak", Qt::CaseInsensitive)) {
            continue;
        }
        
        // Get app ID from the name column's user role
        QModelIndex nameIndex = m_installedModel->index(index.row(), 1); // Name column
        QString name = m_installedModel->data(nameIndex).toString();
        QString appId = m_installedModel->data(nameIndex, Qt::UserRole).toString();
        
        if (appId.isEmpty()) {
            // If no app ID stored, fall back to using the name
            appId = name;
        }
        
        QModelIndex versionIndex = m_installedModel->index(index.row(), 2); // Version column
        QString version = m_installedModel->data(versionIndex).toString();
        
        packageNames.append(name);
        packageAppIds.append(appId);
        packageDetails.append(tr("%1 (%2)").arg(name).arg(version));
    }
    
    // Check if we found any Flatpak packages
    if (packageNames.isEmpty()) {
        showStatusMessage(tr("No Flatpak packages selected for removal"), 3000);
        return;
    }
    
    // Confirm removal
    QString message;
    if (packageNames.size() == 1) {
        message = tr("Do you want to remove the following Flatpak package?\n\n%1").arg(packageDetails.first());
    } else {
        message = tr("Do you want to remove the following %1 Flatpak packages?\n\n%2")
            .arg(packageNames.size())
            .arg(packageDetails.join("\n"));
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        tr("Confirm Flatpak Removal"),
        message,
        QMessageBox::Yes|QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Show progress dialog
    QProgressDialog progress(tr("Removing Flatpak packages..."), tr("Cancel"), 0, packageAppIds.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);
    
    // Remove packages one by one
    bool allSuccessful = true;
    for (int i = 0; i < packageAppIds.size(); i++) {
        QString appId = packageAppIds[i];
        QString name = packageNames[i];
        
        progress.setLabelText(tr("Removing %1...").arg(name));
        progress.setValue(i);
        
        if (progress.wasCanceled()) {
            break;
        }
        
        // Remove the package
        bool success = m_packageManager.remove_flatpak_package(appId.toStdString());
        
        if (!success) {
            allSuccessful = false;
            QMessageBox::warning(
                this,
                tr("Removal Failed"),
                tr("Failed to remove %1: %2")
                    .arg(name)
                    .arg(QString::fromStdString(m_packageManager.get_last_error())),
                QMessageBox::Ok
            );
        }
    }
    
    progress.setValue(packageAppIds.size());
    
    // Show status message
    if (allSuccessful) {
        showStatusMessage(tr("Successfully removed Flatpak packages"), 3000);
    } else {
        showStatusMessage(tr("Some Flatpak packages failed to remove"), 3000);
    }
    
    // Refresh installed packages list
    refreshInstalledFlatpakPackages();
}

// Add Flatpak search functions
void MainWindow::onSearchFlatpakPackages()
{
    // Get the search text
    QString searchText = m_searchInput->text();
    
    // Perform the Flatpak search
    if (!searchText.isEmpty()) {
        performAsyncFlatpakSearch(searchText);
    } else {
        showStatusMessage(tr("Please enter a search term"), 3000);
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("PacmanGUI", "PacmanGUI");
    
    // Save window geometry
    settings.setValue("window/geometry", saveGeometry());
    
    // Save theme settings
    settings.setValue("appearance/darkTheme", m_darkTheme);
    
    // Ensure settings are written to disk
    settings.sync();
}

// Add implementation for setupFlatpakTab using our new FlatpakManagerTab class
void MainWindow::setupFlatpakTab() {
    // Only setup if Flatpak is available
    if (!m_packageManager.is_flatpak_available()) {
        return;
    }
    
    qDebug() << "Setting up Flatpak management tab using FlatpakManagerTab class";
    
    // Create the Flatpak manager tab
    m_flatpakManagerTab = new FlatpakManagerTab(this, &m_packageManager);
    
    // Connect signals
    connect(m_flatpakManagerTab, &FlatpakManagerTab::statusMessage, 
            this, &MainWindow::onFlatpakStatusMessage);
    
    // Add tab to main tab widget
    m_tabWidget->addTab(m_flatpakManagerTab, tr("Flatpak Manager"));
}

void MainWindow::onFlatpakStatusMessage(const QString& message, int timeout) {
    showStatusMessage(message, timeout);
}

// Replace refreshFlatpakList with a method that delegates to our FlatpakManagerTab
void MainWindow::refreshFlatpakList() {
    if (m_flatpakManagerTab) {
        m_flatpakManagerTab->refreshFlatpakList();
    }
}

void MainWindow::refreshFlatpakRemotes() {
    if (m_flatpakManagerTab) {
        m_flatpakManagerTab->refreshFlatpakRemotes();
    }
}
} // namespace gui
} // namespace pacmangui