/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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