#include "gui/mainwindow.hpp"
#include "gui/settingsdialog.hpp"
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
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QProcess>
#include <QScrollBar>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrent>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QMetaObject>

namespace pacmangui {
namespace gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_searchBox(nullptr)
    , m_searchButton(nullptr)
    , m_settingsButton(nullptr)
    , m_packagesView(nullptr)
    , m_installedView(nullptr)
    , m_batchInstallButton(nullptr)
    , m_systemUpdateTab(nullptr)
    , m_systemUpdateInfoLabel(nullptr)
    , m_systemUpdateButton(nullptr)
    , m_checkUpdatesButton(nullptr)
    , m_systemUpdateLogView(nullptr)
    , m_systemUpdatesView(nullptr)
    , m_systemUpdatesModel(nullptr)
    , m_systemUpdateOverwriteCheckbox(nullptr)
    , m_detailsWidget(nullptr)
    , m_packageNameLabel(nullptr)
    , m_packageVersionLabel(nullptr)
    , m_packageDescLabel(nullptr)
    , m_actionButton(nullptr)
    , m_packageOverwriteCheckbox(nullptr)
    , m_packagesModel(nullptr)
    , m_installedModel(nullptr)
    , m_selectedPackages()
    , m_darkTheme(false)
    , m_settingsDialog(new SettingsDialog(this))
    , m_packageManager()
{
    setWindowTitle("PacmanGUI");
    setMinimumSize(1000, 600);
    
    // Initialize package manager
    if (!m_packageManager.initialize("/", "/var/lib/pacman")) {
        QMessageBox::critical(this, "Error", "Failed to initialize package manager.");
        return;
    }
    
    // Setup UI components
    setupUi();
    setupActions();
    setupMenus();
    setupConnections();
    
    // Load settings
    loadSettings();
    
    // Apply theme
    applyTheme(m_darkTheme);
    
    // Populate the tables
    refreshInstalledPackages();
    refreshUpdatesList();
    
    // Check for AUR helper
    checkAurHelper();
    
    // Show status message
    statusBar()->showMessage("Ready", 3000);
}

MainWindow::~MainWindow()
{
    // Clean up
    delete m_packagesModel;
    delete m_installedModel;
    delete m_systemUpdatesModel;
}

void MainWindow::checkAurHelper()
{
    // Check if AUR is enabled in settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    bool aurEnabled = settings.value("aur/enabled", false).toBool();
    
    if (!aurEnabled) {
        return;  // AUR not enabled, nothing to check
    }
    
    // Get list of common AUR helpers
    const QStringList helpers = {"yay", "paru", "pikaur", "trizen", "pacaur"};
    bool helperFound = false;
    
    // Check if any helper is installed
    for (const QString& helper : helpers) {
        QString path = QStandardPaths::findExecutable(helper);
        if (!path.isEmpty()) {
            helperFound = true;
            // Update the selected AUR helper in settings
            settings.setValue("aur/helper", helper);
            break;
        }
    }
    
    // If no helper found but AUR is enabled, prompt to download yay-bin
    if (!helperFound) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            "AUR Helper Not Found",
            "AUR support is enabled but no AUR helper was found.\n\n"
            "Would you like to download and install 'yay-bin' from GitHub?",
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            downloadYayHelper();
        } else {
            // Disable AUR since no helper is available
            settings.setValue("aur/enabled", false);
            
            // Update UI
            for (QAction* action : findChildren<QAction*>()) {
                if (action->statusTip().contains("AUR")) {
                    action->setEnabled(false);
                }
            }
            
            // Show message
            statusBar()->showMessage("AUR support disabled (no helper found)", 5000);
        }
    }
}

void MainWindow::downloadYayHelper()
{
    // Show progress dialog
    QProgressDialog progress("Downloading yay-bin...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    
    // Use a background thread for download
    QFuture<bool> future = QtConcurrent::run([this, &progress]() {
        try {
            // Step 1: Create a temporary directory
            QTemporaryDir tempDir;
            if (!tempDir.isValid()) {
                throw std::runtime_error("Could not create temporary directory");
            }
            
            progress.setValue(10);
            if (progress.wasCanceled()) return false;
            
            // Step 2: Download yay-bin from GitHub using curl
            std::string downloadCommand = "curl -L -o " + tempDir.path().toStdString() + 
                                        "/yay-bin-latest.tar.gz " +
                                        "https://github.com/Jguer/yay/releases/latest/download/yay_*_x86_64.tar.gz";
            
            int result = system(downloadCommand.c_str());
            if (result != 0) {
                throw std::runtime_error("Failed to download yay-bin");
            }
            
            progress.setValue(40);
            if (progress.wasCanceled()) return false;
            
            // Step 3: Extract the tar.gz file
            std::string extractCommand = "tar -xzf " + tempDir.path().toStdString() + 
                                       "/yay-bin-latest.tar.gz -C " + tempDir.path().toStdString();
            
            result = system(extractCommand.c_str());
            if (result != 0) {
                throw std::runtime_error("Failed to extract yay-bin");
            }
            
            progress.setValue(70);
            if (progress.wasCanceled()) return false;
            
            // Step 4: Get admin password for installation
            bool ok;
            QString password = QInputDialog::getText(nullptr, "Authentication Required",
                                                  "Enter your password to install yay:",
                                                  QLineEdit::Password, "", &ok);
            
            if (!ok || password.isEmpty()) {
                throw std::runtime_error("Authentication cancelled");
            }
            
            // Step 5: Move the binary to /usr/bin
            std::string moveCommand = "cp " + tempDir.path().toStdString() + "/yay /tmp/yay-bin";
            result = system(moveCommand.c_str());
            if (result != 0) {
                throw std::runtime_error("Failed to prepare yay binary");
            }
            
            std::string installCommand = "mv /tmp/yay-bin /usr/bin/yay && chmod +x /usr/bin/yay";
            bool success = m_packageManager.execute_with_sudo(installCommand, password.toStdString());
            
            if (!success) {
                throw std::runtime_error("Failed to install yay. Authentication may have failed.");
            }
            
            progress.setValue(100);
            
            // Update settings
            QSettings settings("PacmanGUI", "PacmanGUI");
            settings.setValue("aur/enabled", true);
            settings.setValue("aur/helper", "yay");
            
            return true;
        }
        catch (const std::exception& e) {
            QString errorMsg = QString("Error: %1").arg(e.what());
            // Use QMetaObject::invokeMethod to safely show message from another thread
            QMetaObject::invokeMethod(this, "showStatusMessage", 
                                     Qt::QueuedConnection,
                                     Q_ARG(QString, errorMsg),
                                     Q_ARG(int, 5000));
            return false;
        }
    });
    
    // Handle future result in the main thread
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
        bool success = watcher->result();
        if (success) {
            QMessageBox::information(this, "Installation Successful", 
                                     "yay was successfully installed.\nAUR support is now enabled.");
            
            // Enable AUR actions
            for (QAction* action : findChildren<QAction*>()) {
                if (action->statusTip().contains("AUR")) {
                    action->setEnabled(true);
                }
            }
            
            statusBar()->showMessage("yay installed. AUR support enabled", 5000);
        } else {
            QMessageBox::warning(this, "Installation Failed", 
                                "Failed to install yay.\nAUR support will be disabled.");
            
            // Disable AUR actions
            for (QAction* action : findChildren<QAction*>()) {
                if (action->statusTip().contains("AUR")) {
                    action->setEnabled(false);
                }
            }
            
            // Update settings
            QSettings settings("PacmanGUI", "PacmanGUI");
            settings.setValue("aur/enabled", false);
        }
        
        watcher->deleteLater();
    });
    
    watcher->setFuture(future);
}

