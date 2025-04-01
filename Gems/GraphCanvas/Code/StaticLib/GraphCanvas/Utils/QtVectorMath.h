/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <cmath>
#include <AzCore/Math/MathUtils.h>

#include <qpoint.h>

namespace GraphCanvas
{
    class QtMath
    {
    public:
        static QPointF PointOnCircle(const QPointF& centerPoint, float radius, float degrees)
        {
            float radians = AZ::DegToRad(degrees);
            return QPointF(centerPoint.x() + radius * cosf(radians), centerPoint.y() + radius * sinf(radians));
        }
    };

    class QtVectorMath
    {
    public:
        static QPointF Transpose(const QPointF& point)
        {
            return QPointF(point.y(), -point.x());
        }

        static float GetLength(const QPointF& point)
        {
            return sqrtf(aznumeric_cast<float>(point.x() * point.x() + point.y() * point.y()));
        }

        static QPointF Normalize(const QPointF& point)
        {
            float length = GetLength(point);

            if (length > 0.0f)
            {
                return point/length;
            }
            else
            {
                return point;
            }
        }

        static int GetMinimumDistanceBetween(const QRectF& rectA, const QRectF& rectB)
        {
            if (rectA.intersects(rectB))
            {
                return 0;
            }

            // Find the line between the two rectangles.
            QLineF directionLine(rectA.center(), rectB.center());

            // Not strictly correct, but correct enough.
            //
            // Finds the two points on the line from center to center, and returns the distance between them.
            AZStd::vector< QPointF > aIntersectionPoints;

            AZStd::vector< QLineF > aLines = {
                QLineF(rectA.topLeft(), rectA.topRight()),
                QLineF(rectA.topRight(), rectA.bottomRight()),
                QLineF(rectA.bottomRight(), rectA.bottomLeft()),
                QLineF(rectA.bottomLeft(), rectA.topLeft())
            };

            for (const auto& aLine : aLines)
            {
                QPointF temporaryPoint;

                if (aLine.intersects(directionLine, &temporaryPoint) == QLineF::IntersectType::BoundedIntersection)
                {
                    aIntersectionPoints.push_back(temporaryPoint);
                }
            }

            AZStd::vector< QPointF > bIntersectionPoints;

            AZStd::vector< QLineF > bLines = {
                QLineF(rectB.topLeft(), rectB.topRight()),
                QLineF(rectB.topRight(), rectB.bottomRight()),
                QLineF(rectB.bottomRight(), rectB.bottomLeft()),
                QLineF(rectB.bottomLeft(), rectB.topLeft())
            };

            for (const auto& bLine : bLines)
            {
                QPointF temporaryPoint;

                if (bLine.intersects(directionLine, &temporaryPoint) == QLineF::IntersectType::BoundedIntersection)
                {
                    bIntersectionPoints.push_back(temporaryPoint);
                }
            }

            int minimumLength = -1;

            for (const QPointF& aPoint : aIntersectionPoints)
            {
                for (const QPointF& bPoint : bIntersectionPoints)
                {
                    int testLength = aznumeric_cast<int>(GetLength(aPoint - bPoint));

                    if (minimumLength < 0 || testLength < minimumLength)
                    {
                        minimumLength = testLength;
                    }
                }
            }

            return minimumLength;
        }
    };
}
