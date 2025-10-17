/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzQtComponents/Utilities/PixmapScaleUtilities.h>

namespace AzQtComponents
{
    QPixmap ScalePixmapForScreenDpi(
        QPixmap pixmap, [[maybe_unused]] QScreen* screen, QSize size, Qt::AspectRatioMode aspectRatioMode, Qt::TransformationMode transformationMode)
    {
        QPixmap scaledPixmap;
        scaledPixmap = pixmap.scaled(size, aspectRatioMode, transformationMode);
        return scaledPixmap;
    }

    QPixmap CropPixmapForScreenDpi(
        QPixmap pixmap, [[maybe_unused]] QScreen* screen, QRect rect)
    {
        QPixmap croppedPixmap = pixmap.copy(rect);
        return croppedPixmap;
    }
}