void MainWindow::showStatusMessage(const QString& message, int timeout)
{
    statusBar()->showMessage(message, timeout);
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
    
    // Create settings button with cog icon
    m_settingsButton = new QToolButton(this);
    m_settingsButton->setIcon(QIcon::fromTheme("preferences-system", QIcon(":/icons/cog.png")));
    m_settingsButton->setIconSize(QSize(20, 20));
    m_settingsButton->setToolTip("Settings");
    
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchBox, 1); // Make search box expandable
    searchLayout->addWidget(m_searchButton);
    searchLayout->addWidget(m_settingsButton);
    
    mainLayout->addLayout(searchLayout);
    
    // Create tab widget for different views
    m_tabWidget = new QTabWidget(this);
    
    // Create tab pages
    QWidget* packagesTab = new QWidget(this);
    QWidget* installedTab = new QWidget(this);
    m_systemUpdateTab = new QWidget(this);
    
    // Set up package list view
    QVBoxLayout* packagesLayout = new QVBoxLayout(packagesTab);
    packagesLayout->setContentsMargins(0, 0, 0, 0);
    
    m_packagesView = new QTableView(packagesTab);
    m_packagesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_packagesView->setSelectionMode(QAbstractItemView::MultiSelection);
    m_packagesView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_packagesView->setAlternatingRowColors(true);
    m_packagesView->setSortingEnabled(true);
    
    m_packagesModel = new QStandardItemModel(0, 5, this);
    m_packagesModel->setHorizontalHeaderLabels(QStringList() << "" << "Name" << "Version" << "Repository" << "Description");
    m_packagesView->setModel(m_packagesModel);
    
    // Set column widths and alignment
    m_packagesView->setColumnWidth(0, 30); // Checkbox column
    m_packagesView->setColumnWidth(1, 180); // Name column
    m_packagesView->setColumnWidth(2, 100); // Version column
    m_packagesView->setColumnWidth(3, 120); // Repository column
    m_packagesView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_packagesView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed); // Fix the checkbox column width
    m_packagesView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch); // Make description column stretch
    
    // Add batch install button
    m_batchInstallButton = new QPushButton("Install Selected Packages", packagesTab);
    m_batchInstallButton->setEnabled(false);
    
    packagesLayout->addWidget(m_packagesView);
    packagesLayout->addWidget(m_batchInstallButton);
    
    // Set up installed packages view
    QVBoxLayout* installedLayout = new QVBoxLayout(installedTab);
    installedLayout->setContentsMargins(0, 0, 0, 0);
    
    m_installedView = new QTableView(installedTab);
    m_installedView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_installedView->setSelectionMode(QAbstractItemView::MultiSelection);
    m_installedView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_installedView->setAlternatingRowColors(true);
    m_installedView->horizontalHeader()->setStretchLastSection(true);
    m_installedView->verticalHeader()->setVisible(false);
    m_installedView->setSortingEnabled(true);
    
    m_installedModel = new QStandardItemModel(0, 4, this);
    m_installedModel->setHorizontalHeaderLabels(QStringList() << "" << "Name" << "Version" << "Repository" << "Description");
    m_installedView->setModel(m_installedModel);
    
    // Set column widths and alignment
    m_installedView->setColumnWidth(0, 30); // Checkbox column
    m_installedView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_installedView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed); // Fix the checkbox column width
    
    installedLayout->addWidget(m_installedView);
    
    // Set up system update tab
    QVBoxLayout* systemUpdateLayout = new QVBoxLayout(m_systemUpdateTab);
    systemUpdateLayout->setContentsMargins(10, 10, 10, 10);
    systemUpdateLayout->setSpacing(15);
    
    // Information label
    m_systemUpdateInfoLabel = new QLabel(
        "<h2>System Update</h2>"
        "<p>Perform a full system update to ensure all packages are up to date.</p>"
        "<p>This will execute <code>sudo pacman -Syu</code> to update your system.</p>"
        "<p><b>Note:</b> You'll need administrator privileges to perform this operation.</p>",
        m_systemUpdateTab);
    m_systemUpdateInfoLabel->setWordWrap(true);
    systemUpdateLayout->addWidget(m_systemUpdateInfoLabel);
    
    // Button layout
    QHBoxLayout* systemUpdateButtonLayout = new QHBoxLayout();
    systemUpdateButtonLayout->setSpacing(10);
    
    // Define a common font for buttons
    QFont buttonFont;
    buttonFont.setBold(true);
    
    // Add check for updates button
    m_checkUpdatesButton = new QPushButton("Check for Updates", m_systemUpdateTab);
    m_checkUpdatesButton->setFont(buttonFont);
    m_checkUpdatesButton->setMinimumHeight(36);
    systemUpdateButtonLayout->addWidget(m_checkUpdatesButton);
    
    // Add system update button
    m_systemUpdateButton = new QPushButton("Update System", m_systemUpdateTab);
    m_systemUpdateButton->setFont(buttonFont);
    m_systemUpdateButton->setMinimumHeight(36);
    systemUpdateButtonLayout->addWidget(m_systemUpdateButton);
    
    systemUpdateLayout->addLayout(systemUpdateButtonLayout);
    
    // Add overwrite checkbox for system update in its own layout
    m_systemUpdateOverwriteCheckbox = new QCheckBox("Use --overwrite \"*\" (forces package installation, can break your system)", m_systemUpdateTab);
    m_systemUpdateOverwriteCheckbox->setToolTip("This option allows pacman to overwrite conflicting files. Use with caution!");
    systemUpdateLayout->addWidget(m_systemUpdateOverwriteCheckbox);
    
    // Add updates table view
    m_systemUpdatesView = new QTableView(m_systemUpdateTab);
    m_systemUpdatesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_systemUpdatesView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_systemUpdatesView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_systemUpdatesView->setAlternatingRowColors(true);
    m_systemUpdatesView->horizontalHeader()->setStretchLastSection(true);
    m_systemUpdatesView->verticalHeader()->setVisible(false);
    m_systemUpdatesView->setSortingEnabled(true);
    
    m_systemUpdatesModel = new QStandardItemModel(0, 2, this);
    m_systemUpdatesModel->setHorizontalHeaderLabels(QStringList() << "Package" << "New Version");
    m_systemUpdatesView->setModel(m_systemUpdatesModel);
    m_systemUpdatesView->setMinimumHeight(150);
    
    systemUpdateLayout->addWidget(m_systemUpdatesView);
    
    // Log viewer
    QLabel* logLabel = new QLabel("System Update Log:", m_systemUpdateTab);
    logLabel->setFont(buttonFont);
    systemUpdateLayout->addWidget(logLabel);
    
    m_systemUpdateLogView = new QTextEdit(m_systemUpdateTab);
    m_systemUpdateLogView->setReadOnly(true);
    m_systemUpdateLogView->setPlaceholderText("System update log will appear here...");
    m_systemUpdateLogView->setMinimumHeight(150);
    systemUpdateLayout->addWidget(m_systemUpdateLogView, 1);
    
    // Add tabs to tab widget
    m_tabWidget->addTab(packagesTab, "All Packages");
    m_tabWidget->addTab(installedTab, "Installed");
    m_tabWidget->addTab(m_systemUpdateTab, "System Update");
    
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
    
    // Add package overwrite checkbox
    m_packageOverwriteCheckbox = new QCheckBox("Use --overwrite \"*\"", this);
    m_packageOverwriteCheckbox->setToolTip("This option allows pacman to overwrite conflicting files. Use with caution!");
    
    // Add widgets to details layout
    detailsLayout->addWidget(m_packageNameLabel);
    detailsLayout->addWidget(m_packageVersionLabel);
    detailsLayout->addWidget(m_packageDescLabel);
    detailsLayout->addWidget(m_packageOverwriteCheckbox);
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
    // Create actions for menu
    QAction* installAction = new QAction("Install Package", this);
    installAction->setStatusTip("Install selected package");
    connect(installAction, &QAction::triggered, this, &MainWindow::onInstallPackage);
    
    QAction* removeAction = new QAction("Remove Package", this);
    removeAction->setStatusTip("Remove selected package");
    connect(removeAction, &QAction::triggered, this, &MainWindow::onRemovePackage);
    
    QAction* updateAction = new QAction("Update Package", this);
    updateAction->setStatusTip("Update selected package");
    connect(updateAction, &QAction::triggered, this, &MainWindow::onUpdatePackage);
    
    QAction* syncAction = new QAction("Sync Databases", this);
    syncAction->setStatusTip("Synchronize package databases");
    connect(syncAction, &QAction::triggered, this, &MainWindow::onSyncAll);
    
    QAction* systemUpdateAction = new QAction("System Update", this);
    systemUpdateAction->setStatusTip("Update all packages on the system");
    connect(systemUpdateAction, &QAction::triggered, this, &MainWindow::onSystemUpdate);
    
    QAction* checkUpdatesAction = new QAction("Check for Updates", this);
    checkUpdatesAction->setStatusTip("Check for available updates");
    connect(checkUpdatesAction, &QAction::triggered, this, &MainWindow::onCheckForUpdates);
    
    // AUR actions
    QAction* aurInstallAction = new QAction("Install AUR Package", this);
    aurInstallAction->setStatusTip("Install selected AUR package");
    connect(aurInstallAction, &QAction::triggered, this, &MainWindow::onInstallAurPackage);
    
    QAction* aurUpdateAction = new QAction("Update AUR Packages", this);
    aurUpdateAction->setStatusTip("Update AUR packages");
    connect(aurUpdateAction, &QAction::triggered, this, &MainWindow::onUpdateAurPackages);
    
    QAction* settingsAction = new QAction("Settings", this);
    settingsAction->setStatusTip("Open settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    
    QAction* aboutAction = new QAction("About", this);
    aboutAction->setStatusTip("About PacmanGUI");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    
    QAction* exitAction = new QAction("Exit", this);
    exitAction->setStatusTip("Exit application");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    // Save the actions for use in the menu setup
    m_actions["install"] = installAction;
    m_actions["remove"] = removeAction;
    m_actions["update"] = updateAction;
    m_actions["sync"] = syncAction;
    m_actions["systemUpdate"] = systemUpdateAction;
    m_actions["checkUpdates"] = checkUpdatesAction;
    m_actions["aurInstall"] = aurInstallAction;
    m_actions["aurUpdate"] = aurUpdateAction;
    m_actions["settings"] = settingsAction;
    m_actions["about"] = aboutAction;
    m_actions["exit"] = exitAction;
    
    // Check if AUR is enabled and disable AUR actions if not
    QSettings settings("PacmanGUI", "PacmanGUI");
    bool aurEnabled = settings.value("aur/enabled", false).toBool();
    aurInstallAction->setEnabled(aurEnabled);
    aurUpdateAction->setEnabled(aurEnabled);
}

