PROJECT(Acceleration)

INCLUDE_DIRECTORIES(.)
ADD_SUBDIRECTORY(opencl)

SET(
	sources
	acceleration_config.h
	AccelerationPlugin.h
	AcceleratableOperations.h
	NonAcceleratedOperations.cpp NonAcceleratedOperations.h
	DefaultAccelerationProvider.cpp DefaultAccelerationProvider.h
)
SOURCE_GROUP("Sources" FILES ${sources})
TRANSLATION_SOURCES(scantailor ${sources})

ADD_DEFINITIONS(-DBUILDING_ACCELERATION)
ADD_LIBRARY(acceleration SHARED ${sources})
TARGET_LINK_LIBRARIES(acceleration dewarping imageproc Qt5::Core Qt5::Gui)

# We want the DLL located where executables can find it.
SET_TARGET_PROPERTIES(
	acceleration PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

INSTALL(TARGETS acceleration DESTINATION lib/scantailor)
