INCLUDE_DIRECTORIES(BEFORE ..)

SET(
        sources
        main.cpp
        TestBinaryImage.cpp TestReduceThreshold.cpp
        TestSlicedHistogram.cpp
        TestConnCompEraser.cpp TestConnCompEraserExt.cpp
        TestGrayscale.cpp
        TestRasterOp.cpp TestShear.cpp
        TestOrthogonalRotation.cpp
        TestSkewFinder.cpp
        TestScale.cpp
        TestTransform.cpp
        TestMorphology.cpp
        TestBinarize.cpp
        TestPolygonRasterizer.cpp
        TestSeedFill.cpp
        TestSEDM.cpp
        TestRastLineFinder.cpp
        Utils.cpp Utils.h
)
SOURCE_GROUP("Sources" FILES ${sources})

SET(
        libs
        imageproc math foundation ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${Boost_PRG_EXECUTION_MONITOR_LIBRARY} ${EXTRA_LIBS}
)

ADD_EXECUTABLE(imageproc_tests ${sources})
TARGET_LINK_LIBRARIES(imageproc_tests Qt5::Widgets Qt5::Xml)
TARGET_LINK_LIBRARIES(imageproc_tests ${libs})

# We want the executable located where we copy all the DLLs.
SET_TARGET_PROPERTIES(
        imageproc_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

ADD_TEST(NAME imageproc_tests COMMAND imageproc_tests --log_level=message)