void MainWindow::setupMenus()
{
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(m_actions["sync"]);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actions["exit"]);
    
    // Operations menu
    QMenu* operationsMenu = menuBar()->addMenu("&Operations");
    
    // Package operations
    operationsMenu->addAction(m_actions["install"]);
    operationsMenu->addAction(m_actions["remove"]);
    operationsMenu->addAction(m_actions["update"]);
    operationsMenu->addSeparator();
    
    // System update operations
    operationsMenu->addAction(m_actions["checkUpdates"]);
    operationsMenu->addAction(m_actions["systemUpdate"]);
    operationsMenu->addSeparator();
    
    // AUR operations
    operationsMenu->addAction(m_actions["aurInstall"]);
    operationsMenu->addAction(m_actions["aurUpdate"]);
    
    // Settings menu
    QMenu* settingsMenu = menuBar()->addMenu("&Settings");
    
    // AUR support setting
    QSettings settings("PacmanGUI", "PacmanGUI");
    bool aurEnabled = settings.value("aur/enabled", false).toBool();
    
    QAction* aurAction = settingsMenu->addAction("Enable &AUR Support");
    aurAction->setCheckable(true);
    aurAction->setChecked(aurEnabled);
    
    connect(aurAction, &QAction::toggled, this, [this](bool checked) {
        QSettings settings("PacmanGUI", "PacmanGUI");
        settings.setValue("aur/enabled", checked);
        
        // Update AUR actions enable state
        m_actions["aurInstall"]->setEnabled(checked);
        m_actions["aurUpdate"]->setEnabled(checked);
        
        QString message = checked ? "AUR support enabled" : "AUR support disabled";
        statusBar()->showMessage(message, 3000);
        
        // Refresh search results if needed
        if (!m_searchBox->text().isEmpty()) {
            searchPackages(m_searchBox->text());
        }
    });
    
    // Add theme toggle
    QAction* themeAction = settingsMenu->addAction("&Dark Theme");
    themeAction->setCheckable(true);
    themeAction->setChecked(m_darkTheme);
    connect(themeAction, &QAction::toggled, this, [this](bool checked) {
        m_darkTheme = checked;
        applyTheme(m_darkTheme);
        
        // Save the setting
        QSettings settings("PacmanGUI", "PacmanGUI");
        settings.setValue("appearance/darkTheme", m_darkTheme);
    });
    
    settingsMenu->addSeparator();
    settingsMenu->addAction(m_actions["settings"]);
    
    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(m_actions["about"]);
}

