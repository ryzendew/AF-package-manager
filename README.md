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

4. Run the application locally:
```bash
./pacmangui
```

### System-wide Installation

To install PacmanGUI system-wide (requires administrator privileges):

```bash
cd build
sudo make install
```

This will:
- Install the executable to `/usr/local/bin/pacmangui`
- Install the desktop file to `/usr/local/share/applications/pacmangui.desktop`
- Install stylesheets to `/usr/local/share/pacmangui/styles/`

After installation, you can:
- Run the application from the command line with `pacmangui`
- Find and launch it from your desktop environment's application menu

### Uninstallation

To uninstall PacmanGUI:

```bash
cd build
sudo make uninstall
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
1. Search for packages you want to install
2. Check the checkboxes in the first column next to the packages you want to install
3. The "Install Selected Packages" button will become enabled once you select packages
4. Click the "Install Selected Packages" button to start the batch installation process
5. Confirm the installation in the dialog that appears, listing all selected packages
6. Enter your password when prompted
7. A progress dialog will show the installation status of each package
8. Once complete, a summary of successful and failed installations will be displayed

#### Package Options
- Use the "Use --overwrite '*'" checkbox when installing packages that have file conflicts
- This option forces installation but may overwrite files from other packages

## Basic Rules to Follow

1. We don't remove things but we refactor them to reduce breaking functionality
2. No nullptr
3. Test building
4. We make sure the GUI does not break and stays stable unless things must be added
5. When adding third-party packages, add them via a submodule so GitHub can see it properly
6. WE DON'T DO STUBS

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
- [x] Consistent blue button theming across the interface
- [x] Improved update checking with detailed logging

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
  - [x] Consistent blue button styling throughout the application
  - [x] Enhanced update checking with direct command output display
  - [x] Detailed logging for system operations

- [x] Display Management
  - [ ] Wayland Support
    - [ ] Native Wayland backend
    - [ ] Wayland protocols support
    - [ ] Wayland-specific optimizations
    - [ ] Wayland security features
  - [x] Window Management
    - [x] Dynamic resizing
    - [x] Multi-monitor support
    - [x] HiDPI scaling
    - [ ] Fractional scaling
    - [x] Window state preservation
  - [x] UI Scaling and Sizing
    - [x] Proportional element scaling
    - [x] Minimum and maximum size constraints
    - [x] Element spacing optimization
    - [x] Button and control sizing
    - [x] Text scaling
    - [ ] Icon scaling
    - [x] Layout optimization
    - [x] Adaptive UI
  - [x] Display Scaling
    - [x] Automatic resolution detection
    - [x] Dynamic scaling
    - [ ] Per-monitor scaling
    - [x] Custom scaling factors
  - [ ] Animation System
    - [ ] Refresh rate synchronization
    - [ ] Smooth transitions
    - [ ] Hardware-accelerated animations
    - [ ] Animation customization
  - [x] Performance Optimization
    - [ ] GPU acceleration
    - [x] Memory-efficient rendering
    - [x] Adaptive performance modes
  - [x] Icon Theme Integration
    - [x] System icon theme detection
    - [x] Dynamic icon loading
    - [x] Icon fallback system
    - [ ] HiDPI icon support
    - [ ] Icon theme change detection
    - [x] Icon caching
  - [x] Stylesheet System
    - [x] Dynamic CSS-like styling
    - [x] Theme-aware styling
    - [x] Widget-specific styling
    - [x] Style inheritance
    - [x] Style overrides

### Display Management Implementation Notes

#### HiDPI Support
- Added Qt's built-in high DPI scaling with Qt::AA_EnableHighDpiScaling
- Using PassThrough scaling policy for better fractional scaling
- Custom scaling factor support through settings
- Adjusts font sizes based on scaling factor

#### UI Scaling
- Dynamic proportional scaling for all UI elements
- Minimum window size constraints (1000x600)
- Responsive table views with automatic column sizing
- Text scaling based on system font settings
- Layout optimized for different screen sizes

#### Stylesheet System
- CSS-like styling using QSS (Qt Style Sheets)
- Dynamic.qss for consistent styling across the application
- Blue button styling with hover effects
- Dark/light theme support with theme-specific styling

#### Window Management
- Window state preservation (maximized state)
- Dynamic resizing with responsive layouts
- Multi-monitor support through Qt window management

#### Performance Optimizations
- Separate threads for long-running operations
- Memory-efficient table views for large package lists
- Optimized rendering for scrolling performance

### Phase 3: Advanced UI and User Experience (Ongoing)
- [x] Multi-package selection with checkboxes
- [x] Batch installation with progress tracking 
- [x] Multiple theme options (light, dark, colorful variants)
- [x] Improved package detail display
- [x] Tab-specific UI behavior
- [ ] Advanced search filters
- [ ] Customizable package views
- [ ] Package statistics display
- [ ] Package history tracking
- [ ] Dependency graph visualization

#### Recent Improvements
- Added checkbox selection for multiple packages in both the All Packages and Installed views
- Implemented batch installation functionality with progress tracking and individual package status reporting
- Fixed detail panel visibility to only show when the appropriate tab is selected
- Improved sync_all functionality with proper password authentication
- Added theme customization with multiple style options
- Applied consistent UI styling across the application
- Fixed UI responsiveness during long-running operations

### Phase 4: Advanced Features (Planned)
- [x] Package group management
- [x] AUR support
- [x] Update management
- [x] System maintenance tools
- [x] Settings and preferences

#### Detailed Implementation Plan
- [ ] AUR Integration
  - [ ] AUR package search
  - [ ] AUR package installation
  - [ ] AUR helper integration (yay/paru)
  - [ ] AUR package updates
- [ ] Additional Tools
  - [ ] Notifier
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

### Phase 5: Polish and Optimization
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

### Dependencies

#### Required Dependencies
- C++17 compatible compiler (GCC 7+ or Clang 5+)
- CMake 3.10 or higher
- Qt6 (Core, Widgets, Gui)
- Pacman (libalpm)
- pkg-config

#### Installing Dependencies

**Arch Linux and derivatives:**

```bash
sudo pacman -S base-devel cmake qt6-base pacman
```

### Simple Build

After installing the dependencies:

```bash
# Clone the repository
git clone https://github.com/yourusername/PacmanGui.git
cd PacmanGui

# Create build directory and build the application
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Run the application
./pacmangui
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

### Troubleshooting Build Issues

If you encounter build issues on Arch Linux:

1. **Missing dependencies:**
   ```bash
   # Make sure you have all required dependencies
   sudo pacman -S base-devel cmake qt6-base pacman
   ```

2. **Missing libalpm or ALPM not found:**
   ```bash
   # Ensure pacman is installed
   sudo pacman -S pacman
   
   # Create symlinks if CMake can't find the headers
   sudo mkdir -p /usr/include/alpm
   sudo ln -s /usr/include/alpm.h /usr/include/alpm/alpm.h
   ```

3. **Clean rebuild:**
   ```bash
   rm -rf build/
   mkdir build && cd build
   cmake ..
   make -j$(nproc) VERBOSE=1
   ```

### System-wide Installation

To install PacmanGUI system-wide (requires administrator privileges):

```bash
cd build
sudo make install
```