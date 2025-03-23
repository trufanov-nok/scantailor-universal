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
TARGET_LINK_LIBRARIES(exporting Qt5::Widgets)
TRANSLATION_SOURCES(scantailor-universal ${sources})
