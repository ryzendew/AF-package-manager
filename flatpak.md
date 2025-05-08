# Flatpak Support Implementation

## Basic Rules to Follow
1. We don't remove things but we refactor them to reduce breaking functionality
3. Test building
4. We make sure the GUI does not break and stays stable unless things must be added
5. When adding third-party packages, add them via a submodule so GitHub can see it properly
6. WE DON'T DO STUBS
7. if we run into segfaults we revert and refactor
8. make sure all UI components remain and nothing gets removed.
9. build with all threads
10. double check and triple check for dups and missing headers and more
11. We keep this layout always https://media.discordapp.net/attachments/1347398051630809218/1361544483820404826/unknown.png?ex=680b021d&is=6809b09d&hm=d8af418c6e828c525f29a849982d997de561f05952f94e3d84729bf6e0a7d519&=&format=webp&quality=lossless
12. We give every major thing it's own cpp/hpp to make things hard to break.

## Overview
This document outlines the implementation of Flatpak support in the PacmanGUI application. Flatpak is a system for building, distributing, and running sandboxed desktop applications on Linux.

## Current Status
- ✅ Search for Flatpak applications
- ✅ View currently installed Flatpaks
- ❌ Install new Flatpak applications
- ❌ Update existing Flatpaks
- ❌ Remove Flatpak applications

## Implementation Status

### Currently Implemented
1. **Basic Structure**
   - FlatpakManager class definition
   - FlatpakApp and FlatpakRemote structures
   - Basic GUI layout for Flatpak tab

2. **Search Functionality**
   - Basic search implementation
   - Search results display
   - Search filtering

3. **Viewing Installed Apps**
   - List of installed Flatpaks
   - Basic app information display
   - App status indicators

### Pending Implementation

#### Phase 1: Core Flatpak Operations
1. **Installation**
   - [ ] Implement `install_app` method
   - [ ] Add progress tracking
   - [ ] Handle installation errors
   - [ ] Add installation confirmation dialog
   - [ ] Implement batch installation

2. **Updates**
   - [ ] Implement `update_app` method
   - [ ] Add update progress tracking
   - [ ] Handle update conflicts
   - [ ] Add update confirmation dialog
   - [ ] Implement batch updates

3. **Removal**
   - [ ] Implement `remove_app` method
   - [ ] Add removal confirmation
   - [ ] Handle removal dependencies
   - [ ] Implement data cleanup
   - [ ] Add batch removal

#### Phase 2: Remote Management
1. **Remote Operations**
   - [ ] Implement remote listing
   - [ ] Add remote addition
   - [ ] Add remote removal
   - [ ] Add remote refresh
   - [ ] Handle remote errors

2. **Remote UI**
   - [ ] Create remote management interface
   - [ ] Add remote status indicators
   - [ ] Add remote configuration options
   - [ ] Implement remote search

#### Phase 3: User Data Management
1. **Data Operations**
   - [ ] Implement data directory tracking
   - [ ] Add data cleanup options
   - [ ] Add data backup/restore
   - [ ] Handle data permissions

2. **Data UI**
   - [ ] Create data management interface
   - [ ] Add data size indicators
   - [ ] Add data cleanup options
   - [ ] Implement data search

#### Phase 4: Advanced Features
1. **Permission Management**
   - [ ] Implement permission tracking
   - [ ] Add permission modification
   - [ ] Add permission recommendations
   - [ ] Handle permission conflicts

2. **Runtime Management**
   - [ ] Implement runtime tracking
   - [ ] Add runtime updates
   - [ ] Handle runtime dependencies
   - [ ] Add runtime configuration

3. **Application Isolation**
   - [ ] Implement isolation settings
   - [ ] Add isolation configuration
   - [ ] Handle isolation conflicts
   - [ ] Add isolation recommendations

## Implementation Plan

### Phase 1: Core Operations
1. **Installation**
   - Implement basic installation
   - Add progress tracking
   - Test installation flow

2. **Updates**
   - Implement update functionality
   - Add update progress
   - Test update flow

3. **Removal**
   - Implement removal functionality
   - Add removal confirmation
   - Test removal flow

### Phase 2: Remote Management
1. **Remote Operations**
   - Implement remote listing
   - Add remote management
   - Test remote operations

2. **Remote UI**
   - Create remote interface
   - Add remote status
   - Test remote UI

3. **Integration**
   - Integrate remote features
   - Test full remote flow
   - Fix any issues

### Phase 3: User Data Management
1. **Data Operations**
   - Implement data tracking
   - Add data management
   - Test data operations