void MainWindow::setupConnections()
{
    // Connect search box and button
    connect(m_searchBox, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked);
    connect(m_searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    
    // Connect settings button
    connect(m_settingsButton, &QToolButton::clicked, this, &MainWindow::openSettings);
    
    // Connect tab changed signal
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    
    // Connect system update buttons
    connect(m_systemUpdateButton, &QPushButton::clicked, this, &MainWindow::onSystemUpdate);
    connect(m_checkUpdatesButton, &QPushButton::clicked, this, &MainWindow::onCheckForUpdates);
    
    // Connect batch install button
    connect(m_batchInstallButton, &QPushButton::clicked, this, &MainWindow::onBatchInstall);
    
    // Handle row clicks for displaying package details
    connect(m_packagesView, &QTableView::clicked, [this](const QModelIndex &index) {
        // Show details for the clicked package
        int row = index.row();
        QString packageName = m_packagesModel->data(m_packagesModel->index(row, 1)).toString();
        QString version = m_packagesModel->data(m_packagesModel->index(row, 2)).toString();
        QString repository = m_packagesModel->data(m_packagesModel->index(row, 3)).toString();
        QString description = m_packagesModel->data(m_packagesModel->index(row, 4)).toString();
        
        m_packageNameLabel->setText(packageName);
        m_packageVersionLabel->setText("Version: " + version + " (" + repository + ")");
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
    });
    
    // Also handle row clicks for installed packages view
    connect(m_installedView, &QTableView::clicked, [this](const QModelIndex &index) {
        // Show details for the clicked package
        int row = index.row();
        QString packageName = m_installedModel->data(m_installedModel->index(row, 1)).toString();
        QString version = m_installedModel->data(m_installedModel->index(row, 2)).toString();
        QString repository = m_installedModel->data(m_installedModel->index(row, 3)).toString();
        QString description = m_installedModel->data(m_installedModel->index(row, 4)).toString();
        
        m_packageNameLabel->setText(packageName);
        m_packageVersionLabel->setText("Version: " + version + " (" + repository + ")");
        m_packageDescLabel->setText(description);
        
        // For installed packages, always show Remove button
        m_actionButton->setText("Remove");
        connect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onRemovePackage, Qt::UniqueConnection);
        
        m_detailsWidget->setVisible(true);
    });
    
    // Connect system update table double click
    connect(m_systemUpdatesView, &QTableView::doubleClicked, [this](const QModelIndex& index) {
        // Get package name from the first column
        QString packageName = m_systemUpdatesModel->data(m_systemUpdatesModel->index(index.row(), 0)).toString();
        QString newVersion = m_systemUpdatesModel->data(m_systemUpdatesModel->index(index.row(), 1)).toString();
        
        // Confirm update
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Update Package",
                                     QString("Do you want to update package '%1' to version %2?")
                                     .arg(packageName)
                                     .arg(newVersion),
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
            
            // Get overwrite checkbox state
            bool useOverwrite = m_systemUpdateOverwriteCheckbox->isChecked();
            
            // Update status
            statusBar()->showMessage(QString("Updating package: %1...%2")
                                   .arg(packageName)
                                   .arg(useOverwrite ? " (with --overwrite)" : ""));
            
            m_systemUpdateLogView->append(QString("<p>Updating package <b>%1</b> to version <b>%2</b>...%3</p>")
                                        .arg(packageName)
                                        .arg(newVersion)
                                        .arg(useOverwrite ? " (using --overwrite=\"*\")" : ""));
            
            // Set wait cursor
            setCursor(Qt::WaitCursor);
            
            // Update the package
            bool success = m_packageManager.update_package(
                packageName.toStdString(), 
                password.toStdString(),
                useOverwrite
            );
            
            // Restore cursor
            setCursor(Qt::ArrowCursor);
            
            if (success) {
                m_systemUpdateLogView->append(QString("<p style='color:green'>Package <b>%1</b> updated successfully.</p>")
                                            .arg(packageName));
                
                // Remove the updated package from the list
                m_systemUpdatesModel->removeRow(index.row());
                
                // Refresh installed packages view
                refreshInstalledPackages();
                
                statusBar()->showMessage(QString("Package '%1' updated successfully.").arg(packageName));
            } else {
                m_systemUpdateLogView->append(QString("<p style='color:red'>Failed to update package <b>%1</b>: %2</p>")
                                            .arg(packageName)
                                            .arg(QString::fromStdString(m_packageManager.get_last_error())));
                
                statusBar()->showMessage(QString("Failed to update package '%1'.").arg(packageName));
            }
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
        QString tableStyle = "QTableView { border: 1px solid #3c3c3c; selection-background-color: #2a82da; }"
                           "QTableView::item { padding-left: 0; padding-right: 0; }"
                           "QTableView::item:first { qproperty-alignment: AlignCenter; }";
        
        m_packagesView->setStyleSheet(tableStyle);
        m_installedView->setStyleSheet(tableStyle);
        m_systemUpdatesView->setStyleSheet(tableStyle);
        m_searchBox->setStyleSheet("QLineEdit { border: 1px solid #3c3c3c; border-radius: 3px; padding: 2px; }");
        m_actionButton->setStyleSheet("QPushButton { background-color: #2a82da; color: white; border-radius: 3px; padding: 6px; }");
        m_systemUpdateButton->setStyleSheet("QPushButton { background-color: #2a82da; color: white; border-radius: 3px; padding: 8px; }");
        m_checkUpdatesButton->setStyleSheet("QPushButton { background-color: #2a82da; color: white; border-radius: 3px; padding: 8px; }");
        m_batchInstallButton->setStyleSheet("QPushButton { background-color: #2a82da; color: white; border-radius: 3px; padding: 8px; }");
        
        // Set checkbox styles
        QString checkboxStyle = "QCheckBox { color: white; }"
                              "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #5f5f5f; background-color: #2d2d2d; border-radius: 3px; }"
                              "QCheckBox::indicator:checked { background-color: #2a82da; }";
        
        m_systemUpdateOverwriteCheckbox->setStyleSheet(checkboxStyle);
        m_packageOverwriteCheckbox->setStyleSheet(checkboxStyle);
    } else {
        // Light theme colors - use system default with some customizations
        palette = QApplication::style()->standardPalette();
        
        // Set light style sheets for specific widgets
        QString tableStyle = "QTableView { border: 1px solid #d0d0d0; selection-background-color: #308cc6; }"
                           "QTableView::item { padding-left: 0; padding-right: 0; }"
                           "QTableView::item:first { qproperty-alignment: AlignCenter; }";
        
        m_packagesView->setStyleSheet(tableStyle);
        m_installedView->setStyleSheet(tableStyle);
        m_systemUpdatesView->setStyleSheet(tableStyle);
        m_searchBox->setStyleSheet("QLineEdit { border: 1px solid #d0d0d0; border-radius: 3px; padding: 2px; }");
        m_actionButton->setStyleSheet("QPushButton { background-color: #308cc6; color: white; border-radius: 3px; padding: 6px; }");
        m_systemUpdateButton->setStyleSheet("QPushButton { background-color: #308cc6; color: white; border-radius: 3px; padding: 8px; }");
        m_checkUpdatesButton->setStyleSheet("QPushButton { background-color: #308cc6; color: white; border-radius: 3px; padding: 8px; }");
        m_batchInstallButton->setStyleSheet("QPushButton { background-color: #308cc6; color: white; border-radius: 3px; padding: 8px; }");
        
        // Set checkbox styles
        QString checkboxStyle = "QCheckBox { color: black; }"
                              "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #bbbbbb; background-color: white; border-radius: 3px; }"
                              "QCheckBox::indicator:checked { background-color: #308cc6; }";
        
        m_systemUpdateOverwriteCheckbox->setStyleSheet(checkboxStyle);
        m_packageOverwriteCheckbox->setStyleSheet(checkboxStyle);
    }
    
    // Always use dark style for log view regardless of theme
    m_systemUpdateLogView->setStyleSheet("QTextEdit { border: 1px solid #3c3c3c; background-color: #1e1e1e; color: #e0e0e0; }");
    
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
            m_detailsWidget->setVisible(false);
            break;
        case 1: // Installed
            statusBar()->showMessage("Viewing installed packages");
            refreshInstalledPackages();
            m_detailsWidget->setVisible(false);
            break;
        case 2: // System Update
            statusBar()->showMessage("Viewing system update");
            // Display welcome message in log if it's empty
            if (m_systemUpdateLogView->toPlainText().isEmpty()) {
                m_systemUpdateLogView->clear();
                m_systemUpdateLogView->append("<b>System Update</b>");
                m_systemUpdateLogView->append("<p>Welcome to the System Update tab. You can check for available updates "
                                           "and install them individually or all at once.</p>");
                m_systemUpdateLogView->append("<p>Click 'Check for Updates' to see what packages need updating.</p>");
            }
            m_detailsWidget->setVisible(false);
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
    
    // Disconnect any existing connections to prevent signal loops
    disconnect(m_packagesModel, &QStandardItemModel::itemChanged, nullptr, nullptr);
    
    // Clear previous search results and selections
    m_packagesModel->removeRows(0, m_packagesModel->rowCount());
    m_selectedPackages.clear();
    updateBatchInstallButton();
    
    // Search for packages in repos
    std::vector<core::Package> results = m_packageManager.search_by_name(searchTerm.toStdString());
    
    // Check if AUR is enabled in settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    bool aurEnabled = settings.value("aur/enabled", false).toBool();
    
    // Also search AUR if enabled
    if (aurEnabled) {
        statusBar()->showMessage("Searching repositories and AUR for: " + searchTerm);
        std::vector<core::Package> aurResults = m_packageManager.search_aur(searchTerm.toStdString());
        
        // Add AUR results to the combined results
        results.insert(results.end(), aurResults.begin(), aurResults.end());
    }
    
    // Update header to include repository column
    m_packagesModel->setHorizontalHeaderLabels(QStringList() << "" << "Name" << "Version" << "Repository" << "Description");
    
    // Display results
    for (const auto& pkg : results) {
        QString packageName = QString::fromStdString(pkg.get_name());
        QString repoName = QString::fromStdString(pkg.get_repository());
        QString repoDisplay;
        
        // Categorize repository source
        if (repoName.contains("cachyos", Qt::CaseInsensitive)) {
            repoDisplay = "CachyOS";
        } else if (repoName == "core" || repoName == "extra" || repoName == "multilib") {
            repoDisplay = "Arch";
        } else if (repoName == "chaotic-aur") {
            repoDisplay = "Chaotic AUR";
        } else if (repoName == "aur") {
            repoDisplay = "AUR";
        } else if (repoName.isEmpty() && !pkg.get_aur_info().empty()) {
            repoDisplay = "AUR";
        } else if (repoName.isEmpty()) {
            repoDisplay = "Unknown";
        } else {
            repoDisplay = repoName;
        }
        
        QList<QStandardItem*> row;
        
        // Create checkbox item
        QStandardItem* checkItem = new QStandardItem();
        checkItem->setCheckable(true);
        checkItem->setCheckState(Qt::Unchecked);
        checkItem->setTextAlignment(Qt::AlignCenter); // Center the checkmark
        
        row << checkItem;
        row << new QStandardItem(packageName);
        row << new QStandardItem(QString::fromStdString(pkg.get_version()));
        row << new QStandardItem(repoDisplay);
        row << new QStandardItem(QString::fromStdString(pkg.get_description()));
        
        m_packagesModel->appendRow(row);
    }
    
    // Connect checkbox changes to selection tracking
    connect(m_packagesModel, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        // Only process checkbox column items
        if (item->column() != 0) return;
        
        // Get the package name from the adjacent cell
        QModelIndex nameIndex = m_packagesModel->index(item->row(), 1);
        QString packageName = m_packagesModel->data(nameIndex).toString();
        
        // Update selected packages set based on check state
        if (item->checkState() == Qt::Checked) {
            m_selectedPackages.insert(packageName);
        } else {
            m_selectedPackages.remove(packageName);
        }
        
        // Update batch install button
        updateBatchInstallButton();
    });
    
    // Switch to the All Packages tab
    m_tabWidget->setCurrentIndex(0);
    
    // Update status bar
    statusBar()->showMessage("Found " + QString::number(results.size()) + " packages matching: " + searchTerm);
}

