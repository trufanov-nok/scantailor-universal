PROJECT("Dewarping library")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

SET(
	sources
	dewarping_config.h
	Curve.cpp Curve.h
	DetectVerticalBounds.cpp DetectVerticalBounds.h
	DepthPerception.cpp DepthPerception.h
	DistortionModel.cpp DistortionModel.h
	DistortionModelBuilder.cpp DistortionModelBuilder.h
	TextLineTracer.cpp TextLineTracer.h
	TextLineRefiner.cpp TextLineRefiner.h
	TextLineSegmenter.cpp TextLineSegmenter.h
	TopBottomEdgeTracer.cpp TopBottomEdgeTracer.h
	CylindricalSurfaceDewarper.cpp CylindricalSurfaceDewarper.h
	RasterDewarper.cpp RasterDewarper.h
	DewarpingImageTransform.cpp DewarpingImageTransform.h
)
SOURCE_GROUP("Sources" FILES ${sources})

ADD_DEFINITIONS(-DBUILDING_DEWARPING)
ADD_LIBRARY(dewarping SHARED ${sources})
TARGET_LINK_LIBRARIES(dewarping imageproc math foundation Qt5::Widgets Qt5::Xml)

# We want the DLL located where executables can find it.
SET_TARGET_PROPERTIES(
	dewarping PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

INSTALL(TARGETS dewarping DESTINATION lib/scantailor)

TRANSLATION_SOURCES(scantailor ${sources})
