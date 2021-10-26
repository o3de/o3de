/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzQtComponents/Utilities/PixmapScaleUtilities.h>

#include <QtGui/private/qhighdpiscaling_p.h>

namespace AzQtComponents
{
    QPixmap ScalePixmapForScreenDpi(
        QPixmap pixmap, QScreen* screen, QSize size, Qt::AspectRatioMode aspectRatioMode, Qt::TransformationMode transformationMode)
    {
        qreal screenDpiFactor = QHighDpiScaling::factor(screen);
        pixmap.setDevicePixelRatio(screenDpiFactor);

        QPixmap scaledPixmap;

        size.setWidth(aznumeric_cast<int>(aznumeric_cast<qreal>(size.width()) * screenDpiFactor));
        size.setHeight(aznumeric_cast<int>(aznumeric_cast<qreal>(size.height()) * screenDpiFactor));

        scaledPixmap = pixmap.scaled(size, aspectRatioMode, transformationMode);

        return scaledPixmap;
    }

    QPixmap CropPixmapForScreenDpi(
        QPixmap pixmap, QScreen* screen, QRect rect)
    {
        qreal screenDpiFactor = QHighDpiScaling::factor(screen);
        pixmap.setDevicePixelRatio(screenDpiFactor);

        QRect cropRect(
            aznumeric_cast<int>(aznumeric_cast<qreal>(rect.left()) * screenDpiFactor),
            aznumeric_cast<int>(aznumeric_cast<qreal>(rect.top()) * screenDpiFactor),
            aznumeric_cast<int>(aznumeric_cast<qreal>(rect.width()) * screenDpiFactor),
            aznumeric_cast<int>(aznumeric_cast<qreal>(rect.height()) * screenDpiFactor)
        );

        QPixmap croppedPixmap = pixmap.copy(cropRect);
        return croppedPixmap;
    }
}
