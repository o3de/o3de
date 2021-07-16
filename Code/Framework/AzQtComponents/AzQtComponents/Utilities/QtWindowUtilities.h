/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QOperatingSystemVersion>
#include <QRect>

class QWidget;
class QMainWindow;
class QPainter;

namespace AzQtComponents
{

    AZ_QT_COMPONENTS_API QRect GetTotalScreenGeometry();
    AZ_QT_COMPONENTS_API void EnsureWindowWithinScreenGeometry(QWidget* widget);
    AZ_QT_COMPONENTS_API void EnsureGeometryWithinScreenTop(QRect& geometry);

    AZ_QT_COMPONENTS_API void SetClipRegionForDockingWidgets(QWidget* widget, QPainter& painter, QMainWindow* mainWindow);
    
    AZ_QT_COMPONENTS_API void SetCursorPos(const QPoint& point);
    AZ_QT_COMPONENTS_API void SetCursorPos(int x, int y);

    // Rationale: There are platform-specific differences in how mouse coordinates are handled, this
    // lets us sample every pixel of a HiDPI screen running at > "100%" scaling.

    struct AZ_QT_COMPONENTS_API MappedPoint
    {
        QPoint native;
        QPoint qt;
    };

    MappedPoint AZ_QT_COMPONENTS_API MappedCursorPosition();

    AZ_QT_COMPONENTS_API void bringWindowToTop(QWidget* widget);

    inline bool isWin10()
    {
        return QOperatingSystemVersion::current() >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10);
    }
} // namespace AzQtComponents

