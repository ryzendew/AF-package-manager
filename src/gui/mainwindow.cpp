#include "gui/mainwindow.hpp"
#include "gui/settingsdialog.hpp"
#include <QApplication>
#include <QGuiApplication>
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
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QMetaObject>
#include <QFile>
#include <QSizePolicy>
#include <QSplitter>
#include <QThreadPool>
#include <QCompleter>
#include <QDir>
#include <QFileInfo>
#include <QFontDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRadioButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <iostream>

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
    , m_detailPanel(nullptr)
    , m_slideAnimation(nullptr)
    , m_opacityEffect(nullptr)
    , m_detailPanelVisible(false)
{
    setWindowTitle("PacmanGUI - Package Manager");
    setMinimumSize(1000, 600);
    
    // Set attribute for responsive resizing
    setAttribute(Qt::WA_DeleteOnClose);
    
    // Setup for responsive window
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCentralWidget(centralWidget);
    
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
    
    // Connect resize events to handle responsiveness
    connect(this, &QMainWindow::windowTitleChanged, this, [this](const QString&) {
        // This is a workaround signal that is triggered more often than we need
        // but will allow us to check window state regularly
        if (isMaximized()) {
            // When maximized, adjust UI for maximum content display
            if (m_packagesView) {
                m_packagesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            }
            if (m_installedView) {
                m_installedView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            }
            if (m_systemUpdatesView) {
                m_systemUpdatesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            }
        } else {
            // When restored, adjust UI for balanced display
            if (m_packagesView) {
                m_packagesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
                m_packagesView->horizontalHeader()->setStretchLastSection(true);
            }
            if (m_installedView) {
                m_installedView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
                m_installedView->horizontalHeader()->setStretchLastSection(true);
            }
            if (m_systemUpdatesView) {
                m_systemUpdatesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
                m_systemUpdatesView->horizontalHeader()->setStretchLastSection(true);
            }
        }
    });
    
    // Setup the detail panel
    setupDetailPanel();
}

MainWindow::~MainWindow()
{
    // Clean up allocated resources
    if (m_slideAnimation) {
        m_slideAnimation->stop();
    }
    
    delete m_packagesModel;
    delete m_installedModel;
    delete m_systemUpdatesModel;
    delete m_settingsDialog;
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
    // Get the central widget that was already set in the constructor
    QWidget* centralWidget = this->centralWidget();
    
    // Create main layout with proper spacing that expands with the window
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Create search bar section that adapts to window width
    QWidget* searchWidget = new QWidget();
    searchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    QHBoxLayout* searchLayout = new QHBoxLayout(searchWidget);
    searchLayout->setContentsMargins(5, 0, 5, 0);
    searchLayout->setSpacing(10);
    
    QLabel* searchLabel = new QLabel("Search:", searchWidget);
    searchLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    m_searchBox = new QLineEdit(searchWidget);
    m_searchBox->setPlaceholderText("Enter package name");
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_searchBox->setMinimumWidth(200);
    
    m_searchButton = new QPushButton("Search", searchWidget);
    m_searchButton->setObjectName("primary");
    m_searchButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    m_settingsButton = new QToolButton();
    m_settingsButton->setIcon(QIcon::fromTheme("preferences-system"));
    m_settingsButton->setToolTip("Settings");
    m_settingsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_settingsButton->setIconSize(QSize(24, 24));
    
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchBox, 1);
    searchLayout->addWidget(m_searchButton);
    searchLayout->addWidget(m_settingsButton);
    
    // Add search widget to main layout
    mainLayout->addWidget(searchWidget);
    
    // Create tab widget that takes all available space
    m_tabWidget = new QTabWidget();
    m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Create "All Packages" tab with a responsive layout
    QWidget* packagesTab = new QWidget();
    packagesTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QVBoxLayout* packagesLayout = new QVBoxLayout(packagesTab);
    packagesLayout->setContentsMargins(10, 10, 10, 10);
    packagesLayout->setSpacing(10);
    
    // Create packages table view with responsive sizing
    m_packagesView = new QTableView();
    m_packagesView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_packagesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_packagesView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_packagesView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_packagesView->setAlternatingRowColors(true);
    m_packagesView->verticalHeader()->setVisible(false);
    m_packagesView->verticalHeader()->setDefaultSectionSize(28);
    m_packagesView->horizontalHeader()->setStretchLastSection(true);
    m_packagesView->setSortingEnabled(true);
    
    // Set the model for packages view
    m_packagesModel = new QStandardItemModel(0, 5, this);
    m_packagesModel->setHorizontalHeaderLabels(QStringList() << "" << "Name" << "Version" << "Repository" << "Description");
    m_packagesView->setModel(m_packagesModel);
    
    // Use ResizeToContents initially for best appearance
    for (int i = 0; i < m_packagesModel->columnCount(); ++i) {
        m_packagesView->horizontalHeader()->setSectionResizeMode(i, i == 4 ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
    }
    
    packagesLayout->addWidget(m_packagesView);
    
    // Create "Installed" tab with a responsive layout
    QWidget* installedTab = new QWidget();
    installedTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QVBoxLayout* installedLayout = new QVBoxLayout(installedTab);
    installedLayout->setContentsMargins(10, 10, 10, 10);
    installedLayout->setSpacing(10);
    
    // Create batch install button in its own layout for proper alignment
    QHBoxLayout* batchButtonLayout = new QHBoxLayout();
    batchButtonLayout->addStretch(1);
    
    m_batchInstallButton = new QPushButton(QIcon::fromTheme("system-software-install"), "Install Selected Packages");
    m_batchInstallButton->setObjectName("primary");
    m_batchInstallButton->setEnabled(false);
    m_batchInstallButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    batchButtonLayout->addWidget(m_batchInstallButton);
    installedLayout->addLayout(batchButtonLayout);
    
    // Create installed packages table view with responsive sizing
    m_installedView = new QTableView();
    m_installedView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_installedView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_installedView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_installedView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_installedView->setAlternatingRowColors(true);
    m_installedView->verticalHeader()->setVisible(false);
    m_installedView->verticalHeader()->setDefaultSectionSize(28);
    m_installedView->horizontalHeader()->setStretchLastSection(true);
    m_installedView->setSortingEnabled(true);
    
    // Set the model for installed view
    m_installedModel = new QStandardItemModel(0, 5, this);
    m_installedModel->setHorizontalHeaderLabels(QStringList() << "" << "Name" << "Version" << "Repository" << "Description");
    m_installedView->setModel(m_installedModel);
    
    // Use ResizeToContents initially for best appearance
    for (int i = 0; i < m_installedModel->columnCount(); ++i) {
        m_installedView->horizontalHeader()->setSectionResizeMode(i, i == 4 ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
    }
    
    installedLayout->addWidget(m_installedView);
    
    // Add tabs to tab widget
    m_tabWidget->addTab(packagesTab, "All Packages");
    m_tabWidget->addTab(installedTab, "Installed");
    
    // Create and setup system update tab
    m_systemUpdateTab = new QWidget();
    setupSystemUpdateTab();
    m_tabWidget->addTab(m_systemUpdateTab, "System Update");
    
    // Create and setup system maintenance tab
    m_maintenanceTab = new QWidget();
    setupMaintenanceTab();
    m_tabWidget->addTab(m_maintenanceTab, "System Maintenance");
    
    // Add tab widget to main layout
    mainLayout->addWidget(m_tabWidget);
    
    // Create details widget for package information
    m_detailsWidget = new QWidget();
    m_detailsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    QVBoxLayout* detailsLayout = new QVBoxLayout(m_detailsWidget);
    detailsLayout->setContentsMargins(10, 10, 10, 10);
    
    // Package details
    m_packageNameLabel = new QLabel();
    m_packageNameLabel->setProperty("heading", true);
    QFont nameFont = m_packageNameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 2);
    m_packageNameLabel->setFont(nameFont);
    
    m_packageVersionLabel = new QLabel();
    m_packageDescLabel = new QLabel();
    m_packageDescLabel->setWordWrap(true);
    
    // Action button
    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->addStretch(1);
    
    m_actionButton = new QPushButton();
    m_actionButton->setObjectName("primary");
    m_actionButton->setMinimumWidth(120);
    m_actionButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    actionLayout->addWidget(m_actionButton);
    
    // Add package overwrite checkbox
    m_packageOverwriteCheckbox = new QCheckBox("Use --overwrite \"*\"", this);
    m_packageOverwriteCheckbox->setToolTip("This option allows pacman to overwrite conflicting files. Use with caution!");
    
    // Add widgets to details layout
    detailsLayout->addWidget(m_packageNameLabel);
    detailsLayout->addWidget(m_packageVersionLabel);
    detailsLayout->addWidget(m_packageDescLabel);
    detailsLayout->addWidget(m_packageOverwriteCheckbox);
    detailsLayout->addLayout(actionLayout);
    
    // Add details widget to main layout
    mainLayout->addWidget(m_detailsWidget);
    
    // Initially hide details widget until a package is selected
    m_detailsWidget->setVisible(false);
    
    // Create status bar with resize grip
    statusBar()->setSizeGripEnabled(true);
    statusBar()->showMessage("Ready");
}

