/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required header files
#include "StandardHeaders.h"
#include "FastMath.h"
#include "Algorithms.h"
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/PackedVector3.h>

namespace MCore
{
    inline float SafeLength(const AZ::Vector3& rhs)
    {
        const float lenSq = rhs.Dot(rhs);
        return (lenSq > FLT_EPSILON) ? sqrtf(lenSq) : 0.0f;
    }

    inline AZ::Vector3 SafeNormalize(const AZ::Vector3& rhs)
    {
        AZ::Vector3 result(0.0f);
        const float len = rhs.Dot(rhs);
        if (len > 0.0f)
        {
            const float invLen = 1.0f / sqrtf(len);
            result.SetX(rhs.GetX() * invLen);
            result.SetY(rhs.GetY() * invLen);
            result.SetZ(rhs.GetZ() * invLen);
        }
        return result;
    }

    MCORE_INLINE AZ::Vector3 Mirror(const AZ::Vector3& vec, const AZ::Vector3& n)
    {
        const float fac = 2.0f * (n.GetX() * vec.GetX() + n.GetY() * vec.GetY() + n.GetZ() * vec.GetZ());
        return AZ::Vector3(vec.GetX() - fac * n.GetX(), vec.GetY() - fac * n.GetY(), vec.GetZ() - fac * n.GetZ());
    }

    MCORE_INLINE AZ::Vector3 Projected(const AZ::Vector3& vec, const AZ::Vector3& projectOnto)
    {
        AZ::Vector3 result = projectOnto;
        const float ontoSqLen = projectOnto.GetLengthSq();
        if (ontoSqLen > 0.0f)
        {
            result *= (projectOnto.Dot(vec) / ontoSqLen);
        }
        return result;
    }

    MCORE_INLINE bool CheckIfIsUniform(const AZ::Vector3& val)
    {
        return ((MCore::Math::Abs(val.GetX() - val.GetY()) <= MCore::Math::epsilon) &&
                (MCore::Math::Abs(val.GetX() - val.GetZ()) < MCore::Math::epsilon));
    }

    template <>
    MCORE_INLINE AZ::Vector3 LinearInterpolate(const AZ::Vector3& source, const AZ::Vector3& target, float timeValue)
    {
        return source * (1.0f - timeValue) + timeValue * target;
    }
}   // namespace MCore
