#include "gui/mainwindow.hpp"
#include "gui/settingsdialog.hpp"
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
    m_searchProgressDialog(nullptr),
    m_updateButton(nullptr),           // Explicitly set to nullptr since we're not creating it
    m_installAurButton(nullptr),       // Explicitly set to nullptr since we're not creating it
    m_updateInstalledButton(nullptr)   // Explicitly set to nullptr since we're not creating it
{
    setWindowTitle(tr("PacmanGUI"));
    setMinimumSize(800, 600);

    // Initialize package manager
    m_packageManager.initialize("/", "/var/lib/pacman");

    // Initialize models before we use them
    // Add a checkbox column at the beginning for multi-selection
    m_packagesModel = new QStandardItemModel(0, 5, this);
    m_packagesModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Version") << tr("Description") << tr("Repository"));
        
    m_installedModel = new QStandardItemModel(0, 5, this);
    m_installedModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Version") << tr("Description") << tr("Repository"));
        
    m_updatesModel = new QStandardItemModel(0, 5, this);
    m_updatesModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Current Version") << tr("New Version") << tr("Repository"));
        
    // Initialize search progress dialog
    m_searchProgressDialog = new QProgressDialog(tr("Searching for packages..."), tr("Cancel"), 0, 0, this);
    m_searchProgressDialog->setWindowModality(Qt::WindowModal);
    m_searchProgressDialog->setMinimumDuration(500);
    m_searchProgressDialog->setAutoClose(true);
    m_searchProgressDialog->setAutoReset(true);
    
    // Set up UI components in the correct order
    setupUi();
    setupActions();
    setupMenus();
    setupSystemUpdateTab(); // Create the tab and its buttons before connecting signals
    setupMaintenanceTab();  // Create maintenance buttons before connecting them
    
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
    
    delete m_searchProgressDialog;
    
    delete m_packagesModel;
    delete m_installedModel;
    delete m_systemUpdatesModel;
    delete m_updatesModel;
    delete m_settingsDialog;
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
    
    // Create central widget and layout
    m_centralWidget = new QWidget(this);
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    setCentralWidget(m_centralWidget);

    // Create tab widget
    qDebug() << "DEBUG: Creating tab widget";
    m_tabWidget = new QTabWidget(m_centralWidget);
    m_mainLayout->addWidget(m_tabWidget);

    // Create search tab
    qDebug() << "DEBUG: Creating search tab";
    m_searchTab = new QWidget();
    m_searchLayout = new QVBoxLayout(m_searchTab);
    
    // Create search controls
    qDebug() << "DEBUG: Creating search controls";
    m_searchControlsLayout = new QHBoxLayout();
    m_searchInput = new QLineEdit(m_searchTab);
    m_searchInput->setPlaceholderText(tr("Search packages..."));
    m_searchButton = new QPushButton(tr("Search"), m_searchTab);
    m_searchButton->setStyleSheet("background-color: #0078d7; color: white; border-radius: 3px;");
    m_searchButton->setMaximumWidth(90);  // Reduced from 100
    m_searchButton->setMaximumHeight(26); // Reduced from 30
    m_searchControlsLayout->addWidget(m_searchInput);
    m_searchControlsLayout->addWidget(m_searchButton);
    
    m_searchLayout->addLayout(m_searchControlsLayout);
    
    // Create package action buttons - now at the top
    qDebug() << "DEBUG: Creating package action buttons";
    m_packageActionsLayout = new QHBoxLayout();
    
    // Only create Install and Remove buttons (remove Update and AUR buttons)
    m_installButton = new QPushButton(tr("Install"), m_searchTab);
    m_removeButton = new QPushButton(tr("Remove"), m_searchTab);
    
    // Make buttons consistent in size and style
    QList<QPushButton*> actionButtons = {m_installButton, m_removeButton};
    for (QPushButton* button : actionButtons) {
        button->setStyleSheet("QPushButton {"
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
                             "}");
        button->setMinimumWidth(100);
        button->setMinimumHeight(26);
        button->setMaximumHeight(30);
        button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }
    
    m_packageActionsLayout->addWidget(m_installButton);
    m_packageActionsLayout->addWidget(m_removeButton);
    m_packageActionsLayout->addStretch(1); // Push buttons to the left
    
    m_searchLayout->addLayout(m_packageActionsLayout);
    
    // Create package table
    qDebug() << "DEBUG: Creating package table";
    m_packagesTable = new QTreeView(m_searchTab);
    m_packagesTable->setSelectionMode(QAbstractItemView::ExtendedSelection); // Allow multiple selection
    m_packagesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_packagesTable->setSortingEnabled(true);
    m_packagesTable->setAlternatingRowColors(true);
    
    m_packagesTable->setModel(m_packagesModel);
    
    // Update column sizing to account for checkbox column
    m_packagesTable->header()->setSectionResizeMode(0, QHeaderView::Fixed);  // Checkbox column
    m_packagesTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Name
    m_packagesTable->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Version
    m_packagesTable->header()->setSectionResizeMode(3, QHeaderView::Stretch);         // Description
    m_packagesTable->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents); // Repository
    
    // Set checkbox column width
    m_packagesTable->setColumnWidth(0, 30);
    
    m_searchLayout->addWidget(m_packagesTable);
    
    // Add search tab to main tab widget
    qDebug() << "DEBUG: Adding search tab to main tab widget";
    m_tabWidget->addTab(m_searchTab, tr("Search Packages"));
    
    // Create installed packages tab
    qDebug() << "DEBUG: Creating installed packages tab";
    m_installedTab = new QWidget();
    m_installedLayout = new QVBoxLayout(m_installedTab);
    
    // Create installed packages action buttons - now at the top
    qDebug() << "DEBUG: Creating installed packages action buttons";
    m_installedActionsLayout = new QHBoxLayout();
    m_removeInstalledButton = new QPushButton(tr("Remove"), m_installedTab);
    
    // Make buttons smaller with consistent styling
    m_removeInstalledButton->setStyleSheet("QPushButton {"
                                          "background-color: #0078d7; "
                                          "color: white;"
                                          "border: none;"
                                          "border-radius: 3px;"
                                          "padding: 6px 12px;"
                                          "margin: 2px 4px;"
                                          "}");
    m_removeInstalledButton->setMinimumWidth(100);
    m_removeInstalledButton->setMinimumHeight(26);
    m_removeInstalledButton->setMaximumHeight(30);
    m_removeInstalledButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    m_installedActionsLayout->addWidget(m_removeInstalledButton);
    m_installedActionsLayout->addStretch(1); // Push buttons to the left
    
    m_installedLayout->addLayout(m_installedActionsLayout);
    
    // Create installed packages table
    qDebug() << "DEBUG: Creating installed packages table";
    m_installedTable = new QTreeView(m_installedTab);
    m_installedTable->setSelectionMode(QAbstractItemView::ExtendedSelection); // Allow multiple selection
    m_installedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_installedTable->setSortingEnabled(true);
    m_installedTable->setAlternatingRowColors(true);
    
    m_installedTable->setModel(m_installedModel);
    
    // Update column sizing to account for checkbox column
    m_installedTable->header()->setSectionResizeMode(0, QHeaderView::Fixed);  // Checkbox column
    m_installedTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Name
    m_installedTable->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Version
    m_installedTable->header()->setSectionResizeMode(3, QHeaderView::Stretch);         // Description
    m_installedTable->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents); // Repository
    
    // Set checkbox column width
    m_installedTable->setColumnWidth(0, 30);
    
    m_installedLayout->addWidget(m_installedTable);
    
    // Add installed packages tab to main tab widget
    qDebug() << "DEBUG: Adding installed packages tab to main tab widget";
    m_tabWidget->addTab(m_installedTab, tr("Installed Packages"));
    
    // Add status bar
    statusBar()->showMessage(tr("Ready"));
    
    qDebug() << "DEBUG: Exiting setupUi()";
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
    
    // Set checkbox column width
    m_systemUpdatesTable->setColumnWidth(0, 30);
    
    m_systemUpdateLayout->addWidget(m_systemUpdatesTable);
    
    // Create update log view
    m_systemUpdateLogView = new QTextEdit(m_systemUpdateTab);
    m_systemUpdateLogView->setReadOnly(true);
    m_systemUpdateLogView->setMaximumHeight(120);
    m_systemUpdateLogView->setPlaceholderText(tr("Update log will appear here..."));
    m_systemUpdateLogView->setStyleSheet("background-color: #f8f9fa; border: 1px solid #e0e0e0; border-radius: 3px;");
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
    m_maintenanceLogView->setStyleSheet("background-color: #f8f9fa; border: 1px solid #e0e0e0; border-radius: 3px;");
    
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
    if (settings.contains("mainwindow/geometry")) {
        restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
    }
    
    // Load window state
    if (settings.contains("mainwindow/state")) {
        restoreState(settings.value("mainwindow/state").toByteArray());
    }
    
    // Load theme preference
    bool isDark = settings.value("appearance/darkTheme", true).toBool();
    applyTheme(isDark);
}