void MainWindow::setupSystemUpdateTab()
{
    QVBoxLayout* systemUpdateLayout = new QVBoxLayout(m_systemUpdateTab);
    systemUpdateLayout->setContentsMargins(10, 10, 10, 10);
    systemUpdateLayout->setSpacing(15);
    
    // Set object name for CSS styling
    m_systemUpdateTab->setObjectName("systemUpdateTab");
    m_systemUpdateTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Create a scroll area for responsive content
    QScrollArea* scrollArea = new QScrollArea(m_systemUpdateTab);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QWidget* contentWidget = new QWidget(scrollArea);
    contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(15);
    
    // Information label
    m_systemUpdateInfoLabel = new QLabel(
        "<h2>System Update</h2>"
        "<p>Perform a full system update to ensure all packages are up to date.</p>"
        "<p>This will execute <code>sudo pacman -Syu</code> to update your system.</p>"
        "<p><b>Note:</b> You'll need administrator privileges to perform this operation.</p>",
        contentWidget);
    m_systemUpdateInfoLabel->setProperty("heading", true);
    m_systemUpdateInfoLabel->setWordWrap(true);
    m_systemUpdateInfoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    contentLayout->addWidget(m_systemUpdateInfoLabel);
    
    // Button layout
    QHBoxLayout* systemUpdateButtonLayout = new QHBoxLayout();
    systemUpdateButtonLayout->setSpacing(10);
    
    // Add check for updates button
    m_checkUpdatesButton = new QPushButton("Check for Updates", contentWidget);
    m_checkUpdatesButton->setObjectName("primary");
    m_checkUpdatesButton->setMinimumHeight(36);
    m_checkUpdatesButton->setMinimumWidth(150);
    m_checkUpdatesButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    systemUpdateButtonLayout->addWidget(m_checkUpdatesButton);
    
    // Add system update button
    m_systemUpdateButton = new QPushButton("Update System", contentWidget);
    m_systemUpdateButton->setObjectName("primary");
    m_systemUpdateButton->setMinimumHeight(36);
    m_systemUpdateButton->setMinimumWidth(150);
    m_systemUpdateButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    systemUpdateButtonLayout->addWidget(m_systemUpdateButton);
    
    // Add overwrite checkbox for system update
    m_systemUpdateOverwriteCheckbox = new QCheckBox("Use --overwrite \"*\"", contentWidget);
    m_systemUpdateOverwriteCheckbox->setToolTip("This option allows pacman to overwrite conflicting files during system update. Use with caution!");
    systemUpdateButtonLayout->addWidget(m_systemUpdateOverwriteCheckbox);
    
    systemUpdateButtonLayout->addStretch(1);
    contentLayout->addLayout(systemUpdateButtonLayout);
    
    // Create a table for available updates
    QLabel* updatesLabel = new QLabel("Available Updates:", contentWidget);
    updatesLabel->setProperty("subheading", true);
    contentLayout->addWidget(updatesLabel);
    
    m_systemUpdatesView = new QTableView(contentWidget);
    m_systemUpdatesView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_systemUpdatesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_systemUpdatesView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_systemUpdatesView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_systemUpdatesView->setAlternatingRowColors(true);
    m_systemUpdatesView->verticalHeader()->setVisible(false);
    m_systemUpdatesView->verticalHeader()->setDefaultSectionSize(28);
    m_systemUpdatesView->horizontalHeader()->setStretchLastSection(true);
    m_systemUpdatesView->setSortingEnabled(true);
    
    m_systemUpdatesModel = new QStandardItemModel(0, 4, this);
    m_systemUpdatesModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Current Version" << "New Version" << "Repository");
    m_systemUpdatesView->setModel(m_systemUpdatesModel);
    
    // Use ResizeToContents initially for best appearance
    for (int i = 0; i < m_systemUpdatesModel->columnCount(); ++i) {
        m_systemUpdatesView->horizontalHeader()->setSectionResizeMode(i, i == 3 ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
    }
    
    contentLayout->addWidget(m_systemUpdatesView);
    
    // Add update log view with stretch
    QLabel* logLabel = new QLabel("Update Log:", contentWidget);
    logLabel->setProperty("subheading", true);
    contentLayout->addWidget(logLabel);
    
    m_systemUpdateLogView = new QTextEdit(contentWidget);
    m_systemUpdateLogView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_systemUpdateLogView->setReadOnly(true);
    m_systemUpdateLogView->setPlaceholderText("System update log will appear here...");
    m_systemUpdateLogView->setMinimumHeight(150);
    contentLayout->addWidget(m_systemUpdateLogView);
    
    // Set the content widget to the scroll area
    scrollArea->setWidget(contentWidget);
    
    // Add scroll area to the main layout
    systemUpdateLayout->addWidget(scrollArea);
}

void MainWindow::setupMaintenanceTab()
{
    // Create a base layout for the maintenance tab
    QVBoxLayout* mainLayout = new QVBoxLayout(m_maintenanceTab);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Set object name for CSS styling
    m_maintenanceTab->setObjectName("maintenanceTab");
    
    // Create a splitter for resizing between maintenance tools and log
    QSplitter* mainSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Create header section
    QWidget* headerWidget = new QWidget();
    QVBoxLayout* headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* titleLabel = new QLabel("System Maintenance");
    titleLabel->setProperty("heading", true);
    
    QLabel* descLabel = new QLabel(
        "Use these tools to maintain your Arch Linux system. Regular maintenance "
        "helps keep your system clean, fast, and stable."
    );
    descLabel->setProperty("subheading", true);
    descLabel->setWordWrap(true);
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(descLabel);
    
    // Create scrollable content for maintenance tools
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(20); // More spacing between sections
    
    // Define button style and uniform size
    const QString buttonStyle = "background-color: #2196F3; color: white;";
    const int buttonHeight = 32;
    const int buttonMinWidth = 120;
    
    // === Package Cache Cleaning ===
    QGroupBox* cacheGroup = new QGroupBox("Package Cache");
    QVBoxLayout* cacheGroupLayout = new QVBoxLayout(cacheGroup);
    
    QHBoxLayout* cacheButtonsLayout = new QHBoxLayout();
    cacheButtonsLayout->setSpacing(10);
    
    m_clearCacheButton = new QPushButton("Clean Cache");
    m_clearCacheButton->setObjectName("primary");
    m_clearCacheButton->setMinimumHeight(buttonHeight);
    m_clearCacheButton->setMinimumWidth(buttonMinWidth);
    m_clearCacheButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    QPushButton* refreshCacheButton = new QPushButton("Refresh");
    refreshCacheButton->setObjectName("primary");
    refreshCacheButton->setMinimumHeight(buttonHeight);
    refreshCacheButton->setMinimumWidth(buttonMinWidth);
    refreshCacheButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    cacheButtonsLayout->addWidget(m_clearCacheButton);
    cacheButtonsLayout->addWidget(refreshCacheButton);
    cacheButtonsLayout->addStretch();
    
    QHBoxLayout* cacheOptionsLayout = new QHBoxLayout();
    m_clearUnusedCacheRadio = new QRadioButton("Clean unused packages (pacman -Sc)");
    m_clearUnusedCacheRadio->setChecked(true);
    m_clearAllCacheRadio = new QRadioButton("Clean all packages (pacman -Scc)");
    
    cacheOptionsLayout->addWidget(m_clearUnusedCacheRadio);
    cacheOptionsLayout->addWidget(m_clearAllCacheRadio);
    cacheOptionsLayout->addStretch();
    
    cacheGroupLayout->addLayout(cacheButtonsLayout);
    cacheGroupLayout->addLayout(cacheOptionsLayout);
    
    // === Orphaned Packages ===
    QGroupBox* orphansGroup = new QGroupBox("Orphaned Packages");
    QVBoxLayout* orphansGroupLayout = new QVBoxLayout(orphansGroup);
    
    QHBoxLayout* orphansButtonsLayout = new QHBoxLayout();
    orphansButtonsLayout->setSpacing(10);
    
    m_findOrphansButton = new QPushButton("Find Orphans");
    m_findOrphansButton->setObjectName("primary");
    m_findOrphansButton->setMinimumHeight(buttonHeight);
    m_findOrphansButton->setMinimumWidth(buttonMinWidth);
    m_findOrphansButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    m_removeOrphansButton = new QPushButton("Remove Orphans");
    m_removeOrphansButton->setObjectName("primary");
    m_removeOrphansButton->setMinimumHeight(buttonHeight);
    m_removeOrphansButton->setMinimumWidth(buttonMinWidth);
    m_removeOrphansButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_removeOrphansButton->setEnabled(false);
    
    orphansButtonsLayout->addWidget(m_findOrphansButton);
    orphansButtonsLayout->addWidget(m_removeOrphansButton);
    orphansButtonsLayout->addStretch();
    
    orphansGroupLayout->addLayout(orphansButtonsLayout);
    
    // === Database Check ===
    QGroupBox* dbCheckGroup = new QGroupBox("Database Check");
    QVBoxLayout* dbCheckGroupLayout = new QVBoxLayout(dbCheckGroup);
    
    QHBoxLayout* dbCheckLayout = new QHBoxLayout();
    dbCheckLayout->setSpacing(10);
    
    m_checkDatabaseButton = new QPushButton("Check Database");
    m_checkDatabaseButton->setObjectName("primary");
    m_checkDatabaseButton->setMinimumHeight(buttonHeight);
    m_checkDatabaseButton->setMinimumWidth(buttonMinWidth);
    m_checkDatabaseButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    m_checkSyncDbsCheckbox = new QCheckBox("Also check sync databases (pacman -Dkk)");
    
    dbCheckLayout->addWidget(m_checkDatabaseButton);
    dbCheckLayout->addWidget(m_checkSyncDbsCheckbox);
    dbCheckLayout->addStretch();
    
    dbCheckGroupLayout->addLayout(dbCheckLayout);
    
    // === Pacnew Files ===
    QGroupBox* pacnewGroup = new QGroupBox("Config File Updates");
    QVBoxLayout* pacnewGroupLayout = new QVBoxLayout(pacnewGroup);
    
    QHBoxLayout* pacnewLayout = new QHBoxLayout();
    pacnewLayout->setSpacing(10);
    
    m_findPacnewButton = new QPushButton("Find .pacnew/.pacsave Files");
    m_findPacnewButton->setObjectName("primary");
    m_findPacnewButton->setMinimumHeight(buttonHeight);
    m_findPacnewButton->setMinimumWidth(buttonMinWidth);
    m_findPacnewButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    pacnewLayout->addWidget(m_findPacnewButton);
    pacnewLayout->addStretch();
    
    pacnewGroupLayout->addLayout(pacnewLayout);
    
    // === Database Backup/Restore ===
    QGroupBox* backupGroup = new QGroupBox("Database Backup/Restore");
    QVBoxLayout* backupGroupLayout = new QVBoxLayout(backupGroup);
    
    QHBoxLayout* backupPathLayout = new QHBoxLayout();
    backupPathLayout->setSpacing(10);
    
    QLabel* backupPathLabel = new QLabel("Backup Path:");
    m_backupPathEdit = new QLineEdit(QDir::homePath() + "/pacman_database_backup.tar.gz");
    m_backupPathEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    m_selectBackupPathButton = new QPushButton("Browse...");
    m_selectBackupPathButton->setMinimumHeight(buttonHeight);
    m_selectBackupPathButton->setMinimumWidth(80);
    m_selectBackupPathButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    backupPathLayout->addWidget(backupPathLabel);
    backupPathLayout->addWidget(m_backupPathEdit, 1);
    backupPathLayout->addWidget(m_selectBackupPathButton);
    
    QHBoxLayout* backupButtonsLayout = new QHBoxLayout();
    backupButtonsLayout->setSpacing(10);
    
    m_backupButton = new QPushButton("Backup Database");
    m_backupButton->setObjectName("primary");
    m_backupButton->setMinimumHeight(buttonHeight);
    m_backupButton->setMinimumWidth(buttonMinWidth);
    m_backupButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    m_restoreButton = new QPushButton("Restore Database");
    m_restoreButton->setObjectName("primary");
    m_restoreButton->setMinimumHeight(buttonHeight);
    m_restoreButton->setMinimumWidth(buttonMinWidth);
    m_restoreButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    backupButtonsLayout->addWidget(m_backupButton);
    backupButtonsLayout->addWidget(m_restoreButton);
    backupButtonsLayout->addStretch();
    
    backupGroupLayout->addLayout(backupPathLayout);
    backupGroupLayout->addLayout(backupButtonsLayout);
    
    // Add all sections to the content layout
    contentLayout->addWidget(cacheGroup);
    contentLayout->addWidget(orphansGroup);
    contentLayout->addWidget(dbCheckGroup);
    contentLayout->addWidget(pacnewGroup);
    contentLayout->addWidget(backupGroup);
    contentLayout->addStretch(1); // Add stretch at the end
    
    // Set the content widget to the scroll area
    scrollArea->setWidget(contentWidget);
    
    // Create a widget for the top part (header + tools)
    QWidget* topWidget = new QWidget();
    QVBoxLayout* topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(10);
    topLayout->addWidget(headerWidget);
    topLayout->addWidget(scrollArea, 1); // Give scrollArea a stretch factor
    
    // Create maintenance log area
    QGroupBox* logGroup = new QGroupBox("Maintenance Log");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    
    m_maintenanceLogView = new QTextEdit();
    m_maintenanceLogView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_maintenanceLogView->setReadOnly(true);
    m_maintenanceLogView->setLineWrapMode(QTextEdit::WidgetWidth);
    m_maintenanceLogView->setMinimumHeight(100);
    
    // Create progress bar for operations
    m_maintenanceProgressBar = new QProgressBar();
    m_maintenanceProgressBar->setRange(0, 0); // Indeterminate
    m_maintenanceProgressBar->setVisible(false);
    
    logLayout->addWidget(m_maintenanceLogView);
    logLayout->addWidget(m_maintenanceProgressBar);
    
    // Add the widgets to the splitter
    mainSplitter->addWidget(topWidget);
    mainSplitter->addWidget(logGroup);
    
    // Set the initial sizes of the splitter (70% top, 30% bottom)
    QList<int> sizes;
    sizes << 700 << 300;
    mainSplitter->setSizes(sizes);
    
    // Add the splitter to the main layout
    mainLayout->addWidget(mainSplitter);
    
    // Connect the refresh cache button
    connect(refreshCacheButton, &QPushButton::clicked, this, [this]() {
        m_maintenanceLogView->append("Checking package cache size...");
        setCursor(Qt::WaitCursor);
        m_maintenanceProgressBar->setVisible(true);
        
        QtConcurrent::run([this]() {
            QString output;
            QProcess process;
            process.start("du", QStringList() << "-sh" << "/var/cache/pacman/pkg/");
            process.waitForFinished();
            output = QString::fromUtf8(process.readAllStandardOutput());
            
            // Update UI from the main thread
            QMetaObject::invokeMethod(this, [this, output]() {
                setCursor(Qt::ArrowCursor);
                m_maintenanceProgressBar->setVisible(false);
                
                if (!output.isEmpty()) {
                    m_maintenanceLogView->append("Package cache size: " + output.trimmed());
                } else {
                    m_maintenanceLogView->append("Failed to check package cache size");
                }
            }, Qt::QueuedConnection);
        });
    });
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
    
    // Fix the connection by using a lambda to handle the toggle
    std::cout << "MainWindow::setupMenus - Setting up theme toggle action" << std::endl;
    connect(themeAction, &QAction::toggled, this, [this](bool checked) {
        std::cout << "MainWindow::setupMenus - Theme action toggled: " << (checked ? "true" : "false") << std::endl;
        toggleTheme(checked);
    });
    
    settingsMenu->addSeparator();
    settingsMenu->addAction(m_actions["settings"]);
    
    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(m_actions["about"]);
}

void MainWindow::setupConnections()
{
    // Add debug log to show we're setting up connections
    std::cout << "MainWindow::setupConnections - Setting up signal/slot connections" << std::endl;
    
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
    
    // Connect models for handling checkbox changes
    connect(m_packagesModel, &QStandardItemModel::itemChanged, this, &MainWindow::onPackageItemChanged);
    connect(m_installedModel, &QStandardItemModel::itemChanged, this, &MainWindow::onPackageItemChanged);
    
    // Handle row clicks for displaying package details
    connect(m_packagesView, &QTableView::clicked, this, &MainWindow::onPackageSelected);
    connect(m_installedView, &QTableView::clicked, this, &MainWindow::onInstalledPackageSelected);
    
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
    
    // Connect maintenance tab buttons
    connect(m_clearCacheButton, &QPushButton::clicked, this, &MainWindow::onClearPackageCache);
    connect(m_findOrphansButton, &QPushButton::clicked, this, [this]() {
        m_findOrphansButton->setEnabled(false);
        m_removeOrphansButton->setEnabled(false);
        setCursor(Qt::WaitCursor);
        m_maintenanceProgressBar->setVisible(true);
        m_maintenanceLogView->append("Searching for orphaned packages...");
        
        QtConcurrent::run([this]() {
            std::vector<std::string> orphans = m_packageManager.get_orphaned_packages();

            // Update UI in main thread
            QMetaObject::invokeMethod(this, [this, orphans]() {
                // Update UI state
                m_findOrphansButton->setEnabled(true);
                m_removeOrphansButton->setEnabled(!orphans.empty());
                m_maintenanceProgressBar->setVisible(false);
                setCursor(Qt::ArrowCursor);

                // Update log
                if (orphans.empty()) {
                    m_maintenanceLogView->append("No orphaned packages found.");
                } else {
                    m_maintenanceLogView->append("Found " + QString::number(orphans.size()) +
                        " orphaned packages:");
                    
                    // Display orphans in the log
                    for (const auto& pkg : orphans) {
                        m_maintenanceLogView->append(" - " + QString::fromStdString(pkg));
                    }
                }
            }, Qt::QueuedConnection);
        });
    });
    
    connect(m_removeOrphansButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::StandardButton confirm = QMessageBox::question(
            this,
            "Remove Orphaned Packages",
            "Are you sure you want to remove all orphaned packages?",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (confirm == QMessageBox::Yes) {
            // Ask for password
            bool ok;
            QString password = QInputDialog::getText(
                this, 
                "Authentication Required",
                "Enter your password to remove orphaned packages:",
                QLineEdit::Password, 
                "", 
                &ok);
            
            if (!ok || password.isEmpty()) {
                m_maintenanceLogView->append("Operation cancelled by user.");
                return;
            }
            
            m_findOrphansButton->setEnabled(false);
            m_removeOrphansButton->setEnabled(false);
            setCursor(Qt::WaitCursor);
            m_maintenanceProgressBar->setVisible(true);
            m_maintenanceLogView->append("Removing orphaned packages...");
            
            QThreadPool::globalInstance()->start([this, password]() {
                // Define output callback for real-time feedback
                auto outputCallback = [this](const std::string& output) {
                    // Update log from the worker thread
                    QMetaObject::invokeMethod(this, [this, line = QString::fromStdString(output)]() {
                        m_maintenanceLogView->append(line);
                        m_maintenanceLogView->verticalScrollBar()->setValue(
                            m_maintenanceLogView->verticalScrollBar()->maximum()
                        );
                        QApplication::processEvents();
                    }, Qt::QueuedConnection);
                };
                
                bool success = m_packageManager.remove_orphaned_packages(
                    password.toStdString(),
                    outputCallback
                );
                
                // Update UI in main thread
                QMetaObject::invokeMethod(this, [this, success]() {
                    // Update UI state
                    m_findOrphansButton->setEnabled(true);
                    m_removeOrphansButton->setEnabled(false);
                    m_maintenanceProgressBar->setVisible(false);
                    setCursor(Qt::ArrowCursor);
                    
                    // Update log
                    if (success) {
                        m_maintenanceLogView->append("Orphaned packages removed successfully.");
                    } else {
                        m_maintenanceLogView->append("Failed to remove orphaned packages: " + 
                            QString::fromStdString(m_packageManager.get_last_error()));
                    }
                }, Qt::QueuedConnection);
            });
        }
    });
    
    connect(m_checkDatabaseButton, &QPushButton::clicked, this, &MainWindow::onCheckDatabase);
    connect(m_findPacnewButton, &QPushButton::clicked, this, [this]() {
        m_findPacnewButton->setEnabled(false);
        setCursor(Qt::WaitCursor);
        m_maintenanceProgressBar->setVisible(true);
        m_maintenanceLogView->append("Searching for .pacnew and .pacsave files...");
        
        QtConcurrent::run([this]() {
            QStringList pacnewFiles;
            
            // Run the find command to locate .pacnew files
            QProcess process;
            process.start("find", QStringList() << "/" << "-name" << "*.pacnew" << "-o" << "-name" << "*.pacsave");
            process.waitForFinished();
            
            QString output = QString::fromUtf8(process.readAllStandardOutput());
            pacnewFiles = output.split('\n', Qt::SkipEmptyParts);
            
            // Update UI in main thread
            QMetaObject::invokeMethod(this, [this, pacnewFiles]() {
                // Update UI state
                m_findPacnewButton->setEnabled(true);
                m_maintenanceProgressBar->setVisible(false);
                setCursor(Qt::ArrowCursor);
                
                // Update log
                if (pacnewFiles.isEmpty()) {
                    m_maintenanceLogView->append("No .pacnew or .pacsave files found.");
                } else {
                    m_maintenanceLogView->append("Found " + QString::number(pacnewFiles.size()) +
                        " .pacnew/.pacsave files:");
                    
                    // Display files in the log
                    for (const auto& file : pacnewFiles) {
                        m_maintenanceLogView->append(" - " + file);
                    }
                }
            }, Qt::QueuedConnection);
        });
    });
    
    connect(m_backupButton, &QPushButton::clicked, this, &MainWindow::onBackupDatabase);
    connect(m_restoreButton, &QPushButton::clicked, this, &MainWindow::onRestoreDatabase);
    connect(m_selectBackupPathButton, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this,
            "Select Backup Location",
            m_backupPathEdit->text(),
            "Tar Archives (*.tar.gz)");
        
        if (!fileName.isEmpty()) {
            m_backupPathEdit->setText(fileName);
        }
    });
    
    // Connect to settings dialog for theme changes
    std::cout << "MainWindow::setupConnections - Setting up connections for settings dialog" << std::endl;
    QSettings settings("PacmanGUI", "PacmanGUI");
    std::cout << "MainWindow::setupConnections - Current theme: " << settings.value("appearance/theme", "dark_colorful").toString().toStdString() << std::endl;
}

