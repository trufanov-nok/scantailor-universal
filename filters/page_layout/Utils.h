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

#ifndef PAGE_LAYOUT_UTILS_H_
#define PAGE_LAYOUT_UTILS_H_

class QColor;
class QPointF;
class QSizeF;
class QRectF;
class QMarginsF;

namespace page_layout
{

class MatchSizeMode;
class Alignment;

class Utils
{
public:
	/**
	 * \brief Calculates margins to extend hard_size_px to aggregate_hard_size_px.
	 *
	 * \param hard_size_px Source size in pixels after applying AbstractImageTransform.
	 * \param aggregate_hard_size_mm Target size in pixels after applying AbstractImageTransform
	 *        and optionally applying additional scaling according to MatchSizeMode::SCALE.
	 * \param match_size_mode Determines whether and how to match the size of other pages.
	 * \param alignment Determines how to grow margins to match the size of other pages.
	 * \return Non-negative margins that extend \p hard_size_px to
	 *         \p aggregate_hard_size_px. If \p match_size_mode is MatchSizeMode::DISABLED,
	 *         zero margins are returned.
	 */
	static QMarginsF calcSoftMarginsPx(
		QSizeF const& hard_size_px,
		QSizeF const& aggregate_hard_size_px,
		MatchSizeMode const& match_size_mode,
		Alignment const& alignment);

	static QColor borderColorForMatchSizeMode(MatchSizeMode const& mode);

	static QColor backgroundColorForMatchSizeMode(MatchSizeMode const& mode);
};

} // namespace page_layout

#endif
