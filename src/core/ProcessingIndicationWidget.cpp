/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ProcessingIndicationWidget.h"
#include "imageproc/ColorInterpolation.h"
#include <QTimerEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QRectF>
#include <math.h>


using namespace imageproc;

QCommonProgressIndicator::QCommonProgressIndicator(QObject* parent): QObject(parent),
    m_ref(0),
    distinction_increase(1.0 / 5.0),
    distinction_decrease(-1.0 / 3.0),
    m_distinctionDelta(distinction_increase),
    m_animation(10),
    m_distinction(1.0)
{
    connect(&m_timer, &QTimer::timeout, this, &QCommonProgressIndicator::triggered, Qt::DirectConnection);
}

void
QCommonProgressIndicator::triggered()
{
    m_distinction += m_distinctionDelta;
    if (m_distinction > 1.0) {
        m_distinction = 1.0;
    } else if (m_distinction <= 0.0) {
        m_distinction = 0.0;
        m_distinctionDelta = distinction_increase;
    }
    m_animation.nextFrame();
    emit timeout();
}

bool
QCommonProgressIndicator::start() {
    m_ref++;
    if (!m_timer.isActive()) {
        m_timer.start(180);
        return true;
    }
    return false;
}

bool
QCommonProgressIndicator::stop() {
    m_ref--;
    if (m_ref <= 0 && m_timer.isActive()) {
        m_timer.stop();
        m_ref = 0;
        return true;
    }
    return false;
}

void
QCommonProgressIndicator::resetAnimation() {
    m_distinction = 1.0;
    m_distinctionDelta = distinction_increase;
}

void
QCommonProgressIndicator::processingRestartedEffect() {
    m_distinction = 1.0;
    m_distinctionDelta = distinction_decrease;
}

static QCommonProgressIndicator _shared_timer;

ProcessingIndicationWidget::ProcessingIndicationWidget(QWidget* parent, const QRect& indicator_size)
    :   QWidget(parent), m_indicator_size(indicator_size)
{
    m_headColor = palette().color(QPalette::Window).darker(200);
    m_tailColor = palette().color(QPalette::Window).darker(130);
    connect(&_shared_timer, &QCommonProgressIndicator::timeout,
            this, [=](){update(animationRect());}, Qt::DirectConnection);
    _shared_timer.start();
}

void
ProcessingIndicationWidget::resetAnimation()
{
    _shared_timer.resetAnimation();
}

void
ProcessingIndicationWidget::processingRestartedEffect()
{
    _shared_timer.processingRestartedEffect();
}

void
ProcessingIndicationWidget::paintEvent(QPaintEvent* event)
{
    QRect animation_rect(animationRect());
    if (!event->rect().contains(animation_rect)) {
        update(animation_rect);
        return;
    }

    QColor head_color(colorInterpolation(m_tailColor, m_headColor,
                                         _shared_timer.m_distinction));

    QPainter painter(this);
    _shared_timer.m_animation.paintFrame(head_color, m_tailColor, &painter, animation_rect);
}

QRect
ProcessingIndicationWidget::animationRect() const
{
    QRect r(m_indicator_size);
    r.moveCenter(rect().center());
    r &= rect();
    return r;
}

QSize
ProcessingIndicationWidget::sizeHint() const
{
    return m_indicator_size.size();
}

ProcessingIndicationWidget::~ProcessingIndicationWidget()
{
    _shared_timer.stop();
}
