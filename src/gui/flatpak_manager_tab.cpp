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

namespace pacmangui {
namespace gui {

FlatpakManagerTab::FlatpakManagerTab(QWidget* parent, core::PackageManager* packageManager)
    : QWidget(parent)
    , m_packageManager(packageManager)
{
    setupUi();
    connectSignals();
    
    // Populate initial data
    refreshFlatpakList();
    refreshFlatpakRemotes();
}

FlatpakManagerTab::~FlatpakManagerTab()
{
    // Clean up any resources if needed
}

void FlatpakManagerTab::setupUi()
{
    // Only setup if Flatpak is available
    if (!m_packageManager->is_flatpak_available()) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        QLabel* notAvailableLabel = new QLabel(tr("Flatpak is not available on this system."), this);
        notAvailableLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(notAvailableLabel);
        return;
    }
    
    // Create the main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Create the splitter
    m_splitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(m_splitter);
    
    // ===== LEFT PANEL =====
    QWidget* leftPanel = new QWidget(m_splitter);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(4, 4, 4, 4);
    leftLayout->setSpacing(4);
    
    // Search input
    QHBoxLayout* topBarLayout = new QHBoxLayout();
    
    m_searchInput = new QLineEdit(leftPanel);
    m_searchInput->setPlaceholderText(tr("Filter Flatpaks..."));
    m_searchInput->setMinimumHeight(24);
    
    m_installNewButton = new QPushButton(tr("Install New"), leftPanel);
    m_installNewButton->setToolTip(tr("Install a new Flatpak application"));
    m_installNewButton->setMaximumHeight(24);
    
    topBarLayout->addWidget(m_searchInput);
    topBarLayout->addWidget(m_installNewButton);
    
    leftLayout->addLayout(topBarLayout);
    
    // Flatpak list view
    m_listView = new QTreeView(leftPanel);
    m_listView->setAlternatingRowColors(true);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_listView->setSortingEnabled(true);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setRootIsDecorated(false);
    m_listView->setMinimumWidth(250);
    m_listView->setMinimumHeight(600); // Make the list view taller
    
    m_listModel = new QStandardItemModel(0, 4, this);
    m_listModel->setHorizontalHeaderLabels(
        QStringList() << tr("Name") << tr("Application ID") << tr("Version") << tr("Origin"));
    m_listView->setModel(m_listModel);
    
    leftLayout->addWidget(m_listView);
    
    // ===== RIGHT PANEL =====
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    QWidget* rightPanel = new QWidget();
    scrollArea->setWidget(rightPanel);
    m_splitter->addWidget(scrollArea);
    
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(8, 8, 8, 8);
    rightLayout->setSpacing(4); // Reduce spacing between elements
    rightLayout->setAlignment(Qt::AlignTop);
    
    // ---- Application Details Group ----
    QGroupBox* detailsGroup = new QGroupBox(tr("Application Details"), rightPanel);
    QFormLayout* detailsLayout = new QFormLayout(detailsGroup);
    detailsLayout->setContentsMargins(6, 6, 6, 6); // Reduce margins
    detailsLayout->setSpacing(2); // Reduce spacing
    detailsLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    
    m_nameLabel = new QLabel(tr(""), detailsGroup);
    m_versionLabel = new QLabel(tr(""), detailsGroup);
    m_branchLabel = new QLabel(tr(""), detailsGroup);
    m_originLabel = new QLabel(tr(""), detailsGroup);
    m_installationLabel = new QLabel(tr(""), detailsGroup);
    m_sizeLabel = new QLabel(tr(""), detailsGroup);
    m_runtimeLabel = new QLabel(tr(""), detailsGroup);
    m_descriptionLabel = new QLabel(tr(""), detailsGroup);
    m_descriptionLabel->setWordWrap(true);
    
    detailsLayout->addRow(tr("Name:"), m_nameLabel);
    detailsLayout->addRow(tr("Version:"), m_versionLabel);
    detailsLayout->addRow(tr("Branch:"), m_branchLabel);
    detailsLayout->addRow(tr("Origin:"), m_originLabel);
    detailsLayout->addRow(tr("Installation:"), m_installationLabel);
    detailsLayout->addRow(tr("Size:"), m_sizeLabel);
    detailsLayout->addRow(tr("Runtime:"), m_runtimeLabel);
    detailsLayout->addRow(tr("Description:"), m_descriptionLabel);
    
    rightLayout->addWidget(detailsGroup);
    
