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
#include <QDateTime>
#include <QList>
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
    void uninstallFlatpak(const QString& appId, const QString& remote);
    void setShowInstalledOnly(bool show);
    void setShowSystemApps(bool show);
    void setShowUserApps(bool show);
    void clearSearchCache();

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

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUi();
    void connectSignals();
    void updateFlatpakDetails(const QString& appId);
    void updateUserDataInfo(const QString& appId);
    QString calculateDirSize(const QString& path);
    QString getDataPath(const QString& appId);
    QString getCurrentAppId() const;
    void performAsyncSearch(const QString& searchTerm);
    void applySearchFilters(const QString& searchTerm);
    void addToSearchCache(const QString& searchTerm, const std::vector<pacmangui::core::FlatpakPackage>& results);
    bool getFromSearchCache(const QString& searchTerm, std::vector<pacmangui::core::FlatpakPackage>& results);
    void filterResults(std::vector<pacmangui::core::FlatpakPackage>& results);
    void updateSearchResults(const std::vector<pacmangui::core::FlatpakPackage>& results);
    
    // GUI components
    QSplitter* m_splitter = nullptr;
    QLineEdit* m_searchInput = nullptr;
    QLineEdit* m_filterInput = nullptr;
    QTreeView* m_listView = nullptr;
    QStandardItemModel* m_listModel = nullptr;
    
    // Search components
    QTreeView* m_searchResultsView = nullptr;
    QStandardItemModel* m_searchResultsModel = nullptr;
    QPushButton* m_searchButton = nullptr;
    QPushButton* m_installSelectedButton = nullptr;
    QFutureWatcher<std::vector<pacmangui::core::FlatpakPackage>>* m_searchWatcher = nullptr;
    
    // Search cache
    struct SearchCacheEntry {
        QString searchTerm;
        std::vector<pacmangui::core::FlatpakPackage> results;
        QDateTime timestamp;
    };
    QList<SearchCacheEntry> m_searchCache;
    static const int MAX_CACHE_SIZE = 100;
    static const int CACHE_TIMEOUT_MS = 300000; // 5 minutes
    
    // Search filters
    QString m_currentFilter;
    bool m_showInstalledOnly = false;
    bool m_showSystemApps = true;
    bool m_showUserApps = true;
    
    // Details panel
    QLabel* m_nameLabel = nullptr;
    QLabel* m_versionLabel = nullptr;
    QLabel* m_branchLabel = nullptr;
    QLabel* m_originLabel = nullptr;
    QLabel* m_installationLabel = nullptr;
    QLabel* m_sizeLabel = nullptr;
    QLabel* m_runtimeLabel = nullptr;
    QLabel* m_descriptionLabel = nullptr;
    
    // Permission widgets
    QWidget* m_filesystemPermsWidget = nullptr;
    QLabel* m_filesystemPermsText = nullptr;
    QWidget* m_devicePermsWidget = nullptr;
    QLabel* m_devicePermsText = nullptr;
    QWidget* m_featurePermsWidget = nullptr;
    QLabel* m_featurePermsText = nullptr;
    QWidget* m_socketPermsWidget = nullptr;
    QLabel* m_socketPermsText = nullptr;
    QWidget* m_otherPermsWidget = nullptr;
    QLabel* m_otherPermsText = nullptr;
    
    // Actions panel
    QPushButton* m_manageUserDataButton = nullptr;
    QPushButton* m_uninstallButton = nullptr;
    QPushButton* m_removeDataButton = nullptr;
    QPushButton* m_createSnapshotButton = nullptr;
    QPushButton* m_restoreSnapshotButton = nullptr;
    
    // Data
    core::PackageManager* m_packageManager = nullptr;
    bool m_selectingFlatpak = false;
};

} // namespace gui
} // namespace pacmangui 