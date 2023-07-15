#include "ImageMetadataCopier.h"

#include <exiv2/exiv2.hpp>
#include <iostream>

#if EXIV2_TEST_VERSION(0,28,0)
typedef Exiv2::Image::UniquePtr ExivUniquePtr;
#else
typedef Exiv2::Image::AutoPtr ExivUniquePtr;
#endif

bool
ImageMetadataCopier::iccProfileDefined(const QString& filename)
{
#ifdef _WIN32
    const ExivUniquePtr src_image = Exiv2::ImageFactory::open(filename.toStdWString());
#else
    const ExivUniquePtr src_image = Exiv2::ImageFactory::open(filename.toStdString());
#endif
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
#ifdef _WIN32
		const ExivUniquePtr src_image = Exiv2::ImageFactory::open(src_img.toStdWString());
#else
		const ExivUniquePtr src_image = Exiv2::ImageFactory::open(src_img.toStdString());
#endif

        if (src_image.get()) {
            src_image->readMetadata();

            if (src_image->iccProfileDefined()) {
                src_image->iccProfile();
#ifdef _WIN32
				const ExivUniquePtr dst_image = Exiv2::ImageFactory::open(dst_img.toStdWString());
#else
				const ExivUniquePtr dst_image = Exiv2::ImageFactory::open(dst_img.toStdString());
#endif
                dst_image->readMetadata();
#if EXIV2_TEST_VERSION(0,28,0)
                Exiv2::DataBuf profile = src_image->iccProfile();
                dst_image->setIccProfile(std::move(profile));
#else
                dst_image->setIccProfile(*src_image->iccProfile());
#endif
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
