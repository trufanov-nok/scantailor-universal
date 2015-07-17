CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)

IF(POLICY CMP0003)
	# Allow linking with full library path instead of -L/path -llibrary
	CMAKE_POLICY(SET CMP0003 NEW)
ENDIF()

IF(POLICY CMP0048)
	# When PROJECT() is missing the VERSION argument, set the corresponding
	# CMake variables to zero.
	CMAKE_POLICY(SET CMP0048 NEW)
ENDIF()

SET(
	CMAKE_USER_MAKE_RULES_OVERRIDE
	"${CMAKE_SOURCE_DIR}/cmake/default_cflags.cmake"
)
SET(
	CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
	"${CMAKE_SOURCE_DIR}/cmake/default_cxxflags.cmake"
)

PROJECT("Scan Tailor")

# Keep the paths outside of the build directory in RPATH
# when installing. That's useful for building with say
# a custom build of Qt.
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Scan Tailor needs to find its shared libraries.
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/scantailor")

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

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

# This has to go quite early on, as otherwise we risk picking
# up an identically named header from a system include path.
INCLUDE_DIRECTORIES(. foundation math interaction zones)

# For config.h
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(TestCXXAcceptsFlag)
INCLUDE(CMakeDependentOption)
INCLUDE(cmake/FindPthreads.cmake)
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

GET_FILENAME_COMPONENT(source_outer_dir "${PROJECT_SOURCE_DIR}/.." ABSOLUTE)
GET_FILENAME_COMPONENT(build_outer_dir "${PROJECT_BINARY_DIR}/.." ABSOLUTE)

INCLUDE(FindOpenGL)
CMAKE_DEPENDENT_OPTION(
	ENABLE_OPENGL "OpenGL may be used for UI acceleration" ON
	"OPENGL_FOUND" OFF
)

INCLUDE(FindOpenCL)
CMAKE_DEPENDENT_OPTION(
	ENABLE_OPENCL "OpenCL may be used for acceleration of image processing" ON
	"OpenCL_FOUND" OFF
)

SET(STAGING_LIBS_DIR "")

IF(WIN32)
	FILE(GLOB eigen_dirs1 "${build_outer_dir}/eigen-*")
	FILE(GLOB eigen_dirs2 "${source_outer_dir}/eigen-*")

	FIND_PATH(
		EIGEN3_INCLUDE_DIR NAMES signature_of_eigen3_matrix_library
		PATHS ${eigen_dirs1} ${eigen_dirs2}
		DOC "Path to Eigen include directory (containing \"Eigen\" subdirectory)"
	)

	FIND_PATH(
		DEPS_BUILD_DIR export-vars.cmake
		HINTS "${build_outer_dir}/scantailor-deps-build"
		DOC "Build directory for Scan Tailor dependencies."
	)
	IF(NOT DEPS_BUILD_DIR)
		MESSAGE(
			FATAL_ERROR "The build directory for Scan Tailor dependencies could not be found! "
			"You can specify it manually in DEPS_BUILD_DIR variable. Make sure you build the dependencies first!"
		)
	ENDIF()
	SET(STAGING_LIBS_DIR "${DEPS_BUILD_DIR}/staging/libs")
	INCLUDE("${DEPS_BUILD_DIR}/export-vars.cmake")

	SET(Qt5Core_DIR "${QT_SRC_DIR}/qtbase/lib/cmake/Qt5Core")
	SET(Qt5Gui_DIR "${QT_SRC_DIR}/qtbase/lib/cmake/Qt5Gui")
	SET(Qt5Widgets_DIR "${QT_SRC_DIR}/qtbase/lib/cmake/Qt5Widgets")
	SET(Qt5Xml_DIR "${QT_SRC_DIR}/qtbase/lib/cmake/Qt5Xml")
	SET(Qt5Network_DIR "${QT_SRC_DIR}/qtbase/lib/cmake/Qt5Network")
	SET(Qt5OpenGL_DIR "${QT_SRC_DIR}/qtbase/lib/cmake/Qt5OpenGL")
	IF(QT_PREBUILT_DIR)
		SET(Qt5LinguistTools_DIR "${QT_PREBUILT_DIR}/lib/cmake/Qt5LinguistTools")
	ELSE()
		SET(Qt5LinguistTools_DIR "${QT_SRC_DIR}/qtbase/lib/cmake/Qt5LinguistTools")
	ENDIF()
