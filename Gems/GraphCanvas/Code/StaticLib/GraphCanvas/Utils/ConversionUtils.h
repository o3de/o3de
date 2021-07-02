/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <qcolor.h>

#include <AzCore/Math/Color.h>

namespace GraphCanvas
{
    class ConversionUtils
    {
    public:
        static QColor AZToQColor(const AZ::Color& color)
        {
            return QColor(
                aznumeric_cast<int>(static_cast<float>(color.GetR()) * 255),
                aznumeric_cast<int>(static_cast<float>(color.GetG()) * 255),
                aznumeric_cast<int>(static_cast<float>(color.GetB()) * 255),
                aznumeric_cast<int>(static_cast<float>(color.GetA()) * 255));
        }

        static QPointF AZToQPoint(const AZ::Vector2& vector)
        {
            return QPointF(vector.GetX(), vector.GetY());
        }

        static AZ::Vector2 QPointToVector(const QPointF& point)
        {
            return AZ::Vector2(aznumeric_cast<float>(point.x()), aznumeric_cast<float>(point.y()));
        }
    };
}
