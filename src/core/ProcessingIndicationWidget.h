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

#ifndef PROCESSING_INDICATION_WIDGET_H_
#define PROCESSING_INDICATION_WIDGET_H_

#include "BubbleAnimation.h"
#include <QWidget>
#include <QColor>
#include <QTimer>

class QRect;

class QCommonProgressIndicator: public QObject
{
    Q_OBJECT
public:
    QCommonProgressIndicator(QObject* parent = nullptr);
    bool start();
    bool stop();
    void resetAnimation();
    void processingRestartedEffect();
signals:
    void timeout();
private slots:
    void triggered();
private:
    int m_ref;
    QTimer m_timer;
    double const distinction_increase;
    double const distinction_decrease;
    double m_distinctionDelta;
public:
    BubbleAnimation m_animation;
    double m_distinction;
};

/**
 * \brief This widget is displayed in the central area of the main window
 *        when an image is being processed.
 */
class ProcessingIndicationWidget : public QWidget
{
public:
    ProcessingIndicationWidget(QWidget* parent = nullptr, const QRect& indicator_size = QRect(0,0,20,20));

    /**
     * \brief Resets animation to the state it had just after
     *        constructing this object.
     */
    void resetAnimation();

    /**
     * \brief Launch the "processing restarted" effect.
     */
    void processingRestartedEffect();

    virtual QSize sizeHint() const override;

    ~ProcessingIndicationWidget();
protected:
    virtual void paintEvent(QPaintEvent* event) override;
private:
    QRect animationRect() const;
    QColor m_headColor;
    QColor m_tailColor;
    QRect m_indicator_size;
    int m_timerId;
};

#endif
