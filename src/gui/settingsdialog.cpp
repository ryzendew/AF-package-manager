#include "gui/settingsdialog.hpp"
#include <QProcess>
#include <QStandardPaths>
#include <QFile>
#include <QMessageBox>
#include <QDir>
#include <QFormLayout>
#include <iostream>

namespace pacmangui {
namespace gui {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_aurTab(nullptr)
    , m_enableAurCheckbox(nullptr)
    , m_aurHelperComboBox(nullptr)
    , m_aurHelperLabel(nullptr)
    , m_appearanceTab(nullptr)
    , m_themeComboBox(nullptr)
    , m_themeLabel(nullptr)
    , m_scalingFactorComboBox(nullptr)
    , m_scalingFactorLabel(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_applyButton(nullptr)
    , m_aurEnabled(false)
    , m_aurHelper("")
    , m_selectedTheme("dark_colorful")
    , m_scalingFactor(1.0)
{
    setWindowTitle("PacmanGUI Settings");
    setMinimumSize(500, 400);
    
    setupUi();
    setupConnections();
    loadSettings();
    detectAurHelpers();
}

SettingsDialog::~SettingsDialog()
{
    // Cleanup if needed
}

bool SettingsDialog::isAurEnabled() const
{
    return m_aurEnabled;
}

QString SettingsDialog::getAurHelper() const
{
    return m_aurHelper;
}

bool SettingsDialog::isDarkThemeEnabled() const
{
    return m_selectedTheme == "dark" || m_selectedTheme == "dark_colorful";
}

bool SettingsDialog::isDarkColorfulThemeEnabled() const
{
    return m_selectedTheme == "dark_colorful";
}

double SettingsDialog::getScalingFactor() const
{
    return m_scalingFactor;
}

void SettingsDialog::setupUi()
{
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Title label
    QLabel* titleLabel = new QLabel("PacmanGUI Settings", this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Tab widget
    m_tabWidget = new QTabWidget(this);
    
    // AUR Tab
    m_aurTab = new QWidget(m_tabWidget);
    QVBoxLayout* aurLayout = new QVBoxLayout(m_aurTab);
    
    QGroupBox* aurGroupBox = new QGroupBox("AUR Support", m_aurTab);
    QVBoxLayout* aurGroupLayout = new QVBoxLayout(aurGroupBox);
    
    m_enableAurCheckbox = new QCheckBox("Enable AUR Support", aurGroupBox);
    aurGroupLayout->addWidget(m_enableAurCheckbox);
    
    QFormLayout* helperLayout = new QFormLayout();
    m_aurHelperLabel = new QLabel("AUR Helper:", aurGroupBox);
    m_aurHelperComboBox = new QComboBox(aurGroupBox);
    m_aurHelperComboBox->addItem("yay");
    m_aurHelperComboBox->addItem("paru");
    m_aurHelperComboBox->setEnabled(false); // Initially disabled
    
    helperLayout->addRow(m_aurHelperLabel, m_aurHelperComboBox);
    aurGroupLayout->addLayout(helperLayout);
    
    aurLayout->addWidget(aurGroupBox);
    aurLayout->addStretch(1);
    
    // Appearance Tab
    m_appearanceTab = new QWidget(m_tabWidget);
    QVBoxLayout* appearanceLayout = new QVBoxLayout(m_appearanceTab);
    
    QGroupBox* themeGroupBox = new QGroupBox("Theme", m_appearanceTab);
    QFormLayout* themeLayout = new QFormLayout(themeGroupBox);
    
    m_themeLabel = new QLabel("Select Theme:", themeGroupBox);
    m_themeComboBox = new QComboBox(themeGroupBox);
    
    // Add theme options
    m_themeComboBox->addItem("Dark Colorful (Preferred)", "dark_colorful");
    m_themeComboBox->addItem("Dark", "dark");
    m_themeComboBox->addItem("Light", "light");
    m_themeComboBox->addItem("Light Colorful", "light_colorful");
    
    themeLayout->addRow(m_themeLabel, m_themeComboBox);
    
    // Add a note about theme changes
    QLabel* themeNoteLabel = new QLabel("Note: The Dark Colorful theme provides the best experience.", themeGroupBox);
    themeNoteLabel->setWordWrap(true);
    themeNoteLabel->setStyleSheet("font-style: italic; color: gray;");
    themeLayout->addRow(themeNoteLabel);
    
    // Add UI scaling section
    QGroupBox* scalingGroupBox = new QGroupBox("Display Scaling", m_appearanceTab);
    QFormLayout* scalingLayout = new QFormLayout(scalingGroupBox);
    
    m_scalingFactorLabel = new QLabel("Scaling Factor:", scalingGroupBox);
    m_scalingFactorComboBox = new QComboBox(scalingGroupBox);
    
    // Add scaling factor options
    m_scalingFactorComboBox->addItem("100% (Default)", 1.0);
    m_scalingFactorComboBox->addItem("125%", 1.25);
    m_scalingFactorComboBox->addItem("150%", 1.5);
    m_scalingFactorComboBox->addItem("175%", 1.75);
    m_scalingFactorComboBox->addItem("200%", 2.0);
    
    scalingLayout->addRow(m_scalingFactorLabel, m_scalingFactorComboBox);
    
    // Add a note about scaling changes requiring app restart
    QLabel* scalingNoteLabel = new QLabel("Note: Scaling changes will take effect after restarting the application.", scalingGroupBox);
    scalingNoteLabel->setWordWrap(true);
    scalingNoteLabel->setStyleSheet("font-style: italic; color: gray;");
    scalingLayout->addRow(scalingNoteLabel);
    
    appearanceLayout->addWidget(themeGroupBox);
    appearanceLayout->addWidget(scalingGroupBox);
    appearanceLayout->addStretch(1);
    
    // Add tabs to tab widget
    m_tabWidget->addTab(m_aurTab, "AUR");
    m_tabWidget->addTab(m_appearanceTab, "Appearance");
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    
    m_okButton = new QPushButton("OK", this);
    m_cancelButton = new QPushButton("Cancel", this);
    m_applyButton = new QPushButton("Apply", this);
    
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_applyButton);
    
    // Add all to main layout
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::setupConnections()
{
    // Connect checkbox state change to enable/disable helper dropdown
    connect(m_enableAurCheckbox, &QCheckBox::stateChanged, this, &SettingsDialog::onAurEnabledChanged);
    
    // Connect buttons
    connect(m_okButton, &QPushButton::clicked, this, &SettingsDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::onCancelClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsDialog::onApplyClicked);
}

void SettingsDialog::onAurEnabledChanged(int state)
{
    m_aurHelperComboBox->setEnabled(state == Qt::Checked);
}

void SettingsDialog::onOkClicked()
{
    saveSettings();
    accept();
}

void SettingsDialog::onCancelClicked()
{
    reject();
}

void SettingsDialog::onApplyClicked()
{
    std::cout << "SettingsDialog::onApplyClicked - Applying settings" << std::endl;
    
    // Store the current settings
    QString previousTheme = m_selectedTheme;
    bool previousAurEnabled = m_aurEnabled;
    std::cout << "SettingsDialog::onApplyClicked - Previous theme: " << previousTheme.toStdString() << std::endl;
    
    // Save settings
    saveSettings();
    
    std::cout << "SettingsDialog::onApplyClicked - New theme: " << m_selectedTheme.toStdString() << std::endl;
    
    // If theme changed, emit signal
    if (previousTheme != m_selectedTheme) {
        std::cout << "SettingsDialog::onApplyClicked - Theme changed, emitting themeChanged signal" << std::endl;
        emit themeChanged(isDarkThemeEnabled());
    }
    
    // If AUR status changed, emit signal
    if (previousAurEnabled != m_aurEnabled) {
        emit aurStatusChanged(m_aurEnabled);
    }
    
    std::cout << "SettingsDialog::onApplyClicked - Settings applied successfully" << std::endl;
}

void SettingsDialog::saveSettings()
{
    QSettings settings("PacmanGUI", "PacmanGUI");
    
    // Save AUR settings
    m_aurEnabled = m_enableAurCheckbox->isChecked();
    m_aurHelper = m_aurHelperComboBox->currentText();
    
    settings.setValue("aur/enabled", m_aurEnabled);
    settings.setValue("aur/helper", m_aurHelper);
    
    // Save appearance settings
    m_selectedTheme = m_themeComboBox->currentData().toString();
    std::cout << "SettingsDialog::saveSettings - Saving theme: " << m_selectedTheme.toStdString() << std::endl;
    settings.setValue("appearance/theme", m_selectedTheme);
    
    // For backward compatibility
    bool isDarkTheme = m_selectedTheme.startsWith("dark");
    std::cout << "SettingsDialog::saveSettings - Dark theme: " << (isDarkTheme ? "true" : "false") << std::endl;
    settings.setValue("appearance/darkTheme", isDarkTheme);
    
    bool isDarkColorful = m_selectedTheme == "dark_colorful";
    std::cout << "SettingsDialog::saveSettings - Dark colorful theme: " << (isDarkColorful ? "true" : "false") << std::endl;
    settings.setValue("appearance/darkColorfulTheme", isDarkColorful);
    
    // Save scaling factor
    m_scalingFactor = m_scalingFactorComboBox->currentData().toDouble();
    std::cout << "SettingsDialog::saveSettings - Scaling factor: " << m_scalingFactor << std::endl;
    settings.setValue("appearance/scalingFactor", m_scalingFactor);
    
    // Synchronize to ensure settings are saved
    settings.sync();
    std::cout << "SettingsDialog::saveSettings - Settings saved to " << settings.fileName().toStdString() << std::endl;
}

void SettingsDialog::loadSettings()
{
    QSettings settings("PacmanGUI", "PacmanGUI");
    
    // Load AUR settings
    m_aurEnabled = settings.value("aur/enabled", false).toBool();
    m_aurHelper = settings.value("aur/helper", "yay").toString();
    
    m_enableAurCheckbox->setChecked(m_aurEnabled);
    int helperIndex = m_aurHelperComboBox->findText(m_aurHelper);
    if (helperIndex >= 0) {
        m_aurHelperComboBox->setCurrentIndex(helperIndex);
    }
    m_aurHelperComboBox->setEnabled(m_aurEnabled);
    
    // Load appearance settings
    m_selectedTheme = settings.value("appearance/theme", "dark_colorful").toString();
    
    // For backward compatibility
    bool darkTheme = settings.value("appearance/darkTheme", true).toBool();
    bool darkColorful = settings.value("appearance/darkColorfulTheme", true).toBool();
    
    if (!settings.contains("appearance/theme")) {
        if (darkColorful) {
            m_selectedTheme = "dark_colorful";
        } else if (darkTheme) {
            m_selectedTheme = "dark";
        } else {
            m_selectedTheme = "light";
        }
    }
    
    int themeIndex = m_themeComboBox->findData(m_selectedTheme);
    if (themeIndex >= 0) {
        m_themeComboBox->setCurrentIndex(themeIndex);
    }
    
    // Load scaling factor settings
    m_scalingFactor = settings.value("appearance/scalingFactor", 1.0).toDouble();
    for (int i = 0; i < m_scalingFactorComboBox->count(); ++i) {
        double factor = m_scalingFactorComboBox->itemData(i).toDouble();
        if (qFuzzyCompare(factor, m_scalingFactor)) {
            m_scalingFactorComboBox->setCurrentIndex(i);
            break;
        }
    }
}

void SettingsDialog::detectAurHelpers()
{
    // Store the current selection
    QString currentHelper = m_aurHelperComboBox->currentText();
    
    // Clear the combobox
    m_aurHelperComboBox->clear();
    
    // Check for yay
    if (checkHelperExists("yay")) {
        m_aurHelperComboBox->addItem("yay");
    }
    
    // Check for paru
    if (checkHelperExists("paru")) {
        m_aurHelperComboBox->addItem("paru");
    }
    
    // Check for pikaur
    if (checkHelperExists("pikaur")) {
        m_aurHelperComboBox->addItem("pikaur");
    }
    
    // Check for trizen
    if (checkHelperExists("trizen")) {
        m_aurHelperComboBox->addItem("trizen");
    }
    
    // Check for pacaur
    if (checkHelperExists("pacaur")) {
        m_aurHelperComboBox->addItem("pacaur");
    }
    
    // If no AUR helpers found, disable AUR support
    if (m_aurHelperComboBox->count() == 0) {
        m_enableAurCheckbox->setChecked(false);
        m_enableAurCheckbox->setEnabled(false);
        m_aurHelperComboBox->setEnabled(false);
        m_aurHelperLabel->setEnabled(false);
        
        // Add a placeholder item
        m_aurHelperComboBox->addItem("No AUR helpers found");
        
        // Show a message suggesting installation of an AUR helper
        QLabel* helperInstallLabel = new QLabel(
            "No AUR helpers found. Please install yay, paru, pikaur, trizen, or pacaur "
            "to enable AUR support.", m_aurTab);
        helperInstallLabel->setWordWrap(true);
        helperInstallLabel->setStyleSheet("color: red;");
        
        // Find the layout of aurTab
        if (QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(m_aurTab->layout())) {
            layout->addWidget(helperInstallLabel);
        }
    } else {
        m_enableAurCheckbox->setEnabled(true);
        m_aurHelperLabel->setEnabled(true);
        
        // Restore previous selection if it exists
        int index = m_aurHelperComboBox->findText(currentHelper);
        if (index >= 0) {
            m_aurHelperComboBox->setCurrentIndex(index);
        }
    }
}

bool SettingsDialog::checkHelperExists(const QString& helper)
{
    // Check if the command exists in the system
    QString path = QStandardPaths::findExecutable(helper);
    return !path.isEmpty();
}

} // namespace gui
} // namespace pacmangui 