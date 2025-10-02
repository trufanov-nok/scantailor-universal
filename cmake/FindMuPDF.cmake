# Find MuPDF library
#
# This module defines:
#  MuPDF_FOUND        - True if MuPDF is found
#  MuPDF_INCLUDE_DIRS - Include directories for MuPDF
#  MuPDF_LIBRARIES    - Libraries to link against
#  MuPDF_DEFINITIONS  - Compiler definitions
#
# And the following imported targets:
#  MuPDF::MuPDF       - The MuPDF library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(MuPDF QUIET mupdf)
endif()

find_path(MuPDF_INCLUDE_DIR
    NAMES mupdf/fitz.h
    HINTS ${MuPDF_INCLUDE_DIRS}
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        /opt/homebrew/include
        /opt/homebrew/Cellar/mupdf/*/include
        ${CMAKE_INSTALL_PREFIX}/include
)

find_library(MuPDF_LIBRARY
    NAMES mupdf
    HINTS ${MuPDF_LIBRARY_DIRS}
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /opt/homebrew/lib
        /opt/homebrew/Cellar/mupdf/*/lib
        ${CMAKE_INSTALL_PREFIX}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MuPDF
    REQUIRED_VARS MuPDF_LIBRARY MuPDF_INCLUDE_DIR
    VERSION_VAR MuPDF_VERSION
)

if(MuPDF_FOUND)
    set(MuPDF_INCLUDE_DIRS ${MuPDF_INCLUDE_DIR})
    set(MuPDF_LIBRARIES ${MuPDF_LIBRARY})
    set(MuPDF_DEFINITIONS ${MuPDF_CFLAGS_OTHER})

    if(NOT TARGET MuPDF::MuPDF)
        add_library(MuPDF::MuPDF UNKNOWN IMPORTED)
        set_target_properties(MuPDF::MuPDF PROPERTIES
            IMPORTED_LOCATION "${MuPDF_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MuPDF_INCLUDE_DIR}"
            INTERFACE_COMPILE_OPTIONS "${MuPDF_CFLAGS_OTHER}"
        )
    endif()
endif()

mark_as_advanced(MuPDF_INCLUDE_DIR MuPDF_LIBRARY)
