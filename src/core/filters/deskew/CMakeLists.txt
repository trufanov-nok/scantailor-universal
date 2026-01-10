PROJECT("Deskew Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

SET(
        sources
        ImageView.cpp ImageView.h
        Thumbnail.cpp Thumbnail.h
        Filter.cpp Filter.h
        OptionsWidget.cpp OptionsWidget.h
        Settings.cpp Settings.h
        Task.cpp Task.h
        CacheDrivenTask.cpp CacheDrivenTask.h
        Dependencies.cpp Dependencies.h
        Params.cpp Params.h
        OrderByAngleProvider.h
)
SOURCE_GROUP("Sources" FILES ${sources})
FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

ADD_LIBRARY(deskew STATIC ${sources} ${ui_sources})
TARGET_LINK_LIBRARIES(deskew Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

TRANSLATION_SOURCES(scantailor-universal ${sources} ${ui_files})
