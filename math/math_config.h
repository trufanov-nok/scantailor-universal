#ifndef MATH_CONFIG_H_
#define MATH_CONFIG_H_

#include <QtGlobal>

#if defined(BUILDING_MATH)
#	define MATH_EXPORT Q_DECL_EXPORT
#else
#	define MATH_EXPORT Q_DECL_IMPORT
#endif

#endif