// Add implementation for isDarkThemeEnabled
bool MainWindow::isDarkThemeEnabled() const {
    QSettings settings("PacmanGUI", "PacmanGUI");
    return settings.value("appearance/darkTheme", true).toBool();
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
    // Ensure model is properly initialized
    if (!m_installedModel) {
        m_installedModel = new QStandardItemModel(0, 5, this);
        m_installedModel->setHorizontalHeaderLabels(
            QStringList() << tr("") << tr("Name") << tr("Version") << tr("Description") << tr("Repository"));
    }
    
    // Clear the installed packages model
    m_installedModel->clear();
    m_installedModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Version") << tr("Description") << tr("Repository"));
    
    // Get installed packages from the package manager
    std::vector<pacmangui::core::Package> installedPackages = m_packageManager.get_installed_packages();
    
    // Add packages to the model
    for (const auto& pkg : installedPackages) {
        QList<QStandardItem*> row;
        
        // Add checkbox item
        QStandardItem* checkItem = new QStandardItem();
        checkItem->setCheckable(true);
        checkItem->setCheckState(Qt::Unchecked);
        checkItem->setData(Qt::AlignCenter, Qt::TextAlignmentRole);
        
        QStandardItem* nameItem = new QStandardItem(QString::fromStdString(pkg.get_name()));
        QStandardItem* versionItem = new QStandardItem(QString::fromStdString(pkg.get_version()));
        QStandardItem* descItem = new QStandardItem(QString::fromStdString(pkg.get_description()));
        QStandardItem* repoItem = new QStandardItem(QString::fromStdString(pkg.get_repository()));
        
        row << checkItem << nameItem << versionItem << descItem << repoItem;
        m_installedModel->appendRow(row);
    }
    
    // Update status bar
    showStatusMessage(tr("Loaded %1 installed packages").arg(installedPackages.size()), 3000);
}

