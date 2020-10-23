#ifndef DISPLAYPREFERENCES_H
#define DISPLAYPREFERENCES_H

#include <QColor>
#include <QDomDocument>

namespace publish {

class DisplayPreferences
{
public:
    DisplayPreferences();
    DisplayPreferences(QDomElement const& el);

    QDomElement toXml(QDomDocument& doc, QString const& name) const;
    bool operator !=(const DisplayPreferences& other) const;


    uint thumbSize() const { return m_thumb_size; }
    void setThumbSize(uint val) { m_thumb_size = val; }
    const QString& background() const { return m_background; }
    void setBackground(const QString& val) { m_background = val; }
    const QString& zoom() const { return m_zoom; }
    void setZoom(const QString& val) { m_zoom = val; }
    const QString& mode() const { return m_mode; }
    void setMode(const QString& val) { m_mode = val; }
    const QString& alignY() const { return m_alignY; }
    void setAlignY(const QString& val) { m_alignY = val; }
    const QString& alignX() const { return m_alignX; }
    void setAlignX(const QString& val) { m_alignX = val; }

    bool hasAnnotations() const {
        return  !(m_background.isEmpty() &&
                m_zoom.isEmpty() && m_mode.isEmpty() &&
                m_alignX.isEmpty() && m_alignY.isEmpty());
    }

    bool isEmpty() const { return m_thumb_size == 0 && !hasAnnotations(); }

private:
    uint m_thumb_size;
    QString m_background;
    QString m_zoom;
    QString m_mode;
    QString m_alignY;
    QString m_alignX;
};

}

#endif // DISPLAYPREFERENCES_H
