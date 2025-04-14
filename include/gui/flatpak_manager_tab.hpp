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
    void onInstallNew();

private slots:
    void onManageUserData();
    void onManageVersions();
    void onUninstall();
    void onRemoveUserData();
    void onCreateSnapshot();
    void onRestoreSnapshot();
    void onAddRemote();
    void onRemoveRemote();
    void onBatchOperation();
    void scanForOrphanedData();

private:
    void setupUi();
    void connectSignals();
    void updateFlatpakDetails(const QString& appId);
    void updateUserDataInfo(const QString& appId);
    void updateAvailableVersions(const QString& appId);
    QString calculateDirSize(const QString& path);
    QString getDataPath(const QString& appId);
    QString getCurrentAppId() const;
    
    // GUI components
    QSplitter* m_splitter = nullptr;
    QLineEdit* m_searchInput = nullptr;
    QTreeView* m_listView = nullptr;
    QStandardItemModel* m_listModel = nullptr;
    QPushButton* m_installNewButton = nullptr;
    
    // Details panel
    QLabel* m_nameLabel = nullptr;
    QLabel* m_versionLabel = nullptr;
    QLabel* m_branchLabel = nullptr;
    QLabel* m_originLabel = nullptr;
    QLabel* m_installationLabel = nullptr;
    QLabel* m_sizeLabel = nullptr;
    QLabel* m_runtimeLabel = nullptr;
    QTextEdit* m_permissionsText = nullptr;
    
    // User data panel
    QLabel* m_userDataSizeLabel = nullptr;
    QLabel* m_userDataPathLabel = nullptr;
    QPushButton* m_manageUserDataButton = nullptr;
    
    // Versions panel
    QComboBox* m_versionsCombo = nullptr;
    QPushButton* m_changeVersionButton = nullptr;
    
    // Actions panel
    QPushButton* m_uninstallButton = nullptr;
    QPushButton* m_removeDataButton = nullptr;
    QPushButton* m_createSnapshotButton = nullptr;
    QPushButton* m_restoreSnapshotButton = nullptr;
    
    // Repository management
    QComboBox* m_remotesCombo = nullptr;
    QPushButton* m_addRemoteButton = nullptr;
    QPushButton* m_removeRemoteButton = nullptr;
    
    // Batch operations
    QButtonGroup* m_batchModeGroup = nullptr;
    QRadioButton* m_batchUninstallRadio = nullptr;
    QRadioButton* m_batchRemoveDataRadio = nullptr;
    QRadioButton* m_batchCopyIdRadio = nullptr;
    QPushButton* m_batchOperationButton = nullptr;
    
    // Orphaned data management
    QPushButton* m_scanOrphanedButton = nullptr;
    QTextEdit* m_orphanedText = nullptr;
    
    // Data
    core::PackageManager* m_packageManager = nullptr;
};

} // namespace gui
} // namespace pacmangui 