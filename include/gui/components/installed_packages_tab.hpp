#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>
#include <QStandardItemModel>
#include <QProgressDialog>
#include <QFutureWatcher>

#include "core/package_manager.hpp"
#include "core/package.hpp"

namespace pacmangui {
namespace gui {

/**
 * @brief The InstalledPackagesTab class displays and manages installed packages
 */
class InstalledPackagesTab : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent The parent widget
     */
    explicit InstalledPackagesTab(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~InstalledPackagesTab();

    /**
     * @brief Set the package manager for this tab
     * @param packageManager The package manager instance
     */
    void setPackageManager(core::PackageManager* packageManager);

    /**
     * @brief Refresh the installed packages list
     */
    void refreshInstalledPackages();

signals:
    /**
     * @brief Signal emitted when a package is selected
     * @param packageName The name of the selected package
     */
    void packageSelected(const QString& packageName);

    /**
     * @brief Signal emitted when packages are requested to be removed
     * @param packageNames List of package names to remove
     */
    void removePackageRequested(const QStringList& packageNames);

    /**
     * @brief Signal emitted to display a status message
     * @param message The message to display
     * @param timeout The timeout in milliseconds (0 for no timeout)
     */
    void statusMessageRequested(const QString& message, int timeout);

private slots:
    /**
     * @brief Handle search input changes
     * @param text The new search text
     */
    void onSearchTextChanged(const QString& text);

    /**
     * @brief Handle search button click
     */
    void onSearchClicked();

    /**
     * @brief Handle package selection
     * @param index The index of the selected package
     */
    void onPackageSelected(const QModelIndex& index);

    /**
     * @brief Handle package double-click
     * @param index The index of the double-clicked package
     */
    void onPackageDoubleClicked(const QModelIndex& index);

    /**
     * @brief Handle remove button click
     */
    void onRemoveButtonClicked();

    /**
     * @brief Handle async search completion
     */
    void onSearchCompleted();

private:
    /**
     * @brief Setup UI components
     */
    void setupUi();

    /**
     * @brief Connect signals and slots
     */
    void connectSignals();

    /**
     * @brief Update the search results display
     * @param results The search results to display
     */
    void updateSearchResults(const std::vector<core::Package>& results);

    /**
     * @brief Get the names of selected packages
     * @return QStringList of package names
     */
    QStringList getSelectedPackageNames() const;

    /**
     * @brief Create a styled button
     * @param button Pointer to button pointer
     * @param text Button text
     */
    void createButton(QPushButton** button, const QString& text);

    /**
     * @brief Perform asynchronous search
     * @param searchText The text to search for
     */
    void performAsyncSearch(const QString& searchText);

    // UI components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_searchControlsLayout;
    QLineEdit* m_searchInput;
    QPushButton* m_searchButton;
    QHBoxLayout* m_packageActionsLayout;
    QPushButton* m_removeButton;
    QTreeView* m_packagesTable;
    QStandardItemModel* m_packagesModel;

    // Async search components
    QProgressDialog* m_searchProgressDialog;
    QFutureWatcher<std::vector<core::Package>>* m_searchWatcher;

    // Core components
    core::PackageManager* m_packageManager;
    std::vector<core::Package> m_installedPackages;
};

} // namespace gui
} // namespace pacmangui 