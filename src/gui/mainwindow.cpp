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
    , m_detailsWidget(nullptr)
    , m_packageNameLabel(nullptr)
    , m_packageVersionLabel(nullptr)
    , m_packageDescLabel(nullptr)
    , m_actionButton(nullptr)
    , m_packagesModel(nullptr)
    , m_installedModel(nullptr)
    , m_darkTheme(false)
    , m_systemUpdateTab(nullptr)
    , m_systemUpdateInfoLabel(nullptr)
    , m_systemUpdateButton(nullptr)
    , m_checkUpdatesButton(nullptr)
    , m_systemUpdateLogView(nullptr)
    , m_systemUpdatesView(nullptr)
    , m_systemUpdatesModel(nullptr)
    , m_systemUpdateOverwriteCheckbox(nullptr)
    , m_packageOverwriteCheckbox(nullptr)
    , m_selectedPackages()
    , m_settingsDialog(nullptr)
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
    delete m_systemUpdatesModel;
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
    m_packagesView->horizontalHeader()->setStretchLastSection(true);
    m_packagesView->verticalHeader()->setVisible(false);
    m_packagesView->setSortingEnabled(true);
    
    m_packagesModel = new QStandardItemModel(0, 4, this);
    m_packagesModel->setHorizontalHeaderLabels(QStringList() << "" << "Name" << "Version" << "Description");
    m_packagesView->setModel(m_packagesModel);
    
    // Set column widths and alignment
    m_packagesView->setColumnWidth(0, 30); // Checkbox column
    m_packagesView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_packagesView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed); // Fix the checkbox column width
    
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
    m_installedModel->setHorizontalHeaderLabels(QStringList() << "" << "Name" << "Version" << "Description");
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
    
    QAction* systemUpdateAction = new QAction("System Update", this);
    systemUpdateAction->setStatusTip("Update all packages on the system");
    connect(systemUpdateAction, &QAction::triggered, this, &MainWindow::onSystemUpdate);
    
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
    toolbar->addAction(systemUpdateAction);
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
    packageMenu->addSeparator();
    packageMenu->addAction("System Update", this, &MainWindow::onSystemUpdate);
    
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
        QString description = m_packagesModel->data(m_packagesModel->index(row, 3)).toString();
        
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
    });
    
    // Also handle row clicks for installed packages view
    connect(m_installedView, &QTableView::clicked, [this](const QModelIndex &index) {
        // Show details for the clicked package
        int row = index.row();
        QString packageName = m_installedModel->data(m_installedModel->index(row, 1)).toString();
        QString version = m_installedModel->data(m_installedModel->index(row, 2)).toString();
        QString description = m_installedModel->data(m_installedModel->index(row, 3)).toString();
        
        m_packageNameLabel->setText(packageName);
        m_packageVersionLabel->setText("Version: " + version);
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
    
    // Search for packages
    std::vector<core::Package> results = m_packageManager.search_by_name(searchTerm.toStdString());
    
    // Display results
    for (const auto& pkg : results) {
        QString packageName = QString::fromStdString(pkg.get_name());
        
        QList<QStandardItem*> row;
        
        // Create checkbox item
        QStandardItem* checkItem = new QStandardItem();
        checkItem->setCheckable(true);
        checkItem->setCheckState(Qt::Unchecked);
        checkItem->setTextAlignment(Qt::AlignCenter); // Center the checkmark
        
        row << checkItem;
        row << new QStandardItem(packageName);
        row << new QStandardItem(QString::fromStdString(pkg.get_version()));
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
    
    // Get installed packages
    std::vector<core::Package> installed = m_packageManager.get_installed_packages();
    
    // Display packages
    for (const auto& pkg : installed) {
        QString packageName = QString::fromStdString(pkg.get_name());
        
        QList<QStandardItem*> row;
        
        // Create checkbox item
        QStandardItem* checkItem = new QStandardItem();
        checkItem->setCheckable(true);
        checkItem->setCheckState(Qt::Unchecked);
        checkItem->setTextAlignment(Qt::AlignCenter); // Center the checkmark
        
        row << checkItem;
        row << new QStandardItem(packageName);
        row << new QStandardItem(QString::fromStdString(pkg.get_version()));
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
    // Create settings dialog if it doesn't exist
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
    }
    
    // Show the dialog and apply settings if accepted
    if (m_settingsDialog->exec() == QDialog::Accepted) {
        // Load settings that may have changed
        bool isDarkTheme = QSettings("PacmanGUI", "PacmanGUI").value("appearance/darkTheme", m_darkTheme).toBool();
        
        // Apply theme if it changed
        if (isDarkTheme != m_darkTheme) {
            applyTheme(isDarkTheme);
        }
        
        // Other settings can be applied here
        statusBar()->showMessage("Settings updated", 3000);
    }
}

} // namespace gui
} // namespace pacmangui 