#include "ImageMetadataCopier.h"

#include <exiv2/exiv2.hpp>
#include <iostream>

bool
ImageMetadataCopier::iccProfileDefined(const QString& filename)
{
    const Exiv2::Image::AutoPtr src_image = Exiv2::ImageFactory::open(filename.toStdString());
    if (src_image.get()) {
        src_image->readMetadata();
        return src_image->iccProfileDefined();
    }
    return false;
}

bool
ImageMetadataCopier::copyMetadata(const QString& src_img,
                                  const QString& dst_img)
{
    try
    {
        const Exiv2::Image::AutoPtr src_image = Exiv2::ImageFactory::open(src_img.toStdString());
        if (src_image.get()) {
            src_image->readMetadata();

            if (src_image->iccProfileDefined()) {
                src_image->iccProfile();
                Exiv2::Image::AutoPtr dst_image = Exiv2::ImageFactory::open(dst_img.toStdString());
                dst_image->readMetadata();
                dst_image->setIccProfile(*src_image->iccProfile());
                dst_image->writeMetadata();
                return true;
            }
        }
    }
    catch (Exiv2::Error& e) {
        std::cout << "Caught Exiv2 exception '" << e.what() << "'\n";
        return false;
    }
    return false;
}
