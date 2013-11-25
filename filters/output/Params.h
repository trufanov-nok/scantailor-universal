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

#ifndef OUTPUT_PARAMS_H_
#define OUTPUT_PARAMS_H_

#include "ColorParams.h"
#include "DespeckleLevel.h"

class QDomDocument;
class QDomElement;

namespace output
{

class Params
{
public:
	Params();
	
	Params(QDomElement const& el);

	ColorParams const& colorParams() const { return m_colorParams; }

	void setColorParams(ColorParams const& params) { m_colorParams = params; }

	DespeckleLevel despeckleLevel() const { return m_despeckleLevel; }

	void setDespeckleLevel(DespeckleLevel level) { m_despeckleLevel = level; }
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	static ColorParams::ColorMode parseColorMode(QString const& str);
	
	static QString formatColorMode(ColorParams::ColorMode mode);
	
	ColorParams m_colorParams;
	DespeckleLevel m_despeckleLevel;
};

} // namespace output

#endif
