PROJECT("Publishing Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

SET(
        sources
        Filter.cpp Filter.h
        Task.cpp Task.h
        CacheDrivenTask.cpp CacheDrivenTask.h
        OptionsWidget.cpp OptionsWidget.h
	Params.cpp Params.h
        SourceImagesInfo.cpp SourceImagesInfo.h
        OCRParams.cpp OCRParams.h
        Settings.cpp Settings.h
	DjbzDispatcher.cpp DjbzDispatcher.h
	OutputParams.cpp OutputParams.h
        DisplayPreferences.cpp DisplayPreferences.h
        OrderByFileSize.cpp OrderByFileSize.h
        OrderByDjbz.cpp OrderByDjbz.h
        OrderByLayers.cpp OrderByLayers.h
        DjbzManagerDialog.cpp DjbzManagerDialog.h
        DjVuBookmarkDispatcher.cpp DjVuBookmarkDispatcher.h
        ContentsManagerDialog.cpp ContentsManagerDialog.h
        FreeImageFilters.cpp FreeImageFilters.h
        EncodingProgressWidget.cpp EncodingProgressWidget.h
        EncodingProgressInfo.h
        ClickableLabel.cpp ClickableLabel.h
        MetadataEditorDialog.cpp MetadataEditorDialog.h
        ManageDisplayPreferencesDialog.cpp ManageDisplayPreferencesDialog.h
        djview4/qdjvuwidget.cpp djview4/qdjvuwidget.h
        djview4/qdjvu.cpp djview4/qdjvu.h
        tesseract2djvused/hocr2djvused.cpp tesseract2djvused/hocr2djvused.h

)

# we link tesseract2djvused/hocr2djvused.cpp as well as generate standalone exe for usage from a script
ADD_SUBDIRECTORY(tesseract2djvused)

SOURCE_GROUP("Sources" FILES ${sources})
FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

QT5_WRAP_UI(ui_sources ${ui_files})
QT5_ADD_RESOURCES(resource_files djview4/qdjvuwidget.qrc resources/dialogs.qrc)

ADD_LIBRARY(publish STATIC ${sources} ${ui_sources} ${resource_files})
QT5_USE_MODULES(publish Widgets Xml)
TRANSLATION_SOURCES(scantailor-universal ${sources} ${ui_files})
