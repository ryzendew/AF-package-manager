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
find_package(ALPM REQUIRED)

# Following Octopi's approach for handling alpm
add_definitions(-DALPM_HANDLE=void*)
add_definitions(-DALPM_TRANS_T=void*)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/gui
    ${ALPM_INCLUDE_DIRS}
)

# Source files - core components
set(CORE_SOURCES
    src/core/package.cpp
    src/core/repository.cpp
    src/core/transaction.cpp
    src/core/packagemanager.cpp
)

# Source files - GUI components
set(GUI_SOURCES
    src/gui/mainwindow.cpp
)

# Headers with Q_OBJECT macro that need to be processed by MOC
set(MOC_HEADERS
    include/gui/mainwindow.hpp
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
    ${MOC_HEADERS}
    ${RESOURCE_FILES}
)

# Link libraries
target_link_libraries(pacmangui PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    ALPM::ALPM
)

# Install rules
install(TARGETS pacmangui DESTINATION bin) 