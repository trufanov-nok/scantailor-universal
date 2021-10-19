# Find EXIV2
# ~~~~~~~~~~
# CMake module to search for EXIV2 library
#
# If it's found it sets EXIV2_FOUND to TRUE
# and following variables are set:
#    EXIV2_INCLUDE_DIR
#    EXIV2_LIBRARY
#

# A copy of https://github.com/qgis/QGIS/blob/master/cmake/FindEXIV2.cmake
# Under GPL-2.0 License 
IF(WIN32)
GET_FILENAME_COMPONENT(build_outer_dir "${PROJECT_BINARY_DIR}/.." ABSOLUTE)
FIND_PATH(
    DEPS_BUILD_DIR build-qt.bat
    HINTS "${build_outer_dir}/scantailor-universal-deps-build"
    DOC "Build directory for Scan Tailor dependencies."
    )
SET(STAGING_LIBS_DIR "${DEPS_BUILD_DIR}/staging/libs")
INCLUDE("${DEPS_BUILD_DIR}/export-vars.cmake")
ENDIF(WIN32)

FIND_PATH(EXIV2_INCLUDE_DIR exiv2/exiv2.hpp $ENV{LIB_DIR}/include /usr/local/include /usr/include "${EXIV2_DIR}/include")
FIND_LIBRARY(EXIV2_LIBRARY NAMES exiv2 PATHS $ENV{LIB_DIR}/lib /usr/local/lib /usr/lib HINTS ${STAGING_LIBS_DIR})

IF (EXIV2_INCLUDE_DIR AND EXIV2_LIBRARY)
    SET(EXIV2_FOUND TRUE)
    MESSAGE(STATUS "Found exiv2: ${EXIV2_LIBRARY}")
ELSE (EXIV2_INCLUDE_DIR AND EXIV2_LIBRARY)
    MESSAGE(EXIV2_INCLUDE_DIR=${EXIV2_INCLUDE_DIR})
    MESSAGE(EXIV2_LIBRARY=${EXIV2_LIBRARY})
    MESSAGE(WARNING "Could not find exiv2")
ENDIF (EXIV2_INCLUDE_DIR AND EXIV2_LIBRARY)
