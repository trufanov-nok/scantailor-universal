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

#ifndef SELECT_CONTENT_PARAMS_H_
#define SELECT_CONTENT_PARAMS_H_

#include "Dependencies.h"
#include "AutoManualMode.h"
#include "ContentBox.h"
#include <QSizeF>

class QDomDocument;
class QDomElement;
class QString;

namespace select_content
{

class Params
{
	// Member-wise copying is OK.
public:
	/**
	 * @param content_box The content box.
	 * @param content_size_px Used for page sorting. Corresponds to
	 *        content_box.toTransformedRect(transform).size()
	 * @param deps Dependencies are used to decide when we need to
	 *        re-process the image due to changes made in previous stages.
	 * @param mode Tells whether the content box was detected automatically or set manually.
	 */
	Params(ContentBox const& content_box, QSizeF const& content_size_px,
		Dependencies const& deps, AutoManualMode mode);
	
	Params(Dependencies const& deps);
	
	Params(QDomElement const& filter_el);
	
	~Params();
	
	ContentBox const& contentBox() const { return m_contentBox; }

	void setContentBox(ContentBox const& content_box) { m_contentBox = content_box; }

	/** Used for page sorting by width / height. */
	QSizeF const& contentSizePx() const { return m_contentSizePx; }

	void setContentSizePx(QSizeF const& size) { m_contentSizePx = size; }
	
	Dependencies const& dependencies() const { return m_deps; }

	void setDependencies(Dependencies const& deps) { m_deps = deps; }
	
	AutoManualMode mode() const { return m_mode; }

	void setMode(AutoManualMode mode) { m_mode = mode; }
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	ContentBox m_contentBox;
	QSizeF m_contentSizePx;
	Dependencies m_deps;
	AutoManualMode m_mode;
};

} // namespace select_content

#endif
