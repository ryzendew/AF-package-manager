#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
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
#include "core/packagemanager.hpp"

// Forward declaration
namespace pacmangui {
namespace gui {
    class SettingsDialog;
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

private:
    void setupUi();
    void setupActions();
    void setupMenus();
    void setupConnections();
    void setupSystemUpdateTab();
    void setupMaintenanceTab();
    void setupDetailPanel();
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
    
    // Callable from other threads
    Q_INVOKABLE void showStatusMessage(const QString& message, int timeout = 0);
    
    // UI components
    QTabWidget* m_tabWidget;
    QLineEdit* m_searchBox;
    QPushButton* m_searchButton;
    QToolButton* m_settingsButton;
    
    // Package listing views
    QTableView* m_packagesView;
    QTableView* m_installedView;
    
    // Batch installation
    QPushButton* m_batchInstallButton;
    QSet<QString> m_selectedPackages;
    
    // System update tab
    QWidget* m_systemUpdateTab;
    QLabel* m_systemUpdateInfoLabel;
    QPushButton* m_systemUpdateButton;
    QPushButton* m_checkUpdatesButton;
    QTextEdit* m_systemUpdateLogView;
    QTableView* m_systemUpdatesView;
    QStandardItemModel* m_systemUpdatesModel;
    QCheckBox* m_systemUpdateOverwriteCheckbox;
    
    // System maintenance tab
    QWidget* m_maintenanceTab;
    QTextEdit* m_maintenanceLogView;
    
    // Package cache cleaning
    QGroupBox* m_cacheClearGroup;
    QRadioButton* m_clearUnusedCacheRadio;
    QRadioButton* m_clearAllCacheRadio;
    QPushButton* m_clearCacheButton;
    
    // Orphaned packages
    QGroupBox* m_orphansGroup;
    QPushButton* m_findOrphansButton;
    QPushButton* m_removeOrphansButton;
    
    // Database check
    QGroupBox* m_databaseCheckGroup;
    QCheckBox* m_checkSyncDbsCheckbox;
    QPushButton* m_checkDatabaseButton;
    
    // Pacnew files
    QGroupBox* m_pacnewGroup;
    QPushButton* m_findPacnewButton;
    
    // Database backup/restore
    QGroupBox* m_databaseBackupGroup;
    QLineEdit* m_backupPathEdit;
    QPushButton* m_selectBackupPathButton;
    QPushButton* m_backupButton;
    QPushButton* m_restoreButton;
    
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
};

} // namespace gui
} // namespace pacmangui 