ENDIF()

FIND_PACKAGE(Eigen3)
IF(NOT EIGEN3_FOUND)
	MESSAGE(FATAL_ERROR
		"Could not find Eigen (version 3) headers.\n"
		"You may need to install Eigen3 or set EIGEN3_INCLUDE_DIR manually."
	)
ENDIF()
INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})

# Once Eigen starts using C++11 alignas() feature, this can be removed.
ADD_DEFINITIONS(-DEIGEN_DONT_ALIGN_STATICALLY)

SET(CMAKE_AUTOMOC ON)
SET(qt_min_version 5.3)
FIND_PACKAGE(Qt5Core ${qt_min_version} REQUIRED)
FIND_PACKAGE(Qt5Gui ${qt_min_version} REQUIRED)
FIND_PACKAGE(Qt5Widgets ${qt_min_version} REQUIRED)
FIND_PACKAGE(Qt5Xml ${qt_min_version} REQUIRED)
FIND_PACKAGE(Qt5Network ${qt_min_version} REQUIRED)
FIND_PACKAGE(Qt5LinguistTools ${qt_min_version} REQUIRED)
IF(ENABLE_OPENGL)
	FIND_PACKAGE(Qt5OpenGL ${qt_min_version} REQUIRED)
ENDIF()

FIND_PATH(
	JPEG_INCLUDE_DIR jpeglib.h
	PATHS /usr/local/include /usr/include
	HINTS ${JPEG_DIR} # JPEG_DIR may come from export-vars.cmake
	DOC "Path to libjpeg headers."
)
IF(NOT JPEG_INCLUDE_DIR)
	MESSAGE(
		FATAL_ERROR
		"Could not find jpeg headers.\n"
		"You may need to install a package named libjpeg62-dev or similarly."
	)
ENDIF()

INCLUDE_DIRECTORIES("${JPEG_INCLUDE_DIR}")

FIND_LIBRARY(
	JPEG_LIBRARY NAMES jpeg libjpeg.lib
	PATHS /usr/local/lib /usr/lib
	HINTS ${STAGING_LIBS_DIR}
	DOC "Path to jpeg library."
)
IF(NOT JPEG_LIBRARY)
	MESSAGE(
		FATAL_ERROR
		"Could not find jpeg library.\n"
		"You may need to install a package named libjpeg62-dev or similarly."
	)
ENDIF()


FIND_PATH(
	ZLIB_INCLUDE_DIR zlib.h
	PATHS /usr/local/include /usr/include
	HINTS ${ZLIB_DIR} # ZLIB_DIR may come from export-vars.cmake
	DOC "Path to zlib headers."
)
IF(NOT ZLIB_INCLUDE_DIR)
	MESSAGE(
		FATAL_ERROR
		"Could not find zlib headers.\n"
		"You may need to install a package named zlib1g-dev or similarly."
	)
ENDIF()

INCLUDE_DIRECTORIES("${ZLIB_INCLUDE_DIR}")

FIND_LIBRARY(
	ZLIB_LIBRARY NAMES z zdll.lib
	PATHS /usr/local/lib /usr/lib
	HINTS ${STAGING_LIBS_DIR}
	DOC "Path to zlib library."
)
IF(NOT ZLIB_LIBRARY)
	MESSAGE(
		FATAL_ERROR
		"Could not find zlib library.\n"
		"You may need to install a package named zlib1g-dev or similarly."
	)
ENDIF()

FIND_PATH(
	PNG_INCLUDE_DIR png.h
	PATHS /usr/local/include /usr/include
	HINTS ${PNG_DIR} # PNG_DIR may come from export-vars.cmake
	DOC "Path to libpng headers."
)
IF(NOT PNG_INCLUDE_DIR)
	MESSAGE(
		FATAL_ERROR
		"Could not find libpng headers.\n"
		"You may need to install a package named libpng12-dev or similarly."
	)
ENDIF()

INCLUDE_DIRECTORIES("${PNG_INCLUDE_DIR}")

FIND_LIBRARY(
	PNG_LIBRARY NAMES png libpng.lib
	PATHS /usr/local/lib /usr/lib
	HINTS ${STAGING_LIBS_DIR}
	DOC "Path to png library."
)
IF(NOT PNG_LIBRARY)
	MESSAGE(
		FATAL_ERROR
		"Could not find libpng library.\n"
		"You may need to install a package named libpng12-dev or similarly."
	)
