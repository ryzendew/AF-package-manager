#include "gui/flatpak_manager_tab.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLineEdit>
#include <QTreeView>
#include <QHeaderView>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QUrl>
#include <QApplication>
#include <QClipboard>
#include <QProgressDialog>
#include <QDateTime>
#include <QFormLayout>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QMenu>
#include <QAction>
#include <QStandardPaths>
#include <QDebug>
#include <QRegularExpression>
#include <QScrollArea>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QCheckBox>
#include "gui/flatpak_process_dialog.hpp"

namespace pacmangui {
namespace gui {

FlatpakManagerTab::FlatpakManagerTab(QWidget* parent, core::PackageManager* packageManager)
    : QWidget(parent)
    , m_packageManager(packageManager)
{
    qDebug() << "FlatpakManagerTab::constructor - start";
    setupUi();
    qDebug() << "FlatpakManagerTab::constructor - after setupUi";
    connectSignals();
    qDebug() << "FlatpakManagerTab::constructor - after connectSignals";
    // Populate initial data
    refreshFlatpakList();
    qDebug() << "FlatpakManagerTab::constructor - after refreshFlatpakList";
    refreshFlatpakRemotes();
    qDebug() << "FlatpakManagerTab::constructor - after refreshFlatpakRemotes";
    qDebug() << "FlatpakManagerTab::constructor - end";
}

FlatpakManagerTab::~FlatpakManagerTab()
{
    // Clean up any resources if needed
}

void FlatpakManagerTab::setupUi()
{
    qDebug() << "FlatpakManagerTab::setupUi - start";
    // Only setup if Flatpak is available
    if (!m_packageManager->is_flatpak_available()) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        QLabel* notAvailableLabel = new QLabel(tr("Flatpak is not available on this system."), this);
        notAvailableLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(notAvailableLabel);
        return;
    }

    // Create the main splitter
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);

    // ===== LEFT PANEL =====
    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    // Search bar section (always visible)
    QHBoxLayout* searchBarLayout = new QHBoxLayout();
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText(tr("Search for Flatpak packages..."));
    m_searchInput->setMinimumHeight(32);
    m_searchInput->setProperty("class", "search-input");
    searchBarLayout->addWidget(m_searchInput);
    m_searchButton = new QPushButton(tr("Search"), this);
    m_searchButton->setMinimumHeight(32);
    m_searchButton->setProperty("class", "primary-button");
    searchBarLayout->addWidget(m_searchButton);
    leftLayout->addLayout(searchBarLayout);
    qDebug() << "FlatpakManagerTab::setupUi - search bar created";

    // Results table (always visible)
    m_searchResultsView = new QTreeView(this);
    m_searchResultsView->setAlternatingRowColors(true);
    m_searchResultsView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_searchResultsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_searchResultsView->setSortingEnabled(true);
    m_searchResultsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_searchResultsView->setRootIsDecorated(false);
    m_searchResultsView->setMinimumHeight(200);
    m_searchResultsView->setProperty("class", "search-results");
    m_searchResultsView->setFrameShape(QFrame::NoFrame);
    m_searchResultsModel = new QStandardItemModel(0, 5, this);
    m_searchResultsModel->setHorizontalHeaderLabels(
        QStringList() << tr("Name") << tr("Application ID") << tr("Version") << tr("Remote") << tr("Description"));
    m_searchResultsView->setModel(m_searchResultsModel);
    m_searchResultsView->setColumnWidth(0, 200);
    m_searchResultsView->setColumnWidth(1, 250);
    m_searchResultsView->setColumnWidth(2, 100);
    m_searchResultsView->setColumnWidth(3, 100);
    m_searchResultsView->header()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_searchResultsView->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_searchResultsView->header()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_searchResultsView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_searchResultsView->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_searchResultsView->header()->setSectionResizeMode(4, QHeaderView::Stretch);
    leftLayout->addWidget(m_searchResultsView);
    qDebug() << "FlatpakManagerTab::setupUi - search results view created";

    // Install button for search results
    m_installSelectedButton = new QPushButton(tr("Install Selected"), this);
    m_installSelectedButton->setMinimumHeight(32);
    m_installSelectedButton->setProperty("class", "primary-button");
    m_installSelectedButton->setEnabled(false);
    leftLayout->addWidget(m_installSelectedButton);
    qDebug() << "FlatpakManagerTab::setupUi - install button created";

    // Installed packages section
    QGroupBox* installedGroup = new QGroupBox(tr("Installed Packages"), leftPanel);
    QVBoxLayout* installedLayout = new QVBoxLayout(installedGroup);
    installedLayout->setContentsMargins(8, 16, 8, 8);
    installedLayout->setSpacing(8);

    // Filter input for installed packages
    QHBoxLayout* filterLayout = new QHBoxLayout();
    m_filterInput = new QLineEdit(installedGroup);
    m_filterInput->setPlaceholderText(tr("Filter installed packages..."));
    m_filterInput->setMinimumHeight(32);
    m_filterInput->setProperty("class", "filter-input");
    filterLayout->addWidget(m_filterInput);
    installedLayout->addLayout(filterLayout);

    // Installed packages list
    m_listView = new QTreeView(installedGroup);
    m_listView->setAlternatingRowColors(true);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_listView->setSortingEnabled(true);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setRootIsDecorated(false);
    m_listView->setMinimumHeight(300);
    m_listView->setProperty("class", "list-view");
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listModel = new QStandardItemModel(0, 4, this);
    m_listModel->setHorizontalHeaderLabels(
        QStringList() << tr("Name") << tr("Application ID") << tr("Version") << tr("Origin"));
    m_listView->setModel(m_listModel);
    installedLayout->addWidget(m_listView);
    leftLayout->addWidget(installedGroup);

    // ===== RIGHT PANEL =====
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setProperty("class", "details-panel");

