/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

/**
 * Utility functions for convertings math types into strings.
 */
namespace AZ
{
    class Vector2;
    class Vector3;
    class Vector4;
    class Quaternion;
    class Transform;
    class Matrix3x3;
    class Matrix4x4;
    class Color;

    AZStd::string ToString(const Vector2& vector2);
    AZStd::string ToString(const Vector3& vector3);
    AZStd::string ToString(const Vector4& vector4);
    AZStd::string ToString(const Quaternion& quaternion);
    AZStd::string ToString(const Transform& transform);
    AZStd::string ToString(const Matrix3x3& matrix33);
    AZStd::string ToString(const Matrix4x4& matrix44);
    AZStd::string ToString(const Color& color);
}
