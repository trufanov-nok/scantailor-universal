CMAKE_MINIMUM_REQUIRED(VERSION 3.18.4)

IF (POLICY CMP0071)
        CMAKE_POLICY(SET CMP0071 OLD)
ENDIF()

IF (POLICY CMP0072)
        CMAKE_POLICY(SET CMP0072 OLD)
ENDIF()

SET(
        CMAKE_USER_MAKE_RULES_OVERRIDE
        "${CMAKE_SOURCE_DIR}/cmake/default_cflags.cmake"
)
SET(
        CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
        "${CMAKE_SOURCE_DIR}/cmake/default_cxxflags.cmake"
)

PROJECT("Scan Tailor Universal")

IF(MSVC)
        # Disable checked iterators for extra performance.
        ADD_DEFINITIONS(-D_SECURE_SCL=0)
ENDIF()

IF(DEBUG_CLI)
        ADD_DEFINITIONS(-DDEBUG_CLI)
ENDIF(DEBUG_CLI)

ENABLE_TESTING()

# An undocumented side-effect of CONFIGURE_FILE() is that it makes
# the whole project depend on the file we are parsing / copying.
CONFIGURE_FILE(
        "${PROJECT_SOURCE_DIR}/version.h"
        "${PROJECT_BINARY_DIR}/.version.h" COPYONLY
)

# Prevent this leftover from old builds to be used in favour
# of the one in ${PROJECT_SOURCE_DIR}
IF(NOT "${PROJECT_BINARY_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
        FILE(REMOVE "${PROJECT_BINARY_DIR}/version.h")
ENDIF()

# Extract VERSION and VERSION_QUAD from version.h
FILE(READ "${PROJECT_SOURCE_DIR}/version.h" version_h_contents)
STRING(
        REGEX REPLACE
        ".*#define[ \\t]+VERSION[ \\t]+\"([^\"]*)\".*"
        "\\1" VERSION "${version_h_contents}"
)
IF("${VERSION}" STREQUAL "${version_h_contents}")
        MESSAGE(FATAL_ERROR "Failed to extract VERSION from version.h")
ENDIF()

# VERSION_QUAD must be either empty or be in the form of X.Y.Z.Y
STRING(
        REGEX REPLACE
        ".*#define[ \\t]+VERSION_QUAD[ \\t]+\"(([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)?)\".*"
        "\\1" VERSION_QUAD "${version_h_contents}"
)
IF("${VERSION_QUAD}" STREQUAL "${version_h_contents}")
        MESSAGE(FATAL_ERROR "Failed to extract VERSION_QUAD from version.h")
ENDIF()

# For config.h
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(TestCXXAcceptsFlag)
INCLUDE(cmake/FindPthreads.cmake)
IF(UNIX AND NOT APPLE)
INCLUDE(cmake/FindCanberra.cmake)
ENDIF(UNIX AND NOT APPLE)
INCLUDE(cmake/FindEXIV2.cmake)
INCLUDE(cmake/SetDefaultBuildType.cmake)
INCLUDE(cmake/SetDefaultGccFlags.cmake)
INCLUDE(cmake/ToNativePath.cmake)
INCLUDE(cmake/UpdateTranslations.cmake)
INCLUDE(cmake/CopyToBuildDir.cmake)
INCLUDE(cmake/LibToDLL.cmake)

ST_SET_DEFAULT_BUILD_TYPE(Release)
IF(CMAKE_COMPILER_IS_GNUCC)
        ST_SET_DEFAULT_GCC_FLAGS()
ENDIF(CMAKE_COMPILER_IS_GNUCC)

# Define DEBUG macro for Debug builds to enable debug output
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
        ADD_DEFINITIONS(-DDEBUG)
ENDIF()

GET_FILENAME_COMPONENT(source_outer_dir "${PROJECT_SOURCE_DIR}/.." ABSOLUTE)
GET_FILENAME_COMPONENT(build_outer_dir "${PROJECT_BINARY_DIR}/.." ABSOLUTE)

SET(STAGING_LIBS_DIR "")


INCLUDE(FindOpenGL)
SET(use_opengl OFF)
IF(OPENGL_FOUND AND QT_QTOPENGL_FOUND)
        # Disabled for now due to various problems associated with it.
        # SET(use_opengl ON)
ENDIF()
OPTION(ENABLE_OPENGL "OpenGL may be used for UI acceleration" ${use_opengl})


SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_AUTORCC ON)

