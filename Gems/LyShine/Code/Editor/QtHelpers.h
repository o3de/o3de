/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace QtHelpers
{
    AZ::Vector2 QPointFToVector2(const QPointF& point);

    AZ::Vector2 MapGlobalPosToLocalVector2(const QWidget* widget, const QPoint& pos);

    bool IsGlobalPosInWidget(const QWidget* widget, const QPoint& pos);

    float GetHighDpiScaleFactor(const QWidget& widget);

    QSize GetDpiScaledViewportSize(const QWidget& widget);

}   // namespace QtHelpers