ENDIF()


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

SET(Boost_USE_MULTITHREADED ON)
FIND_PACKAGE(Boost 1.35.0 COMPONENTS unit_test_framework prg_exec_monitor)
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

ADD_DEFINITIONS(-DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION)

IF(WIN32)
	LIST(APPEND EXTRA_LIBS winmm imm32 ws2_32 ole32 oleaut32 uuid gdi32 comdlg32 winspool)
ENDIF()

LIST(APPEND EXTRA_LIBS ${TIFF_LIBRARY} ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${JPEG_LIBRARY})

SET(MAYBE_QT_OPENGL_MODULE "")
IF(ENABLE_OPENGL)
	LIST(APPEND EXTRA_LIBS ${OPENGL_LIBRARIES})
	SET(MAYBE_QT_OPENGL_MODULE Qt5::OpenGL)
ENDIF()

# Crash reporter, only Windows + MSVC supported for now.
# Must go before generating config.h, as ENABLE_CRASH_REPORTER
# goes in there.
IF(MSVC)
	OPTION(ENABLE_CRASH_REPORTER "Enable crash reporter (only for official builds)" OFF)
ENDIF(MSVC)

# Prepare config.h
IF(WIN32)
	SET(TRANSLATION_DIRS ".:translations")
	SET(PLUGIN_DIRS ".")
ELSE()
	SET(TRANSLATION_DIRS ".:${CMAKE_INSTALL_PREFIX}/share/scantailor/translations")
	SET(PLUGIN_DIRS ".:${CMAKE_INSTALL_PREFIX}/lib/scantailor")
ENDIF()

CONFIGURE_FILE(config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)

ADD_SUBDIRECTORY(ui)
INCLUDE_DIRECTORIES("${toplevel_ui_sources_BINARY_DIR}")

# crash_reporter is included unconditionally to collect translation sources from there.
ADD_SUBDIRECTORY(crash_reporter)
ADD_SUBDIRECTORY(dewarping)
ADD_SUBDIRECTORY(foundation)
ADD_SUBDIRECTORY(math)
ADD_SUBDIRECTORY(imageproc)
ADD_SUBDIRECTORY(interaction)
ADD_SUBDIRECTORY(zones)
ADD_SUBDIRECTORY(acceleration)
ADD_SUBDIRECTORY(tests)

ADD_SUBDIRECTORY(filters/fix_orientation)
ADD_SUBDIRECTORY(filters/page_split)
ADD_SUBDIRECTORY(filters/deskew)
ADD_SUBDIRECTORY(filters/select_content)
ADD_SUBDIRECTORY(filters/page_layout)
ADD_SUBDIRECTORY(filters/output)

SET(resource_files resources/resources.qrc resources/icons/COPYING)
IF(WIN32)
	LIST(APPEND resource_files resources/win32/resources.rc)
ENDIF()
SOURCE_GROUP("Resources" FILES ${resource_files})

QT5_ADD_RESOURCES(resource_sources resources/resources.qrc)
SOURCE_GROUP("Generated" FILES ${resource_sources})

