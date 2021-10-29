/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QPixmap>
#include <QRect>
#include <QScreen>

namespace AzQtComponents
{
    AZ_QT_COMPONENTS_API QPixmap ScalePixmapForScreenDpi(QPixmap pixmap, QScreen* screen, QSize size, Qt::AspectRatioMode aspectRatioMode, Qt::TransformationMode transformationMode);
    AZ_QT_COMPONENTS_API QPixmap CropPixmapForScreenDpi(QPixmap pixmap, QScreen* screen, QRect rect);
}; // namespace AzQtComponents
