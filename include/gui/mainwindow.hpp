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
#include "core/packagemanager.hpp"

// Forward declaration
namespace pacmangui {
namespace gui {
    class SettingsDialog;
}
}

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
    
    // System update
    void onSystemUpdate();
    void onCheckForUpdates();
    
    // Theme
    void toggleTheme();
    
    // Settings
    void openSettings();

private:
    void setupUi();
    void setupActions();
    void setupMenus();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void applyTheme(bool isDark);
    void searchPackages(const QString& searchTerm);
    void refreshInstalledPackages();
    void refreshUpdatesList();
    void updateBatchInstallButton();
    
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
    
    // Package detail view
    QWidget* m_detailsWidget;
    QLabel* m_packageNameLabel;
    QLabel* m_packageVersionLabel;
    QLabel* m_packageDescLabel;
    QPushButton* m_actionButton;
    QCheckBox* m_packageOverwriteCheckbox;
    
    // Models for tables
    QStandardItemModel* m_packagesModel;
    QStandardItemModel* m_installedModel;
    
    // Core functionality
    core::PackageManager m_packageManager;
    
    // Theme settings
    bool m_darkTheme;
    
    // Settings dialog
    SettingsDialog* m_settingsDialog;
};

} // namespace gui
} // namespace pacmangui 