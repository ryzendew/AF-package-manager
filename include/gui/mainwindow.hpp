#pragma once

#include <QMainWindow>
#include <QTableView>
#include <QLineEdit>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QComboBox>
#include "core/packagemanager.hpp"
#include "gui/packageview.hpp"

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
     * 
     * @param parent Parent widget
     * @param package_manager Pointer to package manager
     */
    explicit MainWindow(QWidget *parent, core::PackageManager *package_manager);
    
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
    /**
     * @brief Handle package install
     */
    void onInstallPackage();
    
    /**
     * @brief Handle package remove
     */
    void onRemovePackage();
    
    /**
     * @brief Handle package update
     */
    void onUpdatePackage();
    
    /**
     * @brief Handle sync all packages
     */
    void onSyncAll();
    
    /**
     * @brief Handle package search
     */
    void onSearch();
    
    /**
     * @brief Handle repository selection change
     * 
     * @param index Index of selected repository
     */
    void onRepositoryChanged(int index);
    
    /**
     * @brief Handle about action
     */
    void onAbout();

private:
    /**
     * @brief Create actions for menus and toolbars
     */
    void createActions();
    
    /**
     * @brief Create menu bar
     */
    void createMenus();
    
    /**
     * @brief Create tool bar
     */
    void createToolBars();
    
    /**
     * @brief Create status bar
     */
    void createStatusBar();
    
    /**
     * @brief Load settings from config
     */
    void loadSettings();
    
    /**
     * @brief Save settings to config
     */
    void saveSettings();
    
    /**
     * @brief Refresh package list
     */
    void refreshPackageList();
    
    /**
     * @brief Show error message
     * 
     * @param title Error title
     * @param message Error message
     */
    void showError(const QString &title, const QString &message);

    core::PackageManager *m_package_manager;  ///< Pointer to package manager
    
    // UI components
    PackageView *m_package_view;              ///< Package view widget
    QLineEdit *m_search_edit;                 ///< Search input field
    QComboBox *m_repo_combo;                  ///< Repository selection dropdown
    QLabel *m_status_label;                   ///< Status information label
    
    // Actions
    QAction *m_install_action;                ///< Install package action
    QAction *m_remove_action;                 ///< Remove package action
    QAction *m_update_action;                 ///< Update package action
    QAction *m_sync_action;                   ///< Sync packages action
    QAction *m_search_action;                 ///< Search packages action
    QAction *m_exit_action;                   ///< Exit application action
    QAction *m_about_action;                  ///< Show about dialog action
    
    // Menus
    QMenu *m_file_menu;                       ///< File menu
    QMenu *m_packages_menu;                   ///< Packages menu
    QMenu *m_help_menu;                       ///< Help menu
    
    // Toolbars
    QToolBar *m_main_toolbar;                 ///< Main toolbar
};

} // namespace gui
} // namespace pacmangui 