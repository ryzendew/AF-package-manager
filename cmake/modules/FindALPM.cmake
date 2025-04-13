# FindALPM.cmake - Find ALPM (Arch Linux Package Management) library

# Try to find the ALPM library
find_path(ALPM_INCLUDE_DIR
  NAMES alpm.h 
  PATHS /usr/include
        /usr/local/include
        /usr/include/alpm
  PATH_SUFFIXES alpm
)

# Find alpm_trans_t type definition
if(ALPM_INCLUDE_DIR)
  file(READ "${ALPM_INCLUDE_DIR}/alpm.h" ALPM_H_CONTENT)
  string(FIND "${ALPM_H_CONTENT}" "typedef struct _alpm_trans_t alpm_trans_t" ALPM_TRANS_DEF)
  
  if(ALPM_TRANS_DEF EQUAL -1)
    message(STATUS "ALPM API: alpm_trans_t type not found directly")
    # Some pacman versions may define it differently or it might be in another header
    set(ALPM_HANDLE_API_VERSION_7 TRUE)
  else()
    message(STATUS "ALPM API: alpm_trans_t type found")
    set(ALPM_HANDLE_API_VERSION_6 TRUE)
  endif()
endif()

find_library(ALPM_LIBRARY
  NAMES alpm
  PATHS /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/local/lib64
)

# Handle the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ALPM DEFAULT_MSG ALPM_LIBRARY ALPM_INCLUDE_DIR)

# Create the imported target
if(ALPM_FOUND)
  set(ALPM_LIBRARIES ${ALPM_LIBRARY})
  set(ALPM_INCLUDE_DIRS ${ALPM_INCLUDE_DIR})
  
  # Add parent directory if it's in a subdirectory
  get_filename_component(ALPM_INCLUDE_PARENT_DIR "${ALPM_INCLUDE_DIR}" DIRECTORY)
  if(NOT "${ALPM_INCLUDE_PARENT_DIR}" STREQUAL "/usr/include")
    list(APPEND ALPM_INCLUDE_DIRS ${ALPM_INCLUDE_PARENT_DIR})
  endif()
  
  if(NOT TARGET ALPM::ALPM)
    add_library(ALPM::ALPM UNKNOWN IMPORTED)
    set_target_properties(ALPM::ALPM PROPERTIES
      IMPORTED_LOCATION "${ALPM_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${ALPM_INCLUDE_DIRS}"
    )
  endif()
endif()

mark_as_advanced(ALPM_INCLUDE_DIR ALPM_LIBRARY)

# Display helpful message about ALPM location
if(ALPM_FOUND)
  message(STATUS "Found ALPM:")
  message(STATUS "  - Include: ${ALPM_INCLUDE_DIRS}")
  message(STATUS "  - Library: ${ALPM_LIBRARY}")
else()
  message(WARNING "ALPM not found. Please install pacman development files.")
  message(WARNING "On Arch Linux: 'sudo pacman -S pacman'")
endif() 