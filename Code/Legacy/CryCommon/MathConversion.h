/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Component/EntityId.h>
#include <Cry_Math.h>
#include <Cry_Color.h>

inline AZ::Vector2 LYVec2ToAZVec2(const Vec2& source)
{
    return AZ::Vector2(source.x, source.y);
}

inline Vec2 AZVec2ToLYVec2(const AZ::Vector2& source)
{
    return Vec2(source.GetX(), source.GetY());
}

inline AZ::Vector3 LYVec3ToAZVec3(const Vec3& source)
{
    return AZ::Vector3(source.x, source.y, source.z);
}

inline Vec3 AZVec3ToLYVec3(const AZ::Vector3& source)
{
    return Vec3(source.GetX(), source.GetY(), source.GetZ());
}

inline AZ::Vector4 LYVec4ToAZVec4(const Vec4& source)
{
    return AZ::Vector4(source.x, source.y, source.z, source.w);
}

inline Vec4 AZVec4ToLYVec4(const AZ::Vector4& source)
{
    return Vec4(source.GetX(), source.GetY(), source.GetZ(), source.GetW());
}

inline Vec4 AZColorToLYVec4(const AZ::Color& source)
{
    return Vec4(source.GetR(), source.GetG(), source.GetB(), source.GetA());
}

inline ColorF AZColorToLYColorF(const AZ::Color& source)
{
    return ColorF(source.ToU32());
}

inline AZ::Color LYColorFToAZColor(const ColorF& source)
{
    return AZ::Color(source.r, source.g, source.b, source.a);
}

inline ColorB AZColorToLYColorB(const AZ::Color& source)
{
    return ColorB(source.ToU32());
}

inline AZ::Color LYColorBToAZColor(const ColorB& source)
{
    return AZ::Color(source.r, source.g, source.b, source.a);
}


inline AZ::Quaternion LYQuaternionToAZQuaternion(const Quat& source)
{
    const float f4[4] = { source.v.x, source.v.y, source.v.z, source.w };
    return AZ::Quaternion::CreateFromFloat4(f4);
}

inline Quat AZQuaternionToLYQuaternion(const AZ::Quaternion& source)
{
    float f4[4];
    source.StoreToFloat4(f4);
    return Quat(f4[3], f4[0], f4[1], f4[2]);
}

inline Matrix34 AZTransformToLYTransform(const AZ::Transform& source)
{
    return Matrix34::CreateFromVectors(
        AZVec3ToLYVec3(source.GetBasisX()),
        AZVec3ToLYVec3(source.GetBasisY()),
        AZVec3ToLYVec3(source.GetBasisZ()),
        AZVec3ToLYVec3(source.GetTranslation()));
}

inline Matrix33 AZMatrix3x3ToLYMatrix3x3(const AZ::Matrix3x3& source)
{
    return Matrix33::CreateFromVectors(
        AZVec3ToLYVec3(source.GetColumn(0)),
        AZVec3ToLYVec3(source.GetColumn(1)),
        AZVec3ToLYVec3(source.GetColumn(2)));
}

inline AZ::Matrix3x3 LyMatrix3x3ToAzMatrix3x3(const Matrix33& source)
{
    return AZ::Matrix3x3::CreateFromColumns(
        LYVec3ToAZVec3(source.GetColumn(0)),
        LYVec3ToAZVec3(source.GetColumn(1)),
        LYVec3ToAZVec3(source.GetColumn(2)));
}

inline Matrix34 AZMatrix3x4ToLYMatrix3x4(const AZ::Matrix3x4& source)
{
    AZ::Vector3 col0;
    AZ::Vector3 col1;
    AZ::Vector3 col2;
    AZ::Vector3 col3;
    source.GetBasisAndTranslation(&col0, &col1, &col2, &col3);

    return Matrix34(
        col0.GetX(), col1.GetX(), col2.GetX(), col3.GetX(),
        col0.GetY(), col1.GetY(), col2.GetY(), col3.GetY(),
        col0.GetZ(), col1.GetZ(), col2.GetZ(), col3.GetZ());
}

inline AZ::Transform LYTransformToAZTransform(const Matrix34& source)
{
    const AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromRowMajorFloat12(source.GetData());
    return AZ::Transform::CreateFromMatrix3x4(matrix3x4);
}

inline AZ::Matrix3x4 LYTransformToAZMatrix3x4(const Matrix34& source)
{
    return AZ::Matrix3x4::CreateFromRowMajorFloat12(source.GetData());
}