void MainWindow::refreshInstalledPackages()
{
    statusBar()->showMessage("Refreshing installed packages...");
    
    // Disconnect any existing connections to prevent signal loops
    disconnect(m_installedModel, &QStandardItemModel::itemChanged, nullptr, nullptr);
    
    // Clear previous list
    m_installedModel->removeRows(0, m_installedModel->rowCount());
    
    // Update header to include repository column
    m_installedModel->setHorizontalHeaderLabels(QStringList() << "" << "Name" << "Version" << "Repository" << "Description");
    
    // Get installed packages
    std::vector<core::Package> installed = m_packageManager.get_installed_packages();
    
    // Display packages
    for (const auto& pkg : installed) {
        QString packageName = QString::fromStdString(pkg.get_name());
        QString repoName = QString::fromStdString(pkg.get_repository());
        QString repoDisplay;
        
        // Categorize repository source
        if (repoName.contains("cachyos", Qt::CaseInsensitive)) {
            repoDisplay = "CachyOS";
        } else if (repoName == "core" || repoName == "extra" || repoName == "multilib") {
            repoDisplay = "Arch";
        } else if (repoName == "chaotic-aur") {
            repoDisplay = "Chaotic AUR";
        } else if (repoName == "aur") {
            repoDisplay = "AUR";
        } else if (repoName.isEmpty() && !pkg.get_aur_info().empty()) {
            repoDisplay = "AUR";
        } else if (repoName.isEmpty()) {
            repoDisplay = "Unknown";
        } else {
            repoDisplay = repoName;
        }
        
        QList<QStandardItem*> row;
        
        // Create checkbox item
        QStandardItem* checkItem = new QStandardItem();
        checkItem->setCheckable(true);
        checkItem->setCheckState(Qt::Unchecked);
        checkItem->setTextAlignment(Qt::AlignCenter); // Center the checkmark
        
        row << checkItem;
        row << new QStandardItem(packageName);
        row << new QStandardItem(QString::fromStdString(pkg.get_version()));
        row << new QStandardItem(repoDisplay);
        row << new QStandardItem(QString::fromStdString(pkg.get_description()));
        
        m_installedModel->appendRow(row);
    }
    
    // Connect checkbox changes to selection tracking
    connect(m_installedModel, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        // Only process checkbox column items
        if (item->column() != 0) return;
        
        // Get the package name from the adjacent cell
        QModelIndex nameIndex = m_installedModel->index(item->row(), 1);
        QString packageName = m_installedModel->data(nameIndex).toString();
        
        // Update selected packages set based on check state
        if (item->checkState() == Qt::Checked) {
            m_selectedPackages.insert(packageName);
        } else {
            m_selectedPackages.remove(packageName);
        }
        
        // Update batch install button
        updateBatchInstallButton();
    });
    
    // Update status bar
    statusBar()->showMessage("Found " + QString::number(installed.size()) + " installed packages");
}

