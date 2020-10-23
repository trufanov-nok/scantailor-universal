#ifndef OPENWITHMENUPROVIDER_H
#define OPENWITHMENUPROVIDER_H

#include <QMenu>

class OpenWithMenuProvider
{
public:
    static QMenu* getOpenWithMenu(const QString &filename);
};

#endif // OPENWITHMENUPROVIDER_H
