# AUR Manager Implementation Plan

## Overview
Create a dedicated AUR management system with its own tab, functionality, and clean code organization.

## Directory Structure
```
src/
  aur/
    - aur_manager.cpp
    - aur_manager.hpp
    - aur_package.cpp
    - aur_package.hpp
    - aur_helper.cpp
    - aur_helper.hpp
  gui/
    components/
      - aur_tab.cpp
      - aur_tab.hpp
      - terminal_dialog.cpp
      - terminal_dialog.hpp
```

## Components

### 1. AUR Manager (src/aur/aur_manager.{cpp,hpp})
Core functionality for AUR operations:
- Search AUR packages using RPC API
- Install/remove AUR packages
- Update AUR packages
- Track installed AUR packages
- Handle AUR helper detection and commands

### 2. AUR Package (src/aur/aur_package.{cpp,hpp})
Dedicated class for AUR packages:
- Package name, version, description
- Maintainer, votes, popularity
- Dependencies
- Install/build status
- Git URL and last updated

### 3. AUR Helper (src/aur/aur_helper.{cpp,hpp})
Class to manage AUR helpers:
- Auto-detect installed helpers (yay, paru, etc.)
- Store helper-specific commands
- Handle command execution
- Validate helper functionality

### 4. AUR Tab (src/gui/components/aur_tab.{cpp,hpp})
Dedicated tab for AUR operations:
- Search bar for AUR packages
- Results list with package details
- Installed AUR packages list
- Install/Remove buttons
- Update check button
- Progress indicators
- Filter options (by votes, popularity, etc.)

### 5. Terminal Dialog (src/gui/components/terminal_dialog.{cpp,hpp})
Popup window for installation progress:
- Embedded terminal view
- Progress bar
- Cancel button
- Output capture and display
- Error highlighting

## Features

### AUR Tab Layout
```
+----------------------------------+
|  Search AUR Packages: [        ] |
|  [Search] [Refresh] [Settings]   |
+----------------------------------+
|  Search Results                  |
|  +------------------------------+|
|  | Name | Version | Votes | ... ||
|  |...                          ||
|  +------------------------------+|
|  [Install Selected]              |
+----------------------------------+
|  Installed AUR Packages         |
|  +------------------------------+|
|  | Name | Version | Status     ||
|  |...                          ||
|  +------------------------------+|
|  [Remove Selected] [Check Updates]|
+----------------------------------+
```

### AUR Helper Detection
1. Auto-detect installed AUR helpers
2. Priority order: yay > paru > aurman > trizen > others
3. Store helper-specific commands
4. Allow manual override in settings

### Installation Process
1. Show confirmation dialog with package details
2. Display dependencies and disk space required
3. Open Terminal Dialog
4. Execute installation with progress updates
5. Show build/install logs in real-time
6. Handle errors and allow retry
7. Update installed packages list

### Update Process
1. Add "Check AUR Updates" button to System Update tab
2. Show available updates with version comparison
3. Allow selective updates
4. Show update progress in Terminal Dialog
5. Handle partial update failures

### Settings Integration
1. AUR helper selection
2. Default sort order for search results
3. Auto-update check interval
4. Terminal appearance settings
5. Build directory location

## Implementation Steps

1. Create directory structure and base classes
2. Implement AUR Helper detection and management
3. Develop AUR Package class with RPC integration
4. Create AUR Manager core functionality
5. Design and implement Terminal Dialog
6. Build AUR Tab UI and functionality
7. Integrate with System Update tab
8. Add settings and configuration
9. Test and refine error handling
10. Document usage and configuration

## Error Handling

1. AUR helper not found
2. Network connectivity issues
3. Build failures
4. Dependency conflicts
5. Authentication failures
6. Disk space issues
7. Package conflicts

## Future Enhancements

1. Package build customization
2. PKGBUILD editing
3. Split package handling
4. VCS package support
5. Build log archiving
6. Batch operations
7. Package statistics
8. Comment/review integration 