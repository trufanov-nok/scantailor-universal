PROJECT("Export functionality")

SET(
	sources
	ExportModes.h ExportSettings.h
	ImageSplitOps.h ImageSplitOps.cpp
	ExportThread.h ExportThread.cpp
)

SOURCE_GROUP("Sources" FILES ${sources})

ADD_LIBRARY(exporting STATIC ${sources})
QT5_USE_MODULES(exporting Widgets)
TRANSLATION_SOURCES(scantailor ${sources})