void MainWindow::onBackupDatabase()
{
    // Get the backup path
    QString backupPath = m_backupPathEdit->text();
    
    // Confirm the operation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Backup Pacman Database",
        "Are you sure you want to backup the pacman database to:\n" + backupPath,
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Show progress bar
    m_maintenanceProgressBar->setVisible(true);
    
    // Disable UI elements during operation
    m_backupButton->setEnabled(false);
    m_restoreButton->setEnabled(false);
    m_selectBackupPathButton->setEnabled(false);
    m_backupPathEdit->setEnabled(false);
    
    // Clear the log view
    m_maintenanceLogView->clear();
    m_maintenanceLogView->append("<b>Starting pacman database backup...</b>");
    
    // Define output callback for real-time updates
    auto outputCallback = [this](const std::string& output) {
        QString line = QString::fromStdString(output);
        m_maintenanceLogView->append(line);
        
        // Ensure log view scrolls to show the most recent output
        m_maintenanceLogView->verticalScrollBar()->setValue(
            m_maintenanceLogView->verticalScrollBar()->maximum()
        );
        
        // Process events to keep UI responsive
        QApplication::processEvents();
    };
    
    // Execute the operation in a separate thread
    QtConcurrent::run([=]() {
        bool success = m_packageManager.backup_database(
            backupPath.toStdString(), 
            outputCallback
        );
        
        // Use signal-slot to update UI from the main thread
        QMetaObject::invokeMethod(
            this, 
            "onMaintenanceTaskFinished", 
            Qt::QueuedConnection,
            Q_ARG(bool, success), 
            Q_ARG(QString, QString("Database backup %1").arg(success ? "completed successfully" : "failed"))
        );
    });
}