SET(
	common_sources
	BackgroundExecutor.cpp BackgroundExecutor.h
	OpenGLSupport.cpp OpenGLSupport.h
	PixmapRenderer.cpp PixmapRenderer.h
	BubbleAnimation.cpp BubbleAnimation.h
	ProcessingIndicationWidget.cpp ProcessingIndicationWidget.h
	NonOwningWidget.cpp NonOwningWidget.h
	ContentBox.cpp ContentBox.h
	SmartFilenameOrdering.cpp SmartFilenameOrdering.h
	AbstractRelinker.h
	RelinkablePath.cpp RelinkablePath.h
	ImageInfo.cpp ImageInfo.h
	ImageFileInfo.h
	ImageMetadata.cpp ImageMetadata.h
	RecentProjects.cpp RecentProjects.h
	OutOfMemoryHandler.cpp OutOfMemoryHandler.h
	CommandLine.cpp CommandLine.h
	PageSelectionAccessor.cpp PageSelectionAccessor.h
	PageSelectionProvider.h
	ContentSpanFinder.cpp ContentSpanFinder.h
	ImagePixmapUnion.h
	ImageViewBase.cpp ImageViewBase.h
	BasicImageView.cpp BasicImageView.h
	DebugImageView.cpp DebugImageView.h
	VectorFieldImageView.cpp VectorFieldImageView.h
	TabbedDebugImages.cpp TabbedDebugImages.h
	ThumbnailLoadResult.h
	ThumbnailPixmapCache.cpp ThumbnailPixmapCache.h
	ThumbnailBase.cpp ThumbnailBase.h
	ThumbnailFactory.cpp ThumbnailFactory.h
	IncompleteThumbnail.cpp IncompleteThumbnail.h
	ContentBoxPropagator.cpp ContentBoxPropagator.h
	PageOrientationPropagator.cpp PageOrientationPropagator.h
	DebugImagesImpl.cpp DebugImagesImpl.h
	DebugViewFactory.h
	ImageId.cpp ImageId.h
	PageId.cpp PageId.h
	PageInfo.cpp PageInfo.h
	BackgroundTask.cpp BackgroundTask.h
	ProcessingTaskQueue.cpp ProcessingTaskQueue.h
	PageSequence.cpp PageSequence.h
	StageSequence.cpp StageSequence.h
	ProjectPages.cpp ProjectPages.h
	ImageMetadataLoader.cpp ImageMetadataLoader.h
	TiffReader.cpp TiffReader.h
	TiffWriter.cpp TiffWriter.h
	PngMetadataLoader.cpp PngMetadataLoader.h
	TiffMetadataLoader.cpp TiffMetadataLoader.h
	JpegMetadataLoader.cpp JpegMetadataLoader.h
	ImageLoader.cpp ImageLoader.h
	ErrorWidget.cpp ErrorWidget.h
	OrthogonalRotation.cpp OrthogonalRotation.h
	WorkerThreadPool.cpp WorkerThreadPool.h
	LoadFileTask.cpp LoadFileTask.h
	FilterOptionsWidget.h
	TaskStatus.h FilterUiInterface.h
	ProjectReader.cpp ProjectReader.h
	ProjectWriter.cpp ProjectWriter.h
	AtomicFileOverwriter.cpp AtomicFileOverwriter.h
	EstimateBackground.cpp EstimateBackground.h
	Despeckle.cpp Despeckle.h
	FileNameDisambiguator.cpp FileNameDisambiguator.h
	OutputFileNameGenerator.cpp OutputFileNameGenerator.h
	PageRange.cpp PageRange.h
	SelectedPage.cpp SelectedPage.h
	Utils.cpp Utils.h
	PageView.h
	AutoManualMode.h
	AbstractCommand.h
	AbstractFilter.h
	BeforeOrAfter.h
	FilterResult.h
	CompositeCacheDrivenTask.h
	ChangedStateItemDelegate.h
	PageOrderProvider.h
	PageOrderOption.h
	PayloadEvent.h
	filter_dc/AbstractFilterDataCollector.h
	filter_dc/ThumbnailCollector.h
	filter_dc/ContentBoxCollector.h
	filter_dc/PageOrientationCollector.h
	version.h
	config.h.in
	${common_ui_files}
)

SET(
	gui_only_sources
	Application.cpp Application.h
	SkinnedButton.cpp SkinnedButton.h
	StageListView.cpp StageListView.h
	RelinkablePathVisualization.cpp RelinkablePathVisualization.h
	RelinkingModel.cpp RelinkingModel.h
	RelinkingSortingModel.cpp RelinkingSortingModel.h
	RelinkingListView.cpp RelinkingListView.h
	RelinkingDialog.cpp RelinkingDialog.h
	SettingsDialog.cpp SettingsDialog.h
	LoadFilesStatusDialog.cpp LoadFilesStatusDialog.h
	ProjectCreationContext.cpp ProjectCreationContext.h
	ProjectOpeningContext.cpp ProjectOpeningContext.h
	OutOfMemoryDialog.cpp OutOfMemoryDialog.h
	ThumbnailSequence.cpp ThumbnailSequence.h
	ProjectFilesDialog.cpp ProjectFilesDialog.h
	NewOpenProjectPanel.cpp NewOpenProjectPanel.h
	SystemLoadWidget.cpp SystemLoadWidget.h
	MainWindow.cpp MainWindow.h
	main.cpp
)

SET(
	cli_only_sources
	ConsoleBatch.cpp ConsoleBatch.h
	main-cli.cpp
)

