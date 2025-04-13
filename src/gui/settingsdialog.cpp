#include "gui/settingsdialog.hpp"
#include <QProcess>
#include <QStandardPaths>
#include <QFile>
#include <QMessageBox>
#include <QDir>
#include <QFormLayout>

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
    , m_darkThemeCheckbox(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_applyButton(nullptr)
    , m_aurEnabled(false)
    , m_aurHelper("")
    , m_darkTheme(false)
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
    QVBoxLayout* themeLayout = new QVBoxLayout(themeGroupBox);
    
    m_darkThemeCheckbox = new QCheckBox("Use Dark Theme", themeGroupBox);
    themeLayout->addWidget(m_darkThemeCheckbox);
    
    appearanceLayout->addWidget(themeGroupBox);
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
    saveSettings();
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
    m_darkTheme = m_darkThemeCheckbox->isChecked();
    settings.setValue("appearance/darkTheme", m_darkTheme);
}

void SettingsDialog::loadSettings()
{
    QSettings settings("PacmanGUI", "PacmanGUI");
    
    // Load AUR settings
    m_aurEnabled = settings.value("aur/enabled", false).toBool();
    m_aurHelper = settings.value("aur/helper", "yay").toString();
    
    m_enableAurCheckbox->setChecked(m_aurEnabled);
    
    // Find and select the helper in combobox
    int index = m_aurHelperComboBox->findText(m_aurHelper);
    if (index >= 0) {
        m_aurHelperComboBox->setCurrentIndex(index);
    }
    
    // Enable/disable the helper combobox based on checkbox state
    m_aurHelperComboBox->setEnabled(m_aurEnabled);
    
    // Load appearance settings
    m_darkTheme = settings.value("appearance/darkTheme", false).toBool();
    m_darkThemeCheckbox->setChecked(m_darkTheme);
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
    
    // If no AUR helpers found, disable AUR support
    if (m_aurHelperComboBox->count() == 0) {
        m_enableAurCheckbox->setChecked(false);
        m_enableAurCheckbox->setEnabled(false);
        m_aurHelperComboBox->setEnabled(false);
        m_aurHelperLabel->setEnabled(false);
        
        // Add a placeholder item
        m_aurHelperComboBox->addItem("No AUR helpers found");
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