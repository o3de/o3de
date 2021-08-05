/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace AZ
{
    // Vector2 conversions
    AZ_MATH_INLINE Vector3 Vector2ToVector3(const Vector2& v)
    {
        return Vector3(v.GetX(), v.GetY(), 0);
    }

    AZ_MATH_INLINE Vector3 Vector2ToVector3(const Vector2& v, const float z)
    {
        return Vector3(v.GetX(), v.GetY(), z);
    }

    AZ_MATH_INLINE Vector4 Vector2ToVector4(const Vector2& v) 
    {
        return Vector4(v.GetX(), v.GetY(), 0, 0);
    }

    AZ_MATH_INLINE Vector4 Vector2ToVector4(const Vector2& v, const float z)
    {
        return Vector4(v.GetX(), v.GetY(), z, 0);
    }

    AZ_MATH_INLINE Vector4 Vector2ToVector4(const Vector2& v, const float z, const float w)
    {
        return Vector4(v.GetX(), v.GetY(), z, w);
    }
    
    // Vector3 conversions
    AZ_MATH_INLINE Vector2 Vector3ToVector2(const Vector3& v)
    {
        return Vector2(Simd::Vec3::ToVec2(v.GetSimdValue()));
    }

    AZ_MATH_INLINE Vector4 Vector3ToVector4(const Vector3& v) 
    {
        return Vector4(v.GetX(), v.GetY(), v.GetZ(), 0);
    }

    AZ_MATH_INLINE Vector4 Vector3ToVector4(const Vector3& v, const float w)
    {
        return Vector4(v.GetX(), v.GetY(), v.GetZ(), w);
    }

    // Vector4 conversions
    AZ_MATH_INLINE Vector2 Vector4ToVector2(const Vector4& v)
    {
        return Vector2(Simd::Vec4::ToVec2(v.GetSimdValue()));
    }

    AZ_MATH_INLINE Vector3 Vector4ToVector3(const Vector4& v) 
    {
        return Vector3(Simd::Vec4::ToVec3(v.GetSimdValue()));
    }
} //namespace AZ