SOURCE_GROUP("Sources" FILES ${common_sources} ${gui_only_sources} ${cli_only_sources})
SOURCE_GROUP("Special Headers" FILES version.h config.h.in)

IF(CMAKE_COMPILER_IS_GNUCXX)
	# There seems to be a bug in either gcc or boost that makes page sorting either crash
	# or go into infinite loop.  It's reproducible on a very simple test case on at least
	# gcc 4.4.3 and gcc 4.4.4.  It's not reproducible on gcc 4.5.1.
	# Adding -fno-strict-aliasing seems to fix it.
	SET_SOURCE_FILES_PROPERTIES(
		ThumbnailSequence.cpp PROPERTIES
		COMPILE_FLAGS "-fno-strict-aliasing"
	)
ENDIF()

SET(win32_resource_file "")
IF(WIN32)
	SET(rc_file "${CMAKE_SOURCE_DIR}/resources/win32/resources.rc")
	FILE(GLOB win32_resources resources/win32/*.ico)
	SET_SOURCE_FILES_PROPERTIES(
		"${rc_file}" PROPERTIES
		OBJECT_DEPENDS ${win32_resources}
	)
	SET(win32_resource_file "${rc_file}")
ENDIF(WIN32)

ADD_LIBRARY(stcore STATIC ${common_sources} ${common_ui_sources})
TARGET_LINK_LIBRARIES(stcore Qt5::Widgets Qt5::Xml ${MAYBE_QT_OPENGL_MODULE})
ADD_DEPENDENCIES(stcore toplevel_ui_sources)

ADD_EXECUTABLE(
	scantailor WIN32 ${gui_only_sources} ${resource_sources} ${win32_resource_file}
)

ADD_EXECUTABLE(scantailor-cli ${cli_only_sources} ${common_ui_sources})

TARGET_LINK_LIBRARIES(
	scantailor
	acceleration page_layout output
	fix_orientation page_split deskew select_content stcore
	dewarping zones interaction imageproc math foundation
	${EXTRA_LIBS}
)
TARGET_LINK_LIBRARIES(
	scantailor-cli
	acceleration page_layout output
	fix_orientation page_split deskew select_content stcore
	dewarping zones interaction imageproc math foundation
	${EXTRA_LIBS}
)

INSTALL(TARGETS scantailor scantailor-cli RUNTIME DESTINATION bin)

IF(ENABLE_CRASH_REPORTER)
	FIND_PATH(
		SYMBOLS_PATH . PATHS "${build_outer_dir}/symbols"
		"${source_outer_dir}/symbols" NO_DEFAULT_PATH
		DOC "The directory to write symbol information into."
	)
	IF(NOT SYMBOLS_PATH)
		MESSAGE(FATAL_ERROR "SYMBOLS_PATH directory is not set.")
	ENDIF()

	# We can't build it, because it requires ATL, which is not part
	# of the Visual Studio Express Edition, so we rely on a pre-built
	# version which can be found in the Mozilla repository.
	FIND_PROGRAM(
		DUMP_SYMS_EXECUTABLE dump_syms PATHS "${build_outer_dir}" "${source_outer_dir}"
		DOC "Path to dump_syms.exe, which can be found in Mozilla repository."
	)
	IF(NOT DUMP_SYMS_EXECUTABLE)
		MESSAGE(
			FATAL_ERROR "dump_syms.exe wasn't found.  Specify its location manually by setting the DUMP_SYMS_EXECUTABLE variable.  dump_syms.exe may be found in the Mozilla repository under /toolkit/crashreporter/tools/win32"
		)
	ENDIF()

	INCLUDE_DIRECTORIES(crash_reporter crash_reporter/google-breakpad)
	TARGET_LINK_LIBRARIES(scantailor crash_handler)
ENDIF()

# Translations
TRANSLATION_SOURCES(
	scantailor
	${common_sources} ${gui_only_sources} ${cli_only_sources}
)
FILE(GLOB TRANSLATION_FILES translations/scantailor_*.ts)
FILE(GLOB CR_TRANSLATION_FILES translations/crashreporter_*.ts)

FINALIZE_TRANSLATION_SET(scantailor ${TRANSLATION_FILES})
FINALIZE_TRANSLATION_SET(crashreporter ${CR_TRANSLATION_FILES})
UPDATE_TRANSLATIONS_TARGET(update_translations scantailor crashreporter)

SET(ts_files ${TRANSLATION_FILES})
IF(ENABLE_CRASH_REPORTER)
	LIST(APPEND ts_files ${CR_TRANSLATION_FILES})
ENDIF(ENABLE_CRASH_REPORTER)

# Don't build *.qm files from *untranslated.ts
SET(FILTERED_TRANSLATION_FILES)
FOREACH(ts_file ${ts_files})
	IF ("${ts_file}" MATCHES ".*untranslated.ts")
		# Just skip it.
	ELSE("${ts_file}" MATCHES ".*untranslated.ts")
		LIST(APPEND FILTERED_TRANSLATION_FILES "${ts_file}")
	ENDIF("${ts_file}" MATCHES ".*untranslated.ts")
ENDFOREACH(ts_file)

QT5_ADD_TRANSLATION(QM_FILES ${FILTERED_TRANSLATION_FILES})
ADD_CUSTOM_TARGET(compile_translations ALL DEPENDS ${QM_FILES})

INSTALL(FILES ${QM_FILES} DESTINATION "share/scantailor/translations")

# Source code packaging
SET(CPACK_CMAKE_GENERATOR "")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "scantailor-${VERSION}")
SET(
	CPACK_SOURCE_IGNORE_FILES
	"/\\\\.svn/"
	"/\\\\.git/"
	"~$"
	"\\\\.pcs$"
	"TODO.txt"
	"CMakeLists.txt.user"
	"/doxygen/"
	"${CMAKE_BINARY_DIR}"
)
INCLUDE(CPack)

IF(WIN32)
	# Copy some DLLs to the staging dir.
	
	SET(
		qt5_libs
		${Qt5Widgets_LIBRARIES} ${Qt5Gui_LIBRARIES}
		${Qt5Core_LIBRARIES} ${Qt5Xml_LIBRARIES}
	)
	IF(ENABLE_OPENGL)
		LIST(APPEND qt5_libs ${Qt5OpenGL_LIBRARIES})
	ENDIF()
	FOREACH(target ${qt5_libs})
		GET_TARGET_PROPERTY(debug_loc "${target}" LOCATION_DEBUG)
		GET_TARGET_PROPERTY(release_loc "${target}" LOCATION_RELEASE)
		COPY_TO_BUILD_DIR("${debug_loc}" CONFIGURATIONS Debug)
		COPY_TO_BUILD_DIR("${release_loc}" CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
	ENDFOREACH()
		
	SET(DLL_DIR "${QT_SRC_DIR}/qtbase/bin")
	COPY_TO_BUILD_DIR(
		"${DLL_DIR}/libpng.dll" "${DLL_DIR}/libjpeg.dll"
		"${DLL_DIR}/libtiff.dll" "${DLL_DIR}/zdll.dll"
	)

	# Qt's plugins.
	SET(PLUGINS_DIR "${QT_SRC_DIR}/qtbase/plugins")
	COPY_TO_BUILD_DIR(
		"${PLUGINS_DIR}/platforms/qwindows.dll" SUBDIR platforms
		CONFIGURATIONS Release MinSizeRel RelWithDebInfo
	)
	COPY_TO_BUILD_DIR(
		"${PLUGINS_DIR}/platforms/qwindowsd.dll" SUBDIR platforms
		CONFIGURATIONS Debug
	)

	IF(EXISTS "${PLUGINS_DIR}/accessible/qtaccessiblewidgets.dll")
		COPY_TO_BUILD_DIR(
			"${PLUGINS_DIR}/accessible/qtaccessiblewidgets.dll" SUBDIR accessible
			CONFIGURATIONS Release MinSizeRel RelWithDebInfo
		)
	ENDIF()

	IF(EXISTS "${PLUGINS_DIR}/accessible/qtaccessiblewidgetsd.dll")
		COPY_TO_BUILD_DIR(
			"${PLUGINS_DIR}/accessible/qtaccessiblewidgetsd.dll" SUBDIR accessible
			CONFIGURATIONS Debug
		)
	ENDIF()
	
	# Generate the target that will actually do the copying.
	GENERATE_COPY_TO_BUILD_DIR_TARGET(copy_to_build_dir)
	
	# Installer for Windows.  Must go last.
	ADD_SUBDIRECTORY(packaging/windows)
ENDIF(WIN32)
