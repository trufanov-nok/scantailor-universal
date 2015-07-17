PROJECT("Math library")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

SET(
	GENERIC_SOURCES
	math_config.h
	HomographicTransform.h
	SidesOfLine.cpp SidesOfLine.h
	ToLineProjector.cpp ToLineProjector.h
	ArcLengthMapper.cpp ArcLengthMapper.h
	LineBoundedByRect.cpp LineBoundedByRect.h
	PolylineIntersector.cpp PolylineIntersector.h
	LinearFunction.cpp LinearFunction.h
	QuadraticFunction.cpp QuadraticFunction.h
	XSpline.cpp XSpline.h
)
SOURCE_GROUP("Sources" FILES ${GENERIC_SOURCES})

SET(
	SPFIT_SOURCES
	spfit/FittableSpline.h
	spfit/FrenetFrame.cpp spfit/FrenetFrame.h
	spfit/ConstraintSet.cpp spfit/ConstraintSet.h
	spfit/SqDistApproximant.cpp spfit/SqDistApproximant.h
	spfit/ModelShape.h
	spfit/PolylineModelShape.cpp spfit/PolylineModelShape.h
	spfit/LinearForceBalancer.cpp spfit/LinearForceBalancer.h
	spfit/OptimizationResult.cpp spfit/OptimizationResult.h
	spfit/Optimizer.cpp spfit/Optimizer.h
	spfit/SplineFitter.cpp spfit/SplineFitter.h
)
SOURCE_GROUP("Sources\\Spline Fitting Framework" FILES ${SPFIT_SOURCES})

SET(
	ADIFF_SOURCES
	adiff/SparseMap.cpp adiff/SparseMap.h
	adiff/Function.cpp adiff/Function.h
)
SOURCE_GROUP("Sources\\Differentiation Framework" FILES ${ADIFF_SOURCES})

ADD_DEFINITIONS(-DBUILDING_MATH)
ADD_LIBRARY(math SHARED ${GENERIC_SOURCES} ${SPFIT_SOURCES} ${ADIFF_SOURCES})
TARGET_LINK_LIBRARIES(math foundation Qt5::Core)

# We want the DLL located where executables can find it.
SET_TARGET_PROPERTIES(
	math PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

INSTALL(TARGETS math DESTINATION lib/scantailor)

ADD_SUBDIRECTORY(spfit/tests)
ADD_SUBDIRECTORY(adiff/tests)
