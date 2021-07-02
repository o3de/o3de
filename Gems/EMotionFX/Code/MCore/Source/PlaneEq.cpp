/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "PlaneEq.h"

namespace MCore
{
    // clip points against the plane
    bool PlaneEq::Clip(const AZStd::vector<AZ::Vector3>& pointsIn, AZStd::vector<AZ::Vector3>& pointsOut) const
    {
        size_t numPoints = pointsIn.size();

        MCORE_ASSERT(&pointsIn != &pointsOut);
        MCORE_ASSERT(numPoints >= 2);

        size_t      vert1       = numPoints - 1;
        float       firstDist   = CalcDistanceTo(pointsIn[vert1]);
        float       nextDist    = firstDist;
        bool        firstIn     = (firstDist >= 0.0f);
        bool        nextIn      = firstIn;

        pointsOut.clear();

        if (numPoints == 2)
        {
            numPoints = 1;
        }

        for (int32 vert2 = 0; vert2 < numPoints; vert2++)
        {
            float   dist = nextDist;
            bool    in   = nextIn;

            nextDist = CalcDistanceTo(pointsIn[vert2]);
            nextIn   = (nextDist >= 0.0f);

            if (in)
            {
                pointsOut.emplace_back(pointsIn[vert1]);
            }

            if ((in != nextIn) && (dist != 0.0f) && (nextDist != 0.0f))
            {
                AZ::Vector3 dir = (pointsIn[vert2] - pointsIn[vert1]);

                float frac = dist / (dist - nextDist);
                if ((frac > 0.0f) && (frac < 1.0f))
                {
                    pointsOut.emplace_back(pointsIn[vert1] + frac * dir);
                }
            }

            vert1 = vert2;
        }

        //if (numPoints == 1)
        //  return (pointsOut.GetLength() > 1);

        return (pointsOut.size() > 1);
    }


    // clip a set of vectors <points> to this plane
    bool PlaneEq::Clip(AZStd::vector<AZ::Vector3>& points) const
    {
        AZStd::vector<AZ::Vector3> pointsOut;
        if (Clip(points, pointsOut))
        {
            points = pointsOut;
            return true;
        }

        return false;
    }
}   // namespace MCore
