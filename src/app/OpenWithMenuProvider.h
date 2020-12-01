#ifndef OPENWITHMENUPROVIDER_H
#define OPENWITHMENUPROVIDER_H

#include <QMenu>

class OpenWithMenuProvider
{
public:
    static QMenu* getOpenWithMenu(const QString& mime_type);
};

#endif // OPENWITHMENUPROVIDER_H
