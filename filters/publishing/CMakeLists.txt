PROJECT("Publishing Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

SET(
        sources
        Filter.cpp Filter.h
        Task.cpp Task.h
        CacheDrivenTask.cpp CacheDrivenTask.h
        OptionsWidget.cpp OptionsWidget.h
        MissingApplicationsWidget.cpp MissingApplicationsWidget.h
        ChangeDpiDialog.cpp ChangeDpiDialog.h
        ImageInfo.cpp ImageInfo.h
        Params.cpp Params.h
        Settings.cpp Settings.h
        AppDependency.cpp AppDependency.h
        djvuencoder.cpp djvuencoder.h
        QuickWidgetAccessHelper.cpp QuickWidgetAccessHelper.h
        QMLLoader.cpp QMLLoader.h        
        OrderByFileSize.cpp OrderByFileSize.h
        djview4/qdjvuwidget.cpp djview4/qdjvuwidget.h
        djview4/qdjvu.cpp djview4/qdjvu.h
)

SOURCE_GROUP("Sources" FILES ${sources})
FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

QT5_WRAP_UI(ui_sources ${ui_files})
QT5_ADD_RESOURCES(resource_files djview4/qdjvuwidget.qrc OPTIONS -no-compress)

ADD_LIBRARY(publishing STATIC ${sources} ${ui_sources} ${resource_files})
QT5_USE_MODULES(publishing Widgets Xml Qml QuickWidgets QuickControls2 )
TRANSLATION_SOURCES(scantailor ${sources} ${ui_files})

file(GLOB qml_files
    "*.qml"
)

add_custom_command(TARGET publishing PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${qml_files} $<TARGET_FILE_DIR:publishing>)
