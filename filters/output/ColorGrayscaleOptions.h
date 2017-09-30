/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef OUTPUT_COLOR_GRAYSCALE_OPTIONS_H_
#define OUTPUT_COLOR_GRAYSCALE_OPTIONS_H_

class QString;
class QDomDocument;
class QDomElement;

namespace output
{

class ColorGrayscaleOptions
{
public:
    ColorGrayscaleOptions(bool whiteMargins = false, bool normalizeIllumination = false, bool autoLayer = true, bool colorLayer = false)
    : m_whiteMargins(whiteMargins), m_normalizeIllumination(normalizeIllumination),
      m_autoLayerEnabled(autoLayer), m_colorLayerEnabled(colorLayer) {}
	
    ColorGrayscaleOptions(QDomElement const& el, bool mixed_mode);
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
	
	bool whiteMargins() const { return m_whiteMargins; }
	
	void setWhiteMargins(bool val) { m_whiteMargins = val; }
	
	bool normalizeIllumination() const { return m_normalizeIllumination; }
	
	void setNormalizeIllumination(bool val) { m_normalizeIllumination = val; }

    bool colorLayerEnabled() const { return m_colorLayerEnabled; }

    void setColorLayerEnabled(bool enabled) { m_colorLayerEnabled = enabled; }

    bool autoLayerEnabled() const { return m_autoLayerEnabled; }

    void setAutoLayerEnabled(bool enabled) { m_autoLayerEnabled = enabled; }
	
	bool operator==(ColorGrayscaleOptions const& other) const;
	
	bool operator!=(ColorGrayscaleOptions const& other) const;
private:
	bool m_whiteMargins;
	bool m_normalizeIllumination;
    bool m_autoLayerEnabled;
    bool m_colorLayerEnabled;
};

} // namespace output

#endif
