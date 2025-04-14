# PacmanGUI Improvement Plan

## Completed Fixes
1. ✅ Improved UI layout for all tabs with better button placement
2. ✅ Made buttons smaller and more proportional
3. ✅ Fixed "Check for Updates" and "Update System" functionality
4. ✅ Added context-sensitive "Install" / "Install Selected Packages" button behavior
5. ✅ Added working package installation and removal
6. ✅ Added maintenance functionality for clearing package cache and removing orphaned packages
7. ✅ Implemented settings dialog functionality
8. ✅ Added dark/light theme toggling
9. ✅ Fixed Wayland segmentation fault issue
10. ✅ Added asynchronous search implementation
11. ✅ Disabled auto-search on keystroke
12. ✅ Removed "Install from AUR" button
13. ✅ Removed Update buttons
14. ✅ Simplified AUR settings tab
15. ✅ Added checkboxes for multi-package selection

## Critical Issues to Address

### 1. Broken Light Theme
- **Issue**: Light theme is broken and doesn't apply correctly
- **Fix**:
  - Fix the light theme stylesheet in resources/styles/light.qss
  - Ensure proper color contrast and readability
  - Test theme switching to ensure it applies consistently
  - Create a fallback mechanism that guarantees a readable UI

### 2. Authentication for System Updates Not Working
- **Issue**: No password prompt appears when performing system updates
- **Fix**:
  - Fix the pkexec implementation to properly prompt for authentication
  - Ensure polkit integration works correctly
  - Test on different desktop environments for compatibility
  - Add fallback authentication methods if needed

### 3. Poor UI/UX Design
- **Issue**: GUI looks unprofessional with alignment and design issues
- **Fix**:
  - Implement pixel-perfect alignment for all UI elements
  - Create a clean, modern, flat, and colorful design language
  - Standardize margins, padding, and spacing throughout the application
  - Add proper visual hierarchy with consistent typography
  - Improve visual feedback for interactive elements
  - Redesign table headers and content alignment

## Detailed Design Improvements

### 1. UI Layout and Alignment
- **Issue**: Elements are not properly aligned, creating a messy appearance
- **Fix**:
  - Implement a strict grid system with consistent spacing
  - Align all buttons, labels, and inputs to the same baseline
  - Use fixed margins (12px) between UI elements for consistency
  - Ensure table columns are properly sized and aligned

### 2. Button Design and Styling
- **Issue**: Buttons look squashed and don't follow modern design principles
- **Fix**:
  - Redesign buttons with proper padding (10px horizontal, i8px vertical)
  - Add subtle hover and click animations
  - Use a consistent color palette for button states
  - Implement proper spacing between buttons (8px)
  - Add subtle shadows for depth

### 3. Table Design and Readability
- **Issue**: Tables lack visual hierarchy and are difficult to scan
- **Fix**:
  - Redesign table headers with distinct styling
  - Improve alternating row colors for better readability
  - Add subtle dividers between columns
  - Implement consistent padding in table cells
  - Improve selection highlighting

### 4. Color Palette and Visual Identity
- **Issue**: Inconsistent use of colors throughout the application
- **Fix**:
  - Define a professional color palette with primary, secondary, and accent colors
  - Ensure proper color contrast for accessibility
  - Use color consistently to convey meaning and state
  - Create separate light and dark theme palettes that share the same design language

## Implementation Tasks

### Fix Light Theme
```css
/* light.qss - Example improvements */
QWidget {
    background-color: #f8f9fa;
    color: #212529;
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
}

QMainWindow {
    background-color: #ffffff;
}

QPushButton {
    background-color: #0078d7;
    color: white;
    border: none;
    border-radius: 4px;
    padding: 8px 16px;
    font-weight: medium;
}

QPushButton:hover {
    background-color: #106ebe;
}

QPushButton:pressed {
    background-color: #005a9e;
}

QTreeView, QTableView {
    background-color: #ffffff;
    alternate-background-color: #f5f7fa;
    border: 1px solid #e0e0e0;
    selection-background-color: #cce5ff;
    selection-color: #212529;
}

QHeaderView::section {
    background-color: #e9ecef;
    color: #495057;
    padding: 6px;
    border: none;
    border-right: 1px solid #dee2e6;
    border-bottom: 1px solid #dee2e6;
    font-weight: bold;
}
```