void MainWindow::refreshUpdatesList()
{
    statusBar()->showMessage("Checking for updates...");
    
    // Get the list of available updates
    std::vector<std::pair<std::string, std::string>> updates = m_packageManager.check_updates();
    
    // Update the tab text to show number of updates
    if (!updates.empty()) {
        m_tabWidget->setTabText(2, QString("System Update (%1)").arg(updates.size()));
        statusBar()->showMessage(QString("Found %1 packages with updates available").arg(updates.size()));
    } else {
        m_tabWidget->setTabText(2, "System Update");
        statusBar()->showMessage("No updates available. Your system is up to date.");
    }
}

void MainWindow::onInstallPackage()
{
    // Get package name from the details view
    QString packageName = m_packageNameLabel->text();
    
    if (packageName.isEmpty()) {
        QMessageBox::warning(this, "No Package Selected", "Please select a package to install.");
        return;
    }
    
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
        
        // Get overwrite checkbox state
        bool useOverwrite = m_packageOverwriteCheckbox->isChecked();
        
        setCursor(Qt::WaitCursor);
        statusBar()->showMessage(QString("Installing package: %1%2")
                                .arg(packageName)
                                .arg(useOverwrite ? " (with --overwrite)" : ""));
        
        bool success = m_packageManager.install_package(
            packageName.toStdString(), 
            password.toStdString(),
            useOverwrite
        );
        
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
    // Get package name from the details view
    QString packageName = m_packageNameLabel->text();
    
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
    // Get package name from the details view
    QString packageName = m_packageNameLabel->text();
    
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
        
        // Get overwrite checkbox state
        bool useOverwrite = m_packageOverwriteCheckbox->isChecked();
        
        setCursor(Qt::WaitCursor);
        statusBar()->showMessage(QString("Updating package: %1%2")
                                .arg(packageName)
                                .arg(useOverwrite ? " (with --overwrite)" : ""));
        
        bool success = m_packageManager.update_package(
            packageName.toStdString(), 
            password.toStdString(),
            useOverwrite
        );
        
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
                                               "Enter your password to refresh package databases:",
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

void MainWindow::onSystemUpdate()
{
    statusBar()->showMessage("Preparing system update...");
    
    // Clear previous log
    m_systemUpdateLogView->clear();
    m_systemUpdateLogView->append("<b>System Update Log:</b>");
    m_systemUpdateLogView->append("<p>Preparing to update system...</p>");
    
    // Confirm update
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "System Update",
                                 "Do you want to perform a full system update?\n"
                                 "This will update all packages on your system.",
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Add password dialog
        bool ok;
        QString password = QInputDialog::getText(this, "Authentication Required",
                                               "Enter your password to perform system update:",
                                               QLineEdit::Password, "", &ok);
        
        if (!ok || password.isEmpty()) {
            statusBar()->showMessage("System update cancelled");
            m_systemUpdateLogView->append("<p style='color:orange'>Update cancelled by user</p>");
            return;
        }
        
        // Get overwrite checkbox state
        bool useOverwrite = m_systemUpdateOverwriteCheckbox->isChecked();
        
        if (useOverwrite) {
            m_systemUpdateLogView->append("<p style='color:orange;'><b>Warning:</b> Using --overwrite=\"*\" option. "
                                        "This may overwrite conflicting files and can potentially break your system.</p>");
        }
        
        m_systemUpdateLogView->append("<p>Starting full system update (pacman -Syu)...</p>");
        m_systemUpdateButton->setEnabled(false);
        m_checkUpdatesButton->setEnabled(false);
        setCursor(Qt::WaitCursor);
        statusBar()->showMessage(QString("Updating system...%1")
                               .arg(useOverwrite ? " (with --overwrite)" : ""));
        
        // Define a callback function to handle real-time output
        auto outputCallback = [this](const std::string& output) {
            // Format the output with proper HTML
            QString qOutput = QString::fromStdString(output);
            
            // Strip line endings for proper HTML formatting
            qOutput = qOutput.trimmed();
            
            // Skip empty lines
            if (!qOutput.isEmpty()) {
                // Colorize important parts of the output
                if (qOutput.contains("error", Qt::CaseInsensitive)) {
                    qOutput = "<span style='color:red'>" + qOutput + "</span>";
                } else if (qOutput.contains("warning", Qt::CaseInsensitive)) {
                    qOutput = "<span style='color:orange'>" + qOutput + "</span>";
                } else if (qOutput.contains("installing", Qt::CaseInsensitive) || 
                          qOutput.contains("upgrading", Qt::CaseInsensitive)) {
                    qOutput = "<span style='color:green'>" + qOutput + "</span>";
                } else if (qOutput.contains("downloading", Qt::CaseInsensitive)) {
                    qOutput = "<span style='color:blue'>" + qOutput + "</span>";
                }
                
                // Append to log view with proper HTML formatting
                m_systemUpdateLogView->append("<pre>" + qOutput + "</pre>");
                
                // Scroll to bottom to show the latest output
                m_systemUpdateLogView->verticalScrollBar()->setValue(
                    m_systemUpdateLogView->verticalScrollBar()->maximum());
                
                // Process events to update the UI
                QApplication::processEvents();
            }
        };
        
        // Execute the system update with real-time output callback
        bool success = m_packageManager.update_system(password.toStdString(), outputCallback, useOverwrite);
        
        setCursor(Qt::ArrowCursor);
        m_systemUpdateButton->setEnabled(true);
        m_checkUpdatesButton->setEnabled(true);
        
        if (success) {
            m_systemUpdateLogView->append("<p style='color:green'><b>System update completed successfully!</b></p>");
            QMessageBox::information(this, "Success", "System updated successfully.");
            statusBar()->showMessage("System updated successfully.");
            
            // Clear the updates table since everything is now up to date
            m_systemUpdatesModel->removeRows(0, m_systemUpdatesModel->rowCount());
            
            // Refresh all package lists
            refreshInstalledPackages();
            refreshUpdatesList();
        } else {
            m_systemUpdateLogView->append("<p style='color:red'><b>System update failed.</b></p>");
            m_systemUpdateLogView->append("<p style='color:red'>" + 
                                        QString::fromStdString(m_packageManager.get_last_error()) + 
                                        "</p>");
            
            QMessageBox::critical(this, "Update Failed", 
                                "Failed to update system.\n" + 
                                QString::fromStdString(m_packageManager.get_last_error()));
            statusBar()->showMessage("Update failed: " + QString::fromStdString(m_packageManager.get_last_error()));
        }
    } else {
        statusBar()->showMessage("System update cancelled");
        m_systemUpdateLogView->append("<p style='color:orange'>Update cancelled by user</p>");
    }
}

