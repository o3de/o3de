/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        const AZ::Vector3 diff = v - m_center;
        const float dist = diff.Dot(diff);

        // if the current sphere doesn't contain the point, grow the sphere so that it contains the point
        if (dist > m_radiusSq)
        {
            const AZ::Vector3 diff2 = diff.GetNormalized() * m_radius;
            const AZ::Vector3 delta = 0.5f * (diff - diff2);
            m_center             += delta;
            // TODO: KB- Was a 'safe' function, is there an AZ equivalent?
            float length = delta.GetLengthSq();
            if (length >= FLT_EPSILON)
            {
                m_radius += sqrtf(length);
            }
            else
            {
                m_radius = 0.0f;
            }
            m_radiusSq = m_radius * m_radius;
        }
    }


    // check if the sphere intersects with a given AABB
    bool BoundingSphere::Intersects(const AABB& b) const
    {
        float distance = 0.0f;

        for (int32_t t = 0; t < 3; ++t)
        {
            const AZ::Vector3& minVec = b.GetMin();
            if (m_center.GetElement(t) < minVec.GetElement(t))
            {
                distance += (m_center.GetElement(t) - minVec.GetElement(t)) * (m_center.GetElement(t) - minVec.GetElement(t));
                if (distance > m_radiusSq)
                {
                    return false;
                }
            }
            else
            {
                const AZ::Vector3& maxVec = b.GetMax();
                if (m_center.GetElement(t) > maxVec.GetElement(t))
                {
                    distance += (m_center.GetElement(t) - maxVec.GetElement(t)) * (m_center.GetElement(t) - maxVec.GetElement(t));
                    if (distance > m_radiusSq)
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
        for (int32_t t = 0; t < 3; ++t)
        {
            const AZ::Vector3& maxVec = b.GetMax();
            if (m_center.GetElement(t) < maxVec.GetElement(t))
            {
                distance += (m_center.GetElement(t) - maxVec.GetElement(t)) * (m_center.GetElement(t) - maxVec.GetElement(t));
            }
            else
            {
                const AZ::Vector3& minVec = b.GetMin();
                if (m_center.GetElement(t) > minVec.GetElement(t))
                {
                    distance += (m_center.GetElement(t) - minVec.GetElement(t)) * (m_center.GetElement(t) - minVec.GetElement(t));
                }
            }

            if (distance > m_radiusSq)
            {
                return false;
            }
        }

        return true;
    }
}   // namespace MCore
