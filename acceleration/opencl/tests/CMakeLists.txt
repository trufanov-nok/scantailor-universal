INCLUDE_DIRECTORIES(BEFORE ..)

SET(
	sources
	"${CMAKE_SOURCE_DIR}/tests/main.cpp"
	TestCopy.cpp
	TestTranspose.cpp
	TestGaussBlur.cpp
	Utils.cpp Utils.h
)
SOURCE_GROUP("Sources" FILES ${sources})

SET(
	libs
	opencl imageproc foundation Qt5::Core ${OpenCL_LIBRARIES}
	${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
	${Boost_PRG_EXECUTION_MONITOR_LIBRARY} ${EXTRA_LIBS}
)

ADD_EXECUTABLE(opencl_tests ${sources} ${resource_sources})
TARGET_LINK_LIBRARIES(opencl_tests ${libs})

# We want the executable located where we copy all the DLLs.
SET_TARGET_PROPERTIES(
	opencl_tests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

ADD_TEST(NAME opencl_tests COMMAND opencl_tests --log_level=message)