2. **Data UI**
   - Create data interface
   - Add data indicators
   - Test data UI

3. **Integration**
   - Integrate data features
   - Test full data flow
   - Fix any issues

### Phase 4: Advanced Features
1. **Permissions**
   - Implement permission system
   - Add permission UI
   - Test permissions

2. **Runtime**
   - Implement runtime management
   - Add runtime UI
   - Test runtime features

3. **Final Integration**
   - Integrate all features
   - Perform full testing
   - Fix any remaining issues

## Testing Strategy

### Unit Tests
1. **Core Operations**
   - Test installation
   - Test updates
   - Test removal
   - Test error handling

2. **Remote Management**
   - Test remote operations
   - Test remote errors
   - Test remote UI

3. **Data Management**
   - Test data operations
   - Test data errors
   - Test data UI

### Integration Tests
1. **Full Flow Tests**
   - Test complete installation flow
   - Test complete update flow
   - Test complete removal flow

2. **Error Handling**
   - Test network errors
   - Test permission errors
   - Test dependency errors

3. **UI Tests**
   - Test all UI components
   - Test user interactions
   - Test error displays

## Documentation

### API Documentation
1. **Core Classes**
   - Document FlatpakManager
   - Document FlatpakApp
   - Document FlatpakRemote

2. **Methods**
   - Document all public methods
   - Document error handling
   - Document return values

### User Documentation
1. **User Guide**
   - Installation guide
   - Update guide
   - Removal guide
   - Remote management guide
   - Data management guide

2. **Troubleshooting**
   - Common issues
   - Error messages
   - Solutions

## Notes

- Follow the project's core rules:
  - No removal of functionality, only refactoring
  - No use of nullptr
  - Test building
  - Maintain GUI stability
  - Use submodules for third-party dependencies
  - No stubs

- Ensure proper error handling and user feedback
- Maintain consistent UI/UX with existing features
- Follow security best practices
- Document all new features and APIs
- Implement proper progress tracking
- Handle user data management
- Support batch operations
- Manage remote repositories
- Implement proper permission handling

## Implementation Requirements

### 1. Dependencies
- `flatpak` package must be installed on the system
- `libflatpak` development library for integration
- Qt6 for GUI components

### 2. Core Components

#### FlatpakManager Class
```cpp
class FlatpakManager {
public:
    // Initialize Flatpak
    bool initialize();
    
    // Search for Flatpak applications
    std::vector<FlatpakApp> search_apps(const std::string& query);
    
    // Get installed Flatpak applications
    std::vector<FlatpakApp> get_installed_apps();
    
    // Install a Flatpak application
    bool install_app(const std::string& app_id, const std::string& remote = "flathub");
    
    // Update a Flatpak application
    bool update_app(const std::string& app_id);
    
    // Remove a Flatpak application
    bool remove_app(const std::string& app_id);
    
    // Get application details
    FlatpakApp get_app_details(const std::string& app_id);

    // Additional methods based on Warehouse implementation
    bool install_app_with_progress(const std::string& app_id, const std::string& remote, 
                                 std::function<void(int)> progress_callback);
    bool remove_app_with_data(const std::string& app_id, bool remove_data = false);
    bool update_all_apps(std::function<void(int)> progress_callback);
    std::vector<FlatpakRemote> get_remotes();
    bool add_remote(const std::string& name, const std::string& url);
    bool remove_remote(const std::string& name);
    bool refresh_remotes();
};
```

#### FlatpakApp Structure
```cpp
struct FlatpakApp {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string remote;
    bool is_installed;
    std::string size;
    std::string runtime;
    std::string branch;
    std::string arch;
    std::string origin;
    std::string ref;
    std::string commit;
    std::string latest_commit;
    std::string deploy_dir;
    std::string data_dir;
    std::vector<std::string> permissions;
};
```

#### FlatpakRemote Structure
```cpp
struct FlatpakRemote {
    std::string name;
    std::string url;
    std::string title;
    std::string comment;
    std::string description;
    std::string homepage;
    std::string icon;
    std::string gpg_key;
    bool is_system;
    bool is_enabled;
};
```

### 3. GUI Integration

#### FlatpakTab Component
- Search bar for Flatpak applications
- List view of available applications
- Installation/Update/Remove buttons
- Application details panel
- Progress indicators for operations
- Remote management interface
- Batch operations support
- User data management

#### Required UI Elements
- Flatpak search input
- Application list with columns:
  - Name
  - Version
  - Size
  - Remote
  - Status
  - Branch
  - Architecture
