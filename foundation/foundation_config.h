#ifndef FOUNDATION_CONFIG_H_
#define FOUNDATION_CONFIG_H_

#include <QtGlobal>

#if defined(BUILDING_FOUNDATION)
#	define FOUNDATION_EXPORT Q_DECL_EXPORT
#else
#	define FOUNDATION_EXPORT Q_DECL_IMPORT
#endif

#endif