    // ---- Action Buttons ----
    QGridLayout* actionButtonsLayout = new QGridLayout();
    actionButtonsLayout->setSpacing(4);
    
    m_manageUserDataButton = new QPushButton(tr("Manage User Data"), rightPanel);
    m_uninstallButton = new QPushButton(tr("Uninstall"), rightPanel);
    m_removeDataButton = new QPushButton(tr("Remove Data"), rightPanel);
    m_createSnapshotButton = new QPushButton(tr("Create Snapshot"), rightPanel);
    m_restoreSnapshotButton = new QPushButton(tr("Restore Snapshot"), rightPanel);
    
    // Set fixed height for all buttons
    QList<QPushButton*> buttons = {m_manageUserDataButton, m_uninstallButton, m_removeDataButton, 
                                  m_createSnapshotButton, m_restoreSnapshotButton};
    for (auto* button : buttons) {
        button->setMaximumHeight(24);
        button->setMinimumHeight(24);
    }
    
    actionButtonsLayout->addWidget(m_manageUserDataButton, 0, 0);
    actionButtonsLayout->addWidget(m_uninstallButton, 0, 1);
    actionButtonsLayout->addWidget(m_removeDataButton, 1, 0);
    actionButtonsLayout->addWidget(m_createSnapshotButton, 1, 1);
    actionButtonsLayout->addWidget(m_restoreSnapshotButton, 2, 0, 1, 2);
    
    rightLayout->addLayout(actionButtonsLayout);
    
    // ---- Permissions Group ----
    QGroupBox* permissionsGroup = new QGroupBox(tr("Permissions"), rightPanel);
    QVBoxLayout* permissionsLayout = new QVBoxLayout(permissionsGroup);
    permissionsLayout->setContentsMargins(6, 6, 6, 6);
    permissionsLayout->setSpacing(2);
    
    m_permissionsText = new QTextEdit(permissionsGroup);
    m_permissionsText->setReadOnly(true);
    m_permissionsText->setMinimumHeight(100);
    m_permissionsText->setMaximumHeight(100);
    m_permissionsText->setFrameShape(QFrame::NoFrame);
    m_permissionsText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    
    permissionsLayout->addWidget(m_permissionsText);
    rightLayout->addWidget(permissionsGroup);
    
    // ---- User Data Group ----
    QGroupBox* userDataGroup = new QGroupBox(tr("User Data"), rightPanel);
    QFormLayout* userDataLayout = new QFormLayout(userDataGroup);
    userDataLayout->setContentsMargins(6, 6, 6, 6);
    userDataLayout->setSpacing(2);
    
    m_userDataSizeLabel = new QLabel(tr("Size: Unknown"), userDataGroup);
    m_userDataPathLabel = new QLabel(tr("Path: Unknown"), userDataGroup);
    m_userDataPathLabel->setWordWrap(true);
    
    userDataLayout->addRow(tr("Size:"), m_userDataSizeLabel);
    userDataLayout->addRow(tr("Path:"), m_userDataPathLabel);
    
    rightLayout->addWidget(userDataGroup);
    
    // ---- Version Management Group ----
    QGroupBox* versionGroup = new QGroupBox(tr("Version Management"), rightPanel);
    QHBoxLayout* versionLayout = new QHBoxLayout(versionGroup);
    versionLayout->setContentsMargins(6, 6, 6, 6);
    versionLayout->setSpacing(4);
    
    m_versionsCombo = new QComboBox(versionGroup);
    m_changeVersionButton = new QPushButton(tr("Change Version"), versionGroup);
    m_changeVersionButton->setEnabled(false);
    m_changeVersionButton->setMaximumHeight(24);
    
    versionLayout->addWidget(m_versionsCombo);
    versionLayout->addWidget(m_changeVersionButton);
    
    rightLayout->addWidget(versionGroup);
    
    // ---- Remote Management Group ----
    QGroupBox* remoteGroup = new QGroupBox(tr("Remote Management"), rightPanel);
    QHBoxLayout* remoteLayout = new QHBoxLayout(remoteGroup);
    remoteLayout->setContentsMargins(6, 6, 6, 6);
    remoteLayout->setSpacing(4);
    
    m_remotesCombo = new QComboBox(remoteGroup);
    m_addRemoteButton = new QPushButton(tr("Add Remote"), remoteGroup);
    m_removeRemoteButton = new QPushButton(tr("Remove Remote"), remoteGroup);
    
    m_addRemoteButton->setMaximumHeight(24);
    m_removeRemoteButton->setMaximumHeight(24);
    
