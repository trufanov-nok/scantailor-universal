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

#ifndef PUBLISHING_PARAMS_H_
#define PUBLISHING_PARAMS_H_

#include "Dpi.h"
#include "RegenParams.h"
#include <QJSValue>

class QDomDocument;
class QDomElement;

namespace publishing
{

class Params: public RegenParams
{
public:
	Params();
	Params(QDomElement const& el);
	
	Dpi const& outputDpi() const { return m_dpi; }
    void setOutputDpi(Dpi const& dpi) { m_dpi = dpi; }
    QJSValue const& encoderState() const { return m_encoderState; }
    void setEncoderState(QJSValue const& val) { m_encoderState = val; }
    QJSValue const& converterState() const { return m_converterState; }
    void setConverterState(QJSValue const& val) { m_converterState = val; }
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
    Dpi m_dpi;
    QJSValue m_encoderState;
    QJSValue m_converterState;
};

} // namespace publishing

#endif
