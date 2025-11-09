set(CPACK_GENERATOR "NSIS")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
SET(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/src/app/resources/win32/icon.ico")
SET(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
SET(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
SET(CPACK_NSIS_MODIFY_PATH ON)
SET(CPACK_NSIS_DISPLAY_NAME "${CMAKE_PROJECT_NAME} ${VERSION}")
SET(CPACK_NSIS_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
SET(CPACK_NSIS_INSTALLED_ICON_NAME "scantailor-universal.exe")
SET(CPACK_NSIS_EXECUTABLES_DIRECTORY .)
# Install Runtime
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
INCLUDE(InstallRequiredSystemLibraries)
# Install Qt
SET(
        qt5_libs
        ${Qt5Widgets_LIBRARIES} ${Qt5Gui_LIBRARIES}
        ${Qt5Core_LIBRARIES} ${Qt5Xml_LIBRARIES}  ${Qt5Svg_LIBRARIES}
)
IF(ENABLE_OPENGL)
        LIST(APPEND qt5_libs ${Qt5OpenGL_LIBRARIES})
ENDIF()
FOREACH(target ${qt5_libs})
        GET_TARGET_PROPERTY(debug_loc "${target}" LOCATION_DEBUG)
        GET_TARGET_PROPERTY(release_loc "${target}" LOCATION_RELEASE)
        install(FILES "${debug_loc}"   DESTINATION . CONFIGURATIONS Debug)
        install(FILES "${release_loc}" DESTINATION . CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
ENDFOREACH()
# Install Qt's plugins.
CMAKE_PATH(SET Qt5_DIR NORMALIZE "${Qt5Core_DIR}/../../..")
CMAKE_PATH(APPEND Qt5_DIR "plugins" OUTPUT_VARIABLE PLUGINS_DIR)
# Install system plugin
install(FILES "${PLUGINS_DIR}/platforms/qwindows.dll"  DESTINATION platforms CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
install(FILES "${PLUGINS_DIR}/platforms/qwindowsd.dll" DESTINATION platforms CONFIGURATIONS Debug)
# Install style plugin
install(FILES "${PLUGINS_DIR}/styles/qwindowsvistastyle.dll"  DESTINATION styles CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
install(FILES "${PLUGINS_DIR}/styles/qwindowsvistastyled.dll" DESTINATION styles CONFIGURATIONS Debug)
# Install accessibility plugin
IF (Qt5Widgets_VERSION VERSION_LESS 5.4.0)
        install(FILES "${PLUGINS_DIR}/accessible/qtaccessiblewidgets.dll"  DESTINATION accessible CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
        install(FILES "${PLUGINS_DIR}/accessible/qtaccessiblewidgetsd.dll" DESTINATION accessible CONFIGURATIONS Debug)
ENDIF()
# Install image format plugins
install(FILES "${PLUGINS_DIR}/imageformats/qjpeg.dll"  DESTINATION imageformats CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
install(FILES "${PLUGINS_DIR}/imageformats/qjpegd.dll" DESTINATION imageformats CONFIGURATIONS Debug)
install(FILES "${PLUGINS_DIR}/imageformats/qtiff.dll"  DESTINATION imageformats CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
install(FILES "${PLUGINS_DIR}/imageformats/qtiffd.dll" DESTINATION imageformats CONFIGURATIONS Debug)
# Install image format plugins required to display images in stylesheets
install(FILES "${PLUGINS_DIR}/imageformats/qico.dll"   DESTINATION imageformats CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
install(FILES "${PLUGINS_DIR}/imageformats/qicod.dll"  DESTINATION imageformats CONFIGURATIONS Debug)
install(FILES "${PLUGINS_DIR}/imageformats/qsvg.dll"   DESTINATION imageformats CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
install(FILES "${PLUGINS_DIR}/imageformats/qsvgd.dll"  DESTINATION imageformats CONFIGURATIONS Debug)
install(FILES "${PLUGINS_DIR}/iconengines/qsvgicon.dll"  DESTINATION iconengines CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
install(FILES "${PLUGINS_DIR}/iconengines/qsvgicond.dll" DESTINATION iconengines CONFIGURATIONS Debug)
# Install Qt's translations
FOREACH(qm_file ${QM_FILES})
        STRING(REGEX REPLACE "^.*scantailor-universal_" "${Qt5_DIR}translations/qtbase_" qtbase_qm_file "${qm_file}")
        IF(EXISTS "${qtbase_qm_file}")
                INSTALL(FILES "${qtbase_qm_file}" DESTINATION translations)
        ENDIF()
ENDFOREACH()
# Install stylesheets
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/stylesheets"
        DESTINATION .
        PATTERN "CMakeLists.txt" EXCLUDE
        PATTERN ".gitignore" EXCLUDE
        PATTERN ".git" EXCLUDE)
# Install libpng
FIND_FILE(LIBPNG_DLL
        NAMES libpng.dll libpng16.dll
        PATHS ${CMAKE_PREFIX_PATH}
        PATH_SUFFIXES bin lib
        NO_DEFAULT_PATH)
IF(NOT LIBPNG_DLL)
        MESSAGE(SEND_ERROR "Could not find LIBPNG dll. You may specify it manually.")
ENDIF()
install(FILES "${LIBPNG_DLL}"  DESTINATION .)
# Install libjpeg
FIND_FILE(LIBJPEG_DLL
        NAMES libjpeg.dll jpeg.dll jpeg62.dll libjpeg-62.dll
        PATHS ${CMAKE_PREFIX_PATH}
        PATH_SUFFIXES bin lib
        NO_DEFAULT_PATH)
IF(NOT LIBJPEG_DLL)
        MESSAGE(SEND_ERROR "Could not find LIBJPEG dll. You may specify it manually.")
ENDIF()
install(FILES "${LIBJPEG_DLL}"  DESTINATION .)
# Install libtigg
FIND_FILE(LIBTIFF_DLL
        NAMES libtiff.dll tiff.dll
        PATHS ${CMAKE_PREFIX_PATH}
        PATH_SUFFIXES bin lib
        NO_DEFAULT_PATH)
IF(NOT LIBTIFF_DLL)
        MESSAGE(SEND_ERROR "Could not find LIBTIFF dll. You may specify it manually.")
ENDIF()
install(FILES "${LIBTIFF_DLL}"  DESTINATION .)
# Install zlib
FIND_FILE(ZLIB_DLL
        NAMES zdll.dll zlib.dll zlib1.dll z.dll libzlib.dll
        PATHS ${CMAKE_PREFIX_PATH}
        PATH_SUFFIXES bin lib
        NO_DEFAULT_PATH)
IF(NOT ZLIB_DLL)
        MESSAGE(SEND_ERROR "Could not find ZLIB dll. You may specify it manually.")
ENDIF()
install(FILES "${ZLIB_DLL}"  DESTINATION .)
# Install openjpeg
IF(ENABLE_OPENJPEG)
        FIND_FILE(OPENJPEG_DLL
                NAMES openjp2.dll libopenjp2.dll
                PATHS ${CMAKE_PREFIX_PATH}
                PATH_SUFFIXES bin lib
                NO_DEFAULT_PATH)
        IF(NOT OPENJPEG_DLL)
                MESSAGE(SEND_ERROR "Could not find OPENJPEG dll. You may specify it manually.")
        ENDIF()
        install(FILES "${OPENJPEG_DLL}"  DESTINATION .)
ENDIF()
# Install exiv
IF(ENABLE_EXIV2)
        FIND_FILE(EXIV2_DLL
                NAMES exiv2.dll libexiv2.dll
                PATHS ${CMAKE_PREFIX_PATH}
                PATH_SUFFIXES bin lib
                NO_DEFAULT_PATH)
        IF(NOT EXIV2_DLL)
                MESSAGE(SEND_ERROR "Could not find EXIV2 dll. You may specify it manually.")
        ENDIF()
        install(FILES "${EXIV2_DLL}"  DESTINATION .)
ENDIF()

INCLUDE(CPack)
