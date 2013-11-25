/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef PAGE_LAYOUT_MATCH_SIZE_MODE_H_
#define PAGE_LAYOUT_MATCH_SIZE_MODE_H_

class QDomDocument;
class QDomElement;
class QString;

namespace page_layout
{

class MatchSizeMode
{
public:
	enum Mode {
		/** The size of this page doesn't affect and is not affected by other pages. */
		DISABLED,

		/**
		 * To match the size of the widest / tallest page, soft margins grow as necessary.
		 * This page will also compete for the widest / tallest page title itself.
		 */
		GROW_MARGINS,

		/**
		 * To match the size of the widest / tallest page, this page's content as well as
		 * hard margins will be scaled. Because the scaling coefficients in horizontal and
		 * vertical directions are locked to each other, soft margins may need to grow
		 * as well. This page will also compete for the widest / tallest page title itself.
		 */
		SCALE
	};
	
	MatchSizeMode(Mode mode = DISABLED) : m_mode(mode) {}
	
	MatchSizeMode(QDomElement const& el);

	Mode get() const { return m_mode; }
	
	void set(Mode mode) { m_mode = mode; }
	
	bool operator==(MatchSizeMode const& other) const {
		return m_mode == other.m_mode;
	}
	
	bool operator!=(MatchSizeMode const& other) const {
		return !(*this == other);
	}
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	Mode m_mode;
};

} // namespace page_layout

#endif
