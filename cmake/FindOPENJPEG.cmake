# Find OPENJPEG
# ~~~~~~~~~~
# CMake module to search for OPENJPEG library
#
# If it's found it sets OPENJPEG_FOUND to TRUE
# and following variables are set:
#    OPENJPEG_INCLUDE_DIR
#    OPENJPEG_LIBRARY

FIND_PATH(
    OPENJPEG_INCLUDE_DIR
    NAMES openjpeg.h
    PATHS "${OPENJPEG_DIR}/include" $ENV{LIB_DIR}/include /usr/local/include /usr/include
    PATH_SUFFIXES
    openjpeg-2.0 openjpeg-2.1 openjpeg-2.2 openjpeg-2.3 openjpeg-2.4
    openjpeg-2.5 openjpeg-2.6 openjpeg-2.7 openjpeg-2.8 openjpeg-2.9
)

FIND_LIBRARY(
    OPENJPEG_LIBRARY
    NAMES openjp2
    PATHS "${OPENJPEG_DIR}/lib" $ENV{LIB_DIR}/lib /usr/local/lib /usr/lib
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    OPENJPEG
    DEFAULT_MSG
    OPENJPEG_LIBRARY
    OPENJPEG_INCLUDE_DIR
)

IF(OPENJPEG_FOUND)
    SET(OPENJPEG_INCLUDE_DIRS ${OPENJPEG_INCLUDE_DIR})

    IF(NOT OPENJPEG_LIBRARIES)
      SET(OPENJPEG_LIBRARIES ${OPENJPEG_LIBRARY})
    ENDIF()

    MARK_AS_ADVANCED(
        OPENJPEG_INCLUDE_DIRS
        OPENJPEG_LIBRARIES
     )
ENDIF()

MARK_AS_ADVANCED(
    OPENJPEG_INCLUDE_DIR
    OPENJPEG_LIBRARY
)
