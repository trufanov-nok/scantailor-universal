PROJECT("Export functionality")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/core)

SET(
        sources
        ExportModes.h ExportSettings.h
        ImageSplitOps.h ImageSplitOps.cpp
        ExportThread.h ExportThread.cpp
)

SOURCE_GROUP("Sources" FILES ${sources})

ADD_LIBRARY(exporting STATIC ${sources})
QT5_USE_MODULES(exporting Widgets)
TRANSLATION_SOURCES(scantailor-universal ${sources})