- Action buttons:
  - Install
  - Update
  - Remove
  - Remove with Data
  - View Details
  - Manage Permissions
- Progress dialog for operations
- Error handling and user feedback
- Remote management section
- Batch operations panel
- User data management section

### 4. Implementation Steps

1. **Core Integration**
   - Add FlatpakManager class to core components
   - Implement basic Flatpak operations
   - Add error handling and logging
   - Implement progress tracking
   - Add remote management
   - Add user data management

2. **GUI Development**
   - Create FlatpakTab component
   - Implement search functionality
   - Add application listing
   - Create installation/update/remove dialogs
   - Add progress tracking UI
   - Implement remote management interface
   - Add batch operations support
   - Create user data management interface

3. **Testing**
   - Unit tests for FlatpakManager
   - Integration tests for GUI components
   - End-to-end testing of Flatpak operations
   - Performance testing
   - Security testing

4. **Documentation**
   - API documentation
   - User guide for Flatpak operations
   - Troubleshooting guide
   - Security guidelines

### 5. Security Considerations

- Proper sandboxing of Flatpak applications
- Secure handling of remote repositories
- User authentication for system-wide operations
- Validation of application signatures
- Permission management
- Data isolation
- Secure storage of user data
- Remote verification

### 6. Error Handling

- Network connectivity issues
- Repository access problems
- Installation failures
- Update conflicts
- Removal dependencies
- Permission issues
- Data access problems
- Remote synchronization errors
- Progress tracking failures

### 7. Performance Optimization

- Caching of application lists
- Asynchronous operations
- Progress tracking
- Resource usage monitoring
- Batch operation optimization
- Remote synchronization optimization
- Data management optimization

## Usage Examples

### Searching for Flatpaks
```cpp
FlatpakManager manager;
manager.initialize();
auto results = manager.search_apps("firefox");
```

### Installing a Flatpak
```cpp
FlatpakManager manager;
manager.initialize();
bool success = manager.install_app("org.mozilla.firefox", "flathub");
```

### Installing with Progress
```cpp
FlatpakManager manager;
manager.initialize();
bool success = manager.install_app_with_progress(
    "org.mozilla.firefox",
    "flathub",
    [](int progress) {
        std::cout << "Installation progress: " << progress << "%" << std::endl;
    }
);
```

### Updating Flatpaks
```cpp
FlatpakManager manager;
manager.initialize();
bool success = manager.update_app("org.mozilla.firefox");
```

### Managing Remotes
```cpp
FlatpakManager manager;
manager.initialize();

// Add a new remote
bool success = manager.add_remote("flathub-beta", "https://flathub.org/beta-repo/flathub-beta.flatpakrepo");

// Remove a remote
success = manager.remove_remote("flathub-beta");

// Refresh all remotes
success = manager.refresh_remotes();
```

### Managing User Data
```cpp
FlatpakManager manager;
manager.initialize();

// Remove app with its data
bool success = manager.remove_app_with_data("org.mozilla.firefox", true);
```

## Future Enhancements

1. **Batch Operations**
   - Install multiple applications
   - Update all applications
   - Remove multiple applications
   - Batch permission management
   - Batch data management

2. **Repository Management**
   - Add/remove repositories
   - Update repository information
   - Repository status monitoring
   - Repository verification
   - Repository synchronization

3. **Advanced Features**
   - Application permissions management
   - Runtime management
   - Application isolation settings
   - Data backup and restore
   - Application snapshots
   - Version rollback support

4. **User Experience**
   - Installation progress tracking
   - Better error reporting
   - Application recommendations
   - Usage statistics
   - Update notifications
   - Permission recommendations

## Dependencies

Add the following to CMakeLists.txt:
```cmake
find_package(Flatpak REQUIRED)
target_link_libraries(pacmangui PRIVATE Flatpak::Flatpak)
```

## Notes

- Follow the project's core rules:
  - No removal of functionality, only refactoring
  - No use of nullptr
  - Test building
  - Maintain GUI stability
  - Use submodules for third-party dependencies
  - No stubs

- Ensure proper error handling and user feedback
- Maintain consistent UI/UX with existing features
- Follow security best practices
- Document all new features and APIs
- Implement proper progress tracking
- Handle user data management
- Support batch operations
- Manage remote repositories
- Implement proper permission handling

## Settings Window Integration

### Flatpak Settings Tab
1. **Remote Management**
   - Default remote selection
   - Remote auto-refresh interval
   - Remote verification settings
   - Remote priority order
   - Remote proxy settings