if(NOT DEFINED QtPkgList)
    set(QtPkgList "Qt5;Qt6")
endif()

foreach(QtPkg ${QtPkgList})
  if(ENABLE_OPENGL)
        find_package(QT NAMES ${QtPkg} HINTS "${QT_ROOT_DIR}" CONFIG REQUIRED COMPONENTS Core Widgets Gui Xml Network Svg LinguistTools OpenGL)
  else()
        find_package(QT NAMES ${QtPkg} HINTS "${QT_ROOT_DIR}" CONFIG REQUIRED COMPONENTS Core Widgets Gui Xml Network Svg LinguistTools)
  endif()

  if(QT_FOUND)
        break()
  endif()
endforeach()

if(ENABLE_OPENGL)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets Gui Xml Network Svg LinguistTools OpenGL REQUIRED)
else()
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets Gui Xml Network Svg LinguistTools REQUIRED)
endif()

SET(QT_MIN_VERSION 5.7.0)
if (QT_VERSION VERSION_LESS ${QT_MIN_VERSION})
        message(FATAL_ERROR "Minimum supported Qt version is ${QT_MIN_VERSION}!")
endif()


IF(APPLE)
   # Not used but required for loading cocoa plugin
   FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS PrintSupport REQUIRED)
   # hardcoded path to Qt5CoreConfig.cmake etc. [truf]
   # Also set Homebrew Qt5 path if available
   LIST(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/Cellar/qt@5/5.15.17/lib/cmake")
   LIST(APPEND CMAKE_PREFIX_PATH "/Users/robert/build/Qt/5.8/clang_64/")
ENDIF(APPLE)

find_package(PkgConfig REQUIRED) #used in FindMuPDF.cmake

option(ENABLE_MUPDF "Enable pdf files as image source support" on)
IF (ENABLE_MUPDF)
# Find MuPDF for PDF support (minimum 1.18.0 required for compatibility with newer API changes)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
FIND_PACKAGE(MuPDF 1.18.0)
    IF (MuPDF_FOUND)
        add_definitions(-DENABLE_MUPDF)
    ELSE()
        set(ENABLE_MUPDF off)
    ENDIF()
ENDIF()

IF(ENABLE_OPENGL)
        FIND_PACKAGE(Qt5OpenGL ${Qt_MIN_VERSION} REQUIRED)
ENDIF()

FIND_PACKAGE(JPEG REQUIRED)
INCLUDE_DIRECTORIES("${JPEG_INCLUDE_DIRS}")
LIST(APPEND EXTRA_LIBS ${JPEG_LIBRARIES})

OPTION(ENABLE_OPENJPEG "OpenJPEG library may be used to support JPEG 2000 (.jp2) images." ON)
IF (ENABLE_OPENJPEG)
        find_package(OPENJPEG REQUIRED)
        INCLUDE_DIRECTORIES("${OPENJPEG_INCLUDE_DIR}")

        LIST(APPEND EXTRA_LIBS ${OPENJPEG_LIBRARY})
ENDIF(ENABLE_OPENJPEG)


find_package(ZLIB REQUIRED)
IF(NOT ZLIB_LIBRARY)
        MESSAGE(
                FATAL_ERROR
                "Could not find zlib library.\n"
                "You may need to install a package named zlib1g-dev or similarly."
        )
ENDIF()
INCLUDE_DIRECTORIES("${ZLIB_INCLUDE_DIR}")

find_package(PNG REQUIRED)
IF(NOT PNG_LIBRARY)
        MESSAGE(
                FATAL_ERROR
                "Could not find libpng library.\n"
                "You may need to install a package named libpng12-dev or similarly."
        )
ENDIF()
INCLUDE_DIRECTORIES("${PNG_INCLUDE_DIR}")

FIND_PATH(
        TIFF_INCLUDE_DIR tiff.h
	PATHS /usr/local/include /usr/include
	HINTS ${TIFF_DIR} # TIFF_DIR may come from export-vars.cmake
	PATH_SUFFIXES libtiff
	DOC "Path to libtiff headers."
)
IF(NOT TIFF_INCLUDE_DIR)
        MESSAGE(
                FATAL_ERROR
                "Could not find libtiff headers.\n"
                "You may need to install a package named libtiff4-dev or similarly."
        )
ENDIF()

INCLUDE_DIRECTORIES("${TIFF_INCLUDE_DIR}")

FIND_LIBRARY(
        TIFF_LIBRARY tiff libtiff.lib
	PATHS /usr/local/lib /usr/lib
	HINTS ${STAGING_LIBS_DIR}
	PATH_SUFFIXES libtiff
	DOC "Path to tiff library."
)
IF(NOT TIFF_LIBRARY)
        MESSAGE(
                FATAL_ERROR
                "Could not find libtiff library.\n"
                "You may need to install a package named libtiff4-dev or similarly."
        )
ENDIF()

IF(WIN32)
        ADD_DEFINITIONS(-DUSE_LIBTIFF_DLL)
ENDIF()


IF(WIN32)
        SET(Boost_USE_STATIC_LIBS ON)
ELSE(WIN32)
        ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
ENDIF(WIN32)

ADD_DEFINITIONS(-DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
ADD_DEFINITIONS(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)

SET(Boost_USE_MULTITHREADED ON)
IF(ENABLE_TESTS)
        IF(POLICY CMP0167)
                FIND_PACKAGE(Boost 1.70.0 COMPONENTS unit_test_framework REQUIRED CONFIG)
        ELSE()
                FIND_PACKAGE(Boost 1.35.0 COMPONENTS unit_test_framework REQUIRED)
        ENDIF()
ELSE()
        IF(POLICY CMP0167)
                FIND_PACKAGE(Boost 1.70.0 REQUIRED CONFIG)
        ELSE()
                FIND_PACKAGE(Boost 1.35.0 REQUIRED)
        ENDIF()
ENDIF()

IF(NOT Boost_FOUND)
        MESSAGE(
                FATAL_ERROR
                "Could not find boost headers or libraries. "
                "You may need to install a package named libboost1.35-dev or similarly. "
                "Hint: create a Boost_DEBUG variable in cmake and set it to YES."
        )
ENDIF(NOT Boost_FOUND)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

IF (EXIV2_FOUND)
        ADD_DEFINITIONS(-DHAVE_EXIV2)
        INCLUDE_DIRECTORIES("${EXIV2_INCLUDE_DIR}")
ENDIF(EXIV2_FOUND)

SET(EXTRA_LIBS "")

IF(UNIX)
        FindPthreads()
	IF(PTHREADS_FOUND)
	        ADD_DEFINITIONS(${PTHREADS_CFLAGS})
		LINK_LIBRARIES(${PTHREADS_LIBS})
		ELSE(PTHREADS_FOUND)
		MESSAGE(
		        FATAL_ERROR
			"Could not detect threading flags.\n"
			"Try specifying them manually in PTHREADS_CFLAGS and PTHREADS_LIBS."
			)
	ENDIF(PTHREADS_FOUND)
ELSEIF(WIN32)
        ADD_DEFINITIONS(-DNOMINMAX)
ENDIF(UNIX)

IF(CANBERRA_FOUND)
        ADD_DEFINITIONS(-DHAVE_CANBERRA)
        INCLUDE_DIRECTORIES("${CANBERRA_INCLUDE_DIRS}")
ENDIF(CANBERRA_FOUND)


CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
IF(NOT HAVE_STDINT_H)
        CONFIGURE_FILE(compat/pstdint.h "${CMAKE_CURRENT_BINARY_DIR}/stdint.h" COPYONLY)
        INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")
ELSE(NOT HAVE_STDINT_H)
        FILE(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/stdint.h")
ENDIF(NOT HAVE_STDINT_H)

LIST(APPEND EXTRA_LIBS ${TIFF_LIBRARY} ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${JPEG_LIBRARY})
IF(CANBERRA_FOUND)
	LIST(APPEND EXTRA_LIBS ${CANBERRA_LIBRARIES})
ENDIF(CANBERRA_FOUND)

IF (EXIV2_FOUND)
        LIST(APPEND EXTRA_LIBS ${EXIV2_LIBRARY})
ENDIF(EXIV2_FOUND)

IF(ENABLE_OPENJPEG AND OpenJPEG_FOUND)
   INCLUDE_DIRECTORIES(${OPENJPEG_INCLUDE_DIR})
   LIST(APPEND EXTRA_LIBS ${OPENJPEG_LIBRARY})
ENDIF()

# Add MuPDF dependencies in the correct order (dependent libraries first)
IF(MuPDF_FOUND)
    # MuPDF static library needs its dependencies explicitly linked
    # Order: mupdf, freetype, jbig2dec, jpeg, openjpeg, zlib, math, harfbuzz, mujs, gumbo
    IF(WIN32)
        find_package(Freetype REQUIRED)

        find_path(JBIG2DEC_INCLUDE_DIR NAMES jbig2.h REQUIRED)
        find_library(JBIG2DEC_LIBRARY NAMES jbig2decd jbig2dec NAMES_PER_DIR REQUIRED)
        find_package(HARFBUZZ NAMES harfbuzz REQUIRED)
    else()
        pkg_check_modules(FREETYPE REQUIRED freetype2)
        pkg_check_modules(JBIG2DEC REQUIRED jbig2dec)
        pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
        set(HARFBUZZ_LIBRARY ${HARFBUZZ_LINK_LIBRARIES})
    endif()

    pkg_check_modules(GUMBO REQUIRED gumbo)
    set(GUMBO_LIBRARY ${GUMBO_LINK_LIBRARIES})

    # FIND_LIBRARY(MATH_LIBRARY NAMES m PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")

    pkg_check_modules(MUJS REQUIRED IMPORTED_TARGET mujs)

    # For Ubuntu static builds, we need explicit dependencies
    IF(UNIX AND NOT APPLE)
        # Ensure math library is linked first
        IF(MATH_LIBRARY)
            LIST(APPEND EXTRA_LIBS ${MATH_LIBRARY})
        ENDIF()
        # Ensure zlib is linked (required by libtiff via MuPDF)
        LIST(APPEND EXTRA_LIBS ${ZLIB_LIBRARY})
    ENDIF()

    IF(NOT APPLE)
      # Add MuJS for JavaScript support in MuPDF
      IF(MUJS_LIBRARY)
          LIST(APPEND EXTRA_LIBS ${MUJS_LIBRARY})
      ENDIF()
      # Add Gumbo for HTML parsing in MuPDF
      IF(GUMBO_LIBRARY)
          LIST(APPEND EXTRA_LIBS ${GUMBO_LIBRARY})
      ENDIF()
      # Add HarfBuzz for text rendering
      IF(HARFBUZZ_LIBRARY)
        LIST(APPEND EXTRA_LIBS ${HARFBUZZ_LIBRARY})
      ENDIF()
    ENDIF()

    LIST(APPEND EXTRA_LIBS ${MuPDF_LIBRARIES})

    IF(FREETYPE_LIBRARY)
        LIST(APPEND EXTRA_LIBS ${FREETYPE_LIBRARY})
    ENDIF()
    IF(JBIG2DEC_LIBRARY)
        LIST(APPEND EXTRA_LIBS ${JBIG2DEC_LIBRARY})
    ENDIF()
    # jpeg, openjpeg, zlib, math are handled above or in pkg-config

    # For Ubuntu static builds, ensure JPEG dependency is met
    IF(UNIX AND NOT APPLE)
        LIST(APPEND EXTRA_LIBS ${JPEG_LIBRARY})
    ENDIF()
ENDIF()

IF(WIN32)
   LIST(APPEND EXTRA_LIBS "ws2_32.lib")
ENDIF(WIN32)


SET(MAYBE_QT_OPENGL_MODULE "")
IF(ENABLE_OPENGL)
        LIST(APPEND EXTRA_LIBS ${OPENGL_LIBRARIES})
        SET(MAYBE_QT_OPENGL_MODULE Qt${QT_VERSION_MAJOR}::OpenGL)
ENDIF()

# Prepare config.h
IF(WIN32)
        SET(TRANSLATIONS_DIR_REL "translations")
        SET(STYLESHEETS_DIR_REL "stylesheets")
ELSEIF(APPLE)
        SET(TRANSLATIONS_DIR_REL "../Resources/translations")
        SET(STYLESHEETS_DIR_REL "../Resources/stylesheets")
ELSE()
        SET(TRANSLATIONS_DIR_REL "share/scantailor-universal/translations")
        SET(STYLESHEETS_DIR_REL "share/scantailor-universal/stylesheets")
        SET(PIXMAPS_DIR_ABS "${CMAKE_INSTALL_PREFIX}/share/pixmaps/scantailor-universal/stylesheets")
ENDIF()
SET(TRANSLATIONS_DIR_ABS "${CMAKE_INSTALL_PREFIX}/${TRANSLATIONS_DIR_REL}")
SET(STYLESHEETS_DIR_ABS "${CMAKE_INSTALL_PREFIX}/${STYLESHEETS_DIR_REL}")


CONFIGURE_FILE(config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)


ADD_SUBDIRECTORY(src)


# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

ADD_SUBDIRECTORY(packaging)
