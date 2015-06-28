/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "Utils.h"
#include "MatchSizeMode.h"
#include "Alignment.h"
#include <QColor>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QMarginsF>

namespace page_layout
{

QMarginsF
Utils::calcSoftMarginsPx(
	QSizeF const& hard_size_px, QSizeF const& aggregate_hard_size_px,
	MatchSizeMode const& match_size_mode, Alignment const& alignment)
{
	if (match_size_mode.get() == MatchSizeMode::DISABLED) {
		// We are not aligning this page with others.
		return QMarginsF();
	}

	qreal top = 0.0;
	qreal bottom = 0.0;
	qreal left = 0.0;
	qreal right = 0.0;

	qreal const delta_width = aggregate_hard_size_px.width() - hard_size_px.width();
	if (delta_width > 0.0) {
		switch (alignment.horizontal()) {
			case Alignment::LEFT:
				right = delta_width;
				break;
			case Alignment::HCENTER:
				left = right = 0.5 * delta_width;
				break;
			case Alignment::RIGHT:
				left = delta_width;
				break;
		}
	}

	qreal const delta_height = aggregate_hard_size_px.height() - hard_size_px.height();
	if (delta_height > 0.0) {
		switch (alignment.vertical()) {
			case Alignment::TOP:
				bottom = delta_height;
				break;
			case Alignment::VCENTER:
				top = bottom = 0.5 * delta_height;
				break;
			case Alignment::BOTTOM:
				top = delta_height;
				break;
		}
	}
	
	return QMarginsF(left, top, right, bottom);
}

QColor
Utils::borderColorForMatchSizeMode(MatchSizeMode const& mode)
{
	QColor color;

	switch (mode.get()) {
		case MatchSizeMode::DISABLED:
			color = QColor(0x00, 0x52, 0xff);
			break;
		case MatchSizeMode::GROW_MARGINS:
			color = QColor(0xbe, 0x5b, 0xec);
			break;
		case MatchSizeMode::SCALE:
			color = QColor(0xff, 0x80, 0x00);
			break;
	}

	return color;
}

QColor
Utils::backgroundColorForMatchSizeMode(MatchSizeMode const& mode)
{
	QColor color;

	switch (mode.get()) {
		case MatchSizeMode::DISABLED:
			color = QColor(0x58, 0x7f, 0xf4, 70);
			break;
		case MatchSizeMode::GROW_MARGINS:
			color = QColor(0xbb, 0x00, 0xff, 40);
			break;
		case MatchSizeMode::SCALE:
			color = QColor(0xff, 0xa9, 0x00, 70);
			break;
	}

	return color;
}

} // namespace page_layout
