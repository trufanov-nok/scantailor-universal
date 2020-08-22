
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/src/core")

SET(
        cli_only_sources
        ConsoleBatch.cpp ConsoleBatch.h
        main-cli.cpp
)

ADD_EXECUTABLE(scantailor-universal-cli ${cli_only_sources} )

TARGET_LINK_LIBRARIES(
        scantailor-universal-cli
        fix_orientation page_split deskew select_content page_layout output publish stcore
        dewarping zones interaction imageproc math foundation exporting
)

# Widgets module is used statically but not at runtime.
QT5_USE_MODULES(scantailor-universal-cli Widgets Xml)

IF(EXTRA_LIBS)
        TARGET_LINK_LIBRARIES(scantailor-universal-cli ${EXTRA_LIBS})
ENDIF()

IF(APPLE)
        INSTALL(TARGETS scantailor-universal-cli BUNDLE DESTINATION . RUNTIME DESTINATION bin)
ELSE(APPLE)
        INSTALL(TARGETS scantailor-universal-cli RUNTIME DESTINATION bin)
ENDIF(APPLE)
