#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QTreeView>
#include <QLabel>
#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QStatusBar>
#include <QSplitter>
#include <QListView>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QCheckBox>
#include <QSet>
#include <QToolButton>
#include <QMap>
#include <QRadioButton>
#include <QGroupBox>
#include <QListWidget>
#include <QComboBox>
#include <QProgressBar>
#include <QFileDialog>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QScrollArea>
#include <QFutureWatcher>
#include <QProgressDialog>
#include "core/packagemanager.hpp"
#include <functional>

// Forward declarations
namespace pacmangui {
namespace gui {
    class SettingsDialog;
}
namespace wayland {
    class WaylandBackend;
    class WaylandProtocols;
    class WaylandSecurity;
    class WaylandOptimization;
}
}

// Constants for the slide panel
#define SLIDE_PANEL_WIDTH_PERCENT 25

namespace pacmangui {
namespace gui {

/**
 * @brief Main application window
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit MainWindow(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~MainWindow();

protected:
    /**
     * @brief Close event handler
     * 
     * @param event Close event
     */
    void closeEvent(QCloseEvent *event) override;
    
    /**
     * @brief Resize event handler
     * 
     * @param event Resize event
     */
    void resizeEvent(QResizeEvent *event) override;
    
    /**
     * @brief Event filter handler
     * 
     * @param watched The watched object
     * @param event The event
     * @return true if the event was handled, false otherwise
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    // Navigation
    void onTabChanged(int index);
    
    // Search
    void onSearchTextChanged(const QString& text);
    void onSearchClicked();
    
    // Package operations
    void onInstallPackage();
    void onRemovePackage();
    void onUpdatePackage();
    void onSyncAll();
    void onBatchInstall();
    
    // Package selection
    void onPackageSelected(const QModelIndex& index);
    void onInstalledPackageSelected(const QModelIndex& index);
    void onPackageItemChanged(QStandardItem *item);
    
    // AUR operations
    void onInstallAurPackage();
    void onUpdateAurPackages();
    
    // System update
    void onSystemUpdate();
    void onCheckForUpdates();
    
    // System maintenance
    void onClearPackageCache();
    void onRemoveOrphans();
    void onCheckDatabase();
    void onFindPacnewFiles();
    void onBackupDatabase();
    void onRestoreDatabase();
    void onMaintenanceTaskFinished(bool success, const QString& message);
    void onCleanCache();
    void onClearPackageLock();
    void onCheckIntegrityAllPackages();
    void onRefreshMirrorList();
    void checkForUpdatesAfterSync();
    
    // Theme
    void toggleTheme();
    void toggleTheme(bool isDark);
    
    // Settings
    void openSettings();
    
    // About
    void onAbout();
    
    // Detail panel
    void onDetailPanelAnimationFinished();
    void closeDetailPanel();
    
    // Wayland Support
    void onWaylandBackendAvailabilityChanged(bool available);
    void onWaylandOutputChanged();
    void onWaylandPermissionChanged(const QString& featureName, bool granted);
    void onWaylandSecurityEvent(const QString& eventType, const QString& details);
    void onWaylandHardwareAccelerationStatusChanged(bool available);
    void onWaylandPerformanceMetricsUpdated(const QVariantMap& metrics);

private:
    void setupUi();
    void setupActions();
    void setupMenus();
    void setupConnections();
    void setupSystemUpdateTab();
    void setupMaintenanceTab();
    void setupDetailPanel();
    void setupWaylandSupport();
    void loadSettings();
    void saveSettings();
    void applyTheme(bool isDark);
    void loadThemeStylesheet(const QString& fileName);
    void createMenus();
    void searchPackages(const QString& searchTerm);
    void refreshInstalledPackages();
    void refreshUpdatesList();
    void updateBatchInstallButton();
    void checkAurHelper();
    void downloadYayHelper();
    void showDetailPanel(const QString& packageName, const QString& version, const QString& repo, const QString& description);
    void checkForUpdates();
    void updateInstallButtonText();
    
    // Table styling helper
    void applyTableStyle(QTreeView* tableView);
    
    // Async search helper method
    void performAsyncSearch(const QString& searchTerm);
    
    // Wayland Support
    void applyWaylandOptimizations();
    void enableWaylandSecurityFeatures();
    void configureWaylandDisplay();
    void handleWaylandPermissions();
    
    // Theme functions
    bool isDarkThemeEnabled() const;
    
    // Callable from other threads
    Q_INVOKABLE void showStatusMessage(const QString& message, int timeout = 0);
    
    // UI components
    QTabWidget* m_tabWidget;
    QLineEdit* m_searchBox;
    QPushButton* m_searchButton;
    QToolButton* m_settingsButton;
    
    // New UI components
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    
    QWidget* m_searchTab;
    QVBoxLayout* m_searchLayout;
    QHBoxLayout* m_searchControlsLayout;
    QLineEdit* m_searchInput;
    
    QTreeView* m_packagesTable;
    QHBoxLayout* m_packageActionsLayout;
    QPushButton* m_installButton;
    QPushButton* m_updateButton; 
    QPushButton* m_removeButton;
    QPushButton* m_installAurButton;
    
    QWidget* m_installedTab;
    QVBoxLayout* m_installedLayout;
    QTreeView* m_installedTable;
    QHBoxLayout* m_installedActionsLayout;
    QPushButton* m_updateInstalledButton;
    QPushButton* m_removeInstalledButton;
    
    // System update components (removing duplicates)
    QVBoxLayout* m_systemUpdateLayout;
    QTreeView* m_systemUpdatesTable;
    QHBoxLayout* m_systemUpdateActionsLayout;
    QStandardItemModel* m_systemUpdatesModel;
    QPushButton* m_systemUpdateButton;
    QPushButton* m_checkUpdatesButton;
    
    // System update tab (keep this part, remove duplicates below)
    QWidget* m_systemUpdateTab;
    QLabel* m_systemUpdateInfoLabel;
    QTextEdit* m_systemUpdateLogView;
    QTableView* m_systemUpdatesView;
    QCheckBox* m_systemUpdateOverwriteCheckbox;
    
    // System maintenance tab
    QWidget* m_maintenanceTab;
    QTextEdit* m_maintenanceLogView;
    
    // Package cache cleaning
    QGroupBox* m_cacheClearGroup;
    QRadioButton* m_clearUnusedCacheRadio;
    QRadioButton* m_clearAllCacheRadio;
    
    // Orphaned packages
    QGroupBox* m_orphansGroup;
    
    // Pacnew files
    QGroupBox* m_pacnewGroup;
    
    // Progress indicator for maintenance operations
    QProgressBar* m_maintenanceProgressBar;
    
    // Package detail view
    QWidget* m_detailsWidget;
    QLabel* m_packageNameLabel;
    QLabel* m_packageVersionLabel;
    QLabel* m_packageDescLabel;
    QPushButton* m_actionButton;
    QCheckBox* m_packageOverwriteCheckbox;
    
    // Slide-in detail panel
    QWidget* m_detailPanel;
    QScrollArea* m_detailScrollArea;
    QLabel* m_detailTitle;
    QLabel* m_detailVersion;
    QLabel* m_detailRepo;
    QLabel* m_detailDescription;
    QLabel* m_detailIcon;
    QPushButton* m_detailCloseButton;
    QPropertyAnimation* m_slideAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;
    bool m_detailPanelVisible;
    
    // Models for tables
    QStandardItemModel* m_packagesModel;
    QStandardItemModel* m_installedModel;
    
    // Actions map for menu setup
    QMap<QString, QAction*> m_actions;
    
    // Core functionality
    core::PackageManager m_packageManager;
    
    // Theme settings
    bool m_darkTheme;
    
    // Settings dialog
    SettingsDialog* m_settingsDialog;
    
    // Wayland support
    bool m_waylandSupported;
    QMenu* m_waylandMenu;
    QAction* m_waylandSecurityAction;
    QAction* m_waylandOptimizationsAction;
    
    // Maintenance components
    QVBoxLayout* m_maintenanceLayout;
    QPushButton* m_clearCacheButton;
    QPushButton* m_removeOrphansButton;
    QPushButton* m_checkDatabaseButton;
    QPushButton* m_findPacnewButton;
    QPushButton* m_backupDatabaseButton;
    QPushButton* m_restoreDatabaseButton;
    
    // Detail panel components
    QVBoxLayout* m_detailLayout;
    QPushButton* m_closeDetailButton;
    QLabel* m_detailNameLabel;
    QLabel* m_detailVersionLabel;
    QLabel* m_detailDescriptionLabel;
    QLabel* m_detailRepositoryLabel;
    QLabel* m_detailInstalledLabel;
    QLabel* m_detailDownloadSizeLabel;
    QLabel* m_detailInstalledSizeLabel;
    QLabel* m_detailDependenciesLabel;
    QTextEdit* m_detailDependenciesText;
    
    // Menu components
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_packageMenu;
    QMenu* m_maintenanceMenu;
    QMenu* m_viewMenu;
    QMenu* m_helpMenu;
    
    // Actions
    QAction* m_exitAction;
    QAction* m_settingsAction;
    QAction* m_syncAllAction;
    QAction* m_checkForUpdatesAction;
    QAction* m_batchInstallAction;
    QAction* m_clearPackageCacheAction;
    QAction* m_removeOrphansAction;
    QAction* m_checkDatabaseAction;
    QAction* m_findPacnewFilesAction;
    QAction* m_backupDatabaseAction;
    QAction* m_restoreDatabaseAction;
    QAction* m_toggleThemeAction;
    QAction* m_aboutAction;
    
    // AUR helper
    QString m_aurHelper;
    
    // Updates tab components
    QWidget* m_updatesTab;
    QVBoxLayout* m_updatesLayout;
    QTreeView* m_updatesTable;
    QStandardItemModel* m_updatesModel;
    QHBoxLayout* m_updatesActionsLayout;
    QPushButton* m_updateSelectedButton;
    QPushButton* m_updateAllButton;
    QPushButton* m_refreshUpdatesButton;

    // Maintenance callbacks
    std::function<void(const QString&)> m_currentMaintenanceLogCallback;
    std::function<void(int)> m_currentMaintenanceProgressCallback;

    // Status
    QString m_currentStatusMessage;

    // Async search variables
    QFutureWatcher<std::vector<pacmangui::core::Package>>* m_searchWatcher;
};

} // namespace gui
} // namespace pacmangui 