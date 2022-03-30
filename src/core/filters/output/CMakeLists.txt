PROJECT("Output Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

IF(EXIV2_FOUND)
        SET( exiv2_sources
            ImageMetadataCopier.cpp ImageMetadataCopier.h)
ENDIF()

SET(
        sources
        ChangeDpiWidget.cpp ChangeDpiWidget.h
        ImageView.cpp ImageView.h
        ImageViewTab.h
        TabbedImageView.cpp TabbedImageView.h
        Filter.cpp Filter.h
        OptionsWidget.cpp OptionsWidget.h
        Task.cpp Task.h
        CacheDrivenTask.cpp CacheDrivenTask.h
        OutputGenerator.cpp OutputGenerator.h
        OutputMargins.h
        Settings.cpp Settings.h
        Thumbnail.cpp Thumbnail.h
        Utils.cpp Utils.h
        Params.cpp Params.h
        BlackWhiteOptions.cpp BlackWhiteOptions.h
        ColorGrayscaleOptions.cpp ColorGrayscaleOptions.h
        RenderParams.cpp RenderParams.h
        ColorParams.cpp ColorParams.h
        OutputImageParams.cpp OutputImageParams.h
        OutputFileParams.cpp OutputFileParams.h
        OutputParams.cpp OutputParams.h
        PictureLayerProperty.cpp PictureLayerProperty.h
        ZoneCategoryProperty.cpp ZoneCategoryProperty.h
        VirtualZoneProperty.cpp VirtualZoneProperty.h
        PictureZonePropFactory.cpp PictureZonePropFactory.h
        PictureZonePropDialog.cpp PictureZonePropDialog.h
        PictureZoneComparator.cpp PictureZoneComparator.h
        PictureZoneEditor.cpp PictureZoneEditor.h
        FillColorProperty.cpp FillColorProperty.h
        FillZonePropFactory.cpp FillZonePropFactory.h
        FillZoneComparator.cpp FillZoneComparator.h
        FillZoneEditor.cpp FillZoneEditor.h
        ColorPickupInteraction.cpp ColorPickupInteraction.h
        DespeckleState.cpp DespeckleState.h
        DespeckleView.cpp DespeckleView.h
        DespeckleVisualization.cpp DespeckleVisualization.h
        DespeckleLevel.cpp DespeckleLevel.h
        DewarpingView.cpp DewarpingView.h
        DewarpingMode.cpp DewarpingMode.h
        ChangeDewarpingWidget.cpp ChangeDewarpingWidget.h
        DepthPerception.cpp DepthPerception.h
        OrderByModeProvider.cpp OrderByModeProvider.h
        OrderBySourceColor.cpp OrderBySourceColor.h
        OrderByDewarpingModeProvider.cpp OrderByDewarpingModeProvider.h
        ${exiv2_sources}
)
SOURCE_GROUP("Sources" FILES ${sources})
FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

QT5_WRAP_UI(ui_sources ${ui_files})

SET(output_filter_resource_files resources/output_filter_resources.qrc)
SOURCE_GROUP("OutputFilterResources" FILES ${resource_files})

ADD_LIBRARY(output STATIC ${sources} ${ui_sources})
QT5_USE_MODULES(output Widgets Xml)
ADD_DEPENDENCIES(output toplevel_ui_sources)

TRANSLATION_SOURCES(scantailor-universal ${sources} ${ui_files})
