#include "DisplayPreferences.h"

namespace publish {

DisplayPreferences::DisplayPreferences():
    m_thumb_size(0)
{
}

DisplayPreferences::DisplayPreferences(QDomElement const& el)
{
    m_thumb_size = el.attribute("thumb_size", "0").toUInt();
    m_background = el.attribute("backgroud", "");
    m_mode       = el.attribute("mode", "");
    m_zoom       = el.attribute("zoom", "");
    m_alignY     = el.attribute("aligny", "");
    m_alignX     = el.attribute("alignx", "");

}

QDomElement
DisplayPreferences::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));

    if (m_thumb_size > 0) {
        el.setAttribute("thumb_size", m_thumb_size);
    }

    if (!m_background.isEmpty()) {
        el.setAttribute("backgroud", m_background);
    }
    if (!m_mode.isEmpty()) {
        el.setAttribute("mode", m_mode);
    }
    if (!m_zoom.isEmpty()) {
        el.setAttribute("zoom", m_zoom);
    }
    if (!m_alignY.isEmpty()) {
        el.setAttribute("aligny", m_alignY);
    }
    if (!m_alignX.isEmpty()) {
        el.setAttribute("alignx", m_alignX);
    }
    return el;
}

bool
DisplayPreferences::operator !=(const DisplayPreferences& other) const
{
    return (m_thumb_size != other.m_thumb_size ||
            m_background != other.m_background ||
            m_zoom != other.m_zoom ||
            m_mode != other.m_mode ||
            m_alignY != other.m_mode ||
            m_alignX != other.m_mode);
}

}