    QWidget* rightPanel = new QWidget();
    scrollArea->setWidget(rightPanel);

    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    rightLayout->setSpacing(16);
    rightLayout->setAlignment(Qt::AlignTop);
    
    // Application Details Group
    QGroupBox* detailsGroup = new QGroupBox(tr("Application Details"), rightPanel);
    detailsGroup->setProperty("class", "details-group");
    QGridLayout* detailsLayout = new QGridLayout(detailsGroup);
    detailsLayout->setColumnStretch(1, 1);
    detailsLayout->setVerticalSpacing(8);
    detailsLayout->setHorizontalSpacing(12);
    
    int row = 0;
    
    // Name
    QLabel* nameTitle = new QLabel(tr("Name:"), detailsGroup);
    nameTitle->setProperty("class", "details-label");
    m_nameLabel = new QLabel(detailsGroup);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailsLayout->addWidget(nameTitle, row, 0);
    detailsLayout->addWidget(m_nameLabel, row++, 1);
    
    // Version
    QLabel* versionTitle = new QLabel(tr("Version:"), detailsGroup);
    versionTitle->setProperty("class", "details-label");
    m_versionLabel = new QLabel(detailsGroup);
    m_versionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailsLayout->addWidget(versionTitle, row, 0);
    detailsLayout->addWidget(m_versionLabel, row++, 1);
    
    // Branch
    QLabel* branchTitle = new QLabel(tr("Branch:"), detailsGroup);
    branchTitle->setProperty("class", "details-label");
    m_branchLabel = new QLabel(detailsGroup);
    m_branchLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailsLayout->addWidget(branchTitle, row, 0);
    detailsLayout->addWidget(m_branchLabel, row++, 1);
    
    // Origin
    QLabel* originTitle = new QLabel(tr("Origin:"), detailsGroup);
    originTitle->setProperty("class", "details-label");
    m_originLabel = new QLabel(detailsGroup);
    m_originLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailsLayout->addWidget(originTitle, row, 0);
    detailsLayout->addWidget(m_originLabel, row++, 1);
    
    // Installation
    QLabel* installationTitle = new QLabel(tr("Installation:"), detailsGroup);
    installationTitle->setProperty("class", "details-label");
    m_installationLabel = new QLabel(detailsGroup);
    m_installationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailsLayout->addWidget(installationTitle, row, 0);
    detailsLayout->addWidget(m_installationLabel, row++, 1);
    
    // Size
    QLabel* sizeTitle = new QLabel(tr("Size:"), detailsGroup);
    sizeTitle->setProperty("class", "details-label");
    m_sizeLabel = new QLabel(detailsGroup);
    m_sizeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailsLayout->addWidget(sizeTitle, row, 0);
    detailsLayout->addWidget(m_sizeLabel, row++, 1);
    
    // Runtime
    QLabel* runtimeTitle = new QLabel(tr("Runtime:"), detailsGroup);
    runtimeTitle->setProperty("class", "details-label");
    m_runtimeLabel = new QLabel(detailsGroup);
    m_runtimeLabel->setWordWrap(true);
    m_runtimeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailsLayout->addWidget(runtimeTitle, row, 0);
    detailsLayout->addWidget(m_runtimeLabel, row++, 1);
    
    // Description
    QLabel* descriptionTitle = new QLabel(tr("Description:"), detailsGroup);
    descriptionTitle->setProperty("class", "details-label");
    m_descriptionLabel = new QLabel(detailsGroup);
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailsLayout->addWidget(descriptionTitle, row, 0);
    detailsLayout->addWidget(m_descriptionLabel, row++, 1);
    
    rightLayout->addWidget(detailsGroup);
    
    // Permissions Group
    QGroupBox* permissionsGroup = new QGroupBox(tr("Permissions"), rightPanel);
    permissionsGroup->setProperty("class", "details-group");
    QVBoxLayout* permissionsLayout = new QVBoxLayout(permissionsGroup);
    permissionsLayout->setContentsMargins(12, 16, 12, 12);
    permissionsLayout->setSpacing(8);

    // Create a scroll area for permissions
    QScrollArea* permissionsScroll = new QScrollArea(permissionsGroup);
    permissionsScroll->setWidgetResizable(true);
    permissionsScroll->setFrameShape(QFrame::NoFrame);
    permissionsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    QWidget* permissionsContainer = new QWidget(permissionsScroll);
    QVBoxLayout* permissionsContainerLayout = new QVBoxLayout(permissionsContainer);
    permissionsContainerLayout->setContentsMargins(0, 0, 0, 0);
    permissionsContainerLayout->setSpacing(12);

    // Create sections for different permission categories
    m_filesystemPermsWidget = new QWidget(permissionsContainer);
    m_filesystemPermsWidget->setProperty("class", "permission-section");
    QVBoxLayout* filesystemLayout = new QVBoxLayout(m_filesystemPermsWidget);
    filesystemLayout->setContentsMargins(0, 0, 0, 0);
    filesystemLayout->setSpacing(4);
    
    QLabel* filesystemTitle = new QLabel(tr("📁 Filesystem Access"), m_filesystemPermsWidget);
    filesystemTitle->setProperty("class", "permission-category");
    filesystemLayout->addWidget(filesystemTitle);
    m_filesystemPermsText = new QLabel(m_filesystemPermsWidget);
    m_filesystemPermsText->setWordWrap(true);
    m_filesystemPermsText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_filesystemPermsText->setProperty("class", "permission-content");
    filesystemLayout->addWidget(m_filesystemPermsText);
    permissionsContainerLayout->addWidget(m_filesystemPermsWidget);

    m_devicePermsWidget = new QWidget(permissionsContainer);
    m_devicePermsWidget->setProperty("class", "permission-section");
    QVBoxLayout* deviceLayout = new QVBoxLayout(m_devicePermsWidget);
    deviceLayout->setContentsMargins(0, 0, 0, 0);
    deviceLayout->setSpacing(4);
    
