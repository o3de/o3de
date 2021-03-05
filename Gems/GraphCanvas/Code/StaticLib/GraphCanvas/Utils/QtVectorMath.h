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
            return QPointF(centerPoint.x() + radius * std::cos(radians), centerPoint.y() + radius * std::sin(radians));
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
            return std::sqrt(aznumeric_cast<float>(point.x() * point.x() + point.y() * point.y()));
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

            int distance = -1;

            // Find the line between the two rectangles.
            QPointF direction = rectA.center() - rectB.center();
            QLineF directionLine(rectA.center(), rectB.center());

            QLineF aLine1;
            QLineF aLine2;
            QLineF aFinalLine;

            QLineF bLine1;
            QLineF bLine2;
            QLineF bFinalLine;


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
