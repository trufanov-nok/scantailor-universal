PROJECT("Foundation library")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

SET(
	sources
	NonCopyable.h IntrusivePtr.h RefCountable.h
	CopyableByMemcpy.h
	ObjectSwapper.h
	ObjectSwapperFactory.cpp ObjectSwapperFactory.h
	ObjectSwapperImpl.h
	ObjectSwapperImplGrid.h
	ObjectSwapperImplQImage.cpp ObjectSwapperImplQImage.h
	AlignedArray.h
	CachingFactory.h
	FastQueue.h
	SafeDeletingQObjectPtr.h
	ScopedIncDec.h ScopedDecInc.h
	Span.h VirtualFunction.h FlagOps.h
	AutoRemovingFile.cpp AutoRemovingFile.h
	Proximity.cpp Proximity.h
	Property.h
	PropertyFactory.cpp PropertyFactory.h
	PropertySet.cpp PropertySet.h
	PerformanceTimer.cpp PerformanceTimer.h
	QtSignalForwarder.cpp QtSignalForwarder.h
	GridLineTraverser.cpp GridLineTraverser.h
	StaticPool.h
	DynamicPool.h
	MaxAccumulator.h
	MinMaxAccumulator.h
	NumericTraits.h
	VecNT.h
	VecT.h
	MatMNT.h
	MatT.h
	PriorityQueue.h
	Grid.h
	GridAccessor.h
	ValueConv.h
	IndexSequence.h
)
SOURCE_GROUP("Sources" FILES ${sources})

ADD_LIBRARY(foundation STATIC ${sources})
QT5_USE_MODULES(foundation Gui Core Xml)