void MainWindow::onRestoreDatabase()
{
    // Get the backup path
    QString backupPath = m_backupPathEdit->text();
    
    // Check if the file exists
    QFileInfo fileInfo(backupPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::warning(
            this, 
            "Restore Failed",
            "The specified backup file does not exist:\n" + backupPath
        );
        return;
    }
    
    // Show a strong warning
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this, 
        "Restore Pacman Database",
        "<b>WARNING: This operation will replace your current pacman database!</b>\n\n"
        "Restoring an old database can cause serious problems with your system, "
        "especially if package versions don't match your currently installed files.\n\n"
        "Are you absolutely sure you want to restore the pacman database from:\n" + backupPath,
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Get a second confirmation
    reply = QMessageBox::warning(
        this, 
        "Confirm Restore",
        "This is your final warning. Are you really sure you want to proceed?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Get authentication
    bool ok;
    QString password = QInputDialog::getText(
        this, 
        "Authentication Required",
        "Enter your password to restore the pacman database:",
        QLineEdit::Password, 
        "", 
        &ok
    );
    
    if (!ok || password.isEmpty()) {
        m_maintenanceLogView->append("<span style='color:orange'>Operation cancelled</span>");
        return;
    }
    
    // Show progress bar
    m_maintenanceProgressBar->setVisible(true);
    
    // Disable UI elements during operation
    m_backupButton->setEnabled(false);
    m_restoreButton->setEnabled(false);
    m_selectBackupPathButton->setEnabled(false);
    m_backupPathEdit->setEnabled(false);
    
    // Clear the log view
    m_maintenanceLogView->clear();
    m_maintenanceLogView->append("<b>Starting pacman database restore...</b>");
    m_maintenanceLogView->append("<span style='color:red'>WARNING: This is a potentially dangerous operation!</span>");
    
    // Define output callback for real-time updates
    auto outputCallback = [this](const std::string& output) {
        QString line = QString::fromStdString(output);
        
        // Format output based on content
        if (line.contains("error", Qt::CaseInsensitive)) {
            line = "<span style='color:red'>" + line + "</span>";
        } else if (line.contains("warning", Qt::CaseInsensitive)) {
            line = "<span style='color:orange'>" + line + "</span>";
        }
        
        m_maintenanceLogView->append(line);
        
        // Ensure log view scrolls to show the most recent output
        m_maintenanceLogView->verticalScrollBar()->setValue(
            m_maintenanceLogView->verticalScrollBar()->maximum()
        );
        
        // Process events to keep UI responsive
        QApplication::processEvents();
    };
    
    // Execute the operation in a separate thread
    QtConcurrent::run([=]() {
        bool success = m_packageManager.restore_database(
            backupPath.toStdString(), 
            password.toStdString(), 
            outputCallback
        );
        
        // Use signal-slot to update UI from the main thread
        QMetaObject::invokeMethod(
            this, 
            "onMaintenanceTaskFinished", 
            Qt::QueuedConnection,
            Q_ARG(bool, success), 
            Q_ARG(QString, QString("Database restore %1").arg(success ? "completed successfully" : "failed"))
        );
    });
}

void MainWindow::onMaintenanceTaskFinished(bool success, const QString& message)
{
    // Hide progress bar
    m_maintenanceProgressBar->setVisible(false);
    
    // Show message in log
    m_maintenanceLogView->append(message);
    
    // Update status bar
    statusBar()->showMessage(message, 3000);
    
    // Re-enable buttons
    m_findOrphansButton->setEnabled(true);
    m_removeOrphansButton->setEnabled(success);
}

void MainWindow::onSyncAll()
{
    // Confirm operation with the user
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Synchronize Databases",
        "Do you want to synchronize package databases?\n\n"
        "This will refresh all package database information from the configured repositories.",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Ask for password
    bool ok;
    QString password = QInputDialog::getText(
        this, 
        "Authentication Required",
        "Enter your password to synchronize package databases:",
        QLineEdit::Password, 
        "", 
        &ok);
    
    if (!ok || password.isEmpty()) {
        statusBar()->showMessage("Database synchronization cancelled", 3000);
        return;
    }
    
    // Set cursor to wait cursor
    setCursor(Qt::WaitCursor);
    
    // Show status message
    statusBar()->showMessage("Synchronizing package databases...");
    
    // Sync all databases with provided password
    bool success = m_packageManager.sync_all(password.toStdString());
    
    // Reset cursor
    setCursor(Qt::ArrowCursor);
    
    if (success) {
        // Show success message
        statusBar()->showMessage("Package databases refreshed successfully", 3000);
        
        // Refresh any views that depend on package database info
        refreshInstalledPackages();
        refreshUpdatesList();
    } else {
        // Show error message
        QString errorMessage = QString::fromStdString(m_packageManager.get_last_error());
        QMessageBox::critical(
            this, 
            "Synchronization Failed",
            "Failed to synchronize package databases.\n\n" + errorMessage);
        
        statusBar()->showMessage("Database synchronization failed", 3000);
    }
}

void MainWindow::onTabChanged(int index)
{
    qDebug() << "Tab changed to index:" << index;
    statusBar()->showMessage("Current tab: " + m_tabWidget->tabText(index), 2000);

    // Show/hide detail panel based on tab index
    if (index < 2) { // All Packages (0) or Installed (1)
        // Only show if a package is currently selected
        if (index == 0 && m_packagesView->selectionModel()->hasSelection()) {
            m_detailsWidget->setVisible(true);
            // Ensure sidebar panel is open for selected package
            if (m_detailPanelVisible) {
                m_detailPanel->setVisible(true);
            }
        } else if (index == 1 && m_installedView->selectionModel()->hasSelection()) {
            m_detailsWidget->setVisible(true);
            // Ensure sidebar panel is open for selected package
            if (m_detailPanelVisible) {
                m_detailPanel->setVisible(true);
            }
        } else {
            m_detailsWidget->setVisible(false);
            // Close sidebar panel if no selection
            if (m_detailPanelVisible) {
                closeDetailPanel();
                m_detailPanelVisible = false;
            }
        }
    } else {
        // For other tabs, always hide the detail panel and sliding panel
        m_detailsWidget->setVisible(false);
        if (m_detailPanelVisible) {
            closeDetailPanel();
            m_detailPanelVisible = false;
        }
    }
}

void MainWindow::onSearchTextChanged(const QString& text)
{
    // Enable/disable search button based on if there's text
    m_searchButton->setEnabled(!text.isEmpty());
}

void MainWindow::onSearchClicked()
{
    // Perform search based on the text in the search box
    searchPackages(m_searchBox->text());
}

void MainWindow::onInstallPackage()
{
    // Get currently selected package
    QString packageName = m_packageNameLabel->text();
    if (packageName.isEmpty()) {
        return;
    }
    
    // Ask for password
    bool ok;
    QString password = QInputDialog::getText(
        this, 
        "Authentication Required",
        "Enter your password to install package:",
        QLineEdit::Password, 
        "", 
        &ok);
    
    if (!ok || password.isEmpty()) {
        statusBar()->showMessage("Installation cancelled", 3000);
        return;
    }
    
    // Check if overwrite is enabled
    bool useOverwrite = m_packageOverwriteCheckbox->isChecked();
    
    // Install the package
    setCursor(Qt::WaitCursor);
    bool success = m_packageManager.install_package(
        packageName.toStdString(), 
        password.toStdString(), 
        useOverwrite);
    setCursor(Qt::ArrowCursor);
    
    if (success) {
        statusBar()->showMessage("Package installed successfully", 3000);
        refreshInstalledPackages();
    } else {
        QString errorMessage = QString::fromStdString(m_packageManager.get_last_error());
        QMessageBox::critical(
            this, 
            "Installation Failed",
            "Failed to install package.\n\n" + errorMessage);
        statusBar()->showMessage("Installation failed", 3000);
    }
}

void MainWindow::onRemovePackage()
{
    // Get currently selected package
    QString packageName = m_packageNameLabel->text();
    if (packageName.isEmpty()) {
        return;
    }
    
    // Confirm removal
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Remove Package",
        "Are you sure you want to remove " + packageName + "?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Ask for password
    bool ok;
    QString password = QInputDialog::getText(
        this, 
        "Authentication Required",
        "Enter your password to remove package:",
        QLineEdit::Password, 
        "", 
        &ok);
    
    if (!ok || password.isEmpty()) {
        statusBar()->showMessage("Removal cancelled", 3000);
        return;
    }
    
    // Remove the package
    setCursor(Qt::WaitCursor);
    bool success = m_packageManager.remove_package(
        packageName.toStdString(), 
        password.toStdString());
    setCursor(Qt::ArrowCursor);
    
    if (success) {
        statusBar()->showMessage("Package removed successfully", 3000);
        refreshInstalledPackages();
    } else {
        QString errorMessage = QString::fromStdString(m_packageManager.get_last_error());
        QMessageBox::critical(
            this, 
            "Removal Failed",
            "Failed to remove package.\n\n" + errorMessage);
        statusBar()->showMessage("Removal failed", 3000);
    }
}

void MainWindow::onUpdatePackage()
{
    // Get currently selected package
    QString packageName = m_packageNameLabel->text();
    if (packageName.isEmpty()) {
        return;
    }
    
    // Ask for password
    bool ok;
    QString password = QInputDialog::getText(
        this, 
        "Authentication Required",
        "Enter your password to update package:",
        QLineEdit::Password, 
        "", 
        &ok);
    
    if (!ok || password.isEmpty()) {
        statusBar()->showMessage("Update cancelled", 3000);
        return;
    }
    
    // Check if overwrite is enabled
    bool useOverwrite = m_packageOverwriteCheckbox->isChecked();
    
    // Update the package
    setCursor(Qt::WaitCursor);
    bool success = m_packageManager.update_package(
        packageName.toStdString(), 
        password.toStdString(), 
        useOverwrite);
    setCursor(Qt::ArrowCursor);
    
    if (success) {
        statusBar()->showMessage("Package updated successfully", 3000);
        refreshInstalledPackages();
    } else {
        QString errorMessage = QString::fromStdString(m_packageManager.get_last_error());
        QMessageBox::critical(
            this, 
            "Update Failed",
            "Failed to update package.\n\n" + errorMessage);
        statusBar()->showMessage("Update failed", 3000);
    }
}

void MainWindow::onBatchInstall()
{
    if (m_selectedPackages.isEmpty()) {
        return;
    }
    
    // Convert selected packages to a list for display
    QStringList packageList;
    for (const QString& pkg : m_selectedPackages) {
        packageList.append(pkg);
    }
    
    // Confirm installation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Batch Install Packages",
        "Are you sure you want to install the following packages?\n\n" + 
        packageList.join("\n"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Ask for password
    bool ok;
    QString password = QInputDialog::getText(
        this, 
        "Authentication Required",
        "Enter your password to install packages:",
        QLineEdit::Password, 
        "", 
        &ok);
    
    if (!ok || password.isEmpty()) {
        statusBar()->showMessage("Batch installation cancelled", 3000);
        return;
    }
    
    // Check if overwrite is enabled
    bool useOverwrite = m_packageOverwriteCheckbox->isChecked();
    
    // Create progress dialog
    QProgressDialog progress("Installing packages...", "Cancel", 0, packageList.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);
    
    // Initialize success counter
    int successCount = 0;
    
    // Set cursor
    setCursor(Qt::WaitCursor);
    
    // Install each package
    for (int i = 0; i < packageList.size(); ++i) {
        QString packageName = packageList.at(i);
        
        // Update progress dialog
        progress.setValue(i);
        progress.setLabelText(QString("Installing package %1 of %2: %3")
                             .arg(i+1)
                             .arg(packageList.size())
                             .arg(packageName));
        QApplication::processEvents();
        
        // Check if user cancelled the operation
        if (progress.wasCanceled()) {
            break;
        }
        
        // Install the package
        bool success = m_packageManager.install_package(
            packageName.toStdString(), 
            password.toStdString(), 
            useOverwrite);
        
        if (success) {
            successCount++;
        } else {
            QString errorMessage = QString::fromStdString(m_packageManager.get_last_error());
            QMessageBox::warning(
                this, 
                "Installation Failed",
                QString("Failed to install package %1:\n\n%2")
                .arg(packageName)
                .arg(errorMessage));
        }
    }
    
    // Reset cursor
    setCursor(Qt::ArrowCursor);
    
    // Close progress dialog
    progress.setValue(packageList.size());
    
    // Show result message
    if (successCount == packageList.size()) {
        statusBar()->showMessage(QString("Successfully installed all %1 packages").arg(successCount), 3000);
    } else if (successCount > 0) {
        statusBar()->showMessage(QString("Installed %1 of %2 packages").arg(successCount).arg(packageList.size()), 3000);
    } else {
        statusBar()->showMessage("Failed to install packages", 3000);
    }
    
    // Refresh installed packages view
    refreshInstalledPackages();
    
    // Clear selection
    m_selectedPackages.clear();
    updateBatchInstallButton();
}

void MainWindow::onSystemUpdate()
{
    // Check if overwrite is enabled
    bool useOverwrite = m_systemUpdateOverwriteCheckbox->isChecked();
    
    // Confirm update
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "System Update",
        "Are you sure you want to update your system?\n\nThis will execute 'sudo pacman -Syu'" +
        QString(useOverwrite ? " with --overwrite=\"*\"" : ""),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Ask for password
    bool ok;
    QString password = QInputDialog::getText(
        this, 
        "Authentication Required",
        "Enter your password to update system:",
        QLineEdit::Password, 
        "", 
        &ok);
    
    if (!ok || password.isEmpty()) {
        statusBar()->showMessage("System update cancelled", 3000);
        return;
    }
    
    // Clear log view
    m_systemUpdateLogView->clear();
    m_systemUpdateLogView->append("<b>Starting system update...</b>");
    
    // Define output callback for real-time updates
    auto outputCallback = [this](const std::string& output) {
        QString line = QString::fromStdString(output);
        m_systemUpdateLogView->append(line);
        
        // Ensure log view scrolls to show the most recent output
        m_systemUpdateLogView->verticalScrollBar()->setValue(
            m_systemUpdateLogView->verticalScrollBar()->maximum()
        );
        
        // Process events to keep UI responsive
        QApplication::processEvents();
    };
    
    // Switch to the system update tab
    m_tabWidget->setCurrentIndex(m_tabWidget->indexOf(m_systemUpdateTab));
    
    // Disable update buttons during operation
    m_systemUpdateButton->setEnabled(false);
    m_checkUpdatesButton->setEnabled(false);
    
    // Update system
    setCursor(Qt::WaitCursor);
    
    // Execute the operation in a separate thread
    QtConcurrent::run([=]() {
        bool success = m_packageManager.update_system(
            password.toStdString(), 
            outputCallback, 
            useOverwrite);
        
        // Update UI from the main thread
        QMetaObject::invokeMethod(this, [this, success]() {
            setCursor(Qt::ArrowCursor);
            
            // Re-enable buttons
            m_systemUpdateButton->setEnabled(true);
            m_checkUpdatesButton->setEnabled(true);
            
            if (success) {
                m_systemUpdateLogView->append("<span style='color:green'><b>System update completed successfully!</b></span>");
                statusBar()->showMessage("System updated successfully", 3000);
                
                // Refresh package information
                refreshInstalledPackages();
                refreshUpdatesList();
            } else {
                QString errorMessage = QString::fromStdString(m_packageManager.get_last_error());
                m_systemUpdateLogView->append("<span style='color:red'><b>System update failed:</b> " + 
                    errorMessage + "</span>");
                statusBar()->showMessage("System update failed", 3000);
            }
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onCheckForUpdates()
{
    // Clear the updates model
    m_systemUpdatesModel->setRowCount(0);
    
    // Switch to the system update tab
    m_tabWidget->setCurrentIndex(m_tabWidget->indexOf(m_systemUpdateTab));
    
    // Disable update buttons during check
    m_systemUpdateButton->setEnabled(false);
    m_checkUpdatesButton->setEnabled(false);
    
    // Show status message
    statusBar()->showMessage("Checking for updates...");
    
    // Set cursor
    setCursor(Qt::WaitCursor);
    
    // Clear the log view and show the process is starting
    m_systemUpdateLogView->clear();
    m_systemUpdateLogView->append("<b>Checking for system updates...</b>");
    
    // Execute the operation in a separate thread
    QtConcurrent::run([this]() {
        QProcess process;
        process.start("pacman", QStringList() << "-Qu");
        process.waitForFinished(-1); // Wait indefinitely
        
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QString error = QString::fromUtf8(process.readAllStandardError());
        int exitCode = process.exitCode();
        
        // Parse the output to extract package updates
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        std::vector<std::pair<std::string, std::string>> updates;
        
        for (const QString& line : lines) {
            // Parse output line (format: package_name old_version -> new_version)
            int firstSpace = line.indexOf(' ');
            if (firstSpace != -1) {
                QString packageName = line.left(firstSpace);
                
                // Find the arrow separator
                int arrowPos = line.indexOf("->");
                if (arrowPos != -1) {
                    // Extract new version (skip spaces after arrow)
                    int versionStart = arrowPos + 2;
                    while (versionStart < line.size() && line[versionStart].isSpace()) {
                        versionStart++;
                    }
                    
                    QString newVersion = line.mid(versionStart).trimmed();
                    updates.push_back(std::make_pair(packageName.toStdString(), newVersion.toStdString()));
                }
            }
        }
        
        // Update UI from the main thread
        QMetaObject::invokeMethod(this, [this, updates, output, error, exitCode]() {
            setCursor(Qt::ArrowCursor);
            
            // Clear the updates model
            m_systemUpdatesModel->setRowCount(0);
            
            // Check if the command was successful
            if (exitCode == 0) {
                // Update system updates view
                for (const auto& update : updates) {
                    QList<QStandardItem*> items;
                    items.append(new QStandardItem(QString::fromStdString(update.first)));
                    items.append(new QStandardItem(QString::fromStdString(update.second)));
                    m_systemUpdatesModel->appendRow(items);
                }
                
                if (!updates.empty()) {
                    // Update count in button
                    m_systemUpdateButton->setText(QString("Update System (%1)").arg(updates.size()));
                    m_systemUpdateButton->setEnabled(true);
                    
                    // Show status message
                    statusBar()->showMessage(QString("%1 updates available").arg(updates.size()), 3000);
                    
                    // Update log view
                    m_systemUpdateLogView->append(QString("<span style='color:green'><b>Found %1 packages to update:</b></span>").arg(updates.size()));
                    m_systemUpdateLogView->append(output);
                } else {
                    // No updates available
                    m_systemUpdateButton->setText("Update System");
                    m_systemUpdateButton->setEnabled(false);
                    
                    // Show status message
                    statusBar()->showMessage("System is up to date", 3000);
                    
                    // Update log view
                    m_systemUpdateLogView->append("<span style='color:green'><b>System is up to date. No packages need updating.</b></span>");
                }
            } else {
                // Command failed
                m_systemUpdateButton->setText("Update System");
                m_systemUpdateButton->setEnabled(false);
                
                // Show status message
                statusBar()->showMessage("Failed to check for updates", 3000);
                
                // Update log view with error
                m_systemUpdateLogView->append("<span style='color:red'><b>Failed to check for updates:</b></span>");
                if (!error.isEmpty()) {
                    m_systemUpdateLogView->append(error);
                } else {
                    m_systemUpdateLogView->append("Unknown error occurred while checking for updates.");
                }
            }
            
            // Re-enable check button
            m_checkUpdatesButton->setEnabled(true);
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onInstallAurPackage()
{
    // TODO: Implement AUR package installation
    statusBar()->showMessage("AUR package installation not implemented yet", 3000);
}

void MainWindow::onUpdateAurPackages()
{
    // TODO: Implement AUR package updates
    statusBar()->showMessage("AUR package updates not implemented yet", 3000);
}

void MainWindow::onClearPackageCache()
{
    bool cleanAll = m_clearAllCacheRadio->isChecked();
    QString operation = cleanAll ? "all" : "unused";
    
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this,
        "Clear Package Cache",
        "Are you sure you want to clear " + operation + " packages from the cache?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (confirm != QMessageBox::Yes) {
        return;
    }
    
    // Setup UI for operation
    m_clearCacheButton->setEnabled(false);
    setCursor(Qt::WaitCursor);
    m_maintenanceProgressBar->setVisible(true);
    m_maintenanceLogView->append("Clearing " + operation + " packages from cache...");
    
    // Ask for password
    bool ok;
    QString password = QInputDialog::getText(
        this, 
        "Authentication Required",
        "Enter your password to clear the package cache:",
        QLineEdit::Password, 
        "", 
        &ok);
    
    if (!ok || password.isEmpty()) {
        m_clearCacheButton->setEnabled(true);
        setCursor(Qt::ArrowCursor);
        m_maintenanceProgressBar->setVisible(false);
        m_maintenanceLogView->append("Operation cancelled by user.");
        return;
    }
    
    QtConcurrent::run([this, cleanAll, password]() {
        QProcess process;
        QString command = cleanAll ? "pacman -Scc --noconfirm" : "pacman -Sc --noconfirm";
        
        // Run the command with sudo
        process.start("sudo", QStringList() << "-S" << "-k" << "sh" << "-c" << command);
        process.write(password.toUtf8() + "\n");
        process.closeWriteChannel();
        process.waitForFinished(-1); // Wait indefinitely
        
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QString error = QString::fromUtf8(process.readAllStandardError());
        int exitCode = process.exitCode();
        
        // Update UI in main thread
        QMetaObject::invokeMethod(this, [this, exitCode, output, error]() {
            // Re-enable UI elements
            m_clearCacheButton->setEnabled(true);
            setCursor(Qt::ArrowCursor);
            m_maintenanceProgressBar->setVisible(false);
            
            if (exitCode == 0) {
                m_maintenanceLogView->append("Package cache cleared successfully.");
                if (!output.isEmpty()) {
                    m_maintenanceLogView->append("Output:");
                    for (const QString& line : output.split('\n', Qt::SkipEmptyParts)) {
                        m_maintenanceLogView->append("  " + line);
                    }
                }
            } else {
                m_maintenanceLogView->append("Failed to clear package cache:");
                if (!error.isEmpty()) {
                    for (const QString& line : error.split('\n', Qt::SkipEmptyParts)) {
                        m_maintenanceLogView->append("  " + line);
                    }
                }
            }
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onCheckDatabase()
{
    bool checkSyncDbs = m_checkSyncDbsCheckbox->isChecked();
    QString operation = checkSyncDbs ? "all" : "local";
    
    // Setup UI for operation
    m_checkDatabaseButton->setEnabled(false);
    setCursor(Qt::WaitCursor);
    m_maintenanceProgressBar->setVisible(true);
    m_maintenanceLogView->append("Checking " + operation + " databases...");
    
    QtConcurrent::run([this, checkSyncDbs]() {
        QProcess process;
        QStringList args;
        
        if (checkSyncDbs) {
            args << "-k" << "-Dkk";
        } else {
            args << "-k" << "-Dk";
        }
        
        process.start("pacman", args);
        process.waitForFinished(-1); // Wait indefinitely
        
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QString error = QString::fromUtf8(process.readAllStandardError());
        int exitCode = process.exitCode();
        
        // Update UI in main thread
        QMetaObject::invokeMethod(this, [this, exitCode, output, error, checkSyncDbs]() {
            // Re-enable UI elements
            m_checkDatabaseButton->setEnabled(true);
            setCursor(Qt::ArrowCursor);
            m_maintenanceProgressBar->setVisible(false);
            
            if (exitCode == 0) {
                m_maintenanceLogView->append("Database check completed successfully.");
                if (output.isEmpty()) {
                    m_maintenanceLogView->append("No issues found. All " + QString(checkSyncDbs ? "databases" : "packages") + " are valid.");
                } else {
                    m_maintenanceLogView->append("Results:");
                    for (const QString& line : output.split('\n', Qt::SkipEmptyParts)) {
                        m_maintenanceLogView->append("  " + line);
                    }
                }
            } else {
                m_maintenanceLogView->append("Database check failed:");
                if (!error.isEmpty()) {
                    for (const QString& line : error.split('\n', Qt::SkipEmptyParts)) {
                        m_maintenanceLogView->append("  " + line);
                    }
                }
            }
        }, Qt::QueuedConnection);
    });
}

void MainWindow::openSettings()
{
    std::cout << "MainWindow::openSettings - Opening settings dialog" << std::endl;
    
    // Create settings dialog
    SettingsDialog dialog(this);
    
    // Connect signals for theme changes
    connect(&dialog, &SettingsDialog::themeChanged, this, [this](bool isDark) {
        std::cout << "MainWindow::openSettings - Theme changed signal received, isDark: " << (isDark ? "true" : "false") << std::endl;
        applyTheme(isDark);
    });
    
    // Execute dialog
    int result = dialog.exec();
    std::cout << "MainWindow::openSettings - Dialog closed with result: " << result << std::endl;
    
    if (result == QDialog::Accepted) {
        std::cout << "MainWindow::openSettings - Settings accepted" << std::endl;
        
        // Update dark theme state
        bool darkTheme = dialog.isDarkThemeEnabled();
        std::cout << "MainWindow::openSettings - Dark theme enabled: " << (darkTheme ? "true" : "false") << std::endl;
        
        // Apply theme if it changed
        if (m_darkTheme != darkTheme) {
            std::cout << "MainWindow::openSettings - Theme changed, applying new theme" << std::endl;
            toggleTheme(darkTheme);
        } else {
            std::cout << "MainWindow::openSettings - Theme unchanged" << std::endl;
        }
        
        // Update AUR state
        bool aurEnabled = dialog.isAurEnabled();
        QSettings settings("PacmanGUI", "PacmanGUI");
        settings.setValue("aur/enabled", aurEnabled);
        
        // Update AUR actions enable state
        m_actions["aurInstall"]->setEnabled(aurEnabled);
        m_actions["aurUpdate"]->setEnabled(aurEnabled);
        
        // Show confirmation message
        std::cout << "MainWindow::openSettings - Settings applied successfully" << std::endl;
        statusBar()->showMessage("Settings updated successfully", 3000);
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About PacmanGUI",
        "<h2>PacmanGUI</h2>"
        "<p>A graphical user interface for the Arch Linux package manager pacman.</p>"
        "<p>Version 0.1</p>"
        "<p>Copyright © 2023</p>");
}

void MainWindow::toggleTheme()
{
    std::cout << "MainWindow::toggleTheme() - Toggling theme" << std::endl;
    toggleTheme(!m_darkTheme);
}

void MainWindow::toggleTheme(bool isDark)
{
    std::cout << "MainWindow::toggleTheme(bool) - Setting dark theme to: " << (isDark ? "true" : "false") << std::endl;
    m_darkTheme = isDark;
    applyTheme(m_darkTheme);
    
    // Save the setting
    QSettings settings("PacmanGUI", "PacmanGUI");
    settings.setValue("appearance/darkTheme", m_darkTheme);
    
    // Also update the theme name setting for consistency
    QString themeName;
    if (isDark) {
        bool colorful = settings.value("appearance/darkColorfulTheme", true).toBool();
        themeName = colorful ? "dark_colorful" : "dark";
    } else {
        bool colorful = settings.value("appearance/lightColorfulTheme", false).toBool();
        themeName = colorful ? "light_colorful" : "light";
    }
    
    std::cout << "MainWindow::toggleTheme(bool) - Setting theme name to: " << themeName.toStdString() << std::endl;
    settings.setValue("appearance/theme", themeName);
}

void MainWindow::loadSettings()
{
    QSettings settings("PacmanGUI", "PacmanGUI");
    m_darkTheme = settings.value("appearance/darkTheme", true).toBool();
}

void MainWindow::saveSettings()
{
    QSettings settings("PacmanGUI", "PacmanGUI");
    settings.setValue("appearance/darkTheme", m_darkTheme);
}

void MainWindow::applyTheme(bool isDark)
{
    QSettings settings("PacmanGUI", "PacmanGUI");
    QString selectedTheme = settings.value("appearance/theme", "dark_colorful").toString();
    
    std::cout << "MainWindow::applyTheme - Applying theme: " << selectedTheme.toStdString() << std::endl;
    
    // Set Fusion style as the base style
    qApp->setStyle("Fusion");
    std::cout << "MainWindow::applyTheme - Set base style to Fusion" << std::endl;
    
    // Apply color palette based on the selected theme
    QPalette palette;
    
    if (selectedTheme == "dark" || selectedTheme == "dark_colorful") {
        std::cout << "MainWindow::applyTheme - Applying dark palette" << std::endl;
        // Dark or Dark Colorful theme
        QColor darkColor = QColor(30, 30, 46); // Match #1e1e2e
        QColor disabledColor = QColor(166, 173, 200); // Match #a6adc8
        QColor baseColor = QColor(24, 24, 37); // Match #181825
        QColor textColor = QColor(248, 248, 242); // Match #f8f8f2
        QColor accentColor = QColor(30, 102, 245); // Match #1e66f5
        
        palette.setColor(QPalette::Window, darkColor);
        palette.setColor(QPalette::WindowText, textColor);
        palette.setColor(QPalette::Base, baseColor);
        palette.setColor(QPalette::AlternateBase, darkColor);
        palette.setColor(QPalette::ToolTipBase, textColor);
        palette.setColor(QPalette::ToolTipText, textColor);
        palette.setColor(QPalette::Text, textColor);
        palette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
        palette.setColor(QPalette::Button, darkColor);
        palette.setColor(QPalette::ButtonText, textColor);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
        palette.setColor(QPalette::BrightText, textColor);
        palette.setColor(QPalette::Link, accentColor);
        palette.setColor(QPalette::Highlight, accentColor);
        palette.setColor(QPalette::HighlightedText, textColor);
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);
    } else {
        std::cout << "MainWindow::applyTheme - Applying light palette" << std::endl;
        // Light or Light Colorful theme
        QColor lightColor = QColor(242, 244, 248); // Match #f2f4f8
        QColor textColor = QColor(24, 24, 37); // Match #181825
        QColor baseColor = QColor(255, 255, 255); // White
        QColor disabledColor = QColor(88, 91, 112); // Match #585b70
        QColor accentColor = QColor(30, 102, 245); // Match #1e66f5
        
        palette.setColor(QPalette::Window, lightColor);
        palette.setColor(QPalette::WindowText, textColor);
        palette.setColor(QPalette::Base, baseColor);
        palette.setColor(QPalette::AlternateBase, lightColor);
        palette.setColor(QPalette::ToolTipBase, baseColor);
        palette.setColor(QPalette::ToolTipText, textColor);
        palette.setColor(QPalette::Text, textColor);
        palette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
        palette.setColor(QPalette::Button, lightColor);
        palette.setColor(QPalette::ButtonText, textColor);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
        palette.setColor(QPalette::BrightText, textColor);
        palette.setColor(QPalette::Link, accentColor);
        palette.setColor(QPalette::Highlight, accentColor);
        palette.setColor(QPalette::HighlightedText, baseColor);
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);
    }
    
    // Apply the palette
    qApp->setPalette(palette);
    std::cout << "MainWindow::applyTheme - Applied palette to application" << std::endl;
    
    // Save the dark theme state for backward compatibility
    m_darkTheme = (selectedTheme == "dark" || selectedTheme == "dark_colorful");
    
    // Load the appropriate QSS file based on theme selection
    QString qssFile;
    if (selectedTheme == "dark_colorful") {
        qssFile = "dark_colorful.qss";
    } else if (selectedTheme == "light_colorful") {
        qssFile = "light_colorful.qss";
    } else if (selectedTheme == "dark") {
        qssFile = "dark.qss";
    } else { // light
        qssFile = "light.qss";
    }
    
    std::cout << "MainWindow::applyTheme - Selected QSS file: " << qssFile.toStdString() << std::endl;
    
    // Try to load the selected theme from various paths
    loadThemeStylesheet(qssFile);
    
    std::cout << "MainWindow::applyTheme - Theme application completed" << std::endl;
}

void MainWindow::searchPackages(const QString& searchTerm)
{
    if (searchTerm.isEmpty()) {
        return;
    }
    
    // Clear existing results
    m_packagesModel->setRowCount(0);
    
    // Show status message
    statusBar()->showMessage("Searching for packages...");
    
    // Set cursor
    setCursor(Qt::WaitCursor);
    
    // Execute search in a separate thread
    QtConcurrent::run([this, searchTerm]() {
        std::vector<pacmangui::core::Package> results = 
            m_packageManager.search_by_name(searchTerm.toStdString());
        
        // Update UI from the main thread
        QMetaObject::invokeMethod(this, [this, results, searchTerm]() {
            setCursor(Qt::ArrowCursor);
            
            // Fill results in table
            for (const auto& pkg : results) {
                QList<QStandardItem*> items;
                
                // Checkbox column for package selection
                QStandardItem* checkItem = new QStandardItem();
                checkItem->setCheckable(true);
                checkItem->setCheckState(Qt::Unchecked);
                
                // If already installed, add a visual indicator but keep it checkable
                if (m_packageManager.is_package_installed(pkg.get_name())) {
                    checkItem->setText("✓");
                    checkItem->setTextAlignment(Qt::AlignCenter);
                }
                items.append(checkItem);
                
                // Package information
                items.append(new QStandardItem(QString::fromStdString(pkg.get_name())));
                items.append(new QStandardItem(QString::fromStdString(pkg.get_version())));
                items.append(new QStandardItem(QString::fromStdString(pkg.get_repository())));
                items.append(new QStandardItem(QString::fromStdString(pkg.get_description())));
                
                m_packagesModel->appendRow(items);
            }
            
            // Using Stretch mode instead of resizing columns
            // m_packagesView->resizeColumnsToContents();
            
            // Show status message
            statusBar()->showMessage(
                QString("Found %1 packages matching '%2'")
                .arg(results.size())
                .arg(searchTerm), 
                3000);
        }, Qt::QueuedConnection);
    });
}

void MainWindow::refreshInstalledPackages()
{
    // Clear existing items
    m_installedModel->setRowCount(0);
    
    // Set cursor
    setCursor(Qt::WaitCursor);
    
    // Execute in separate thread
    QtConcurrent::run([this]() {
        std::vector<pacmangui::core::Package> installedPackages = 
            m_packageManager.get_installed_packages();
        
        // Update UI from the main thread
        QMetaObject::invokeMethod(this, [this, installedPackages]() {
            setCursor(Qt::ArrowCursor);
            
            // Fill results in table
            for (const auto& pkg : installedPackages) {
                QList<QStandardItem*> items;
                
                // Checkbox column for package selection
                QStandardItem* checkItem = new QStandardItem();
                checkItem->setCheckable(true);
                checkItem->setCheckState(Qt::Unchecked);
                checkItem->setText("✓");
                checkItem->setTextAlignment(Qt::AlignCenter);
                items.append(checkItem);
                
                // Package information
                items.append(new QStandardItem(QString::fromStdString(pkg.get_name())));
                items.append(new QStandardItem(QString::fromStdString(pkg.get_version())));
                items.append(new QStandardItem(QString::fromStdString(pkg.get_repository())));
                items.append(new QStandardItem(QString::fromStdString(pkg.get_description())));
                
                m_installedModel->appendRow(items);
            }
            
            // Using Stretch mode instead of resizing columns
            // m_installedView->resizeColumnsToContents();
            
            // Show status message
            statusBar()->showMessage(
                QString("Found %1 installed packages")
                .arg(installedPackages.size()), 
                3000);
        }, Qt::QueuedConnection);
    });
}

void MainWindow::refreshUpdatesList()
{
    // Clear current model
    m_systemUpdatesModel->removeRows(0, m_systemUpdatesModel->rowCount());
    
    // Set cursor
    setCursor(Qt::WaitCursor);
    
    // Execute in separate thread
    QThreadPool::globalInstance()->start([this]() {
        auto updates = m_packageManager.check_updates();
        
        // Update UI from the main thread
        QMetaObject::invokeMethod(this, [this, updates]() {
            setCursor(Qt::ArrowCursor);
            
            // Clear the updates model
            m_systemUpdatesModel->setRowCount(0);
            
            // Update system updates view
            for (const auto& update : updates) {
                QList<QStandardItem*> items;
                items.append(new QStandardItem(QString::fromStdString(update.first)));
                items.append(new QStandardItem(QString::fromStdString(update.second)));
                m_systemUpdatesModel->appendRow(items);
            }
            
            // Using Stretch mode instead of resizing columns
            // m_systemUpdatesView->resizeColumnsToContents();
            
            if (!updates.empty()) {
                // Update count in button
                m_systemUpdateButton->setText(QString("Update System (%1)").arg(updates.size()));
                m_systemUpdateButton->setEnabled(true);
                
                // Show status message
                statusBar()->showMessage(QString("%1 updates available").arg(updates.size()), 3000);
            } else {
                // No updates available
                m_systemUpdateButton->setText("Update System");
                m_systemUpdateButton->setEnabled(false);
                
                // Show status message
                statusBar()->showMessage("System is up to date", 3000);
            }
        }, Qt::QueuedConnection);
    });
}

void MainWindow::updateBatchInstallButton()
{
    m_batchInstallButton->setEnabled(!m_selectedPackages.isEmpty());
}

void MainWindow::onRemoveOrphans()
{
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this,
        "Remove Orphaned Packages",
        "Are you sure you want to remove all orphaned packages?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (confirm == QMessageBox::Yes) {
        // Ask for password
        bool ok;
        QString password = QInputDialog::getText(
            this, 
            "Authentication Required",
            "Enter your password to remove orphaned packages:",
            QLineEdit::Password, 
            "", 
            &ok);
        
        if (!ok || password.isEmpty()) {
            m_maintenanceLogView->append("Operation cancelled by user.");
            return;
        }
        
        m_findOrphansButton->setEnabled(false);
        m_removeOrphansButton->setEnabled(false);
        setCursor(Qt::WaitCursor);
        m_maintenanceProgressBar->setVisible(true);
        m_maintenanceLogView->append("Removing orphaned packages...");
        
        QThreadPool::globalInstance()->start([this, password]() {
            // Define output callback for real-time feedback
            auto outputCallback = [this](const std::string& output) {
                // Update log from the worker thread
                QMetaObject::invokeMethod(this, [this, line = QString::fromStdString(output)]() {
                    m_maintenanceLogView->append(line);
                    m_maintenanceLogView->verticalScrollBar()->setValue(
                        m_maintenanceLogView->verticalScrollBar()->maximum()
                    );
                    QApplication::processEvents();
                }, Qt::QueuedConnection);
            };
            
            bool success = m_packageManager.remove_orphaned_packages(
                password.toStdString(),
                outputCallback
            );
            
            // Update UI in main thread
            QMetaObject::invokeMethod(this, [this, success]() {
                // Update UI state
                m_findOrphansButton->setEnabled(true);
                m_removeOrphansButton->setEnabled(false);
                m_maintenanceProgressBar->setVisible(false);
                setCursor(Qt::ArrowCursor);
                
                // Update log
                if (success) {
                    m_maintenanceLogView->append("Orphaned packages removed successfully.");
                } else {
                    m_maintenanceLogView->append("Failed to remove orphaned packages: " + 
                        QString::fromStdString(m_packageManager.get_last_error()));
                }
            }, Qt::QueuedConnection);
        });
    }
}

void MainWindow::onFindPacnewFiles()
{
    m_findPacnewButton->setEnabled(false);
    setCursor(Qt::WaitCursor);
    m_maintenanceProgressBar->setVisible(true);
    m_maintenanceLogView->append("Searching for .pacnew and .pacsave files...");
    
    QThreadPool::globalInstance()->start([this]() {
        QStringList pacnewFiles;
        
        // Run the find command to locate .pacnew files
        QProcess process;
        process.start("find", QStringList() << "/" << "-name" << "*.pacnew" << "-o" << "-name" << "*.pacsave");
        process.waitForFinished();
        
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        pacnewFiles = output.split('\n', Qt::SkipEmptyParts);
        
        // Update UI in main thread
        QMetaObject::invokeMethod(this, [this, pacnewFiles]() {
            // Update UI state
            m_findPacnewButton->setEnabled(true);
            m_maintenanceProgressBar->setVisible(false);
            setCursor(Qt::ArrowCursor);
            
            // Update log
            if (pacnewFiles.isEmpty()) {
                m_maintenanceLogView->append("No .pacnew or .pacsave files found.");
            } else {
                m_maintenanceLogView->append("Found " + QString::number(pacnewFiles.size()) +
                    " .pacnew/.pacsave files:");
                
                // Display files in the log
                for (const auto& file : pacnewFiles) {
                    m_maintenanceLogView->append(" - " + file);
                }
            }
        }, Qt::QueuedConnection);
    });
}

void MainWindow::setupDetailPanel()
{
    // Create the detail panel widget that will slide in from the right
    m_detailPanel = new QWidget(this);
    m_detailPanel->setObjectName("detailPanel");
    m_detailPanel->setAutoFillBackground(true);
    
    // Set up opacity effect for fade-in animation
    m_opacityEffect = new QGraphicsOpacityEffect(m_detailPanel);
    m_opacityEffect->setOpacity(0.0);
    m_detailPanel->setGraphicsEffect(m_opacityEffect);
    
    // Calculate panel width (25% of main window width)
    int panelWidth = width() * SLIDE_PANEL_WIDTH_PERCENT / 100;
    
    // Position the panel initially outside the visible area
    m_detailPanel->setGeometry(width(), 0, panelWidth, height());
    m_detailPanel->setMinimumWidth(panelWidth);
    m_detailPanel->setMaximumWidth(panelWidth);
    
    // Style the panel with a border and background
    QPalette palette = m_detailPanel->palette();
    palette.setColor(QPalette::Window, QColor(240, 240, 240));
    if (m_darkTheme) {
        palette.setColor(QPalette::Window, QColor(45, 45, 45));
        palette.setColor(QPalette::WindowText, Qt::white);
    }
    m_detailPanel->setPalette(palette);
    
    // Create a layout for the panel content
    QVBoxLayout* panelLayout = new QVBoxLayout(m_detailPanel);
    panelLayout->setContentsMargins(15, 15, 15, 15);
    panelLayout->setSpacing(10);
    
    // Add a scroll area for the panel content
    m_detailScrollArea = new QScrollArea();
    m_detailScrollArea->setWidgetResizable(true);
    m_detailScrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget* scrollContent = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(15);
    
    // Add close button at the top
    QHBoxLayout* headerLayout = new QHBoxLayout();
    m_detailCloseButton = new QPushButton("×");
    m_detailCloseButton->setObjectName("detailCloseButton");
    m_detailCloseButton->setFixedSize(28, 28);
    m_detailCloseButton->setToolTip("Close detail panel");
    m_detailCloseButton->setCursor(Qt::PointingHandCursor);
    headerLayout->addStretch();
    headerLayout->addWidget(m_detailCloseButton);
    scrollLayout->addLayout(headerLayout);
    
    // Add package icon
    m_detailIcon = new QLabel();
    m_detailIcon->setAlignment(Qt::AlignCenter);
    m_detailIcon->setFixedSize(96, 96);
    m_detailIcon->setScaledContents(true);
    QIcon defaultIcon = QIcon::fromTheme("package", QIcon::fromTheme("application-x-executable"));
    m_detailIcon->setPixmap(defaultIcon.pixmap(96, 96));
    
    QHBoxLayout* iconLayout = new QHBoxLayout();
    iconLayout->addStretch();
    iconLayout->addWidget(m_detailIcon);
    iconLayout->addStretch();
    scrollLayout->addLayout(iconLayout);
    
    // Add package title
    m_detailTitle = new QLabel();
    m_detailTitle->setObjectName("detailTitle");
    m_detailTitle->setAlignment(Qt::AlignCenter);
    m_detailTitle->setWordWrap(true);
    QFont titleFont = m_detailTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_detailTitle->setFont(titleFont);
    scrollLayout->addWidget(m_detailTitle);
    
    // Add package version
    m_detailVersion = new QLabel();
    m_detailVersion->setObjectName("detailVersion");
    m_detailVersion->setAlignment(Qt::AlignCenter);
    scrollLayout->addWidget(m_detailVersion);
    
    // Add repository
    m_detailRepo = new QLabel();
    m_detailRepo->setObjectName("detailRepo");
    m_detailRepo->setAlignment(Qt::AlignCenter);
    scrollLayout->addWidget(m_detailRepo);
    
    // Add separator line
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    scrollLayout->addWidget(line);
    
    // Add description
    m_detailDescription = new QLabel();
    m_detailDescription->setObjectName("detailDescription");
    m_detailDescription->setWordWrap(true);
    m_detailDescription->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_detailDescription->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollLayout->addWidget(m_detailDescription);
    
    // Add spacing at the bottom
    scrollLayout->addStretch();
    
    // Set the scroll content
    m_detailScrollArea->setWidget(scrollContent);
    panelLayout->addWidget(m_detailScrollArea);
    
    // Create the slide animation
    m_slideAnimation = new QPropertyAnimation(m_detailPanel, "geometry");
    m_slideAnimation->setDuration(300);
    m_slideAnimation->setEasingCurve(QEasingCurve::OutCubic);
    
    // Connect signals for the detail panel
    connect(m_detailCloseButton, &QPushButton::clicked, this, &MainWindow::closeDetailPanel);
    connect(m_slideAnimation, &QPropertyAnimation::finished, this, &MainWindow::onDetailPanelAnimationFinished);
    
    // Initially hide the panel
    m_detailPanel->setVisible(false);
}

void MainWindow::showDetailPanel(const QString& packageName, const QString& version, const QString& repo, const QString& description)
{
    // Stop any running animations
    if (m_slideAnimation->state() == QPropertyAnimation::Running) {
        m_slideAnimation->stop();
    }
    
    // Update panel contents
    m_detailTitle->setText(packageName);
    m_detailVersion->setText("Version: " + version);
    m_detailRepo->setText("Repository: " + repo);
    m_detailDescription->setText(description);
    
    // Try to load package icon - using safe fallbacks
    QIcon packageIcon;
    QString iconName = packageName.toLower();
    
    if (QIcon::hasThemeIcon(iconName)) {
        packageIcon = QIcon::fromTheme(iconName);
    } else if (QIcon::hasThemeIcon("package")) {
        packageIcon = QIcon::fromTheme("package");
    } else if (QIcon::hasThemeIcon("application-x-executable")) {
        packageIcon = QIcon::fromTheme("application-x-executable");
    } else {
        // Last resort, create a simple colored square
        QPixmap pixmap(96, 96);
        pixmap.fill(m_darkTheme ? QColor(64, 81, 181) : QColor(41, 121, 255));
        packageIcon = QIcon(pixmap);
    }
    
    // Set the icon pixmap safely
    m_detailIcon->setPixmap(packageIcon.pixmap(96, 96));
    
    // Make sure panel is the correct size
    int panelWidth = width() * SLIDE_PANEL_WIDTH_PERCENT / 100;
    m_detailPanel->setMinimumWidth(panelWidth);
    m_detailPanel->setMaximumWidth(panelWidth);
    
    // Prepare the animation
    QRect startGeometry = QRect(width(), 0, panelWidth, height());
    QRect endGeometry = QRect(width() - panelWidth, 0, panelWidth, height());
    
    m_detailPanel->setGeometry(startGeometry);
    m_detailPanel->setVisible(true);
    
    // Start the slide animation
    m_slideAnimation->setStartValue(startGeometry);
    m_slideAnimation->setEndValue(endGeometry);
    m_slideAnimation->start();
    
    // Animate opacity separately (safely)
    m_opacityEffect->setOpacity(0.0);
    QPropertyAnimation* fadeAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    fadeAnimation->setDuration(300);
    fadeAnimation->setStartValue(0.0);
    fadeAnimation->setEndValue(1.0);
    fadeAnimation->start(QPropertyAnimation::DeleteWhenStopped);
    
    m_detailPanelVisible = true;
}

void MainWindow::closeDetailPanel()
{
    if (!m_detailPanelVisible) return;
    
    // Stop any running animations
    if (m_slideAnimation->state() == QPropertyAnimation::Running) {
        m_slideAnimation->stop();
    }
    
    // Prepare the animation
    int panelWidth = m_detailPanel->width();
    QRect startGeometry = m_detailPanel->geometry();
    QRect endGeometry = QRect(width(), 0, panelWidth, height());
    
    // Start the slide animation
    m_slideAnimation->setStartValue(startGeometry);
    m_slideAnimation->setEndValue(endGeometry);
    m_slideAnimation->start();
    
    // Animate opacity separately (safely)
    QPropertyAnimation* fadeAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    fadeAnimation->setDuration(300);
    fadeAnimation->setStartValue(m_opacityEffect->opacity());
    fadeAnimation->setEndValue(0.0);
    fadeAnimation->start(QPropertyAnimation::DeleteWhenStopped);
    
    m_detailPanelVisible = false;
}

void MainWindow::onDetailPanelAnimationFinished()
{
    // Hide the panel if the animation was closing it
    if (!m_detailPanelVisible) {
        m_detailPanel->setVisible(false);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // Adjust detail panel size and position when window is resized
    if (m_detailPanel) {
        int panelWidth = width() * SLIDE_PANEL_WIDTH_PERCENT / 100;
        m_detailPanel->setMinimumWidth(panelWidth);
        m_detailPanel->setMaximumWidth(panelWidth);
        
        if (m_detailPanelVisible) {
            // Reposition the panel so it stays visible at the right edge
            m_detailPanel->setGeometry(width() - panelWidth, 0, panelWidth, height());
        } else {
            // Keep the panel outside the visible area
            m_detailPanel->setGeometry(width(), 0, panelWidth, height());
        }
    }
}

// Implement package selection handlers
void MainWindow::onPackageSelected(const QModelIndex& index)
{
    if (!index.isValid()) return;
    
    // Get the row of the selected item
    int row = index.row();
    
    // Get package information from the model
    QString packageName = m_packagesModel->index(row, 1).data().toString();
    QString version = m_packagesModel->index(row, 2).data().toString();
    QString repo = m_packagesModel->index(row, 3).data().toString();
    QString description = m_packagesModel->index(row, 4).data().toString();
    
    // Show the package detail in the sliding panel only if we're in the All Packages tab
    if (m_tabWidget->currentIndex() == 0) {
        showDetailPanel(packageName, version, repo, description);
        
        // Show the bottom details panel
        m_packageNameLabel->setText(packageName);
        m_packageVersionLabel->setText("Version: " + version);
        m_packageDescLabel->setText(description);
        
        // Disconnect previous connections
        disconnect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onInstallPackage);
        disconnect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onRemovePackage);
        disconnect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onUpdatePackage);
        
        bool isInstalled = !m_packagesModel->index(row, 0).data().toString().isEmpty();
        if (isInstalled) {
            m_actionButton->setText("Remove");
            m_actionButton->setIcon(QIcon::fromTheme("edit-delete"));
            connect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onRemovePackage, Qt::UniqueConnection);
        } else {
            m_actionButton->setText("Install");
            m_actionButton->setIcon(QIcon::fromTheme("system-software-install"));
            connect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onInstallPackage, Qt::UniqueConnection);
        }
        
        m_detailsWidget->setVisible(true);
    }
}

void MainWindow::onInstalledPackageSelected(const QModelIndex& index)
{
    if (!index.isValid()) return;
    
    // Get the row of the selected item
    int row = index.row();
    
    // Get package information from the model
    QString packageName = m_installedModel->index(row, 1).data().toString();
    QString version = m_installedModel->index(row, 2).data().toString();
    QString repo = m_installedModel->index(row, 3).data().toString();
    QString description = m_installedModel->index(row, 4).data().toString();
    
    // Show the package detail in the sliding panel only if we're in the Installed tab
    if (m_tabWidget->currentIndex() == 1) {
        showDetailPanel(packageName, version, repo, description);
        
        // Show the bottom details panel
        m_packageNameLabel->setText(packageName);
        m_packageVersionLabel->setText("Version: " + version);
        m_packageDescLabel->setText(description);
        
        // Disconnect previous connections
        disconnect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onInstallPackage);
        disconnect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onRemovePackage);
        disconnect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onUpdatePackage);
        
        m_actionButton->setText("Remove");
        m_actionButton->setIcon(QIcon::fromTheme("edit-delete"));
        connect(m_actionButton, &QPushButton::clicked, this, &MainWindow::onRemovePackage, Qt::UniqueConnection);
        
        m_detailsWidget->setVisible(true);
    }
}

void MainWindow::loadThemeStylesheet(const QString& fileName)
{
    std::cout << "MainWindow::loadThemeStylesheet - Loading stylesheet: " << fileName.toStdString() << std::endl;
    
    // Try to load from resource
    QFile styleFile(":/styles/" + fileName);
    std::cout << "MainWindow::loadThemeStylesheet - Attempting to load from resource path: :/styles/" << fileName.toStdString() << std::endl;
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString style = styleFile.readAll();
        qApp->setStyleSheet(style);
        styleFile.close();
        std::cout << "Successfully loaded " << fileName.toStdString() << " stylesheet from resource" << std::endl;
        return;
    } else {
        std::cerr << "MainWindow::loadThemeStylesheet - Failed to load from resource: " << styleFile.errorString().toStdString() << std::endl;
    }
    
    // First fallback: Try with different resource path
    QFile alternativeStyleFile(":/resources/styles/" + fileName);
    std::cout << "MainWindow::loadThemeStylesheet - Attempting to load from alternative resource path: :/resources/styles/" << fileName.toStdString() << std::endl;
    if (alternativeStyleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString style = alternativeStyleFile.readAll();
        qApp->setStyleSheet(style);
        alternativeStyleFile.close();
        std::cout << "Successfully loaded " << fileName.toStdString() << " stylesheet from alternative resource path" << std::endl;
        return;
    } else {
        std::cerr << "MainWindow::loadThemeStylesheet - Failed to load from alternative resource: " << alternativeStyleFile.errorString().toStdString() << std::endl;
    }
    
    // Second fallback: Try loading from the file system relative to executable
    QFile localStyleFile("resources/styles/" + fileName);
    std::cout << "MainWindow::loadThemeStylesheet - Attempting to load from local file: resources/styles/" << fileName.toStdString() << std::endl;
    if (localStyleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString style = localStyleFile.readAll();
        qApp->setStyleSheet(style);
        localStyleFile.close();
        std::cout << "Successfully loaded " << fileName.toStdString() << " stylesheet from local file" << std::endl;
        return;
    } else {
        std::cerr << "MainWindow::loadThemeStylesheet - Failed to load from local file: " << localStyleFile.errorString().toStdString() << std::endl;
    }
    
    // Third fallback: Try absolute path
    QString sourcePath = QDir::currentPath() + "/../resources/styles/" + fileName;
    std::cout << "MainWindow::loadThemeStylesheet - Attempting to load from source path: " << sourcePath.toStdString() << std::endl;
    QFile sourceStyleFile(sourcePath);
    if (sourceStyleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString style = sourceStyleFile.readAll();
        qApp->setStyleSheet(style);
        sourceStyleFile.close();
        std::cout << "Successfully loaded " << fileName.toStdString() << " stylesheet from source path: " << sourcePath.toStdString() << std::endl;
        return;
    } else {
        std::cerr << "MainWindow::loadThemeStylesheet - Failed to load from source path: " << sourceStyleFile.errorString().toStdString() << std::endl;
    }
    
    // If all fails, try loading dark_colorful.qss as a fallback
    if (fileName != "dark_colorful.qss") {
        std::cerr << "Failed to load " << fileName.toStdString() << " stylesheet from all paths, trying dark_colorful.qss as fallback" << std::endl;
        loadThemeStylesheet("dark_colorful.qss");
    } else {
        std::cerr << "Failed to load all stylesheets" << std::endl;
    }
}

void MainWindow::onPackageItemChanged(QStandardItem *item)
{
    // Only process checkbox column (column 0)
    if (item->column() != 0) return;
    
    // Get the package name from column 1 (same row)
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(sender());
    if (!model) return;
    
    QString packageName = model->item(item->row(), 1)->text();
    
    // Handle the checkbox state
    if (item->checkState() == Qt::Checked) {
        m_selectedPackages.insert(packageName);
    } else {
        m_selectedPackages.remove(packageName);
    }
    
    // Update batch install button state
    updateBatchInstallButton();
}

} // namespace gui
} // namespace pacmangui