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
     * @brief Get the default AUR helper
     * @return QString The default AUR helper (e.g., "yay")
     */
    QString getAurHelper() const;
    
    /**
     * @brief Get whether dark theme is enabled
     * @return bool True if dark theme is enabled
     */
    bool isDarkThemeEnabled() const;
    
    /**
     * @brief Get whether dark colorful theme is enabled
     * @return bool True if dark colorful theme is enabled
     */
    bool isDarkColorfulThemeEnabled() const;
    
    /**
     * @brief Get the UI scaling factor
     * @return double The scaling factor (e.g., 1.0, 1.25, 1.5)
     */
    double getScalingFactor() const;
    
    /**
     * @brief Save settings to QSettings
     */
    void saveSettings();
    
    /**
     * @brief Load settings from QSettings
     */
    void loadSettings();

signals:
    /**
     * @brief Signal emitted when theme is changed
     * @param isDark Whether dark theme is enabled
     */
    void themeChanged(bool isDark);
    
    /**
     * @brief Signal emitted when AUR status is changed
     * @param enabled Whether AUR is enabled
     */
    void aurStatusChanged(bool enabled);

private slots:
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
    
    // Appearance tab
    QWidget* m_appearanceTab;
    QComboBox* m_themeComboBox;
    QLabel* m_themeLabel;
    QComboBox* m_scalingFactorComboBox;
    QLabel* m_scalingFactorLabel;
    
    // Button box
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    
    // Settings
    bool m_aurEnabled;
    QString m_selectedTheme;
    double m_scalingFactor;
};

} // namespace gui
} // namespace pacmangui 