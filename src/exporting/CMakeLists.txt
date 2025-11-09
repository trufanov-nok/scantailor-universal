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
TARGET_LINK_LIBRARIES(exporting Qt${QT_VERSION_MAJOR}::Widgets)
if (${QT_VERSION_MAJOR} VERSION_LESS 6.0.0)
        TRANSLATION_SOURCES(scantailor-universal ${sources})
else()
endif()
