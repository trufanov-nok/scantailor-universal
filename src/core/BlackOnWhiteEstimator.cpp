/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
    Backported from ScanTailor Advanced (src/core/BlackOnWhiteEstimator.cpp).

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

#include "BlackOnWhiteEstimator.h"
#include "ImageTransformation.h"
#include "DebugImages.h"
#include "Despeckle.h"
#include "TaskStatus.h"
#include "Dpi.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/Binarize.h"
#include "imageproc/GrayImage.h"
#include "imageproc/Grayscale.h"
#include "imageproc/Morphology.h"
#include "imageproc/PolygonRasterizer.h"
#include "imageproc/RasterOp.h"
#include "imageproc/Transform.h"
#include <QRect>
#include <QRectF>
#include <QSize>
#include <stdexcept>

using namespace imageproc;

bool
BlackOnWhiteEstimator::isBlackOnWhite(
    GrayImage const& gray_image, ImageTransformation const& xform,
    TaskStatus const& status, DebugImages* dbg)
{
    // The crop area uses the transformed coordinates, but the gray image uses the original coordinates.
    if (isBlackOnWhite(gray_image, xform.transformBack().map(xform.resultingPreCropArea()))) {
        return true;
    }
    // Dark borders around the page can give a wrong result, so check the result again on the content only.
    return isBlackOnWhiteRefining(gray_image, xform, status, dbg);
}

bool
BlackOnWhiteEstimator::isBlackOnWhiteRefining(
    GrayImage const& gray_image, ImageTransformation const& xform,
    TaskStatus const& status, DebugImages* dbg)
{
    BinaryImage bw150;
    {
        ImageTransformation xform_150dpi(xform);
        xform_150dpi.preScaleToDpi(Dpi(150, 150));

        QRect const target_rect(xform_150dpi.resultingRect().toRect());
        if (target_rect.isEmpty()) {
            return true;
        }

        GrayImage const gray150(
            transformToGray(
                gray_image, xform_150dpi.transform(), target_rect,
                OutsidePixels::assumeColor(Qt::white)
            )
        );
        bw150 = binarizeOtsu(gray150);

        Despeckle::despeckleInPlace(bw150, Dpi(150, 150), Despeckle::NORMAL, status);
        bw150.invert();
        Despeckle::despeckleInPlace(bw150, Dpi(150, 150), Despeckle::NORMAL, status);
        bw150.invert();
        if (dbg) {
            dbg->add(bw150, "bw150");
        }
    }

    status.throwIfCancelled();

    BinaryImage content_mask;
    {
        QSize const brick(13, 13);

        // Top-hat transforms keep only small objects such as text, so the mask covers the content areas.
        BinaryImage white_top_hat(bw150);
        rasterOp<RopSubtract<RopDst, RopSrc> >(white_top_hat, openBrick(bw150, brick));

        BinaryImage black_top_hat(bw150);
        rasterOp<RopSubtract<RopSrc, RopDst> >(black_top_hat, closeBrick(bw150, brick));

        content_mask = white_top_hat;
        rasterOp<RopOr<RopSrc, RopDst> >(content_mask, black_top_hat);

        content_mask = closeBrick(content_mask, QSize(200, 200));
        content_mask = dilateBrick(content_mask, QSize(30, 30));
        if (dbg) {
            dbg->add(content_mask, "content_mask");
        }
    }

    status.throwIfCancelled();

    rasterOp<RopAnd<RopSrc, RopDst> >(bw150, content_mask);
    return 2 * bw150.countBlackPixels() <= content_mask.countBlackPixels();
}

bool
BlackOnWhiteEstimator::isBlackOnWhite(GrayImage const& img, BinaryImage const& mask)
{
    if (img.isNull()) {
        throw std::invalid_argument("BlackOnWhiteEstimator: image is null.");
    }
    if (img.size() != mask.size()) {
        throw std::invalid_argument("BlackOnWhiteEstimator: img and mask have different sizes");
    }

    BinaryImage bw_image(img, BinaryThreshold::otsuThreshold(GrayscaleHistogram(img, mask)));
    rasterOp<RopAnd<RopSrc, RopDst> >(bw_image, mask);
    return 2 * bw_image.countBlackPixels() <= mask.countBlackPixels();
}

bool
BlackOnWhiteEstimator::isBlackOnWhite(GrayImage const& img, QPolygonF const& crop_area)
{
    if (img.isNull()) {
        throw std::invalid_argument("BlackOnWhiteEstimator: image is null.");
    }
    if (crop_area.intersected(QRectF(img.rect())).isEmpty()) {
        throw std::invalid_argument("BlackOnWhiteEstimator: the crop area is wrong.");
    }

    BinaryImage mask(img.size(), BLACK);
    PolygonRasterizer::fillExcept(mask, WHITE, crop_area, Qt::WindingFill);
    return isBlackOnWhite(img, mask);
}
