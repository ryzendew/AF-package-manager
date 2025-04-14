#pragma once

#include <QWidget>
#include <QTreeView>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStandardItemModel>
#include <QProgressDialog>
#include <QFutureWatcher>
#include "core/packagemanager.hpp"

namespace pacmangui {
namespace gui {

/**
 * @brief A tab for searching and displaying available packages
 */
class SearchTab : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit SearchTab(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~SearchTab();

    /**
     * @brief Set up the UI components
     */
    void setupUi();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Set the package manager
     * @param packageManager Pointer to the package manager
     */
    void setPackageManager(core::PackageManager* packageManager);
    
    /**
     * @brief Update search results in the table
     * @param results The search results to display
     */
    void updateSearchResults(const std::vector<core::Package>& results);
    
    /**
     * @brief Get the packages table
     * @return Pointer to the packages table
     */
    QTreeView* getPackagesTable() const { return m_packagesTable; }
    
    /**
     * @brief Get the packages model
     * @return Pointer to the packages model
     */
    QStandardItemModel* getPackagesModel() const { return m_packagesModel; }
    
    /**
     * @brief Set AUR helper
     * @param aurHelper The AUR helper name
     */
    void setAurHelper(const QString& aurHelper);

signals:
    /**
     * @brief Signal emitted when a package installation is requested
     * @param packageNames Names of the packages to install
     */
    void installPackageRequested(const QStringList& packageNames);
    
    /**
     * @brief Signal emitted when a package removal is requested
     * @param packageNames Names of the packages to remove
     */
    void removePackageRequested(const QStringList& packageNames);
    
    /**
     * @brief Signal emitted when a package update is requested
     * @param packageNames Names of the packages to update
     */
    void updatePackageRequested(const QStringList& packageNames);
    
    /**
     * @brief Signal emitted when an AUR package installation is requested
     * @param packageNames Names of the packages to install from AUR
     */
    void installAurPackageRequested(const QStringList& packageNames);
    
    /**
     * @brief Signal emitted when a package is selected
     * @param packageName Name of the selected package
     */
    void packageSelected(const QString& packageName);
    
    /**
     * @brief Signal emitted when a status message should be shown
     * @param message The message to show
     * @param timeout Duration in milliseconds, 0 for no timeout
     */
    void statusMessageRequested(const QString& message, int timeout = 0);

private slots:
    /**
     * @brief Handle search button clicked
     */
    void onSearchClicked();
    
    /**
     * @brief Handle search text changed
     * @param text The new search text
     */
    void onSearchTextChanged(const QString& text);
    
    /**
     * @brief Handle package selection
     * @param index The selected model index
     */
    void onPackageSelected(const QModelIndex& index);
    
    /**
     * @brief Handle package double click
     * @param index The selected model index
     */
    void onPackageDoubleClicked(const QModelIndex& index);
    
    /**
     * @brief Handle install button clicked
     */
    void onInstallButtonClicked();
    
    /**
     * @brief Handle update button clicked
     */
    void onUpdateButtonClicked();
    
    /**
     * @brief Handle remove button clicked
     */
    void onRemoveButtonClicked();
    
    /**
     * @brief Handle install from AUR button clicked
     */
    void onInstallAurButtonClicked();
    
    /**
     * @brief Update the install button text based on selection
     */
    void updateInstallButtonText();
    
    /**
     * @brief Handle async search completion
     */
    void onSearchCompleted();

private:
    // UI components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_searchControlsLayout;
    QLineEdit* m_searchInput;
    QPushButton* m_searchButton;
    QHBoxLayout* m_packageActionsLayout;
    QPushButton* m_installButton;
    QPushButton* m_updateButton;
    QPushButton* m_removeButton;
    QPushButton* m_installAurButton;
    QTreeView* m_packagesTable;
    QStandardItemModel* m_packagesModel;
    
    // Progress dialog for search
    QProgressDialog* m_searchProgressDialog;
    
    // Future watcher for async search
    QFutureWatcher<std::vector<core::Package>>* m_searchWatcher;

    // Core functionality
    core::PackageManager* m_packageManager;
    QString m_aurHelper;
    
    // Helper methods
    /**
     * @brief Get the names of selected packages
     * @return List of selected package names
     */
    QStringList getSelectedPackageNames() const;
    
    /**
     * @brief Create a styled button
     * @param button Pointer to the button to create
     * @param text Button text
     */
    void createButton(QPushButton** button, const QString& text);
    
    /**
     * @brief Perform an asynchronous search
     * @param searchText The text to search for
     */
    void performAsyncSearch(const QString& searchText);
};

} // namespace gui
} // namespace pacmangui 