    QLabel* deviceTitle = new QLabel(tr("🔌 Device Access"), m_devicePermsWidget);
    deviceTitle->setProperty("class", "permission-category");
    deviceLayout->addWidget(deviceTitle);
    m_devicePermsText = new QLabel(m_devicePermsWidget);
    m_devicePermsText->setWordWrap(true);
    m_devicePermsText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_devicePermsText->setProperty("class", "permission-content");
    deviceLayout->addWidget(m_devicePermsText);
    permissionsContainerLayout->addWidget(m_devicePermsWidget);

    m_featurePermsWidget = new QWidget(permissionsContainer);
    m_featurePermsWidget->setProperty("class", "permission-section");
    QVBoxLayout* featureLayout = new QVBoxLayout(m_featurePermsWidget);
    featureLayout->setContentsMargins(0, 0, 0, 0);
    featureLayout->setSpacing(4);
    
    QLabel* featureTitle = new QLabel(tr("⚙️ Features & Session"), m_featurePermsWidget);
    featureTitle->setProperty("class", "permission-category");
    featureLayout->addWidget(featureTitle);
    m_featurePermsText = new QLabel(m_featurePermsWidget);
    m_featurePermsText->setWordWrap(true);
    m_featurePermsText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_featurePermsText->setProperty("class", "permission-content");
    featureLayout->addWidget(m_featurePermsText);
    permissionsContainerLayout->addWidget(m_featurePermsWidget);

    m_socketPermsWidget = new QWidget(permissionsContainer);
    m_socketPermsWidget->setProperty("class", "permission-section");
    QVBoxLayout* socketLayout = new QVBoxLayout(m_socketPermsWidget);
    socketLayout->setContentsMargins(0, 0, 0, 0);
    socketLayout->setSpacing(4);
    
    QLabel* socketTitle = new QLabel(tr("🔗 Socket & Network"), m_socketPermsWidget);
    socketTitle->setProperty("class", "permission-category");
    socketLayout->addWidget(socketTitle);
    m_socketPermsText = new QLabel(m_socketPermsWidget);
    m_socketPermsText->setWordWrap(true);
    m_socketPermsText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_socketPermsText->setProperty("class", "permission-content");
    socketLayout->addWidget(m_socketPermsText);
    permissionsContainerLayout->addWidget(m_socketPermsWidget);

    m_otherPermsWidget = new QWidget(permissionsContainer);
    m_otherPermsWidget->setProperty("class", "permission-section");
    QVBoxLayout* otherLayout = new QVBoxLayout(m_otherPermsWidget);
    otherLayout->setContentsMargins(0, 0, 0, 0);
    otherLayout->setSpacing(4);
    
    QLabel* otherTitle = new QLabel(tr("🔧 Other Permissions"), m_otherPermsWidget);
    otherTitle->setProperty("class", "permission-category");
    otherLayout->addWidget(otherTitle);
    m_otherPermsText = new QLabel(m_otherPermsWidget);
    m_otherPermsText->setWordWrap(true);
    m_otherPermsText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_otherPermsText->setProperty("class", "permission-content");
    otherLayout->addWidget(m_otherPermsText);
    permissionsContainerLayout->addWidget(m_otherPermsWidget);

    permissionsContainerLayout->addStretch();
    permissionsScroll->setWidget(permissionsContainer);
    permissionsLayout->addWidget(permissionsScroll);

    rightLayout->addWidget(permissionsGroup);
    
    // Actions Group
    QGroupBox* actionsGroup = new QGroupBox(tr("Actions"), rightPanel);
    actionsGroup->setProperty("class", "details-group");
    QGridLayout* actionsLayout = new QGridLayout(actionsGroup);
    actionsLayout->setSpacing(8);
    
    m_manageUserDataButton = new QPushButton(tr("Manage User Data"), actionsGroup);
    m_uninstallButton = new QPushButton(tr("Uninstall"), actionsGroup);
    m_removeDataButton = new QPushButton(tr("Remove Data"), actionsGroup);
    m_createSnapshotButton = new QPushButton(tr("Create Snapshot"), actionsGroup);
    m_restoreSnapshotButton = new QPushButton(tr("Restore Snapshot"), actionsGroup);
    
    // Set button properties
    QList<QPushButton*> buttons = {
        m_manageUserDataButton, m_uninstallButton, m_removeDataButton,
        m_createSnapshotButton, m_restoreSnapshotButton
    };
    
    for (QPushButton* button : buttons) {
        button->setMinimumHeight(32);
        button->setEnabled(false);
    }
    
    m_uninstallButton->setProperty("class", "danger-button");
    m_removeDataButton->setProperty("class", "danger-button");
    
    actionsLayout->addWidget(m_manageUserDataButton, 0, 0);
    actionsLayout->addWidget(m_uninstallButton, 0, 1);
    actionsLayout->addWidget(m_removeDataButton, 1, 0);
    actionsLayout->addWidget(m_createSnapshotButton, 1, 1);
    actionsLayout->addWidget(m_restoreSnapshotButton, 2, 0, 1, 2);
    
    rightLayout->addWidget(actionsGroup);
    
    rightLayout->addStretch();
    
