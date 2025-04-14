#include "gui/components/search_tab.hpp"

#include <QDebug>
#include <QHeaderView>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QApplication>

namespace pacmangui {
namespace gui {

SearchTab::SearchTab(QWidget* parent)
    : QWidget(parent),
    m_mainLayout(nullptr),
    m_searchControlsLayout(nullptr),
    m_searchInput(nullptr),
    m_searchButton(nullptr),
    m_packageActionsLayout(nullptr),
    m_installButton(nullptr),
    m_updateButton(nullptr),
    m_removeButton(nullptr),
    m_installAurButton(nullptr),
    m_packagesTable(nullptr),
    m_packagesModel(nullptr),
    m_searchProgressDialog(nullptr),
    m_searchWatcher(nullptr),
    m_packageManager(nullptr),
    m_aurHelper("")
{
    setupUi();
    connectSignals();
}

SearchTab::~SearchTab()
{
    // Clean up any resources that aren't automatically handled
    if (m_searchWatcher) {
        m_searchWatcher->cancel();
        m_searchWatcher->waitForFinished();
        delete m_searchWatcher;
    }
    
    if (m_searchProgressDialog) {
        delete m_searchProgressDialog;
    }
}

void SearchTab::setupUi()
{
    qDebug() << "SearchTab: Setting up UI";
    
    // Create main layout
    m_mainLayout = new QVBoxLayout(this);
    
    // Create search controls
    m_searchControlsLayout = new QHBoxLayout();
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText(tr("Search packages..."));
    m_searchButton = new QPushButton(tr("Search"), this);
    m_searchButton->setStyleSheet("background-color: #0078d7; color: white;");
    m_searchButton->setMaximumWidth(100);
    m_searchButton->setMaximumHeight(28);
    m_searchButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    m_searchControlsLayout->addWidget(m_searchInput);
    m_searchControlsLayout->addWidget(m_searchButton);
    
    m_mainLayout->addLayout(m_searchControlsLayout);
    
    // Create package action buttons
    m_packageActionsLayout = new QHBoxLayout();
    
    createButton(&m_installButton, tr("Install"));
    createButton(&m_updateButton, tr("Update"));
    createButton(&m_removeButton, tr("Remove"));
    createButton(&m_installAurButton, tr("Install from AUR"));
    
    // Initially disable AUR button
    m_installAurButton->setEnabled(false);
    
    m_packageActionsLayout->addWidget(m_installButton);
    m_packageActionsLayout->addWidget(m_updateButton);
    m_packageActionsLayout->addWidget(m_removeButton);
    m_packageActionsLayout->addWidget(m_installAurButton);
    m_packageActionsLayout->addStretch(1); // Push buttons to the left
    
    m_mainLayout->addLayout(m_packageActionsLayout);
    
    // Create packages table
    m_packagesTable = new QTreeView(this);
    m_packagesTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_packagesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_packagesTable->setSortingEnabled(true);
    m_packagesTable->setAlternatingRowColors(true);
    
    // Create the model
    m_packagesModel = new QStandardItemModel(0, 4, this);
    m_packagesModel->setHorizontalHeaderLabels(
        QStringList() << tr("Name") << tr("Version") << tr("Description") << tr("Repository"));
    
    m_packagesTable->setModel(m_packagesModel);
    
    // Adjust column widths
    m_packagesTable->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_packagesTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_packagesTable->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_packagesTable->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    
    m_mainLayout->addWidget(m_packagesTable);
    
    // Create progress dialog for async search
    m_searchProgressDialog = new QProgressDialog(tr("Searching..."), tr("Cancel"), 0, 0, this);
    m_searchProgressDialog->setWindowModality(Qt::WindowModal);
    m_searchProgressDialog->setMinimumDuration(500); // Only show if operation takes more than 500ms
    m_searchProgressDialog->reset();
    m_searchProgressDialog->hide();
    
    // Set initial button states
    m_installButton->setEnabled(false);
    m_updateButton->setEnabled(false);
    m_removeButton->setEnabled(false);
}

void SearchTab::connectSignals()
{
    // Connect search signals
    connect(m_searchInput, &QLineEdit::textChanged, this, &SearchTab::onSearchTextChanged);
    connect(m_searchButton, &QPushButton::clicked, this, &SearchTab::onSearchClicked);
    
    // Connect package table signals
    connect(m_packagesTable, &QTreeView::clicked, this, &SearchTab::onPackageSelected);
    connect(m_packagesTable, &QTreeView::doubleClicked, this, &SearchTab::onPackageDoubleClicked);
    
    // Connect selection model changes for dynamic button text
    if (m_packagesTable->selectionModel()) {
        connect(m_packagesTable->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &SearchTab::updateInstallButtonText);
    }
    
    // Connect package action buttons
    connect(m_installButton, &QPushButton::clicked, this, &SearchTab::onInstallButtonClicked);
    connect(m_updateButton, &QPushButton::clicked, this, &SearchTab::onUpdateButtonClicked);
    connect(m_removeButton, &QPushButton::clicked, this, &SearchTab::onRemoveButtonClicked);
    connect(m_installAurButton, &QPushButton::clicked, this, &SearchTab::onInstallAurButtonClicked);
}

void SearchTab::setPackageManager(core::PackageManager* packageManager)
{
    m_packageManager = packageManager;
}

void SearchTab::setAurHelper(const QString& aurHelper)
{
    m_aurHelper = aurHelper;
    m_installAurButton->setEnabled(!m_aurHelper.isEmpty());
}

void SearchTab::updateSearchResults(const std::vector<core::Package>& results)
{
    if (!m_packagesModel) {
        return;
    }
    
    // Clear the model
    m_packagesModel->clear();
    m_packagesModel->setHorizontalHeaderLabels(
        QStringList() << tr("Name") << tr("Version") << tr("Description") << tr("Repository"));
    
    // Add packages to the model
    for (const auto& pkg : results) {
        QList<QStandardItem*> row;
        
        QStandardItem* nameItem = new QStandardItem(QString::fromStdString(pkg.get_name()));
        QStandardItem* versionItem = new QStandardItem(QString::fromStdString(pkg.get_version()));
        QStandardItem* descItem = new QStandardItem(QString::fromStdString(pkg.get_description()));
        QStandardItem* repoItem = new QStandardItem(QString::fromStdString(pkg.get_repository()));
        
        row << nameItem << versionItem << descItem << repoItem;
        m_packagesModel->appendRow(row);
    }
    
    // Emit status message
    emit statusMessageRequested(tr("Found %1 packages").arg(results.size()), 3000);
}

void SearchTab::onSearchClicked()
{
    QString searchText = m_searchInput->text();
    
    if (searchText.isEmpty()) {
        emit statusMessageRequested(tr("Please enter a search term"), 3000);
        return;
    }
    
    performAsyncSearch(searchText);
}

void SearchTab::onSearchTextChanged(const QString& text)
{
    // Auto-search after a short delay if text is at least 2 characters
    if (text.length() >= 2) {
        performAsyncSearch(text);
    } else if (text.isEmpty()) {
        // Clear the search results
        m_packagesModel->clear();
        m_packagesModel->setHorizontalHeaderLabels(
            QStringList() << tr("Name") << tr("Version") << tr("Description") << tr("Repository"));
    }
}

void SearchTab::onPackageSelected(const QModelIndex& index)
{
    Q_UNUSED(index);
    updateInstallButtonText();
    
    // Enable/disable buttons based on selection
    bool hasSelection = m_packagesTable->selectionModel()->hasSelection();
    m_installButton->setEnabled(hasSelection);
    m_updateButton->setEnabled(hasSelection);
    m_removeButton->setEnabled(hasSelection);
    m_installAurButton->setEnabled(hasSelection && !m_aurHelper.isEmpty());
    
    // If a package is selected, emit the packageSelected signal
    if (hasSelection) {
        QModelIndex nameIndex = m_packagesTable->selectionModel()->currentIndex().siblingAtColumn(0);
        QString packageName = m_packagesModel->data(nameIndex).toString();
        emit packageSelected(packageName);
    }
}

void SearchTab::onPackageDoubleClicked(const QModelIndex& index)
{
    Q_UNUSED(index);
    // Double-clicking a package could trigger installation directly
    onInstallButtonClicked();
}

void SearchTab::onInstallButtonClicked()
{
    QStringList packageNames = getSelectedPackageNames();
    if (packageNames.isEmpty()) {
        emit statusMessageRequested(tr("No packages selected for installation"), 3000);
        return;
    }
    
    QStringList packageDetails;
    QModelIndexList selected = m_packagesTable->selectionModel()->selectedRows();
    for (const QModelIndex& index : selected) {
        QModelIndex nameIndex = m_packagesModel->index(index.row(), 0); // Name column
        QModelIndex versionIndex = m_packagesModel->index(index.row(), 1); // Version column
        QModelIndex repoIndex = m_packagesModel->index(index.row(), 3); // Repository column
        
        QString name = m_packagesModel->data(nameIndex).toString();
        QString version = m_packagesModel->data(versionIndex).toString();
        QString repo = m_packagesModel->data(repoIndex).toString();
        
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
    
    // Emit the signal for package installation
    emit installPackageRequested(packageNames);
}

void SearchTab::onUpdateButtonClicked()
{
    QStringList packageNames = getSelectedPackageNames();
    if (packageNames.isEmpty()) {
        emit statusMessageRequested(tr("No packages selected for update"), 3000);
        return;
    }
    
    // Emit the signal for package update
    emit updatePackageRequested(packageNames);
}

void SearchTab::onRemoveButtonClicked()
{
    QStringList packageNames = getSelectedPackageNames();
    if (packageNames.isEmpty()) {
        emit statusMessageRequested(tr("No packages selected for removal"), 3000);
        return;
    }
    
    // Collect package details
    QStringList packageDetails;
    QModelIndexList selected = m_packagesTable->selectionModel()->selectedRows();
    for (const QModelIndex& index : selected) {
        QModelIndex nameIndex = m_packagesModel->index(index.row(), 0); // Name column
        QModelIndex versionIndex = m_packagesModel->index(index.row(), 1); // Version column
        
        QString name = m_packagesModel->data(nameIndex).toString();
        QString version = m_packagesModel->data(versionIndex).toString();
        
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
    
    // Emit the signal for package removal
    emit removePackageRequested(packageNames);
}

void SearchTab::onInstallAurButtonClicked()
{
    QStringList packageNames = getSelectedPackageNames();
    if (packageNames.isEmpty()) {
        emit statusMessageRequested(tr("No packages selected for AUR installation"), 3000);
        return;
    }
    
    // Emit the signal for AUR package installation
    emit installAurPackageRequested(packageNames);
}

void SearchTab::updateInstallButtonText()
{
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

void SearchTab::onSearchCompleted()
{
    if (!m_searchWatcher) {
        return;
    }
    
    // Get search results
    std::vector<core::Package> results = m_searchWatcher->result();
    
    // Update UI with search results
    updateSearchResults(results);
    
    // Clean up
    m_searchProgressDialog->hide();
    m_searchWatcher->deleteLater();
    m_searchWatcher = nullptr;
    
    // Update status bar
    emit statusMessageRequested(tr("Found %1 packages").arg(results.size()), 3000);
}

QStringList SearchTab::getSelectedPackageNames() const
{
    QStringList packageNames;
    QModelIndexList selected = m_packagesTable->selectionModel()->selectedRows();
    
    for (const QModelIndex& index : selected) {
        QModelIndex nameIndex = m_packagesModel->index(index.row(), 0); // Name column
        packageNames.append(m_packagesModel->data(nameIndex).toString());
    }
    
    return packageNames;
}

void SearchTab::createButton(QPushButton** button, const QString& text)
{
    *button = new QPushButton(text, this);
    (*button)->setStyleSheet("background-color: #0078d7; color: white;");
    (*button)->setMaximumWidth(120);
    (*button)->setMaximumHeight(28);
    (*button)->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void SearchTab::performAsyncSearch(const QString& searchText)
{
    if (!m_packageManager) {
        emit statusMessageRequested(tr("Package manager not initialized"), 3000);
        return;
    }
    
    // Cancel any previous search
    if (m_searchWatcher) {
        m_searchWatcher->cancel();
        m_searchWatcher->waitForFinished();
        delete m_searchWatcher;
        m_searchWatcher = nullptr;
    }
    
    // Show searching status
    emit statusMessageRequested(tr("Searching for packages..."), 0);
    
    // Show progress dialog for longer searches
    m_searchProgressDialog->setLabelText(tr("Searching for '%1'...").arg(searchText));
    m_searchProgressDialog->reset();
    
    // Create new watcher for async search
    m_searchWatcher = new QFutureWatcher<std::vector<core::Package>>(this);
    
    // Connect signals
    connect(m_searchWatcher, &QFutureWatcher<std::vector<core::Package>>::finished,
            this, &SearchTab::onSearchCompleted);
    
    // Connect cancel button
    connect(m_searchProgressDialog, &QProgressDialog::canceled, [this]() {
        if (m_searchWatcher) {
            m_searchWatcher->cancel();
        }
    });
    
    // Start async operation
    QFuture<std::vector<core::Package>> future = QtConcurrent::run(
        [this, searchText]() {
            return m_packageManager->search_by_name(searchText.toStdString());
        }
    );
    
    m_searchWatcher->setFuture(future);
    m_searchProgressDialog->show();
}

} // namespace gui
} // namespace pacmangui 