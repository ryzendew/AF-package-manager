cmake_minimum_required(VERSION 3.10)
project(PacmanGui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure Qt Meta-Object Compiler is run
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add custom module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
find_package(Qt6 OPTIONAL_COMPONENTS WaylandClient)
find_package(ALPM REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(QTERMWIDGET6 REQUIRED IMPORTED_TARGET qtermwidget6)

# Wayland support detection
set(ENABLE_WAYLAND_SUPPORT OFF)
add_definitions(-DENABLE_WAYLAND_SUPPORT=0)
message(STATUS "Wayland support manually disabled")

# Following Octopi's approach for handling alpm
add_definitions(-DALPM_HANDLE=void*)
add_definitions(-DALPM_TRANS_T=void*)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/wayland
    ${ALPM_INCLUDE_DIRS}
    ${QTERMWIDGET6_INCLUDE_DIRS}
)

link_directories(${QTERMWIDGET6_LIBRARY_DIRS})

# Source files - core components
set(CORE_SOURCES
    src/core/package.cpp
    src/core/repository.cpp
    src/core/transaction.cpp
    src/core/packagemanager.cpp
    src/core/flatpak_package.cpp
    src/core/flatpak_manager.cpp
)

# Source files - GUI components
set(GUI_SOURCES
    src/gui/mainwindow.cpp
    src/gui/settingsdialog.cpp
    src/gui/util.cpp
    src/gui/flatpak_manager_tab.cpp
    src/gui/flatpak_process_dialog.cpp
    src/gui/install_progress_dialog.cpp
    src/gui/password_prompt_dialog.cpp
)

# Source files - Wayland components
# Wayland support is disabled but we need stub implementations
set(WAYLAND_SOURCES
    src/wayland/wayland_backend.cpp
    src/wayland/wayland_protocols.cpp
    src/wayland/wayland_security.cpp
    src/wayland/wayland_optimization.cpp
)

set(WAYLAND_HEADERS
    include/wayland/wayland_backend.hpp
    include/wayland/wayland_protocols.hpp
    include/wayland/wayland_security.hpp
    include/wayland/wayland_optimization.hpp
)

# Headers with Q_OBJECT macro that need to be processed by MOC
set(MOC_HEADERS
    include/gui/mainwindow.hpp
    include/gui/settingsdialog.hpp
    include/gui/flatpak_manager_tab.hpp
    include/gui/util.hpp
    include/gui/flatpak_process_dialog.hpp
    include/gui/install_progress_dialog.hpp
    include/gui/password_prompt_dialog.hpp
    ${WAYLAND_HEADERS}
)

# Main source
set(MAIN_SOURCE
    src/main.cpp
)

# Resource files
set(RESOURCE_FILES
    resources/resources.qrc
)

# Add executable
add_executable(pacmangui 
    ${MAIN_SOURCE}
    ${CORE_SOURCES}
    ${GUI_SOURCES}
    ${WAYLAND_SOURCES}
    ${MOC_HEADERS}
    ${RESOURCE_FILES}
)

# Link libraries
target_link_libraries(pacmangui PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    ALPM::ALPM
    PkgConfig::QTERMWIDGET6
)

# Add WaylandClient if available
# Wayland support is disabled
# if(Qt6WaylandClient_FOUND)
#     target_link_libraries(pacmangui PRIVATE Qt6::WaylandClient)
# endif()

# Installation paths
include(GNUInstallDirs)

# Install rules
install(TARGETS pacmangui 
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install desktop file
install(FILES resources/desktop/pacmangui.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

# Install icons - use the system-software-install icon 
# If we had custom icons, we'd install them like this:
# install(FILES resources/icons/pacmangui.png
#        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
# )

# Install stylesheets
install(DIRECTORY resources/styles
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pacmangui
        FILES_MATCHING PATTERN "*.qss"
)

# Add uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)
