#include "gui/components/installed_packages_tab.hpp"

#include <QDebug>
#include <QHeaderView>
#include <QMessageBox>
#include <QtConcurrent>

namespace pacmangui {
namespace gui {

InstalledPackagesTab::InstalledPackagesTab(QWidget* parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_searchControlsLayout(nullptr)
    , m_searchInput(nullptr)
    , m_searchButton(nullptr)
    , m_packageActionsLayout(nullptr)
    , m_removeButton(nullptr)
    , m_packagesTable(nullptr)
    , m_packagesModel(nullptr)
    , m_searchProgressDialog(nullptr)
    , m_searchWatcher(nullptr)
    , m_packageManager(nullptr)
{
    setupUi();
    connectSignals();
    
    qDebug() << "InstalledPackagesTab created";
}

InstalledPackagesTab::~InstalledPackagesTab()
{
    if (m_searchWatcher) {
        m_searchWatcher->cancel();
        m_searchWatcher->waitForFinished();
        delete m_searchWatcher;
    }
    
    if (m_searchProgressDialog) {
        delete m_searchProgressDialog;
    }
    
    qDebug() << "InstalledPackagesTab destroyed";
}

void InstalledPackagesTab::setupUi()
{
    qDebug() << "Setting up InstalledPackagesTab UI";
    
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    // Search controls
    m_searchControlsLayout = new QHBoxLayout();
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText("Search installed packages...");
    m_searchControlsLayout->addWidget(m_searchInput);
    
    createButton(&m_searchButton, "Search");
    m_searchControlsLayout->addWidget(m_searchButton);
    
    m_mainLayout->addLayout(m_searchControlsLayout);
    
    // Package actions
    m_packageActionsLayout = new QHBoxLayout();
    createButton(&m_removeButton, "Remove");
    m_removeButton->setEnabled(false);
    m_packageActionsLayout->addWidget(m_removeButton);
    m_packageActionsLayout->addStretch();
    
    m_mainLayout->addLayout(m_packageActionsLayout);
    
    // Packages table
    m_packagesTable = new QTreeView(this);
    m_packagesTable->setAlternatingRowColors(true);
    m_packagesTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_packagesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_packagesTable->setSortingEnabled(true);
    m_packagesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    m_packagesModel = new QStandardItemModel(0, 3, this);
    m_packagesModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Version" << "Description");
    
    m_packagesTable->setModel(m_packagesModel);
    m_packagesTable->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_packagesTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_packagesTable->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    
    m_mainLayout->addWidget(m_packagesTable);
    
    // Search progress dialog
    m_searchProgressDialog = new QProgressDialog("Searching installed packages...", "Cancel", 0, 0, this);
    m_searchProgressDialog->setWindowModality(Qt::WindowModal);
    m_searchProgressDialog->setAutoClose(true);
    m_searchProgressDialog->setAutoReset(true);
    m_searchProgressDialog->setMinimumDuration(500);
    
    // Future watcher for async searches
    m_searchWatcher = new QFutureWatcher<std::vector<core::Package>>(this);
    
    qDebug() << "InstalledPackagesTab UI setup complete";
}

void InstalledPackagesTab::connectSignals()
{
    qDebug() << "Connecting InstalledPackagesTab signals";
    
    // Search controls
    connect(m_searchInput, &QLineEdit::returnPressed, this, &InstalledPackagesTab::onSearchClicked);
    connect(m_searchButton, &QPushButton::clicked, this, &InstalledPackagesTab::onSearchClicked);
    connect(m_searchInput, &QLineEdit::textChanged, this, &InstalledPackagesTab::onSearchTextChanged);
    
    // Package actions
    connect(m_removeButton, &QPushButton::clicked, this, &InstalledPackagesTab::onRemoveButtonClicked);
    
    // Packages table
    connect(m_packagesTable->selectionModel(), &QItemSelectionModel::currentChanged, 
            this, &InstalledPackagesTab::onPackageSelected);
    connect(m_packagesTable, &QTreeView::doubleClicked, 
            this, &InstalledPackagesTab::onPackageDoubleClicked);
    
    // Async search
    connect(m_searchWatcher, &QFutureWatcher<std::vector<core::Package>>::finished, 
            this, &InstalledPackagesTab::onSearchCompleted);
    connect(m_searchProgressDialog, &QProgressDialog::canceled, m_searchWatcher, &QFutureWatcher<std::vector<core::Package>>::cancel);
    
    qDebug() << "InstalledPackagesTab signals connected";
}

void InstalledPackagesTab::setPackageManager(core::PackageManager* packageManager)
{
    qDebug() << "Setting package manager for InstalledPackagesTab";
    m_packageManager = packageManager;
    refreshInstalledPackages();
}

void InstalledPackagesTab::refreshInstalledPackages()
{
    qDebug() << "Refreshing installed packages";
    if (m_packageManager) {
        performAsyncSearch("");
    } else {
        qWarning() << "Cannot refresh installed packages: Package manager not set";
    }
}

void InstalledPackagesTab::onSearchTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        refreshInstalledPackages();
    }
}

void InstalledPackagesTab::onSearchClicked()
{
    qDebug() << "Search button clicked";
    performAsyncSearch(m_searchInput->text());
}

void InstalledPackagesTab::onPackageSelected(const QModelIndex& index)
{
    bool hasSelection = index.isValid();
    m_removeButton->setEnabled(hasSelection);
    
    if (hasSelection) {
        QString packageName = m_packagesModel->item(index.row(), 0)->text();
        emit packageSelected(packageName);
    }
}

void InstalledPackagesTab::onPackageDoubleClicked(const QModelIndex& index)
{
    if (index.isValid()) {
        QString packageName = m_packagesModel->item(index.row(), 0)->text();
        qDebug() << "Package double-clicked:" << packageName;
        // Could show detailed package info here
    }
}

void InstalledPackagesTab::onRemoveButtonClicked()
{
    QStringList packageNames = getSelectedPackageNames();
    if (packageNames.isEmpty()) {
        qDebug() << "No packages selected for removal";
        return;
    }
    
    QString message = packageNames.size() == 1 ? 
                      QString("Do you want to remove the package '%1'?").arg(packageNames.first()) :
                      QString("Do you want to remove the selected %1 packages?").arg(packageNames.size());
    
    QMessageBox::StandardButton response = QMessageBox::question(
        this, "Confirm Package Removal", message,
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (response == QMessageBox::Yes) {
        emit removePackageRequested(packageNames);
    }
}

void InstalledPackagesTab::performAsyncSearch(const QString& searchText)
{
    if (!m_packageManager) {
        qWarning() << "Cannot search: Package manager not set";
        return;
    }
    
    if (m_searchWatcher->isRunning()) {
        m_searchWatcher->cancel();
        m_searchWatcher->waitForFinished();
    }
    
    qDebug() << "Starting async search for installed packages with filter:" << searchText;
    m_searchProgressDialog->setLabelText("Searching installed packages...");
    m_searchProgressDialog->show();
    
    QFuture<std::vector<core::Package>> future = QtConcurrent::run(
        [this, searchText]() -> std::vector<core::Package> {
            if (searchText.isEmpty()) {
                return m_packageManager->getInstalledPackages();
            } else {
                return m_packageManager->searchInstalledPackages(searchText.toStdString());
            }
        }
    );
    
    m_searchWatcher->setFuture(future);
}

void InstalledPackagesTab::onSearchCompleted()
{
    qDebug() << "Async search completed";
    m_searchProgressDialog->hide();
    
    if (!m_searchWatcher->isCanceled()) {
        m_installedPackages = m_searchWatcher->result();
        updateSearchResults(m_installedPackages);
    }
}

void InstalledPackagesTab::updateSearchResults(const std::vector<core::Package>& results)
{
    qDebug() << "Updating installed packages list with" << results.size() << "packages";
    
    m_packagesModel->setRowCount(0);
    
    for (const auto& package : results) {
        QList<QStandardItem*> row;
        
        QStandardItem* nameItem = new QStandardItem(QString::fromStdString(package.name));
        QStandardItem* versionItem = new QStandardItem(QString::fromStdString(package.version));
        QStandardItem* descriptionItem = new QStandardItem(QString::fromStdString(package.description));
        
        row.append(nameItem);
        row.append(versionItem);
        row.append(descriptionItem);
        
        m_packagesModel->appendRow(row);
    }
    
    m_packagesTable->sortByColumn(0, Qt::AscendingOrder);
    
    emit statusMessageRequested(
        QString("Found %1 installed packages").arg(results.size()), 
        3000
    );
}

QStringList InstalledPackagesTab::getSelectedPackageNames() const
{
    QStringList packageNames;
    QModelIndexList selectedIndexes = m_packagesTable->selectionModel()->selectedRows();
    
    for (const QModelIndex& index : selectedIndexes) {
        packageNames.append(m_packagesModel->item(index.row(), 0)->text());
    }
    
    return packageNames;
}

void InstalledPackagesTab::createButton(QPushButton** button, const QString& text)
{
    *button = new QPushButton(text, this);
    (*button)->setMinimumWidth(100);
    (*button)->setMaximumHeight(30);
    
    // Apply styling
    QString buttonStyle = "QPushButton {"
                         "    background-color: #3498db;"
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
    
    (*button)->setStyleSheet(buttonStyle);
}

} // namespace gui
} // namespace pacmangui 