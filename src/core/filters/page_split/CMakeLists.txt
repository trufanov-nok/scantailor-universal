PROJECT("Page Split Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

SET(
        sources
        SplitLineObject.h
        ImageView.cpp ImageView.h
        Thumbnail.cpp Thumbnail.h
        Params.cpp Params.h
        Dependencies.cpp Dependencies.h
        PageLayout.cpp PageLayout.h
        PageLayoutEstimator.cpp PageLayoutEstimator.h
        VertLineFinder.cpp VertLineFinder.h
        Filter.cpp Filter.h
        OptionsWidget.cpp OptionsWidget.h
        SplitModeWidget.cpp SplitModeWidget.h
        Settings.cpp Settings.h
        Task.cpp Task.h
        CacheDrivenTask.cpp CacheDrivenTask.h
        LayoutType.cpp LayoutType.h
        UnremoveButton.cpp UnremoveButton.h
        OrderBySplitTypeProvider.cpp OrderBySplitTypeProvider.h
        OrderByPageSizeProvider.cpp OrderByPageSizeProvider.h
)
SOURCE_GROUP("Sources" FILES ${sources})
FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

ADD_LIBRARY(page_split STATIC ${sources} ${ui_sources})
TARGET_LINK_LIBRARIES(page_split Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)
TRANSLATION_SOURCES(scantailor-universal ${sources} ${ui_files})
