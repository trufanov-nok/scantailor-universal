# This has to go quite early on, as otherwise we risk picking
# up an identically named header from a system include path.
INCLUDE_DIRECTORIES(. foundation math core/interaction core/zones)

FILE(GLOB TRANSLATION_FILES translations/scantailor-universal_*.ts)

ADD_SUBDIRECTORY(dewarping)
ADD_SUBDIRECTORY(foundation)
ADD_SUBDIRECTORY(imageproc)
ADD_SUBDIRECTORY(math)
ADD_SUBDIRECTORY(exporting)
ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(app_cli)
ADD_SUBDIRECTORY(app)

SET(ts_files ${TRANSLATION_FILES})

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

IF(WIN32 OR APPLE)
        # Just copy already compiled qtbase*.qm from translations folder
        # if any of them are present there
        # to support localization of Qt own controls on Win machines
        FILE(GLOB QTBASE_QM_FILES translations/qtbase*.qm)
        LIST(APPEND QM_FILES ${QTBASE_QM_FILES})
ENDIF(WIN32 OR APPLE)

ADD_CUSTOM_TARGET(compile_translations ALL DEPENDS ${QM_FILES})

INSTALL(FILES ${QM_FILES} DESTINATION "${TRANSLATIONS_DIR_REL}/")

# Let QM_FILES be accessable in packaging/windows/
SET (QM_FILES ${QM_FILES} PARENT_SCOPE)
