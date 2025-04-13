#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QGroupBox>
#include <QRadioButton>
#include <QSettings>

namespace pacmangui {
namespace gui {

/**
 * @brief Settings dialog for configuring application options
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit SettingsDialog(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~SettingsDialog();
    
    /**
     * @brief Get whether AUR support is enabled
     * @return bool True if AUR support is enabled
     */
    bool isAurEnabled() const;
    
    /**
     * @brief Get the selected AUR helper
     * @return QString The selected AUR helper (e.g., "yay", "paru")
     */
    QString getAurHelper() const;
    
    /**
     * @brief Save settings to QSettings
     */
    void saveSettings();
    
    /**
     * @brief Load settings from QSettings
     */
    void loadSettings();

private slots:
    /**
     * @brief AUR checkbox state changed
     * @param state New state of the checkbox
     */
    void onAurEnabledChanged(int state);
    
    /**
     * @brief Called when OK button is clicked
     */
    void onOkClicked();
    
    /**
     * @brief Called when Cancel button is clicked
     */
    void onCancelClicked();
    
    /**
     * @brief Called when Apply button is clicked
     */
    void onApplyClicked();
    
    /**
     * @brief Detection of AUR helpers
     */
    void detectAurHelpers();

private:
    void setupUi();
    void setupConnections();
    bool checkHelperExists(const QString& helper);
    
    // UI components
    QTabWidget* m_tabWidget;
    
    // AUR tab
    QWidget* m_aurTab;
    QCheckBox* m_enableAurCheckbox;
    QComboBox* m_aurHelperComboBox;
    QLabel* m_aurHelperLabel;
    
    // Appearance tab
    QWidget* m_appearanceTab;
    QCheckBox* m_darkThemeCheckbox;
    
    // Button box
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    
    // Settings
    bool m_aurEnabled;
    QString m_aurHelper;
    bool m_darkTheme;
};

} // namespace gui
} // namespace pacmangui 