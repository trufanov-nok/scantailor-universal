PROJECT("Deskew Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

SET(
	sources
	ImageView.cpp ImageView.h
	DewarpingMode.cpp DewarpingMode.h
	DewarpingView.cpp DewarpingView.h
	RotationThumbnail.cpp RotationThumbnail.h
	DewarpingThumbnail.cpp DewarpingThumbnail.h
	Filter.cpp Filter.h
	OptionsWidget.cpp OptionsWidget.h
	Settings.cpp Settings.h
	Task.cpp Task.h
	CacheDrivenTask.cpp CacheDrivenTask.h
	Dependencies.cpp Dependencies.h
	DistortionType.cpp DistortionType.h
	RotationParams.cpp RotationParams.h
	PerspectiveParams.cpp PerspectiveParams.h
	DewarpingParams.cpp DewarpingParams.h
	Params.cpp Params.h
	ApplyDialog.cpp ApplyDialog.h
	Utils.cpp Utils.h
)
SOURCE_GROUP("Sources" FILES ${sources})

FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

QT5_WRAP_UI(ui_sources ${ui_files})

ADD_LIBRARY(deskew STATIC ${sources} ${ui_sources})
QT5_USE_MODULES(deskew Widgets Xml)

TRANSLATION_SOURCES(scantailor ${sources} ${ui_files})
