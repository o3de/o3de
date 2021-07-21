/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "BoundingSphere.h"
#include "AABB.h"


namespace MCore
{
    // encapsulate a point in the sphere
    void BoundingSphere::Encapsulate(const AZ::Vector3& v)
    {
        // calculate the squared distance from the center to the point
        const AZ::Vector3 diff = v - mCenter;
        const float dist = diff.Dot(diff);

        // if the current sphere doesn't contain the point, grow the sphere so that it contains the point
        if (dist > mRadiusSq)
        {
            const AZ::Vector3 diff2 = diff.GetNormalized() * mRadius;
            const AZ::Vector3 delta = 0.5f * (diff - diff2);
            mCenter             += delta;
            // TODO: KB- Was a 'safe' function, is there an AZ equivalent?
            float length = delta.GetLengthSq();
            if (length >= FLT_EPSILON)
            {
                mRadius += sqrtf(length);
            }
            else
            {
                mRadius = 0.0f;
            }
            mRadiusSq = mRadius * mRadius;
        }
    }


    // check if the sphere intersects with a given AABB
    bool BoundingSphere::Intersects(const AABB& b) const
    {
        float distance = 0.0f;

        for (uint32 t = 0; t < 3; ++t)
        {
            const AZ::Vector3& minVec = b.GetMin();
            if (mCenter.GetElement(t) < minVec.GetElement(t))
            {
                distance += (mCenter.GetElement(t) - minVec.GetElement(t)) * (mCenter.GetElement(t) - minVec.GetElement(t));
                if (distance > mRadiusSq)
                {
                    return false;
                }
            }
            else
            {
                const AZ::Vector3& maxVec = b.GetMax();
                if (mCenter.GetElement(t) > maxVec.GetElement(t))
                {
                    distance += (mCenter.GetElement(t) - maxVec.GetElement(t)) * (mCenter.GetElement(t) - maxVec.GetElement(t));
                    if (distance > mRadiusSq)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }


    // check if the sphere completely contains a given AABB
    bool BoundingSphere::Contains(const AABB& b) const
    {
        float distance = 0.0f;
        for (uint32 t = 0; t < 3; ++t)
        {
            const AZ::Vector3& maxVec = b.GetMax();
            if (mCenter.GetElement(t) < maxVec.GetElement(t))
            {
                distance += (mCenter.GetElement(t) - maxVec.GetElement(t)) * (mCenter.GetElement(t) - maxVec.GetElement(t));
            }
            else
            {
                const AZ::Vector3& minVec = b.GetMin();
                if (mCenter.GetElement(t) > minVec.GetElement(t))
                {
                    distance += (mCenter.GetElement(t) - minVec.GetElement(t)) * (mCenter.GetElement(t) - minVec.GetElement(t));
                }
            }

            if (distance > mRadiusSq)
            {
                return false;
            }
        }

        return true;
    }
}   // namespace MCore
