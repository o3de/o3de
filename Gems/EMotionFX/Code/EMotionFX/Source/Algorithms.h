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

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/algorithm.h>

namespace EMotionFX
{
    template <typename T, typename Predicate>
    AZ_INLINE AZ::Outcome<size_t> FindIndexIf(const T& vecOfElements, Predicate pred)
    {
        const auto it = AZStd::find_if(vecOfElements.begin(), vecOfElements.end(), pred);
        if (it != vecOfElements.end())
        {
            return AZ::Success(static_cast<size_t>(AZStd::distance(vecOfElements.begin(), it)));
        }
        return AZ::Failure();
    }

    template <class T>
    AZ_INLINE bool IsClose(const T& a, const T& b, float maxError)
    {
        return a.IsClose(b, maxError);
    }

    template <>
    AZ_INLINE bool IsClose(const float& a, const float& b, float maxError)
    {
        return AZ::IsClose(a, b, maxError);
    }

    template <>
    AZ_INLINE bool IsClose(const AZ::Quaternion& a, const AZ::Quaternion& b, float maxError)
    {
        const AZ::Quaternion delta = (a.GetConjugate() * b).GetNormalized();
        const float degreesError = AZ::RadToDeg(2.0f * atan2f(delta.GetImaginary().GetLength(), delta.GetW()));
        return (degreesError <= maxError);
    }

    template <>
    AZ_INLINE bool IsClose(const AZ::Vector3& a, const AZ::Vector3& b, float maxError)
    {
        return ((a - b).GetLength() <= maxError);
    }
} // namespace EMotionFX