// Add implementation for searchPackages
void MainWindow::searchPackages(const QString& searchTerm) {
    // This is now a wrapper around performAsyncSearch
    performAsyncSearch(searchTerm);
}

// Implementation of async search
void MainWindow::performAsyncSearch(const QString& searchTerm) {
    // Ensure model is properly initialized
    if (!m_packagesModel) {
        m_packagesModel = new QStandardItemModel(0, 5, this);
        m_packagesModel->setHorizontalHeaderLabels(
            QStringList() << tr("") << tr("Name") << tr("Version") << tr("Description") << tr("Repository"));
    }
    
    // Clear the search packages model
    m_packagesModel->clear();
    m_packagesModel->setHorizontalHeaderLabels(
        QStringList() << tr("") << tr("Name") << tr("Version") << tr("Description") << tr("Repository"));
    
    // If search term is empty, show nothing
    if (searchTerm.isEmpty()) {
        showStatusMessage(tr("Enter a search term to find packages"), 3000);
                return;
            }
            
    // Show searching status
    showStatusMessage(tr("Searching for packages matching '%1'...").arg(searchTerm), 0);
    
    // Cancel any previous search
    if (m_searchWatcher) {
        if (m_searchWatcher->isRunning()) {
            m_searchWatcher->cancel();
            m_searchWatcher->waitForFinished();
        }
        delete m_searchWatcher;
        m_searchWatcher = nullptr;
    }
    
    // Create new watcher for async search
    m_searchWatcher = new QFutureWatcher<std::vector<pacmangui::core::Package>>(this);
    
    // Connect signals
    connect(m_searchWatcher, &QFutureWatcher<std::vector<pacmangui::core::Package>>::finished, 
            this, [this, searchTerm]() {
        if (!m_searchWatcher) return;
        
        // Get results
        std::vector<pacmangui::core::Package> searchResults = m_searchWatcher->result();
        
        // Clear the model first
        m_packagesModel->clear();
        m_packagesModel->setHorizontalHeaderLabels(
            QStringList() << tr("") << tr("Name") << tr("Version") << tr("Description") << tr("Repository"));
        
        // Add packages to the model
        for (const auto& pkg : searchResults) {
            QList<QStandardItem*> row;
            
            // Add checkbox item
            QStandardItem* checkItem = new QStandardItem();
            checkItem->setCheckable(true);
            checkItem->setCheckState(Qt::Unchecked);
            checkItem->setData(Qt::AlignCenter, Qt::TextAlignmentRole);
            
            QStandardItem* nameItem = new QStandardItem(QString::fromStdString(pkg.get_name()));
            QStandardItem* versionItem = new QStandardItem(QString::fromStdString(pkg.get_version()));
            QStandardItem* descItem = new QStandardItem(QString::fromStdString(pkg.get_description()));
            QStandardItem* repoItem = new QStandardItem(QString::fromStdString(pkg.get_repository()));
            
            row << checkItem << nameItem << versionItem << descItem << repoItem;
            m_packagesModel->appendRow(row);
        }
        
        // Hide progress dialog
        m_searchProgressDialog->hide();
        
        // Clean up
        m_searchWatcher->deleteLater();
        m_searchWatcher = nullptr;
        
        // Update status bar
        showStatusMessage(tr("Found %1 packages matching '%2'")
                         .arg(searchResults.size())
                         .arg(searchTerm), 3000);
    });
    
    // Connect cancel button
    connect(m_searchProgressDialog, &QProgressDialog::canceled, [this]() {
        if (m_searchWatcher && m_searchWatcher->isRunning()) {
            m_searchWatcher->cancel();
            showStatusMessage(tr("Search canceled"), 3000);
        }
    });
    
    // Show progress dialog
    m_searchProgressDialog->setLabelText(tr("Searching for '%1'...").arg(searchTerm));
    m_searchProgressDialog->reset();
    m_searchProgressDialog->show();
    
    // Run the search in background
    QFuture<std::vector<pacmangui::core::Package>> future = QtConcurrent::run(
        [this, searchTerm]() {
            return m_packageManager.search_by_name(searchTerm.toStdString());
        }
    );
    
    m_searchWatcher->setFuture(future);
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
    } else if (text.isEmpty()) {
        // Clear the search results
        m_packagesModel->clear();
        m_packagesModel->setHorizontalHeaderLabels(
            QStringList() << tr("Name") << tr("Version") << tr("Description") << tr("Repository"));
    }
}

