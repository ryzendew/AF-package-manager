#pragma once

#include <QWidget>
#include <QStandardItemModel>
#include <QSplitter>
#include <QLineEdit>
#include <QTreeView>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFutureWatcher>
#include "core/packagemanager.hpp"

namespace pacmangui {
namespace gui {

class FlatpakManagerTab : public QWidget
{
    Q_OBJECT

public:
    explicit FlatpakManagerTab(QWidget* parent = nullptr, core::PackageManager* packageManager = nullptr);
    ~FlatpakManagerTab();

signals:
    void statusMessage(const QString& message, int timeout);

public slots:
    void refreshFlatpakList();
    void refreshFlatpakRemotes();
    void filterFlatpakList(const QString& filter);
    void onFlatpakSelected(const QModelIndex& current, const QModelIndex& previous);
    void installFlatpak(const QString& appId, const QString& remote);

private slots:
    void onManageUserData();
    void onUninstall();
    void onRemoveUserData();
    void onCreateSnapshot();
    void onRestoreSnapshot();
    void onSearchTextChanged(const QString& text);
    void onSearchButtonClicked();
    void onSearchResultSelected(const QModelIndex& current, const QModelIndex& previous);
    void onInstallSelected();
    void onSearchCompleted();

private:
    void setupUi();
    void connectSignals();
    void updateFlatpakDetails(const QString& appId);
    void updateUserDataInfo(const QString& appId);
    QString calculateDirSize(const QString& path);
    QString getDataPath(const QString& appId);
    QString getCurrentAppId() const;
    void performAsyncSearch(const QString& searchTerm);
    
    // GUI components
    QSplitter* m_splitter = nullptr;
    QLineEdit* m_searchInput = nullptr;
    QLineEdit* m_filterInput = nullptr;
    QTreeView* m_listView = nullptr;
    QStandardItemModel* m_listModel = nullptr;
    
    // Search components
    QPushButton* m_searchButton = nullptr;
    QTreeView* m_searchResultsView = nullptr;
    QStandardItemModel* m_searchResultsModel = nullptr;
    QPushButton* m_installSelectedButton = nullptr;
    QFutureWatcher<std::vector<pacmangui::core::FlatpakPackage>>* m_searchWatcher = nullptr;
    
    // Details panel
    QLabel* m_nameLabel = nullptr;
    QLabel* m_versionLabel = nullptr;
    QLabel* m_branchLabel = nullptr;
    QLabel* m_originLabel = nullptr;
    QLabel* m_installationLabel = nullptr;
    QLabel* m_sizeLabel = nullptr;
    QLabel* m_runtimeLabel = nullptr;
    QLabel* m_descriptionLabel = nullptr;
    QTextEdit* m_permissionsText = nullptr;
    
    // Actions panel
    QPushButton* m_manageUserDataButton = nullptr;
    QPushButton* m_uninstallButton = nullptr;
    QPushButton* m_removeDataButton = nullptr;
    QPushButton* m_createSnapshotButton = nullptr;
    QPushButton* m_restoreSnapshotButton = nullptr;
    
    // Data
    core::PackageManager* m_packageManager = nullptr;
};

} // namespace gui
} // namespace pacmangui 