2. **Installation Settings**
   - Default installation location (user/system)
   - Installation confirmation preferences
   - Default branch selection
   - Installation progress display options
   - Installation history retention

3. **Update Settings**
   - Auto-update check interval
   - Update notification preferences
   - Update confirmation preferences
   - Update progress display options
   - Update history retention

4. **Data Management**
   - Default data cleanup behavior
   - Data backup preferences
   - Data retention policy
   - Data location preferences
   - Data size display format

5. **Permission Settings**
   - Default permission templates
   - Permission recommendation settings
   - Permission conflict resolution
   - Permission history retention
   - Permission display format

6. **Runtime Settings**
   - Default runtime selection
   - Runtime update preferences
   - Runtime verification settings
   - Runtime history retention
   - Runtime display format

7. **UI Preferences**
   - List view customization
   - Column visibility options
   - Sort preferences
   - Filter defaults
   - Theme integration

8. **Advanced Settings**
   - Debug logging level
   - Performance optimization options
   - Network timeout settings
   - Cache management
   - Security verification level

### Settings Implementation
```cpp
struct FlatpakSettings {
    // Remote Settings
    std::string default_remote;
    int remote_refresh_interval;
    bool verify_remotes;
    std::vector<std::string> remote_priority;
    std::string proxy_settings;

    // Installation Settings
    bool install_system_wide;
    bool confirm_installations;
    std::string default_branch;
    bool show_progress;
    int history_retention;

    // Update Settings
    int update_check_interval;
    bool notify_updates;
    bool confirm_updates;
    bool show_update_progress;
    int update_history_retention;

    // Data Settings
    bool auto_cleanup_data;
    bool backup_data;
    int data_retention_days;
    std::string data_location;
    std::string size_display_format;

    // Permission Settings
    std::string default_permission_template;
    bool show_permission_recommendations;
    std::string conflict_resolution;
    int permission_history_retention;
    std::string permission_display_format;

    // Runtime Settings
    std::string default_runtime;
    bool auto_update_runtime;
    bool verify_runtime;
    int runtime_history_retention;
    std::string runtime_display_format;

    // UI Settings
    std::string list_view_style;
    std::vector<std::string> visible_columns;
    std::string sort_preference;
    std::string default_filter;
    bool use_system_theme;

    // Advanced Settings
    int log_level;
    bool enable_performance_optimization;
    int network_timeout;
    int cache_size;
    int security_verification_level;
};
```

### Settings Dialog Integration
```cpp
class FlatpakSettingsDialog : public QDialog {
public:
    FlatpakSettingsDialog(QWidget* parent = nullptr);
    ~FlatpakSettingsDialog();

    void loadSettings();
    void saveSettings();
    void applySettings();

private:
    void setupUi();
    void setupConnections();
    void setupRemoteTab();
    void setupInstallationTab();
    void setupUpdateTab();
    void setupDataTab();
    void setupPermissionTab();
    void setupRuntimeTab();
    void setupUITab();
    void setupAdvancedTab();

    FlatpakSettings m_settings;
    QTabWidget* m_tabWidget;
    // ... other UI elements
};
```

### Benefits of Settings Window Integration
1. **Reduced Complexity**
   - Keeps FlatpakManager focused on core operations
   - Separates configuration from functionality
   - Makes code more maintainable

2. **Better User Experience**
   - Centralized settings management
   - Consistent with application design
   - Easier to find and modify settings

3. **Improved Maintainability**
   - Clear separation of concerns
   - Easier to add new settings
   - Better code organization

4. **Enhanced Flexibility**
   - Settings can be changed without modifying core code
   - Easier to implement user preferences
   - Better support for different use cases

### Settings Storage
1. **Configuration File**
   - Store settings in user's config directory
   - Use JSON or INI format
   - Include version information
   - Support migration between versions

2. **Default Values**
   - Provide sensible defaults
   - Document default values
   - Allow system-wide defaults
   - Support user overrides

3. **Settings Validation**
   - Validate settings on load
   - Provide fallback values
   - Log validation errors
   - Notify user of issues

### Settings Usage
```cpp
// In FlatpakManager
class FlatpakManager {
private:
    FlatpakSettings m_settings;

public:
    void loadSettings() {
        // Load settings from configuration
        m_settings = loadSettingsFromFile();
    }

    bool install_app(const std::string& app_id) {
        // Use settings for installation
        if (m_settings.confirm_installations) {
            // Show confirmation dialog
        }
        // ... rest of installation logic
    }
};
``` 