/*
    2023 Mikhail Morozov

    This work is marked with CC0 1.0 Deviant
    See http://creativecommons.org/publicdomain/zero/1.0?ref=chooser-v1
*/
// Wrapper for deprecated functions

#ifndef MULTIPLETARGETSUPPORT_H
#define MULTIPLETARGETSUPPORT_H

#include <QPointF>
#include <QLineF>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <vector>
#include <QWheelEvent>

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
static const QString::SplitBehavior QStringSkipEmptyParts = QString::SkipEmptyParts;
#else
static const Qt::SplitBehavior QStringSkipEmptyParts = Qt::SkipEmptyParts;
#endif


static inline QPointF QWheelEventPosF(const QWheelEvent *event)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    return event->posF();
#else
    return event->position();
#endif
}

static inline int QWheelEventDelta(QWheelEvent* event) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return event->delta();
#else
    QPoint angle_delta = event->angleDelta();

    if(angle_delta.x())
        return angle_delta.x();
    else
        return angle_delta.y();
#endif
}

template <class T>
static inline QVector<T> QVectorFromStdVector(const std::vector<T> &vector)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	return QVector<T>::fromStdVector(vector);
#else
	return QVector<T>(vector.begin(), vector.end());
#endif
}

template <class T>
static inline std::vector<T> StdVectorFromQVector(const QVector<T> &vector)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    return vector.toStdVector();
#else
    return std::vector<T>(vector.begin(), vector.end());
#endif
}

static inline qint64 QDateTimeToSecsSinceEpoch(const QDateTime &datetime)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
    uint t = datetime.toTime_t();

    if(t == (uint)-1) return -1;
    else return t;
#else
    return datetime.toSecsSinceEpoch();
#endif
}

#endif