### Fix Authentication for Updates
```cpp
void MainWindow::onSystemUpdate() {
    // Create a better process for system updates
    QProcess* process = new QProcess(this);
    
    // Ensure we capture all output
    process->setProcessChannelMode(QProcess::MergedChannels);
    
    // Clear the log and update UI
    m_systemUpdateLogView->clear();
    m_systemUpdateLogView->append(tr("Starting system update..."));
    showStatusMessage(tr("System update started"), 0);
    
    // Set up the UI to show progress
    m_systemUpdateProgressBar->setVisible(true);
    m_systemUpdateProgressBar->setRange(0, 0);  // Indeterminate progress
    
    // Connect to capture output
    connect(process, &QProcess::readyReadStandardOutput, [=]() {
        QString output = process->readAllStandardOutput();
        m_systemUpdateLogView->append(output);
        // Auto-scroll to make sure latest output is visible
        m_systemUpdateLogView->verticalScrollBar()->setValue(
            m_systemUpdateLogView->verticalScrollBar()->maximum()
        );
    });
    
    // Connect to process completion
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus) {
            // Hide progress indicator
            m_systemUpdateProgressBar->setVisible(false);
            
            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                m_systemUpdateLogView->append(tr("System update completed successfully."));
                showStatusMessage(tr("System update completed successfully"), 5000);
            } else {
                m_systemUpdateLogView->append(tr("System update failed with exit code %1").arg(exitCode));
                showStatusMessage(tr("System update failed"), 5000);
                
                // Show detailed error dialog
                QMessageBox::critical(this, tr("Update Failed"),
                    tr("The system update failed with exit code %1.\n\n"
                       "Please check the update log for details.").arg(exitCode));
            }
            
            // Clean up
            process->deleteLater();
            
            // Refresh data
            refreshInstalledPackages();
        }
    );
    
    // Use pkexec with full path and proper arguments
    // This should trigger the polkit authentication dialog
    QString pkexecPath = QStandardPaths::findExecutable("pkexec");
    QString pacmanPath = QStandardPaths::findExecutable("pacman");
    
    if (pkexecPath.isEmpty() || pacmanPath.isEmpty()) {
        m_systemUpdateLogView->append(tr("Error: Could not find pkexec or pacman executable"));
        showStatusMessage(tr("Update failed: Missing executables"), 5000);
        return;
    }
    
    // Start the process with explicit paths to ensure it works
    process->start(pkexecPath, QStringList() << pacmanPath << "-Syu" << "--noconfirm");
}
```

### Fix UI Alignment and Styling
```cpp
// Define consistent spacing and style constants
const int MARGIN = 12;
const int PADDING = 8;
const int SPACING = 8;
const int BUTTON_HEIGHT = 32;

void MainWindow::setupUi() {
    // Apply consistent margins and spacing
    m_mainLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    m_mainLayout->setSpacing(SPACING);
    
    // Style the tab widget for better appearance
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabPosition(QTabWidget::North);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #d0d0d0; }"
        "QTabBar::tab { padding: 8px 16px; }"
        "QTabBar::tab:selected { border-bottom: 2px solid #0078d7; }"
    );
    
    // Create search controls with proper alignment
    m_searchInput->setMinimumHeight(BUTTON_HEIGHT);
    m_searchButton->setFixedHeight(BUTTON_HEIGHT);
    m_searchControlsLayout->setContentsMargins(0, 0, 0, 0);
    m_searchControlsLayout->setSpacing(SPACING);
    
    // Align buttons consistently
    m_packageActionsLayout->setContentsMargins(0, PADDING, 0, PADDING);
    m_packageActionsLayout->setSpacing(SPACING);
    
    // Create modern, flat buttons
    for (QPushButton* button : actionButtons) {
        button->setFixedHeight(BUTTON_HEIGHT);
        button->setMinimumWidth(100);
        button->setStyleSheet(
            "QPushButton {"
            "    background-color: #0078d7;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 4px;"
            "    padding: 8px 16px;"
            "    font-weight: medium;"
            "}"
            "QPushButton:hover {"
            "    background-color: #106ebe;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #005a9e;"
            "}"
        );
    }
    
    // Style tables consistently
    QList<QTreeView*> tables = {m_packagesTable, m_installedTable, m_systemUpdatesTable};
    for (QTreeView* table : tables) {
        table->setStyleSheet(
            "QTreeView {"
            "    border: 1px solid #d0d0d0;"
            "    alternate-background-color: #f5f7fa;"
            "    selection-background-color: #cce5ff;"
            "}"
            "QHeaderView::section {"
            "    background-color: #e9ecef;"
            "    color: #495057;"
            "    padding: 6px;"
            "    border: none;"
            "    border-right: 1px solid #dee2e6;"
            "    border-bottom: 1px solid #dee2e6;"
            "    font-weight: bold;"
            "}"
        );
    }
}
```

## Implementation Priority
1. Fix the light theme to ensure it applies properly and looks professional
2. Fix the authentication mechanism for system updates
3. Implement the UI/UX improvements for a cleaner, more professional appearance
4. Test thoroughly on different systems to ensure compatibility

## Timeline
- Week 1: Fix critical issues (theme and authentication)
- Week 2: Implement comprehensive UI/UX redesign
- Week 3: Testing and refinement 