void MainWindow::onCheckForUpdates()
{
    statusBar()->showMessage("Checking for system updates...");
    
    // Clear the updates model
    m_systemUpdatesModel->removeRows(0, m_systemUpdatesModel->rowCount());
    
    // Check for available updates
    std::vector<std::pair<std::string, std::string>> updates = m_packageManager.check_updates();
    
    if (updates.empty()) {
        statusBar()->showMessage("Your system is up to date.");
        m_systemUpdateLogView->append("<p>No updates available. Your system is up to date.</p>");
        
        QMessageBox::information(this, "System Updates", 
                               "Your system is up to date. No packages need updating.");
        return;
    }
    
    // Display updates in the table
    for (const auto& update : updates) {
        QList<QStandardItem*> row;
        row << new QStandardItem(QString::fromStdString(update.first));
        row << new QStandardItem(QString::fromStdString(update.second));
        
        m_systemUpdatesModel->appendRow(row);
    }
    
    // Auto-resize columns to fit content
    m_systemUpdatesView->resizeColumnsToContents();
    
    statusBar()->showMessage(QString("Found %1 available updates.").arg(updates.size()));
    m_systemUpdateLogView->append(QString("<p>Found <b>%1</b> available updates. "
                                    "You can update them all at once or individually.</p>").arg(updates.size()));
    
    // Show message box
    QMessageBox::information(this, "System Updates", 
                           QString("Found %1 package updates available.\n"
                                  "You can update them all at once using the 'Update System' button, "
                                  "or update individual packages by double-clicking them in the list.").arg(updates.size()));
}

void MainWindow::onBatchInstall()
{
    if (m_selectedPackages.isEmpty()) {
        QMessageBox::information(this, "No Packages Selected", "Please select at least one package to install.");
        return;
    }
    
    // Convert set to list for display
    QStringList packagesList = m_selectedPackages.values();
    packagesList.sort();
    
    // Confirm installation
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Batch Install Packages",
                                 QString("Do you want to install the following %1 packages?\n\n%2")
                                     .arg(packagesList.size())
                                     .arg(packagesList.join("\n")),
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Ask for password
        bool ok;
        QString password = QInputDialog::getText(this, "Authentication Required",
                                               "Enter your password to install packages:",
                                               QLineEdit::Password, "", &ok);
        
        if (!ok || password.isEmpty()) {
            statusBar()->showMessage("Batch installation cancelled");
            return;
        }
        
        // Get overwrite checkbox state
        bool useOverwrite = m_packageOverwriteCheckbox->isChecked();
        
        // Setup progress tracking
        int total = packagesList.size();
        int successful = 0;
        int failed = 0;
        QStringList failedPackages;
        
        // Create a progress dialog
        QProgressDialog progress("Installing packages...", "Cancel", 0, total, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);
        
        setCursor(Qt::WaitCursor);
        
        // Install each package
        for (int i = 0; i < packagesList.size(); ++i) {
            QString packageName = packagesList[i];
            
            // Update progress
            progress.setValue(i);
            progress.setLabelText(QString("Installing package %1 of %2:\n%3")
                                .arg(i+1)
                                .arg(total)
                                .arg(packageName));
            
            // Process events to keep UI responsive
            QApplication::processEvents();
            
            // Check if cancelled
            if (progress.wasCanceled()) {
                break;
            }
            
            // Install the package
            statusBar()->showMessage(QString("Installing package: %1%2")
                                   .arg(packageName)
                                   .arg(useOverwrite ? " (with --overwrite)" : ""));
            
            bool success = m_packageManager.install_package(
                packageName.toStdString(), 
                password.toStdString(),
                useOverwrite
            );
            
            if (success) {
                successful++;
            } else {
                failed++;
                failedPackages.append(packageName);
            }
        }
        
        progress.setValue(total);
        
        setCursor(Qt::ArrowCursor);
        
        // Display results
        if (failed == 0) {
            QMessageBox::information(this, "Success", 
                                   QString("All %1 packages were installed successfully.").arg(successful));
            statusBar()->showMessage(QString("%1 packages installed successfully.").arg(successful));
        } else {
            QMessageBox::warning(this, "Installation Results", 
                               QString("%1 packages installed successfully, %2 failed.\n\nFailed packages:\n%3")
                                 .arg(successful)
                                 .arg(failed)
                                 .arg(failedPackages.join("\n")));
            statusBar()->showMessage(QString("%1 packages installed, %2 failed.").arg(successful).arg(failed));
        }
        
        // Clear selections
        m_packagesView->clearSelection();
        m_selectedPackages.clear();
        updateBatchInstallButton();
        
        // Refresh the installed packages list
        refreshInstalledPackages();
    }
}

void MainWindow::updateBatchInstallButton()
{
    // Enable batch install button only if there are selected packages
    m_batchInstallButton->setEnabled(!m_selectedPackages.isEmpty());
    
    // Update the button text to show how many packages are selected
    if (m_selectedPackages.isEmpty()) {
        m_batchInstallButton->setText("Install Selected Packages");
    } else {
        m_batchInstallButton->setText(QString("Install Selected Packages (%1)").arg(m_selectedPackages.size()));
    }
}

void MainWindow::openSettings()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
    }
    
    // Store current AUR settings
    bool previousAurEnabled = m_settingsDialog->isAurEnabled();
    
    // Show the settings dialog
    if (m_settingsDialog->exec() == QDialog::Accepted) {
        // Apply theme immediately if changed
        bool darkTheme = m_settingsDialog->isDarkThemeEnabled();
        if (m_darkTheme != darkTheme) {
            m_darkTheme = darkTheme;
            applyTheme(m_darkTheme);
        }
        
        // Check if AUR settings changed
        bool currentAurEnabled = m_settingsDialog->isAurEnabled();
        if (previousAurEnabled != currentAurEnabled) {
            // Refresh package listings if AUR was enabled/disabled
            statusBar()->showMessage("AUR settings changed, refreshing package lists...");
            
            // Update AUR actions in toolbar and menu
            for (QAction* action : findChildren<QAction*>()) {
                if (action->statusTip().contains("AUR")) {
                    action->setEnabled(currentAurEnabled);
                }
            }
            
            // Refresh package lists to include/exclude AUR packages
            if (!m_searchBox->text().isEmpty()) {
                searchPackages(m_searchBox->text());
            }
            
            QString aurStatus = currentAurEnabled ? "enabled" : "disabled";
            statusBar()->showMessage(QString("AUR support %1").arg(aurStatus), 3000);
        }
    }
}

