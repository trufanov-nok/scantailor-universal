#ifndef IMAGEPROC_CONFIG_H_
#define IMAGEPROC_CONFIG_H_

#include <QtGlobal>

#if defined(BUILDING_IMAGEPROC)
#	define IMAGEPROC_EXPORT Q_DECL_EXPORT
#else
#	define IMAGEPROC_EXPORT Q_DECL_IMPORT
#endif

#endif
