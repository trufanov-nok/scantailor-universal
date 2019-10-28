#ifndef EXPORTMODES_H
#define EXPORTMODES_H

#include <QFlags>

enum ExportMode {
    None = 0,
    Foreground = 1,
    Background = 2,
    Mask = 4,
    Zones = 8,
    WholeImage = 16,
    AutoMask = 32,
    ImageWithoutOutputStage = 64
};

Q_DECLARE_FLAGS(ExportModes, ExportMode)

#endif // EXPORTMODES_H
