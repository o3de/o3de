/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include <QtGui/private/qhighdpiscaling_p.h>

namespace QtHelpers
{
    AZ::Vector2 QPointFToVector2(const QPointF& point)
    {
        return AZ::Vector2(aznumeric_cast<float>(point.x()), aznumeric_cast<float>(point.y()));
    }

    AZ::Vector2 MapGlobalPosToLocalVector2(const QWidget* widget, const QPoint& pos)
    {
        QPoint localPos = widget->mapFromGlobal(pos);
        return QPointFToVector2(localPos);
    }

    bool IsGlobalPosInWidget(const QWidget* widget, const QPoint& pos)
    {
        QPoint localPos = widget->mapFromGlobal(pos);
        const QSize& size = widget->size();
        bool inWidget = (localPos.x() >= 0 && localPos.x() < size.width() && localPos.y() >= 0 && localPos.y() < size.height());
        return inWidget;
    }

    float GetHighDpiScaleFactor(const QWidget& widget)
    {
        return static_cast<float>(QHighDpiScaling::factor(widget.windowHandle()->screen()));
    }

    QSize GetDpiScaledViewportSize(const QWidget& widget)
    {
        float dpiScale = GetHighDpiScaleFactor(widget);
        float width = ceilf(widget.size().width() * dpiScale);
        float height = ceilf(widget.size().height() * dpiScale);
        return QSize(static_cast<int>(width), static_cast<int>(height));
    }

}   // namespace QtHelpers
