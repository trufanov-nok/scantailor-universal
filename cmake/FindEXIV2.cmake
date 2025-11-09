# Find EXIV2
# ~~~~~~~~~~
# CMake module to search for EXIV2 library
#
# If it's found it sets EXIV2_FOUND to TRUE
# and following variables are set:
#    EXIV2_INCLUDE_DIR
#    EXIV2_LIBRARY

FIND_PATH(
    EXIV2_INCLUDE_DIR
    NAMES exiv2/exiv2.hpp
    PATHS "${EXIV2_DIR}/include" $ENV{EXIV2_DIR}/include /usr/local/include /usr/include
)

FIND_LIBRARY(
    EXIV2_LIBRARY
    NAMES exiv2
    PATHS "${EXIV2_DIR}/lib" $ENV{LIB_DIR}/lib /usr/local/lib /usr/lib
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    EXIV2
    DEFAULT_MSG
    EXIV2_LIBRARY
    EXIV2_INCLUDE_DIR
)

IF(EXIV2_FOUND)
    SET(EXIV2_INCLUDE_DIRS ${EXIV2_INCLUDE_DIR})

    IF(NOT EXIV2_LIBRARIES)
      SET(EXIV2_LIBRARIES ${EXIV2_LIBRARY})
    ENDIF()

    MARK_AS_ADVANCED(
        EXIV2_INCLUDE_DIRS
        EXIV2_LIBRARIES
     )
ENDIF()

MARK_AS_ADVANCED(
   EXIV2_INCLUDE_DIR
   EXIV2_LIBRARY
)
