set(CPACK_GENERATOR "Bundle")

set(CPACK_BUNDLE_NAME "Scantailor Universal")
set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/ScanTailor.icns")


INSTALL(IMPORTED_RUNTIME_ARTIFACTS scantailor-universal
            RUNTIME_DEPENDENCY_SET runtime
            BUNDLE DESTINATION .
    )

INSTALL(RUNTIME_DEPENDENCY_SET runtime
        DESTINATION scantailor-universal.app/Contents/Frameworks
        PRE_EXCLUDE_REGEXES "^/System" "^/Library" "^/usr/lib"
        POST_EXCLUDE_REGEXES ""
)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/stylesheets"
        DESTINATION scantailor-universal.app/Contents/Resources)

CMAKE_PATH(SET QT5_PLUGINS_DIR NORMALIZE ${Qt5Core_DIR}/../../../plugins)
set(PLUGINS_DESTINATION scantailor-universal.app/Contents/PlugIns)

INSTALL(FILES ${QT5_PLUGINS_DIR}/platforms/libqcocoa.dylib    DESTINATION ${PLUGINS_DESTINATION}/platforms)

INSTALL(FILES ${QT5_PLUGINS_DIR}/styles/libqmacstyle.dylib    DESTINATION ${PLUGINS_DESTINATION}/styles)

INSTALL(FILES ${QT5_PLUGINS_DIR}/imageformats/libqico.dylib   DESTINATION ${PLUGINS_DESTINATION}/imageformats)
INSTALL(FILES ${QT5_PLUGINS_DIR}/imageformats/libqjpeg.dylib  DESTINATION ${PLUGINS_DESTINATION}/imageformats)
INSTALL(FILES ${QT5_PLUGINS_DIR}/imageformats/libqsvg.dylib   DESTINATION ${PLUGINS_DESTINATION}/imageformats)
INSTALL(FILES ${QT5_PLUGINS_DIR}/imageformats/libqtiff.dylib  DESTINATION ${PLUGINS_DESTINATION}/imageformats)

CMAKE_PATH(SET Qt5_DIR NORMALIZE "${Qt5Core_DIR}/../../..")

FOREACH(qm_file ${QM_FILES})
        STRING(REGEX REPLACE "^.*scantailor-universal_" "${Qt5_DIR}translations/qtbase_" qtbase_qm_file "${qm_file}")
        IF(EXISTS "${qtbase_qm_file}")
                INSTALL(FILES "${qtbase_qm_file}" DESTINATION "scantailor-universal.app/Contents/Resources/translations")
        ENDIF()
        STRING(REGEX REPLACE "^.*scantailor-universal_(.*)\.qm" "\\1" qtbase_qm_name "${qm_file}")
        INSTALL(DIRECTORY DESTINATION "scantailor-universal.app/Contents/Resources/${qtbase_qm_name}.lproj")
ENDFOREACH()

INCLUDE(CPack)