    remoteLayout->addWidget(m_remotesCombo);
    remoteLayout->addWidget(m_addRemoteButton);
    remoteLayout->addWidget(m_removeRemoteButton);
    
    rightLayout->addWidget(remoteGroup);
    
    // Set up splitter proportions - make the right panel narrower
    m_splitter->setStretchFactor(0, 2);
    m_splitter->setStretchFactor(1, 1);
    
    // Disable detail buttons initially - enable when a Flatpak is selected
    m_manageUserDataButton->setEnabled(false);
    m_changeVersionButton->setEnabled(false);
    m_uninstallButton->setEnabled(false);
    m_removeDataButton->setEnabled(false);
    m_createSnapshotButton->setEnabled(false);
    m_restoreSnapshotButton->setEnabled(false);
}

void FlatpakManagerTab::connectSignals()
{
    connect(m_searchInput, &QLineEdit::textChanged, this, &FlatpakManagerTab::filterFlatpakList);
    connect(m_listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlatpakManagerTab::onFlatpakSelected);
    connect(m_manageUserDataButton, &QPushButton::clicked, this, &FlatpakManagerTab::onManageUserData);
    connect(m_changeVersionButton, &QPushButton::clicked, this, &FlatpakManagerTab::onManageVersions);
    connect(m_uninstallButton, &QPushButton::clicked, this, &FlatpakManagerTab::onUninstall);
    connect(m_removeDataButton, &QPushButton::clicked, this, &FlatpakManagerTab::onRemoveUserData);
    connect(m_createSnapshotButton, &QPushButton::clicked, this, &FlatpakManagerTab::onCreateSnapshot);
    connect(m_restoreSnapshotButton, &QPushButton::clicked, this, &FlatpakManagerTab::onRestoreSnapshot);
    connect(m_addRemoteButton, &QPushButton::clicked, this, &FlatpakManagerTab::onAddRemote);
    connect(m_removeRemoteButton, &QPushButton::clicked, this, &FlatpakManagerTab::onRemoveRemote);
    connect(m_installNewButton, &QPushButton::clicked, this, &FlatpakManagerTab::onInstallNew);
}

void FlatpakManagerTab::refreshFlatpakList()
{
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
}

void FlatpakManagerTab::refreshFlatpakRemotes()
{
    // Clear combo box
    m_remotesCombo->clear();
    
    // Run flatpak remotes command
    QProcess process;
    process.start("flatpak", QStringList() << "remotes");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    // Parse results
    for (const QString& line : lines) {
        // Use QRegularExpression instead of QRegExp
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 1) {
            m_remotesCombo->addItem(parts[0]);
        }
    }
    
