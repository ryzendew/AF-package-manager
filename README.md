# Pacman GUI

A modern GUI frontend for the Arch Linux package manager (pacman) built with Qt6.

## Current Features

### Package Management
- Search and browse available packages
- Install, update, and remove packages with a clean user interface
- View detailed package information
- Batch install multiple packages at once
- Password authentication for sudo operations
- Real-time terminal output display for system updates

### User Interface
- Modern Qt6-based interface
- Dark and light theme support
- Multi-selection with checkboxes for batch operations
- Progress tracking for long-running operations
- Package overwrite options for resolving conflicts
- Confirmation dialogs for critical operations

### System Operations
- System-wide updates with detailed progress
- Package database synchronization
- Update checking with package listing
- Clean error handling and reporting

## Installation and Usage

### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/yourusername/PacmanGui.git
cd PacmanGui
```

2. Create a build directory and run CMake:
```bash
mkdir -p build && cd build
cmake ..
```

3. Build the application:
```bash
make -j$(nproc)
```

4. Run the application:
```bash
./pacmangui
```

### Usage Guide

#### Basic Operations
- **Search**: Enter package names in the search box and press Enter or click Search
- **Install**: Select a package and click the Install button, or check multiple packages and click "Install Selected Packages"
- **Remove**: Select an installed package and click the Remove button
- **Update**: Select a package and click the Update button

#### System Update
1. Go to the "System Update" tab
2. Click "Check for Updates" to see available updates
3. Click "Update System" to perform a full system update
4. Enter your password when prompted
5. Monitor the update progress in real-time

#### Batch Installation
1. Search for packages
2. Check the checkboxes next to packages you want to install
3. The "Install Selected Packages" button will show how many packages are selected
4. Click this button to install all selected packages at once
5. Enter your password when prompted

#### Package Options
- Use the "Use --overwrite '*'" checkbox when installing packages that have file conflicts
- This option forces installation but may overwrite files from other packages

## Basic Rules to Follow

1. We don't remove things but we refactor them to reduce breaking functionality
2. No nullptr
3. Test building
4. We make sure the GUI does not break and stays stable unless things must be added
5. When adding third-party packages, add them via a submodule so GitHub can see it properly

## Third-Party Dependencies (Submodules)

The following third-party libraries are included as Git submodules:

- [fmt](https://github.com/fmtlib/fmt) - A modern formatting library

To clone the repository with all submodules:
```bash
git clone --recursive https://github.com/yourusername/pacman-gui.git
```

If you already cloned the repository without submodules, initialize them with:
```bash
git submodule update --init --recursive
```

## Current Progress

### Core Components (Implemented)
- **Package Management**
  - Core package management functionality through ALPM (Arch Linux Package Management)
  - Package class with comprehensive package information handling
  - Package search and information retrieval

- **Transaction Handling**
  - Complete transaction lifecycle management
  - Support for install, remove, and update operations
  - Transaction states and progress tracking
  - Error handling and rollback capabilities

- **Repository Management**
  - Repository synchronization
  - Package database handling
  - Repository status monitoring

### GUI Components (In Progress)
- **Main Window** (Designed)
  - Layout structure for the main application window
  - Integration with core package management

- **Package View** (Designed)
  - UI component for displaying package information
  - Interaction with package management operations

- **Transaction Dialog** (Designed)
  - UI for monitoring transaction progress
  - Error reporting interface

### Testing
- Comprehensive unit tests for core components:
  - Package tests
  - PackageManager tests
  - Transaction tests
  - Repository tests

## Project Phases

### Phase 1: Core Package Management (Current)
- [x] Basic package management functionality
- [x] Package search and information retrieval
- [x] Package installation/removal
- [x] Comprehensive error logging system
- [x] Transaction handling
- [x] Dependency resolution

#### Detailed Implementation Plan
- [x] Basic Pacman Integration
  - [x] Package installation
  - [x] Package removal
  - [x] Package updates
  - [x] Package search
  - [x] Dependency resolution
  - [x] Parallel downloads (1-20 threads)
  - [x] Repository Management
    - [x] Automatic repository refresh on startup
    - [x] Repository status monitoring
    - [x] Refresh progress tracking
    - [x] Refresh error handling
    - [x] Refresh notification system
  - [x] Local Package Installation
    - [x] Open and install .pkg.tar.zst files
    - [x] Drag and drop support
    - [x] Package verification
    - [x] Dependency checking
    - [x] Installation progress tracking
    - [x] Installation history

#### Core Components Implemented
- Package Manager
  - Handles package installation, removal, and updates
  - Manages package dependencies and conflicts
  - Provides package search functionality
  - Integrates with ALPM (Arch Linux Package Management) library

- Transaction Manager
  - Handles package transactions (install, remove, update)
  - Manages transaction states and lifecycle
  - Resolves dependencies and checks for conflicts
  - Provides transaction progress tracking

- Repository Manager
  - Manages package repositories
  - Handles repository synchronization
  - Provides repository status monitoring
  - Implements repository refresh functionality

- Local Package Manager
  - Handles local package installation
  - Verifies package integrity and signatures
  - Checks package dependencies
  - Manages local package database

- Package Search
  - Implements package search functionality
  - Provides package information retrieval
  - Supports searching in both local and remote repositories
  - Implements efficient search algorithms

- Logger
  - Comprehensive logging system
  - Multiple log levels (DEBUG, INFO, WARNING, ERROR, FATAL)
  - Log file management
  - Thread-safe logging operations

### Phase 2: Basic GUI Implementation (In Progress)
- [x] Main window layout
- [x] Package list view
- [x] Search functionality
- [x] Basic package operations UI
- [x] Progress indicators
- [x] Error message display
- [x] Batch package installation
- [x] Password authentication for sudo operations
- [x] Package overwrite options
- [x] Multi-package selection with checkboxes
- [x] Real-time terminal output display

#### Detailed Implementation Plan
- [x] Basic Operations
  - [x] Package installation
  - [x] Package removal
  - [x] Package updates
  - [x] System synchronization
  - [x] System update
  - [x] Batch installation support

- [x] Authentication System
  - [x] Secure password handling for sudo operations
  - [x] Password dialog integration
  - [x] Authentication error handling
  - [x] Session-based authentication

- [x] Advanced Package Management
  - [x] Package overwrite options (`--overwrite "*"`)
  - [x] Multi-package selection and operations
  - [x] Batch installation with progress tracking
  - [x] Package operation status reporting

- [x] User Interface Improvements
  - [x] Dark and light theme support
  - [x] Checkbox-based package selection
  - [x] Real-time terminal output display
  - [x] Progress dialogs for long operations
  - [x] Status bar updates
  - [x] Confirmation dialogs
  - [x] Error handling and display

- [ ] Display Management
  - [ ] Wayland Support
    - [ ] Native Wayland backend
    - [ ] Wayland protocols support
    - [ ] Wayland-specific optimizations
    - [ ] Wayland security features
  - [ ] Window Management
    - [ ] Dynamic resizing
    - [ ] Multi-monitor support
    - [ ] HiDPI scaling
    - [ ] Fractional scaling
    - [ ] Window state preservation
  - [ ] UI Scaling and Sizing
    - [ ] Proportional element scaling
    - [ ] Minimum and maximum size constraints
    - [ ] Element spacing optimization
    - [ ] Button and control sizing
    - [ ] Text scaling
    - [ ] Icon scaling
    - [ ] Layout optimization
    - [ ] Adaptive UI
  - [ ] Display Scaling
    - [ ] Automatic resolution detection
    - [ ] Dynamic scaling
    - [ ] Per-monitor scaling
    - [ ] Custom scaling factors
  - [ ] Animation System
    - [ ] Refresh rate synchronization
    - [ ] Smooth transitions
    - [ ] Hardware-accelerated animations
    - [ ] Animation customization
  - [ ] Performance Optimization
    - [ ] GPU acceleration
    - [ ] Memory-efficient rendering
    - [ ] Adaptive performance modes
  - [ ] Icon Theme Integration
    - [ ] System icon theme detection
    - [ ] Dynamic icon loading
    - [ ] Icon fallback system
    - [ ] HiDPI icon support
    - [ ] Icon theme change detection
    - [ ] Icon caching

### Phase 3: Advanced Features
- [ ] Package group management
- [ ] AUR support
- [ ] Update management
- [ ] System maintenance tools
- [ ] Settings and preferences

#### Detailed Implementation Plan
- [ ] AUR Integration
  - [ ] AUR package search
  - [ ] AUR package installation
  - [ ] AUR helper integration (yay/paru)
  - [ ] AUR package updates
- [ ] Additional Tools
  - [ ] Octopi Notifier
    - [ ] Background service
    - [ ] Update notifications
    - [ ] Notification preferences
  - [ ] Cache Cleaner
    - [ ] Cache size display
    - [ ] Cache cleaning options
    - [ ] Cleanup history
  - [ ] Repository Editor
    - [ ] Repository list
    - [ ] Add/remove repositories
    - [ ] Repository priority settings
- [ ] Distribution Support
  - [ ] Arch Linux
  - [ ] EndeavourOS
  - [ ] CachyOS Support
    - [ ] CachyOS-specific optimizations
    - [ ] CachyOS repository integration
    - [ ] CachyOS kernel support
    - [ ] CachyOS package management
  - [ ] Other Arch-based distributions
  - [ ] Fedora Support
    - [ ] DNF package management
    - [ ] RPM package handling
    - [ ] Fedora-specific optimizations
    - [ ] COPR Repository Manager

### Phase 4: Polish and Optimization
- [ ] Performance optimization
- [ ] UI/UX improvements
- [ ] Accessibility features
- [ ] Documentation
- [ ] Testing and bug fixes

#### Detailed Implementation Plan
- [ ] Customization
  - [ ] Theme Support
    - [ ] Pixel-perfect dark theme
    - [ ] Clean light theme
    - [ ] Flat modern design
    - [ ] Custom color schemes
    - [ ] Font customization
    - [ ] DE Integration
  - [ ] Layout customization
    - [ ] Panel positions
    - [ ] Window size presets
    - [ ] Custom layouts
  - [ ] Keyboard shortcuts
    - [ ] Customizable shortcuts
    - [ ] Shortcut presets
    - [ ] DE-specific shortcuts
- [ ] Package Building
  - [ ] PKGBUILD Generator
  - [ ] PKGBUILD Editor
  - [ ] Build Process
- [ ] Kernel Manager
  - [ ] Kernel Selection
  - [ ] Kernel Configuration
  - [ ] Patch Management
  - [ ] Build System
  - [ ] Installation
  - [ ] System Integration
  - [ ] Maintenance
- [ ] Security Features
  - [ ] Package verification
  - [ ] GPG key management
  - [ ] Security updates priority
- [ ] Performance Optimization
  - [ ] Download speed control
  - [ ] Cache optimization
  - [ ] Network bandwidth management
- [ ] Community Features
  - [ ] Package ratings
  - [ ] User reviews
  - [ ] Package recommendations

## Building the Project

### Prerequisites
- CMake 3.10 or higher
- Qt6 development packages
- Pacman (which provides libalpm)
- C++17 compatible compiler

### Simple Build
```bash
# Make sure you have the required dependencies
sudo pacman -S cmake qt6-base pacman

# Clone the repository
git clone https://github.com/yourusername/pacman-gui.git
cd pacman-gui

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./pacmangui
```

### Troubleshooting Build Issues

If you encounter build issues:

1. Make sure pacman is installed:
   ```bash
   sudo pacman -S pacman
   ```

2. If CMake fails to find ALPM:
   ```bash
   # Create a symlink if needed
   sudo mkdir -p /usr/include/alpm
   sudo ln -s /usr/include/alpm.h /usr/include/alpm/alpm.h
   ```

3. Clear your build directory and try again:
   ```bash
   rm -rf build/
   mkdir build && cd build
   cmake ..
   make -j$(nproc) VERBOSE=1
   ```

4. Check for API compatibility:
   ```bash
   # Check pacman version
   pacman -V
   ```
## Testing
If you want to run tests:

```bash
# From the build directory
make test
```

## Project Structure
- `src/core/` - Core backend components (Package Manager, Transaction, Repository)
- `src/gui/` - GUI components
- `include/` - Header files
- `tests/` - Unit tests
- `resources/` - Resources (icons, UI files, etc.)

## Contributing
Contributions are welcome! Please follow the project's coding style and submit pull requests for review.
