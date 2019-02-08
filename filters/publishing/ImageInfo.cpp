#include "ImageInfo.h"
#include <QCryptographicHash>

namespace publishing
{

// truncated copy of qrgb888 class from qt's gui/painting/qdrawhelper_p.h
class qrgb888
{
public:
    inline qrgb888() {}
    inline qrgb888(quint32 v) {
        data[0] = qRed(v);
        data[1] = qGreen(v);
        data[2] = qBlue(v);
    }

    inline operator quint32() const {
        return qRgb(data[0], data[1], data[2]);
    }
private:
    uchar data[3];
} Q_PACKED;

// modified inline quint16 qt_colorConvert(quint32 color, quint16 dummy)
// from qt's gui/painting/qdrawhelper_p.h
inline quint16 qt_colorConvert(quint32 color)
{
    const int r = qRed(color) << 8;
    const int g = qGreen(color) << 3;
    const int b = qBlue(color) >> 3;

    return (r & 0xf800) | (g & 0x07e0)| (b & 0x001f);
}

// inspired by qIsGrayscale()
inline void checkColor(QRgb rgb, bool& may_be_bw, bool& may_be_gs)
{
    const int r = qRed(rgb);
    const int g = qGreen(rgb);
    const int b = qBlue(rgb);
    const bool same = r == g && r == b;
    if (!same) {
        may_be_bw = false;
        may_be_gs = false;
    } else if (may_be_bw) {
        may_be_bw = r == 0 || r == 0xff;
    }
}


inline ImageInfo::ColorMode form_result(bool may_be_bw, bool may_be_gs)
{
    return (!may_be_gs) ? ImageInfo::Color : ( (!may_be_bw) ?  ImageInfo::Grayscale :  ImageInfo::BlackAndWhite );
}

// modified version of bool QImage::allGray() and QImage::IsGrayscale()
ImageInfo::ColorMode getColors(const QImage& img)
{
    bool may_be_bw = true; // still can be black and white image
    bool may_be_gs = true; // still can be grayscale image

    if (img.depth() == 32) {
        int p = img.width()*img.height();
        const QRgb* b = (const QRgb*)img.bits();
        while (p--) {
            checkColor(*b++, may_be_bw, may_be_gs);
            if (!may_be_gs) {
                break;
            }
        }
        return form_result(may_be_bw, may_be_gs);

    } else if (img.depth() == 16) {
        int p = img.width()*img.height();
        const ushort* b = (const ushort *)img.bits();
        while (p--) {
            checkColor(qt_colorConvert((quint32)*b++), may_be_bw, may_be_gs);
            if (!may_be_gs) {
                break;
            }
        }
        return form_result(may_be_bw, may_be_gs);

    } else if (img.depth() == 16) {
        if (img.format() == QImage::Format_RGB888) {
            int p = img.width()*img.height();
            const uchar* b = (const uchar *)img.bits();
            while (p--) {
                checkColor((uchar)*b++, may_be_bw, may_be_gs);
                if (!may_be_gs) {
                    break;
                }
            }
            return form_result(may_be_bw, may_be_gs);

        } else {
            if (img.colorTable().isEmpty()) {
                int p = img.width()*img.height();
                const qrgb888* b = (const qrgb888 *)img.bits();
                while (p--) {
                    checkColor(qrgb888(*b++), may_be_bw, may_be_gs);
                    if (!may_be_gs) {
                        break;
                    }
                }
            } else {
                for (int i = 0; i < img.colorCount(); i++) {
                    checkColor(img.colorTable()[i], may_be_bw, may_be_gs);
                    if (!may_be_gs) {
                        break;
                    }
                }
            }
            return form_result(may_be_bw, may_be_gs);
        }
    } else { // img.depth() == 8
        for (int i = 0; i < img.colorCount(); i++) {
            checkColor(img.colorTable()[i], may_be_bw, may_be_gs);
            if (!may_be_gs) {
                break;
            }
        }
        return form_result(may_be_bw, may_be_gs);
    }

    //    return ImageInfo::Unknown;
}

ImageInfo::ImageInfo(const QString& filename, const QImage& image)
{
    fileName = filename;
    //    fileSize = QFileInfo(filename).size();

    QCryptographicHash hash(QCryptographicHash::Md4);
    hash.addData((const char*)image.constBits(), image.byteCount());
    imageHash = hash.result();
    imageColorMode = getColors(image);
}
}