// Add implementation for onSearchClicked
void MainWindow::onSearchClicked() {
    // Get the search text
    QString searchText = m_searchInput->text();
    
    // Perform the search
    if (!searchText.isEmpty()) {
        performAsyncSearch(searchText);
                    } else {
        showStatusMessage(tr("Please enter a search term"), 3000);
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
    
    // Collect package names
    QStringList packageNames;
    QStringList packageDetails;
    for (const QModelIndex& index : selected) {
        QModelIndex nameIndex = m_packagesModel->index(index.row(), 0); // Name column
        QModelIndex versionIndex = m_packagesModel->index(index.row(), 1); // Version column
        QModelIndex repoIndex = m_packagesModel->index(index.row(), 3); // Repository column
        
        QString name = m_packagesModel->data(nameIndex).toString();
        QString version = m_packagesModel->data(versionIndex).toString();
        QString repo = m_packagesModel->data(repoIndex).toString();
        
        packageNames.append(name);
        packageDetails.append(tr("%1 (%2) from %3").arg(name).arg(version).arg(repo));
    }
    
    // Confirm installation
    QString message;
    if (packageNames.size() == 1) {
        message = tr("Are you sure you want to install %1?").arg(packageDetails.first());
                } else {
        message = tr("Are you sure you want to install the following %1 packages?\n\n%2")
            .arg(packageNames.size())
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
    
    // Create progress dialog
    QProgressDialog progressDialog(tr("Installing packages..."), tr("Cancel"), 0, 0, this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(500);
    progressDialog.setValue(0);
    progressDialog.setAutoClose(false);
    progressDialog.show();
    
    // Install packages one by one and collect output
    bool success = true;
    
    for (const QString& packageName : packageNames) {
        m_maintenanceLogView->append(tr("Installing %1...").arg(packageName));
        success = m_packageManager.install_package(packageName.toStdString()) && success;
    }
    
    // Update progress dialog
    progressDialog.setValue(100);
    progressDialog.setLabelText(success ? tr("Installation completed") : tr("Installation failed"));
    
    // Show results in log dialog
    QDialog logDialog(this);
    logDialog.setWindowTitle(tr("Installation Log"));
    logDialog.setMinimumSize(600, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(&logDialog);
    QTextEdit* logView = new QTextEdit(&logDialog);
    logView->setReadOnly(true);
    logView->setPlainText(success ? tr("Packages installed successfully.") : tr("Failed to install some packages."));
    layout->addWidget(logView);
    
    QPushButton* closeButton = new QPushButton(tr("Close"), &logDialog);
    layout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &logDialog, &QDialog::accept);
    
    // Update status
    if (success) {
        showStatusMessage(tr("Packages installed successfully"), 5000);
        progressDialog.close();
        logDialog.exec();
        
        // Refresh installed packages list
        refreshInstalledPackages();
    } else {
        showStatusMessage(tr("Error installing packages"), 5000);
        progressDialog.close();
        logDialog.exec();
    }
}

// Add implementation for onRemovePackage
void MainWindow::onRemovePackage() {
    // Determine which tab is active to get the correct selection
    QTreeView* activeTable = nullptr;
    QStandardItemModel* activeModel = nullptr;
    
    if (m_tabWidget->currentWidget() == m_searchTab) {
        activeTable = m_packagesTable;
        activeModel = m_packagesModel;
    } else if (m_tabWidget->currentWidget() == m_installedTab) {
        activeTable = m_installedTable;
        activeModel = m_installedModel;
    } else {
        showStatusMessage(tr("Please select packages to remove from the installed packages tab"), 3000);
        return;
    }
    
    // Get selected packages
    QModelIndexList selected = activeTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        showStatusMessage(tr("No packages selected for removal"), 3000);
        return;
    }
    
    // Collect package names
    QStringList packageNames;
    QStringList packageDetails;
    for (const QModelIndex& index : selected) {
        QModelIndex nameIndex = activeModel->index(index.row(), 0); // Name column
        QModelIndex versionIndex = activeModel->index(index.row(), 1); // Version column
        
        QString name = activeModel->data(nameIndex).toString();
        QString version = activeModel->data(versionIndex).toString();
        
        packageNames.append(name);
        packageDetails.append(tr("%1 (%2)").arg(name).arg(version));
    }
    
    // Confirm removal
    QString message;
    if (packageNames.size() == 1) {
        message = tr("Are you sure you want to remove %1?").arg(packageDetails.first());
    } else {
        message = tr("Are you sure you want to remove the following %1 packages?\n\n%2")
            .arg(packageNames.size())
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
    
    // Create progress dialog
    QProgressDialog progressDialog(tr("Removing packages..."), tr("Cancel"), 0, 0, this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(500);
    progressDialog.setValue(0);
    progressDialog.setAutoClose(false);
    progressDialog.show();
    
    // Remove packages one by one and collect output
    bool success = true;
    
    for (const QString& packageName : packageNames) {
        m_maintenanceLogView->append(tr("Removing %1...").arg(packageName));
        success = m_packageManager.remove_package(packageName.toStdString()) && success;
    }
    
    // Update progress dialog
    progressDialog.setValue(100);
    progressDialog.setLabelText(success ? tr("Removal completed") : tr("Removal failed"));
    
    // Show results in log dialog
    QDialog logDialog(this);
    logDialog.setWindowTitle(tr("Removal Log"));
    logDialog.setMinimumSize(600, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(&logDialog);
    QTextEdit* logView = new QTextEdit(&logDialog);
    logView->setReadOnly(true);
    logView->setPlainText(success ? tr("Packages removed successfully.") : tr("Failed to remove some packages."));
    layout->addWidget(logView);
    
    QPushButton* closeButton = new QPushButton(tr("Close"), &logDialog);
    layout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &logDialog, &QDialog::accept);
    
    // Update status
    if (success) {
        showStatusMessage(tr("Packages removed successfully"), 5000);
        progressDialog.close();
        logDialog.exec();
        
        // Refresh installed packages list
        refreshInstalledPackages();
    } else {
        showStatusMessage(tr("Error removing packages"), 5000);
        progressDialog.close();
        logDialog.exec();
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
        tr("Are you sure you want to update all packages? This will require authentication."),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (confirm != QMessageBox::Yes) {
        return;
    }
    
    // Clear the update log
    m_systemUpdateLogView->clear();
    m_systemUpdateLogView->append(tr("Starting system update..."));
    
    // Use pkexec for proper authentication
    QProcess* process = new QProcess(this);
    
    // Set process to maintain terminal and capture output
    process->setProcessChannelMode(QProcess::MergedChannels);
    
    // Connect signals to capture and display output
    connect(process, &QProcess::readyReadStandardOutput, [=]() {
        QString output = process->readAllStandardOutput();
        if (!output.isEmpty()) {
            m_systemUpdateLogView->append(output);
            // Scroll to bottom to show latest output
            QScrollBar* scrollBar = m_systemUpdateLogView->verticalScrollBar();
            scrollBar->setValue(scrollBar->maximum());
        }
    });
    
    // Connect to process finished signal
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                m_systemUpdateLogView->append(tr("System update completed successfully."));
                showStatusMessage(tr("System update completed successfully."), 5000);
            } else {
                m_systemUpdateLogView->append(tr("System update failed with exit code %1.").arg(exitCode));
                showStatusMessage(tr("System update failed."), 5000);
            }
            
            // Clean up the process
            process->deleteLater();
            
            // Refresh installed packages
        refreshInstalledPackages();
            
            // Clear updates list since we've just updated
            m_systemUpdatesModel->clear();
            m_systemUpdatesModel->setHorizontalHeaderLabels(
                QStringList() << tr("Name") << tr("Current") << tr("New") << tr("Repository"));
        }
    );
    
    // Start process with pkexec which will prompt for password
    process->start("pkexec", QStringList() << "pacman" << "-Syu" << "--noconfirm");
    
    // Show status message
    showStatusMessage(tr("Updating system packages..."), 0);
}

// Add implementation for onCheckForUpdates
void MainWindow::onCheckForUpdates() {
    // Show status message and update UI
    showStatusMessage(tr("Checking for updates..."), 0);
    m_systemUpdateInfoLabel->setText(tr("Checking for updates..."));
    
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
    
    // Clear the log view
    if (m_systemUpdateLogView) {
        m_systemUpdateLogView->clear();
        m_systemUpdateLogView->append(tr("Starting update check..."));
    }
    
    // Refresh the package database first
    bool syncSuccess = m_packageManager.sync_all();
    
    if (!syncSuccess) {
        m_systemUpdateInfoLabel->setText(tr("Error synchronizing package databases."));
        showStatusMessage(tr("Error synchronizing package databases"), 5000);
        m_systemUpdateLogView->append(tr("Failed to synchronize package databases."));
        return;
    }
    
    m_systemUpdateLogView->append(tr("Checking for available updates..."));
    
    // Check for updates
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
                QString name = m_installedModel->item(i, 0)->text();
                if (name == pkgName) {
                    currentVersion = m_installedModel->item(i, 1)->text();
                    repoName = m_installedModel->item(i, 3)->text();
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
        
        // Connect theme changed signal using lambda
        connect(m_settingsDialog, &SettingsDialog::themeChanged, 
                [this](bool isDark) { this->toggleTheme(isDark); });
        
        // Connect AUR status changed signal
        connect(m_settingsDialog, &SettingsDialog::aurStatusChanged, [this](bool enabled) {
            // Update UI based on AUR status
            if (m_installAurButton) {
                m_installAurButton->setEnabled(enabled && !m_aurHelper.isEmpty());
            }
            
            // Refresh AUR helper if enabled
            if (enabled) {
                checkAurHelper();
            }
        });
    }
    
    // Show dialog
    m_settingsDialog->loadSettings();
    m_settingsDialog->exec();
    
    // Handle result
    if (m_settingsDialog->result() == QDialog::Accepted) {
        // Save settings
        m_settingsDialog->saveSettings();
        
        // Update AUR helper
        checkAurHelper();
        
        // Show status message
        showStatusMessage(tr("Settings saved successfully"), 2000);
    }
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
void MainWindow::closeEvent(QCloseEvent* event) {
    QMainWindow::closeEvent(event); // Call base implementation
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
        
} // namespace gui
} // namespace pacmangui