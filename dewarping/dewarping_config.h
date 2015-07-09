#ifndef DEWARPING_CONFIG_H_
#define DEWARPING_CONFIG_H_

#include <QtGlobal>

#if defined(BUILDING_DEWARPING)
#	define DEWARPING_EXPORT Q_DECL_EXPORT
#else
#	define DEWARPING_EXPORT Q_DECL_IMPORT
#endif

#endif
