#ifndef IMAGEINFO_H
#define IMAGEINFO_H

#include <QImage>

namespace publishing
{

struct ImageInfo {

    enum ColorMode {
        Unknown = 0,
        BlackAndWhite = 1,
        Grayscale = 3,
        Color = 7
    };

    ImageInfo(): imageColorMode(Unknown) {}
    ImageInfo(const QString& filename, const QImage& image);

    static QString ColorModeToStr(const ColorMode clr) {
        switch (clr) {
        case BlackAndWhite: return "bw";
        case Grayscale: return "grayscale";
        case Color: return "color";
        default: return "";
        }
    }

    QString fileName;
    QByteArray imageHash;
    ColorMode imageColorMode;
};

}

#endif // IMAGEINFO_H
