/*
    2023 Mikhail Morozov

    This work is marked with CC0 1.0 Deviant
    See http://creativecommons.org/publicdomain/zero/1.0?ref=chooser-v1
*/
// Wrapper for deprecated functions

#ifndef LEGACYSUPPORT_H
#define LEGACYSUPPORT_H

#include <QPointF>
#include <QLineF>
#include <QVector>
#include <QString>
#include <QDateTime>


static inline int QLineIntersect(const QLineF &line1, const QLineF &line2, QPointF *intersectionPoint) {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    return line1.intersect(line2, intersectionPoint);
#else
    return line1.intersects(line2, intersectionPoint);
#endif
}

// template <class T>
// static inline QVector<T> QVectorFromStdVector(const std::vector<T> &vector)
// {
// #if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
// 	return QVector<T>::fromStdVector(vector);
// #else
// 	return QVector<T>(vector.begin(), vector.end());
// #endif
// }



#endif