    // Add panels to splitter
    m_splitter->addWidget(leftPanel);
    m_splitter->addWidget(scrollArea);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 2);

    // Set the main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_splitter);

    // Set initial values for the right panel
    m_nameLabel->setText(tr("Name"));
    m_versionLabel->setText(tr("Version"));
    m_branchLabel->setText(tr("Branch"));
    m_originLabel->setText(tr("Origin"));
    m_installationLabel->setText(tr("Installation"));
    m_sizeLabel->setText(tr("Size"));
    m_runtimeLabel->setText(tr("Runtime"));
    m_descriptionLabel->setText(tr("Description"));
    
    // Disable detail buttons initially
    m_manageUserDataButton->setEnabled(false);
    m_uninstallButton->setEnabled(false);
    m_removeDataButton->setEnabled(false);
    m_createSnapshotButton->setEnabled(false);
    m_restoreSnapshotButton->setEnabled(false);
    
    // Show initial package details if available
    QTimer::singleShot(0, this, [this]() {
        qDebug() << "QTimer::singleShot - checking for initial selection";
        if (m_listModel && m_listModel->rowCount() > 0) {
            int row = 0;
            qDebug() << "QTimer::singleShot - setting current index to row" << row;
            QModelIndex idx = m_listModel->index(row, 0);
            if (idx.isValid()) {
                m_listView->setCurrentIndex(idx);
            } else {
                qDebug() << "QTimer::singleShot - index is not valid for row" << row;
            }
        } else {
            qDebug() << "QTimer::singleShot - no rows in m_listModel or m_listModel is null";
        }
    });
    qDebug() << "FlatpakManagerTab::setupUi - end";
}

void FlatpakManagerTab::connectSignals()
{
    qDebug() << "FlatpakManagerTab::connectSignals - start";
    connect(m_searchInput, &QLineEdit::textChanged, this, &FlatpakManagerTab::onSearchTextChanged);
    connect(m_searchButton, &QPushButton::clicked, this, &FlatpakManagerTab::onSearchButtonClicked);
    connect(m_searchResultsView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &FlatpakManagerTab::onSearchResultSelected);
    connect(m_installSelectedButton, &QPushButton::clicked, this, &FlatpakManagerTab::onInstallSelected);
    
    connect(m_filterInput, &QLineEdit::textChanged, this, &FlatpakManagerTab::filterFlatpakList);
    connect(m_listView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &FlatpakManagerTab::onFlatpakSelected);
    connect(m_manageUserDataButton, &QPushButton::clicked, this, &FlatpakManagerTab::onManageUserData);
    connect(m_uninstallButton, &QPushButton::clicked, this, &FlatpakManagerTab::onUninstall);
    connect(m_removeDataButton, &QPushButton::clicked, this, &FlatpakManagerTab::onRemoveUserData);
    connect(m_createSnapshotButton, &QPushButton::clicked, this, &FlatpakManagerTab::onCreateSnapshot);
    connect(m_restoreSnapshotButton, &QPushButton::clicked, this, &FlatpakManagerTab::onRestoreSnapshot);
    qDebug() << "FlatpakManagerTab::connectSignals - end";
}

void FlatpakManagerTab::refreshFlatpakList()
{
    qDebug() << "FlatpakManagerTab::refreshFlatpakList - start";
    // Clear existing data
    m_listModel->clear();
    m_listModel->setHorizontalHeaderLabels(
        QStringList() << tr("Name") << tr("Application ID") << tr("Version") << tr("Origin"));
    
    // Get all installed Flatpak packages
    std::vector<pacmangui::core::FlatpakPackage> flatpaks = m_packageManager->get_installed_flatpak_packages();
    
    // Add to model
    for (const auto& pkg : flatpaks) {
        QList<QStandardItem*> row;
        
        QStandardItem* nameItem = new QStandardItem(QString::fromStdString(pkg.get_name()));
        QStandardItem* appIdItem = new QStandardItem(QString::fromStdString(pkg.get_app_id()));
        QStandardItem* versionItem = new QStandardItem(QString::fromStdString(pkg.get_version()));
        QStandardItem* originItem = new QStandardItem(QString::fromStdString(pkg.get_repository()));
        
        // Store app ID for selection handling
        nameItem->setData(QString::fromStdString(pkg.get_app_id()), Qt::UserRole);
        
        row << nameItem << appIdItem << versionItem << originItem;
        m_listModel->appendRow(row);
    }
    
    // Auto-size columns with better proportions
    m_listView->header()->setSectionResizeMode(0, QHeaderView::Interactive);  // Name - Interactive to allow manual sizing
    m_listView->header()->setSectionResizeMode(1, QHeaderView::Stretch);      // App ID - Stretch to fill space
    m_listView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Version - Content size
    m_listView->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Origin - Content size
    
    // Set reasonable initial column widths
    m_listView->setColumnWidth(0, 150);  // Name column
    
    // Update status
    emit statusMessage(tr("Loaded %1 Flatpak applications").arg(flatpaks.size()), 3000);
    qDebug() << "FlatpakManagerTab::refreshFlatpakList - end";
}

void FlatpakManagerTab::refreshFlatpakRemotes()
{
    qDebug() << "FlatpakManagerTab::refreshFlatpakRemotes - start";
    // Run flatpak remotes command
    QProcess process;
    process.start("flatpak", QStringList() << "remotes");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    // Update status
    emit statusMessage(tr("Found %1 Flatpak remotes").arg(lines.size()), 3000);
    qDebug() << "FlatpakManagerTab::refreshFlatpakRemotes - end";
}

