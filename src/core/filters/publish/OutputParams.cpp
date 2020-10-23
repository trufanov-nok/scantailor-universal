/*
    Scan Tailor Universal - Interactive post-processing tool for scanned
    pages. A fork of Scan Tailor by Joseph Artsimovich.
    Copyright (C) 2020 Alexander Trufanov <trufanovan@gmail.com>

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


#include "OutputParams.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include "DjbzDispatcher.h"
#include "SourceImagesInfo.h"

#include <QDomDocument>
#include <QDomElement>

namespace publish
{

OutputParams::OutputParams()
{
}

OutputParams::OutputParams(const Params& params, const DjbzParams& djbzParams):
    m_params(params),
    m_djbzParams(djbzParams)
{
    m_params.resetOutputParams();
}

OutputParams::OutputParams(QDomElement const& el):
    m_params(el.namedItem("params").toElement()),
    m_djbzParams(el.namedItem("djbz_dict_params").toElement())
{
}

OutputParams::~OutputParams()
{
}

bool
OutputParams::matches(OutputParams const& other) const
{
    return !(m_params != other.m_params ||
            m_djbzParams != other.m_djbzParams);
}

QDomElement
OutputParams::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));
    el.appendChild(m_params.toXml(doc, "params"));
    el.appendChild(m_djbzParams.toXml(doc, "djbz_dict_params"));

    return el;
}

} // namespace publish