    // Update status
    emit statusMessage(tr("Found %1 Flatpak remotes").arg(m_remotesCombo->count()), 3000);
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
void FlatpakManagerTab::onFlatpakSelected(const QModelIndex& current, const QModelIndex& previous) {
    Q_UNUSED(previous);
    
    // Clear current details
    m_versionsCombo->clear();
    m_permissionsText->clear();
    
    if (!current.isValid()) {
        // Disable action buttons if no selection
        m_manageUserDataButton->setEnabled(false);
        m_changeVersionButton->setEnabled(false);
        m_uninstallButton->setEnabled(false);
        m_removeDataButton->setEnabled(false);
        m_createSnapshotButton->setEnabled(false);
        m_restoreSnapshotButton->setEnabled(false);
        
        // Clear labels
        m_nameLabel->setText(tr("Name: "));
        m_versionLabel->setText(tr("Version: "));
        m_branchLabel->setText(tr("Branch: "));
        m_originLabel->setText(tr("Origin: "));
        m_installationLabel->setText(tr("Installation: "));
        m_sizeLabel->setText(tr("Size: "));
        m_runtimeLabel->setText(tr("Runtime: "));
        m_userDataSizeLabel->setText(tr("Size: Unknown"));
        m_userDataPathLabel->setText(tr("Path: Unknown"));
        
        return;
    }
    
    // Get the app ID from the selected row
    QModelIndex appIdIndex = m_listModel->index(current.row(), 1);
    QString appId = m_listModel->data(appIdIndex).toString();
    
    // Update Flatpak details (this would be implemented with more detail in a real app)
    updateFlatpakDetails(appId);
    
    // Enable action buttons
    m_manageUserDataButton->setEnabled(true);
    m_changeVersionButton->setEnabled(true);
    m_uninstallButton->setEnabled(true);
    m_removeDataButton->setEnabled(true);
    m_createSnapshotButton->setEnabled(true);
    m_restoreSnapshotButton->setEnabled(true);
}

// Stub implementations for all the other required methods
void FlatpakManagerTab::onManageUserData() {
    QString appId = getCurrentAppId();
    if (appId.isEmpty()) return;
    
    emit statusMessage(tr("Managing user data for %1").arg(appId), 3000);
    // Real implementation would open a file manager to the user data location
}

void FlatpakManagerTab::onManageVersions() {
    QString appId = getCurrentAppId();
    if (appId.isEmpty()) return;
    
    emit statusMessage(tr("Managing versions for %1").arg(appId), 3000);
    // Real implementation would show a dialog to choose versions
}

void FlatpakManagerTab::onUninstall() {
    QString appId = getCurrentAppId();
    if (appId.isEmpty()) return;
    
    // Confirm with the user
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Confirm Uninstall"),
                                 tr("Are you sure you want to uninstall %1?").arg(appId),
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Show progress dialog
    QProgressDialog progress(tr("Uninstalling %1...").arg(appId), tr("Cancel"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents();
    
    // Perform the uninstallation
    bool success = m_packageManager->remove_flatpak_package(appId.toStdString());
    
    progress.close();
    
    if (success) {
        emit statusMessage(tr("Successfully uninstalled %1").arg(appId), 3000);
        refreshFlatpakList();
    } else {
        QMessageBox::critical(this, tr("Uninstall Failed"),
                             tr("Failed to uninstall %1. Check the console for errors.").arg(appId));
        emit statusMessage(tr("Failed to uninstall %1").arg(appId), 3000);
    }
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

void FlatpakManagerTab::onAddRemote() {
    // Show a dialog to add a new remote
    QString remoteName = QInputDialog::getText(this, tr("Add Remote"),
                                               tr("Enter the name of the remote:"));
    if (remoteName.isEmpty()) return;
    
    QString remoteUrl = QInputDialog::getText(this, tr("Add Remote"),
                                              tr("Enter the URL of the remote:"));
    if (remoteUrl.isEmpty()) return;
    
    emit statusMessage(tr("Adding remote %1").arg(remoteName), 3000);
    // Real implementation would add the remote
}

void FlatpakManagerTab::onRemoveRemote() {
    QString remoteName = m_remotesCombo->currentText();
    if (remoteName.isEmpty()) return;
    
    emit statusMessage(tr("Removing remote %1").arg(remoteName), 3000);
    // Real implementation would remove the remote
}

void FlatpakManagerTab::onInstallNew() {
    QString appId = QInputDialog::getText(this, tr("Install Flatpak"),
                                         tr("Enter the Application ID to install:"),
                                         QLineEdit::Normal,
                                         "org.example.App");
    
    if (appId.isEmpty()) return;
    
    // Get remote
    QString remote;
    if (m_remotesCombo->count() > 0) {
        remote = m_remotesCombo->currentText();
    } else {
        remote = "flathub";  // Default to flathub if no remotes are available
    }
    
    installFlatpak(appId, remote);
}

void FlatpakManagerTab::updateFlatpakDetails(const QString& appId) {
    if (appId.isEmpty()) return;
    
    // Get all installed packages and find the one matching the app ID
    std::vector<pacmangui::core::FlatpakPackage> packages = m_packageManager->get_installed_flatpak_packages();
    auto it = std::find_if(packages.begin(), packages.end(),
        [&appId](const pacmangui::core::FlatpakPackage& pkg) {
            return pkg.get_app_id() == appId.toStdString();
        });
    
    if (it == packages.end()) {
        return;
    }
    
    const auto& package = *it;
    
    // Update basic information
    m_nameLabel->setText(tr("Name: %1").arg(QString::fromStdString(package.get_name())));
    m_versionLabel->setText(tr("Version: %1").arg(QString::fromStdString(package.get_version())));
    m_originLabel->setText(tr("Origin: %1").arg(QString::fromStdString(package.get_repository())));
    m_runtimeLabel->setText(tr("Runtime: %1").arg(QString::fromStdString(package.get_runtime())));
    
    // Get size information using flatpak list
    QProcess sizeProcess;
    sizeProcess.start("flatpak", QStringList() << "list" << "--columns=size,application" << "--app");
    sizeProcess.waitForFinished();
    
    QString size = "Unknown";
    if (sizeProcess.exitCode() == 0) {
        QString output = sizeProcess.readAllStandardOutput();
        QStringList lines = output.split('\n');
        
        // Skip header line
        for (int i = 1; i < lines.size(); i++) {
            QStringList parts = lines[i].split('\t');
            if (parts.size() >= 2 && parts[1].trimmed() == appId) {
                size = parts[0].trimmed();
                break;
            }
        }
    }
    
    // Get additional information from flatpak info command with metadata
    QProcess process;
    process.start("flatpak", QStringList() << "info" << "--show-metadata" << appId);
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        QString output = process.readAllStandardOutput();
        QStringList lines = output.split('\n');
        
        QString branch;
        QString installationType;
        QString permissionsText;
        QString description;
        
        bool inPermissionsSection = false;
        bool inMetadataSection = false;
        
        // Hardcoded info for common apps
        if (appId == "com.discordapp.Discord") {
            description = tr("All-in-one voice and text chat for gamers");
        } else if (appId == "com.spotify.Client") {
            description = tr("Online music streaming service");
        } else if (appId == "com.vencord.Vesktop") {
            description = tr("A cross-platform Discord client mod with Vencord built-in");
        }
        
        for (const QString& line : lines) {
            if (line.startsWith("[Application]")) {
                inMetadataSection = true;
                continue;
            } else if (line.startsWith("Branch:")) {
                branch = line.mid(7).trimmed();
            } else if (line.startsWith("Installation:")) {
                installationType = line.mid(13).trimmed();
            } else if (line.startsWith("description=") && description.isEmpty()) {
                description = line.mid(12).trimmed();
            } else if (line.contains("Permissions:")) {
                inPermissionsSection = true;
                inMetadataSection = false;
                continue;
            } else if (inPermissionsSection) {
                if (line.trimmed().isEmpty()) {
                    inPermissionsSection = false;
                } else {
                    permissionsText += line.trimmed() + "\n";
                }
            }
        }
        
        // Update UI with additional information
        m_branchLabel->setText(tr("Branch: %1").arg(branch));
        m_installationLabel->setText(tr("Installation: %1").arg(installationType));
        m_sizeLabel->setText(tr("Size: %1").arg(size));
        
        if (!description.isEmpty()) {
            m_descriptionLabel->setText(tr("Description: %1").arg(description));
        }
        
        if (permissionsText.isEmpty()) {
            permissionsText = tr("No specific permissions information available");
        }
        m_permissionsText->setText(permissionsText);
    }
    
    // Update user data info
    updateUserDataInfo(appId);
    
    // Update available versions
    updateAvailableVersions(appId);
}

void FlatpakManagerTab::updateUserDataInfo(const QString& appId) {
    if (appId.isEmpty()) return;
    
    QString dataPath = getDataPath(appId);
    QString dataSize = calculateDirSize(dataPath);
    
    m_userDataSizeLabel->setText(dataSize);
    m_userDataPathLabel->setText(dataPath);
}

void FlatpakManagerTab::updateAvailableVersions(const QString& appId) {
    if (appId.isEmpty()) return;
    
    // In a real implementation, this would get the available versions from the remote
    // For now, we'll just add a placeholder
    m_versionsCombo->addItem("master");
}

QString FlatpakManagerTab::calculateDirSize(const QString& path) {
    // This would calculate the actual size of a directory
    // For now, return a placeholder
    Q_UNUSED(path);
    return tr("Unknown");
}

QString FlatpakManagerTab::getDataPath(const QString& appId) {
    // This would get the actual data path for the Flatpak app
    // For now, return a placeholder
    return QDir::homePath() + "/.var/app/" + appId;
}

QString FlatpakManagerTab::getCurrentAppId() const {
    QModelIndex current = m_listView->currentIndex();
    if (!current.isValid()) return QString();
    
    QModelIndex appIdIndex = m_listModel->index(current.row(), 1);
    return m_listModel->data(appIdIndex).toString();
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
    
    // Show progress dialog
    QProgressDialog progress(tr("Installing %1 from %2...").arg(appId).arg(remote), tr("Cancel"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents();
    
    // Perform the installation
    bool success = m_packageManager->install_flatpak_package(appId.toStdString(), remote.toStdString());
    
    progress.close();
    
    if (success) {
        emit statusMessage(tr("Successfully installed %1").arg(appId), 3000);
        refreshFlatpakList();
    } else {
        QMessageBox::critical(this, tr("Installation Failed"),
                             tr("Failed to install %1. Check the console for errors.").arg(appId));
        emit statusMessage(tr("Failed to install %1").arg(appId), 3000);
    }
}

} // namespace gui
} // namespace pacmangui 