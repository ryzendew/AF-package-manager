#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QProgressBar>
#include <vector>
#include "core/transaction.hpp"
#include "core/package.hpp"

namespace pacmangui {
namespace gui {

/**
 * @brief Dialog for transaction confirmation and progress
 */
class TransactionDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * 
     * @param parent Parent widget
     * @param transaction Transaction to display
     * @param packages Packages affected by the transaction
     * @param dependencies Dependencies required by the transaction
     * @param conflicts Conflicts found in the transaction
     */
    TransactionDialog(QWidget *parent, 
                     core::Transaction *transaction,
                     const std::vector<core::Package>& packages,
                     const std::vector<core::Package>& dependencies,
                     const std::vector<std::string>& conflicts);
    
    /**
     * @brief Destructor
     */
    ~TransactionDialog();

public slots:
    /**
     * @brief Update progress
     * 
     * @param progress Progress percentage
     * @param message Progress message
     */
    void updateProgress(int progress, const QString& message);
    
    /**
     * @brief Show completion
     * 
     * @param success Whether transaction completed successfully
     * @param message Completion message
     */
    void showCompletion(bool success, const QString& message);

private slots:
    /**
     * @brief Handle proceed button click
     */
    void onProceed();
    
    /**
     * @brief Handle cancel button click
     */
    void onCancel();

private:
    /**
     * @brief Create UI elements
     */
    void createUI();

    core::Transaction *m_transaction;            ///< Transaction being displayed
    std::vector<core::Package> m_packages;       ///< Affected packages
    std::vector<core::Package> m_dependencies;   ///< Required dependencies
    std::vector<std::string> m_conflicts;        ///< Conflicting packages
    
    // UI components
    QLabel *m_title_label;                       ///< Dialog title label
    QListWidget *m_package_list;                 ///< List of packages
    QListWidget *m_dependencies_list;            ///< List of dependencies
    QListWidget *m_conflicts_list;               ///< List of conflicts
    QProgressBar *m_progress_bar;                ///< Progress indicator
    QLabel *m_message_label;                     ///< Status message label
    QPushButton *m_proceed_button;               ///< Proceed button
    QPushButton *m_cancel_button;                ///< Cancel button
    QDialogButtonBox *m_button_box;              ///< Button container
};

} // namespace gui
} // namespace pacmangui 