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
    pkg_check_modules(HarfBuzz QUIET harfbuzz)
    if(MuPDF_FOUND)
        # Add HarfBuzz dependencies explicitly for Ubuntu builds
        if(HarfBuzz_FOUND)
            list(APPEND MuPDF_LIBRARIES ${HarfBuzz_LDFLAGS})
            list(APPEND MuPDF_CFLAGS_OTHER ${HarfBuzz_CFLAGS_OTHER})
        endif()
        list(APPEND MuPDF_LIBRARIES ${MuPDF_LDFLAGS_OTHER})
    endif()
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

find_library(MuPDF_SHARED_LIBRARY
    NAMES mupdf
    HINTS ${MuPDF_LIBRARY_DIRS}
    PATHS
        /usr/lib/x86_64-linux-gnu
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /opt/homebrew/lib
        /opt/homebrew/Cellar/mupdf/*/lib
        ${CMAKE_INSTALL_PREFIX}/lib
)

find_library(MuPDF_STATIC_LIBRARY
    NAMES mupdf mupdf.a
    HINTS ${MuPDF_LIBRARY_DIRS}
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /opt/homebrew/lib
        /opt/homebrew/Cellar/mupdf/*/lib
        ${CMAKE_INSTALL_PREFIX}/lib
)

# Prefer shared library over static
if(MuPDF_SHARED_LIBRARY AND NOT "${MuPDF_SHARED_LIBRARY}" MATCHES "\\.a$")
    set(MuPDF_LIBRARY ${MuPDF_SHARED_LIBRARY})
elseif(MuPDF_STATIC_LIBRARY)
    set(MuPDF_LIBRARY ${MuPDF_STATIC_LIBRARY})
elseif(MuPDFFIND_STATIC_LIBRARY)
    set(MuPDF_LIBRARY ${MuPDF_STATIC_LIBRARY})
endif()

# Extract version from header if not found via pkg-config
if(NOT MuPDF_VERSION AND MuPDF_INCLUDE_DIR)
    if(EXISTS "${MuPDF_INCLUDE_DIR}/mupdf/fitz/version.h")
        file(STRINGS "${MuPDF_INCLUDE_DIR}/mupdf/fitz/version.h" VERSION_LINE REGEX "#define FZ_VERSION ")
        if(VERSION_LINE)
            string(REGEX REPLACE ".*#define FZ_VERSION \"([^\"]*)\".*" "\\1" MuPDF_VERSION "${VERSION_LINE}")
        endif()
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MuPDF
    REQUIRED_VARS MuPDF_LIBRARY MuPDF_INCLUDE_DIR
    VERSION_VAR MuPDF_VERSION
)

if(MuPDF_FOUND)
    set(MuPDF_INCLUDE_DIRS ${MuPDF_INCLUDE_DIR})
    set(MuPDF_LIBRARIES ${MuPDF_LIBRARY})
    set(MuPDF_DEFINITIONS ${MuPDF_CFLAGS_OTHER})

    # If pkg-config was found, it may have additional library dependencies
    if(PKG_CONFIG_FOUND AND MuPDF_FOUND)
        foreach(extra_lib ${MuPDF_STATIC_LIBRARIES})
            if (NOT "${extra_lib}" STREQUAL "mupdf") # Avoid duplicating the main lib
                list(APPEND MuPDF_LIBRARIES ${extra_lib})
            endif()
        endforeach()
        # Also include LDFLAGS_OTHER which contains additional linking requirements
        list(APPEND MuPDF_LIBRARIES ${MuPDF_LDFLAGS_OTHER})
    endif()

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