void MainWindow::onInstallAurPackage()
{
    // Get the selected package from the packages view
    QModelIndex index = m_packagesView->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "No Package Selected", "Please select a package to install.");
        return;
    }
    
    // Get the package name from the model
    QString packageName = m_packagesModel->data(m_packagesModel->index(index.row(), 0)).toString();
    QString repoName = m_packagesModel->data(m_packagesModel->index(index.row(), 1)).toString();
    
    // Only proceed if this is an AUR package
    if (repoName != "AUR") {
        QMessageBox::information(this, "Not an AUR Package", 
                                "The selected package is not from AUR. Use the regular install option.");
        return;
    }
    
    // Confirm installation
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Install AUR Package",
                                 "Do you want to install the AUR package '" + packageName + "'?",
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Get AUR helper from settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    QString aurHelper = settings.value("aur/helper", "yay").toString();
    
    // Add authentication dialog
    bool ok;
    QString password = QInputDialog::getText(this, "Authentication Required",
                                           "Enter your password to install AUR package:",
                                           QLineEdit::Password, "", &ok);
    
    if (!ok || password.isEmpty()) {
        statusBar()->showMessage("AUR Package installation cancelled");
        return;
    }
    
    // Install the package
    statusBar()->showMessage("Installing AUR package: " + packageName);
    setCursor(Qt::WaitCursor);
    
    // Use the AUR installation method
    bool success = m_packageManager.install_aur_package(
        packageName.toStdString(), 
        password.toStdString(),
        aurHelper.toStdString()
    );
    
    setCursor(Qt::ArrowCursor);
    
    if (success) {
        QMessageBox::information(this, "Installation Successful", 
                                "AUR Package '" + packageName + "' was installed successfully.");
        statusBar()->showMessage("AUR Package installed: " + packageName);
        
        // Refresh the installed packages list
        refreshInstalledPackages();
    } else {
        QMessageBox::critical(this, "Installation Failed", 
                            "Failed to install AUR package '" + packageName + "'.\n" + 
                            QString::fromStdString(m_packageManager.get_last_error()));
        statusBar()->showMessage("Installation failed: " + QString::fromStdString(m_packageManager.get_last_error()));
    }
}

void MainWindow::onUpdateAurPackages()
{
    // Check if AUR is enabled in settings
    QSettings settings("PacmanGUI", "PacmanGUI");
    bool aurEnabled = settings.value("aur/enabled", false).toBool();
    
    if (!aurEnabled) {
        QMessageBox::information(this, "AUR Support Disabled", 
                               "AUR support is disabled in settings. Enable it in Settings > AUR tab.");
        return;
    }
    
    QString aurHelper = settings.value("aur/helper", "yay").toString();
    if (aurHelper.isEmpty()) {
        QMessageBox::warning(this, "No AUR Helper", 
                            "No AUR helper is configured. Please select an AUR helper in Settings.");
        return;
    }
    
    // First check for updates
    statusBar()->showMessage("Checking for AUR updates...");
    setCursor(Qt::WaitCursor);
    
    std::vector<std::pair<std::string, std::string>> aurUpdates = 
        m_packageManager.check_aur_updates(aurHelper.toStdString());
    
    setCursor(Qt::ArrowCursor);
    
    if (aurUpdates.empty()) {
        QMessageBox::information(this, "No AUR Updates", 
                               "No AUR package updates available.");
        statusBar()->showMessage("No AUR updates available");
        return;
    }
    
    // Confirm update
    QMessageBox::StandardButton reply;
    QString message = QString("Found %1 AUR package updates. Do you want to update them?").arg(aurUpdates.size());
    reply = QMessageBox::question(this, "AUR Updates Available", message,
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        statusBar()->showMessage("AUR update cancelled");
        return;
    }
    
    // Add authentication dialog
    bool ok;
    QString password = QInputDialog::getText(this, "Authentication Required",
                                           "Enter your password to update AUR packages:",
                                           QLineEdit::Password, "", &ok);
    
    if (!ok || password.isEmpty()) {
        statusBar()->showMessage("AUR update cancelled");
        return;
    }
    
    // Update the packages
    statusBar()->showMessage("Updating AUR packages...");
    setCursor(Qt::WaitCursor);
    
    // Prepare output area
    m_systemUpdateLogView->append("<p><b>Starting AUR Update:</b></p>");
    
    // Update with real-time output callback
    auto outputCallback = [this](const std::string& output) {
        QString qOutput = QString::fromStdString(output);
        
        // Strip line endings for proper HTML formatting
        qOutput = qOutput.trimmed();
        
        // Skip empty lines
        if (!qOutput.isEmpty()) {
            // Colorize important parts of the output
            if (qOutput.contains("error", Qt::CaseInsensitive)) {
                qOutput = "<span style='color:red'>" + qOutput + "</span>";
            } else if (qOutput.contains("warning", Qt::CaseInsensitive)) {
                qOutput = "<span style='color:orange'>" + qOutput + "</span>";
            } else if (qOutput.contains("installing", Qt::CaseInsensitive) || 
                      qOutput.contains("upgrading", Qt::CaseInsensitive)) {
                qOutput = "<span style='color:green'>" + qOutput + "</span>";
            } else if (qOutput.contains("downloading", Qt::CaseInsensitive)) {
                qOutput = "<span style='color:blue'>" + qOutput + "</span>";
            }
            
            // Append to log view with proper HTML formatting
            m_systemUpdateLogView->append("<pre>" + qOutput + "</pre>");
            
            // Scroll to bottom to show the latest output
            m_systemUpdateLogView->verticalScrollBar()->setValue(
                m_systemUpdateLogView->verticalScrollBar()->maximum());
            
            // Process events to update the UI
            QApplication::processEvents();
        }
    };
    
    bool success = m_packageManager.update_aur_packages(
        password.toStdString(), 
        aurHelper.toStdString(),
        outputCallback
    );
    
    setCursor(Qt::ArrowCursor);
    
    if (success) {
        QMessageBox::information(this, "AUR Update Successful", 
                               "AUR packages were updated successfully.");
        statusBar()->showMessage("AUR packages updated successfully");
        
        // Refresh the installed packages list
        refreshInstalledPackages();
        
        m_systemUpdateLogView->append("<p style='color:green'><b>AUR update completed successfully!</b></p>");
    } else {
        QMessageBox::critical(this, "AUR Update Failed", 
                            "Failed to update AUR packages.\n" + 
                            QString::fromStdString(m_packageManager.get_last_error()));
        statusBar()->showMessage("AUR update failed: " + QString::fromStdString(m_packageManager.get_last_error()));
        
        m_systemUpdateLogView->append("<p style='color:red'><b>AUR update failed.</b></p>");
        m_systemUpdateLogView->append("<p style='color:red'>" + 
                                    QString::fromStdString(m_packageManager.get_last_error()) + 
                                    "</p>");
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About PacmanGUI",
                      "PacmanGUI - A graphical interface for Pacman\n\n"
                      "Version: 1.0.0\n"
                      "License: GPL-3.0\n\n"
                      "A modern GUI frontend for the Arch Linux package manager.\n"
                      "Features include package search, installation, removal, updates,\n"
                      "AUR support, and more.");
}

} // namespace gui
} // namespace pacmangui 