// Implementation of filterFlatpakList
void FlatpakManagerTab::filterFlatpakList(const QString& filter) {
    // Apply filter to the flatpak list view
    if (filter.isEmpty()) {
        // If filter is empty, show all rows
        for (int row = 0; row < m_listModel->rowCount(); ++row) {
            m_listView->setRowHidden(row, QModelIndex(), false);
        }
        return;
    }
    
    // Filter rows
    for (int row = 0; row < m_listModel->rowCount(); ++row) {
        bool match = false;
        
        // Check all columns for match
        for (int col = 0; col < m_listModel->columnCount(); ++col) {
            QModelIndex index = m_listModel->index(row, col);
            QString text = m_listModel->data(index).toString();
            
            if (text.contains(filter, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        
        // Show or hide row based on match
        m_listView->setRowHidden(row, QModelIndex(), !match);
    }
}

// Implementation of onFlatpakSelected
void FlatpakManagerTab::onFlatpakSelected(const QModelIndex& current, const QModelIndex& previous)
{
    qDebug() << "onFlatpakSelected - start";
    if (m_selectingFlatpak) {
        qDebug() << "onFlatpakSelected - guard active, skipping";
        return;
    }
    m_selectingFlatpak = true;
    Q_UNUSED(previous);
    if (!current.isValid()) {
        qDebug() << "onFlatpakSelected - current index is not valid";
        if (m_nameLabel) m_nameLabel->clear();
        if (m_versionLabel) m_versionLabel->clear();
        if (m_branchLabel) m_branchLabel->clear();
        if (m_originLabel) m_originLabel->clear();
        if (m_installationLabel) m_installationLabel->clear();
        if (m_sizeLabel) m_sizeLabel->clear();
        if (m_runtimeLabel) m_runtimeLabel->clear();
        if (m_descriptionLabel) m_descriptionLabel->clear();
        if (m_manageUserDataButton) m_manageUserDataButton->setEnabled(false);
        if (m_uninstallButton) m_uninstallButton->setEnabled(false);
        if (m_removeDataButton) m_removeDataButton->setEnabled(false);
        if (m_createSnapshotButton) m_createSnapshotButton->setEnabled(false);
        if (m_restoreSnapshotButton) m_restoreSnapshotButton->setEnabled(false);
        m_selectingFlatpak = false;
        return;
    }
    qDebug() << "onFlatpakSelected - current index is valid, row:" << current.row();
    QModelIndex nameIndex = m_listModel ? m_listModel->index(current.row(), 1) : QModelIndex();
    QString appId = nameIndex.isValid() ? m_listModel->data(nameIndex).toString() : QString();
    qDebug() << "onFlatpakSelected - appId:" << appId;
    if (!appId.isEmpty()) {
        updateFlatpakDetails(appId);
    } else {
        qDebug() << "onFlatpakSelected - appId is empty, skipping updateFlatpakDetails";
    }
    if (m_manageUserDataButton) m_manageUserDataButton->setEnabled(true);
    if (m_uninstallButton) m_uninstallButton->setEnabled(true);
    if (m_removeDataButton) m_removeDataButton->setEnabled(true);
    if (m_createSnapshotButton) m_createSnapshotButton->setEnabled(true);
    if (m_restoreSnapshotButton) m_restoreSnapshotButton->setEnabled(true);
    m_selectingFlatpak = false;
    qDebug() << "onFlatpakSelected - end";
}

// Stub implementations for all the other required methods
void FlatpakManagerTab::onManageUserData() {
    QString appId = getCurrentAppId();
    if (appId.isEmpty()) return;
    
    emit statusMessage(tr("Managing user data for %1").arg(appId), 3000);
    // Real implementation would open a file manager to the user data location
}

void FlatpakManagerTab::onUninstall() {
    QString appId = getCurrentAppId();
    if (appId.isEmpty()) return;

    // Get the remote from the current selection
    QModelIndex current = m_listView->currentIndex();
    if (!current.isValid()) return;
    QModelIndex originIndex = m_listModel->index(current.row(), 3); // Origin/Remote column
    QString remote = m_listModel->data(originIndex).toString();
    
    // Call the uninstallFlatpak method which handles the dialogs
    uninstallFlatpak(appId, remote);
}

void FlatpakManagerTab::onRemoveUserData() {
    QString appId = getCurrentAppId();
    if (appId.isEmpty()) return;
    
    emit statusMessage(tr("Removing user data for %1").arg(appId), 3000);
    // Real implementation would confirm with the user and then remove the data
}

void FlatpakManagerTab::onCreateSnapshot() {
    QString appId = getCurrentAppId();
    if (appId.isEmpty()) return;
    
    emit statusMessage(tr("Creating snapshot for %1").arg(appId), 3000);
    // Real implementation would create a backup of the user data
}

void FlatpakManagerTab::onRestoreSnapshot() {
    QString appId = getCurrentAppId();
    if (appId.isEmpty()) return;
    
    emit statusMessage(tr("Restoring snapshot for %1").arg(appId), 3000);
    // Real implementation would let the user select a snapshot to restore
}

void FlatpakManagerTab::onInstallSelected()
{
    QModelIndex current = m_searchResultsView->currentIndex();
    if (!current.isValid()) return;
    
    // Get the app ID and remote from the stored data
    QModelIndex nameIndex = m_searchResultsModel->index(current.row(), 0);
    QString appId = m_searchResultsModel->data(nameIndex, Qt::UserRole).toString();
    QString remote = m_searchResultsModel->data(nameIndex, Qt::UserRole + 1).toString();
    
    if (appId.isEmpty() || remote.isEmpty()) {
        emit statusMessage(tr("Invalid package data"), 3000);
        return;
    }
    
    // Install the package
    installFlatpak(appId, remote);
}

void FlatpakManagerTab::updateFlatpakDetails(const QString& appId)
{
    qDebug() << "updateFlatpakDetails - start for appId:" << appId;
    if (appId.isEmpty()) {
        qDebug() << "updateFlatpakDetails - appId is empty, returning";
        return;
    }
    // Get package details
    auto packages = m_packageManager->get_installed_flatpak_packages();
    qDebug() << "updateFlatpakDetails - got" << packages.size() << "installed packages";
    auto it = std::find_if(packages.begin(), packages.end(),
        [&appId](const auto& pkg) {
            return QString::fromStdString(pkg.get_app_id()) == appId;
        });
    if (it == packages.end()) {
        qDebug() << "updateFlatpakDetails - package not found for appId:" << appId;
        return;
    }
    const auto& package = *it;
    qDebug() << "updateFlatpakDetails - found package, updating labels";
    // Update labels with package information
    m_nameLabel->setText(QString::fromStdString(package.get_name()));
    m_versionLabel->setText(QString::fromStdString(package.get_version()));
    m_branchLabel->setText(QString::fromStdString(package.get_branch()));
    m_originLabel->setText(QString::fromStdString(package.get_repository()));
    m_installationLabel->setText(package.is_system_wide() ? tr("System") : tr("User"));
    m_sizeLabel->setText(QString::fromStdString(package.get_size()));
    m_runtimeLabel->setText(QString::fromStdString(package.get_runtime()));
    m_descriptionLabel->setText(QString::fromStdString(package.get_description()));
    // Clear all previous permissions first
    m_filesystemPermsText->clear();
    m_devicePermsText->clear();
    m_featurePermsText->clear();
    m_socketPermsText->clear();
    m_otherPermsText->clear();
    // Hide all permission sections initially
    m_filesystemPermsWidget->setVisible(false);
    m_devicePermsWidget->setVisible(false);
    m_featurePermsWidget->setVisible(false);
    m_socketPermsWidget->setVisible(false);
    m_otherPermsWidget->setVisible(false);
    // Get permissions
    QProcess process;
    process.start("flatpak", QStringList() << "info" << "--show-permissions" << appId);
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    qDebug() << "Flatpak permissions output:" << output;
    // Parse and categorize permissions
    QStringList lines = output.split('\n');
    QStringList filesystemPerms, devicePerms, featurePerms, socketPerms, otherPerms;
    bool inPermissions = false;
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        qDebug() << "Processing line:" << trimmed;
        if (trimmed.startsWith("Permissions:")) {
            inPermissions = true;
            continue;
        }
        if (inPermissions) {
            if (trimmed.isEmpty()) {
                continue;
            }
            QString perm = trimmed;
            qDebug() << "Found permission:" << perm;
            if (perm.contains("filesystem=") || perm.contains("home") || perm.contains("host") || 
                perm.contains("xdg-") || perm.contains("~/"))
                filesystemPerms << "• " + perm;
            else if (perm.contains("device=") || perm.contains("dri") || perm.contains("audio") || 
                     perm.contains("video") || perm.contains("usb"))
                devicePerms << "• " + perm;
            else if (perm.contains("socket=") || perm.contains("network") || perm.contains("wayland") || 
                     perm.contains("x11") || perm.contains("pulseaudio"))
                socketPerms << "• " + perm;
            else if (perm.contains("session") || perm.contains("feature") || perm.contains("system-") || 
                     perm.contains("dbus"))
                featurePerms << "• " + perm;
            else
                otherPerms << "• " + perm;
        }
    }
    // If no permissions found, show a message
    if (filesystemPerms.isEmpty() && devicePerms.isEmpty() && 
        featurePerms.isEmpty() && socketPerms.isEmpty() && otherPerms.isEmpty()) {
        m_filesystemPermsWidget->setVisible(true);
        m_filesystemPermsText->setText(tr("No special permissions required"));
        m_devicePermsWidget->setVisible(false);
        m_featurePermsWidget->setVisible(false);
        m_socketPermsWidget->setVisible(false);
        m_otherPermsWidget->setVisible(false);
    } else {
        if (!filesystemPerms.isEmpty()) {
            m_filesystemPermsWidget->setVisible(true);
            m_filesystemPermsText->setText(filesystemPerms.join('\n'));
        }
        if (!devicePerms.isEmpty()) {
            m_devicePermsWidget->setVisible(true);
            m_devicePermsText->setText(devicePerms.join('\n'));
        }
        if (!featurePerms.isEmpty()) {
            m_featurePermsWidget->setVisible(true);
            m_featurePermsText->setText(featurePerms.join('\n'));
        }
        if (!socketPerms.isEmpty()) {
            m_socketPermsWidget->setVisible(true);
            m_socketPermsText->setText(socketPerms.join('\n'));
        }
        if (!otherPerms.isEmpty()) {
            m_otherPermsWidget->setVisible(true);
            m_otherPermsText->setText(otherPerms.join('\n'));
        }
    }
    // Update user data info
    updateUserDataInfo(appId);
    qDebug() << "updateFlatpakDetails - end for appId:" << appId;
}

void FlatpakManagerTab::updateUserDataInfo(const QString& appId)
{
    qDebug() << "updateUserDataInfo - start for appId:" << appId;
    QString dataPath = getDataPath(appId);
    qDebug() << "updateUserDataInfo - dataPath:" << dataPath;
    QString dataSize = calculateDirSize(dataPath);
    qDebug() << "updateUserDataInfo - dataSize:" << dataSize;
    // Update status
    emit statusMessage(tr("User data size: %1").arg(dataSize), 3000);
    qDebug() << "updateUserDataInfo - end for appId:" << appId;
}

QString FlatpakManagerTab::calculateDirSize(const QString& path) {
    if (path.isEmpty() || !QDir(path).exists()) {
        return tr("Unknown");
    }

    QProcess process;
    process.start("du", QStringList() << "-sh" << path);
    process.waitForFinished();

    if (process.exitCode() == 0) {
        QString output = process.readAllStandardOutput();
        // du output format is: "size    path"
        return output.split('\t').first().trimmed();
    }

    return tr("Unknown");
}

QString FlatpakManagerTab::getDataPath(const QString& appId) {
    if (appId.isEmpty()) {
        return tr("Unknown");
    }

    // Flatpak data is stored in ~/.var/app/<app-id> for user installations
    QString userPath = QDir::homePath() + "/.var/app/" + appId;
    if (QDir(userPath).exists()) {
        return userPath;
    }

    // For system-wide installations, data is in /var/lib/flatpak/app/<app-id>
    QString systemPath = "/var/lib/flatpak/app/" + appId;
    if (QDir(systemPath).exists()) {
        return systemPath;
    }

    return tr("Unknown");
}

QString FlatpakManagerTab::getCurrentAppId() const
{
    QModelIndex current = m_listView->currentIndex();
    if (!current.isValid()) return QString();
    
    QModelIndex nameIndex = m_listModel->index(current.row(), 1); // Application ID column
    return m_listModel->data(nameIndex).toString();
}

// Add a new method to install Flatpak packages
void FlatpakManagerTab::installFlatpak(const QString& appId, const QString& remote) {
    if (appId.isEmpty() || remote.isEmpty()) {
        QMessageBox::warning(this, tr("Installation Error"), 
                            tr("Application ID and remote are required for installation."));
        return;
    }
    // Confirm with the user
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Confirm Installation"),
                                 tr("Are you sure you want to install %1 from %2?").arg(appId).arg(remote),
                                 QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    QString cmd = QString("flatpak install -y %1 %2").arg(remote, appId);
    QString title = tr("Installing %1 from %2...").arg(appId, remote);
    FlatpakProcessDialog dlg(cmd, title, this);
    dlg.setSuccessString("Installation complete.");
    dlg.exec();
    if (dlg.wasSuccessful()) {
        emit statusMessage(tr("Successfully installed %1").arg(appId), 3000);
        refreshFlatpakList();
    } else {
        QMessageBox::critical(this, tr("Installation Failed"),
                             tr("Failed to install %1. Check the output for errors.").arg(appId));
        emit statusMessage(tr("Failed to install %1").arg(appId), 3000);
    }
}

void FlatpakManagerTab::onSearchTextChanged(const QString& text)
{
    // Enable search button only if we have at least 2 characters
    m_searchButton->setEnabled(text.length() >= 2);
    
    // Auto-search after a delay if we have enough characters
    if (text.length() >= 2) {
        QTimer::singleShot(500, this, [this, text]() {
            if (m_searchInput->text() == text) {
                applySearchFilters(text);
            }
        });
    } else {
        // Clear results if search text is too short
        m_searchResultsModel->clear();
        m_searchResultsModel->setHorizontalHeaderLabels(
            QStringList() << tr("Name") << tr("Application ID") << tr("Version") << tr("Remote") << tr("Description"));
    }
}

void FlatpakManagerTab::onSearchButtonClicked()
{
    QString searchText = m_searchInput->text();
    if (searchText.length() < 2) {
        emit statusMessage(tr("Please enter at least 2 characters to search"), 3000);
        return;
    }
    
    applySearchFilters(searchText);
}

void FlatpakManagerTab::applySearchFilters(const QString& searchTerm) {
    m_currentFilter = searchTerm;
    performAsyncSearch(searchTerm);
}

void FlatpakManagerTab::clearSearchCache() {
    m_searchCache.clear();
}

void FlatpakManagerTab::addToSearchCache(const QString& searchTerm, const std::vector<pacmangui::core::FlatpakPackage>& results) {
    // Remove old entries
    QDateTime now = QDateTime::currentDateTime();
    m_searchCache.erase(
        std::remove_if(m_searchCache.begin(), m_searchCache.end(),
            [now](const SearchCacheEntry& entry) {
                return entry.timestamp.msecsTo(now) > CACHE_TIMEOUT_MS;
            }
        ),
        m_searchCache.end()
    );

    // Add new entry
    SearchCacheEntry entry;
    entry.searchTerm = searchTerm;
    entry.results = results;
    entry.timestamp = now;
    m_searchCache.prepend(entry);

    // Limit cache size
    if (m_searchCache.size() > MAX_CACHE_SIZE) {
        m_searchCache.removeLast();
    }
}

bool FlatpakManagerTab::getFromSearchCache(const QString& searchTerm, std::vector<pacmangui::core::FlatpakPackage>& results) {
    QDateTime now = QDateTime::currentDateTime();
    for (auto it = m_searchCache.begin(); it != m_searchCache.end(); ++it) {
        if (it->searchTerm == searchTerm && it->timestamp.msecsTo(now) <= CACHE_TIMEOUT_MS) {
            results = it->results;
            return true;
        }
    }
    return false;
}

void FlatpakManagerTab::filterResults(std::vector<pacmangui::core::FlatpakPackage>& results) {
    results.erase(
        std::remove_if(results.begin(), results.end(),
            [this](const pacmangui::core::FlatpakPackage& package) {
                // Filter by installation status
                if (m_showInstalledOnly && !package.is_installed()) {
                    return true;
                }

                // Filter by system/user apps
                bool isSystemApp = package.get_repository() == "system";
                if (isSystemApp && !m_showSystemApps) {
                    return true;
                }
                if (!isSystemApp && !m_showUserApps) {
                    return true;
                }

                return false;
            }
        ),
        results.end()
    );
}

void FlatpakManagerTab::performAsyncSearch(const QString& searchTerm) {
    // Clear previous results
    m_searchResultsModel->clear();
    m_searchResultsModel->setHorizontalHeaderLabels(
        QStringList() << tr("Name") << tr("Application ID") << tr("Version") << tr("Remote") << tr("Description"));
    
    // Show searching status
    emit statusMessage(tr("Searching for Flatpak packages matching '%1'...").arg(searchTerm), 0);
    
    // Check cache first
    std::vector<pacmangui::core::FlatpakPackage> cachedResults;
    if (getFromSearchCache(searchTerm, cachedResults)) {
        // Apply filters to cached results
        filterResults(cachedResults);
        
        // Update UI with filtered results
        updateSearchResults(cachedResults);
        return;
    }
    
    // Create a new watcher if needed
    if (!m_searchWatcher) {
        m_searchWatcher = new QFutureWatcher<std::vector<pacmangui::core::FlatpakPackage>>(this);
        connect(m_searchWatcher, &QFutureWatcher<std::vector<pacmangui::core::FlatpakPackage>>::finished,
                this, &FlatpakManagerTab::onSearchCompleted);
    }
    
    // Start the async search
    auto future = QtConcurrent::run([this, searchTerm]() {
        return m_packageManager->search_flatpak_by_name(searchTerm.toStdString());
    });
    
    m_searchWatcher->setFuture(future);
}

void FlatpakManagerTab::onSearchCompleted() {
    auto results = m_searchWatcher->result();
    
    // Cache the results
    addToSearchCache(m_searchInput->text(), results);
    
    // Apply filters
    filterResults(results);
    
    // Update UI with filtered results
    updateSearchResults(results);
}

void FlatpakManagerTab::updateSearchResults(const std::vector<pacmangui::core::FlatpakPackage>& results) {
    // Clear previous results
    m_searchResultsModel->clear();
    m_searchResultsModel->setHorizontalHeaderLabels(
        QStringList() << tr("Name") << tr("Application ID") << tr("Version") << tr("Remote") << tr("Description"));

    qDebug() << "Updating search results model with" << results.size() << "results.";
    // Update the search results model
    for (const auto& package : results) {
        QList<QStandardItem*> row;
        // Get a more user-friendly name
        QString displayName = QString::fromStdString(package.get_name());
        if (displayName.isEmpty() || displayName == QString::fromStdString(package.get_app_id())) {
            QString appId = QString::fromStdString(package.get_app_id());
            QStringList parts = appId.split(".");
            if (parts.size() >= 3) {
                displayName = parts.last();
                displayName[0] = displayName[0].toUpper();
                displayName.replace(".", " ");
            }
        }
        QStandardItem* nameItem = new QStandardItem(displayName);
        QStandardItem* appIdItem = new QStandardItem(QString::fromStdString(package.get_app_id()));
        QStandardItem* versionItem = new QStandardItem(QString::fromStdString(package.get_version()));
        QStandardItem* remoteItem = new QStandardItem(QString::fromStdString(package.get_repository()));
        QStandardItem* descItem = new QStandardItem(QString::fromStdString(package.get_description()));
        nameItem->setData(QString::fromStdString(package.get_app_id()), Qt::UserRole);
        nameItem->setData(QString::fromStdString(package.get_repository()), Qt::UserRole + 1);
        nameItem->setToolTip(displayName);
        appIdItem->setToolTip(QString::fromStdString(package.get_app_id()));
        descItem->setToolTip(QString::fromStdString(package.get_description()));
        row << nameItem << appIdItem << versionItem << remoteItem << descItem;
        m_searchResultsModel->appendRow(row);
    }
    
    // Ensure column sizes are maintained
    m_searchResultsView->setColumnWidth(0, 200);  // Name
    m_searchResultsView->setColumnWidth(1, 250);  // Application ID
    m_searchResultsView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Version
    m_searchResultsView->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Remote
    m_searchResultsView->header()->setSectionResizeMode(4, QHeaderView::Stretch); // Description
    
    // Update status
    if (results.empty()) {
        emit statusMessage(tr("No packages found matching '%1'").arg(m_searchInput->text()), 3000);
    } else {
        emit statusMessage(tr("Found %1 package(s)").arg(results.size()), 3000);
    }
}

void FlatpakManagerTab::onSearchResultSelected(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);
    m_installSelectedButton->setEnabled(current.isValid());
}

void FlatpakManagerTab::resizeEvent(QResizeEvent* event)
{
    qDebug() << "FlatpakManagerTab::resizeEvent - start";
    QWidget::resizeEvent(event);
    // Maintain 75/25 split
    if (m_splitter) {
        int totalWidth = m_splitter->width();
        QList<int> sizes;
        sizes << (totalWidth * 75) / 100 << (totalWidth * 25) / 100;
        m_splitter->setSizes(sizes);
        qDebug() << "FlatpakManagerTab::resizeEvent - splitter resized to" << sizes;
    } else {
        qDebug() << "FlatpakManagerTab::resizeEvent - m_splitter is null, skipping resize logic";
    }
    qDebug() << "FlatpakManagerTab::resizeEvent - end";
}

void FlatpakManagerTab::setShowInstalledOnly(bool show) {
    m_showInstalledOnly = show;
    applySearchFilters(m_searchInput->text());
}

void FlatpakManagerTab::setShowSystemApps(bool show) {
    m_showSystemApps = show;
    applySearchFilters(m_searchInput->text());
}

void FlatpakManagerTab::setShowUserApps(bool show) {
    m_showUserApps = show;
    applySearchFilters(m_searchInput->text());
}

void FlatpakManagerTab::uninstallFlatpak(const QString& appId, const QString& remote) {
    if (appId.isEmpty()) {
        QMessageBox::warning(this, tr("Uninstall Error"),
                            tr("Application ID is required for uninstallation."));
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Confirm Uninstall"),
                                 tr("Are you sure you want to uninstall %1? This cannot be undone.").arg(appId),
                                 QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;

    QString cmd = QString("flatpak uninstall -y %1").arg(appId);
    QString title = tr("Uninstalling %1...").arg(appId);
    
    FlatpakProcessDialog* dialog = new FlatpakProcessDialog(cmd, title, this);
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->setWindowTitle(tr("Uninstalling Application"));
    connect(dialog, &FlatpakProcessDialog::processFinished, this, [this, dialog, appId](bool success) {
        QMessageBox* completionDialog = new QMessageBox(this);
        completionDialog->setWindowModality(Qt::ApplicationModal);
        completionDialog->setWindowTitle(tr("Uninstallation Complete"));
        if (success) {
            completionDialog->setIcon(QMessageBox::Information);
            completionDialog->setText(tr("Successfully uninstalled %1").arg(appId));
        } else {
            completionDialog->setIcon(QMessageBox::Warning);
            completionDialog->setText(tr("Failed to uninstall %1").arg(appId));
        }
        completionDialog->setStandardButtons(QMessageBox::Close);
        completionDialog->exec();
        if (success) {
            refreshFlatpakList();
        }
    });
    dialog->exec();
    dialog->deleteLater();
}

} // namespace gui
} // namespace pacmangui 
