/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class Aabb;
    class Color;
    class Matrix3x3;
    class Matrix4x4;
    class Quaternion;
    class Transform;
    class Vector2;
    class Vector3;
    class Vector4;
} // namespace AZ

namespace AZStd
{
    //! Prints a Vector2 with precision to 8 decimal places.
    void to_string(string& str, const AZ::Vector2& value);

    //! Prints a Vector3 with precision to 8 decimal places.
    void to_string(string& str, const AZ::Vector3& value);

    //! Prints a Vector4 with precision to 8 decimal places.
    void to_string(string& str, const AZ::Vector4& value);

    //! Prints a Quaternion with precision to 8 decimal places.
    void to_string(string& str, const AZ::Quaternion& value);

    //! Prints a 3x3 matrix in row major order over three lines with precision to 8 decimal places.
    void to_string(string& str, const AZ::Matrix3x3& value);

    //! Prints a 4x4 matrix in row major order over four lines with precision to 8 decimal places.
    void to_string(string& str, const AZ::Matrix4x4& value);

    //! Prints a transform as a 3x4 matrix in row major order over four lines with precision to 8 decimal places.
    void to_string(string& str, const AZ::Transform& value);

    //! Prints an AABB as a pair of Vector3s with precision to 8 decimal places.
    void to_string(string& str, const AZ::Aabb& value);

    //! Prints a Color as four unsigned ints representing RGBA.
    void to_string(string& str, const AZ::Color& value);

    inline AZStd::string to_string(const AZ::Vector2& val)
    {
        AZStd::string str;
        to_string(str, val);
        return str;
    }

    inline AZStd::string to_string(const AZ::Vector3& val)
    {
        AZStd::string str;
        to_string(str, val);
        return str;
    }

    inline AZStd::string to_string(const AZ::Vector4& val)
    {
        AZStd::string str;
        to_string(str, val);
        return str;
    }

    inline AZStd::string to_string(const AZ::Quaternion& val)
    {
        AZStd::string str;
        to_string(str, val);
        return str;
    }

    inline AZStd::string to_string(const AZ::Matrix3x3& val)
    {
        AZStd::string str;
        to_string(str, val);
        return str;
    }

    inline AZStd::string to_string(const AZ::Matrix4x4& val)
    {
        AZStd::string str;
        to_string(str, val);
        return str;
    }
    inline AZStd::string to_string(const AZ::Transform& val)
    {
        AZStd::string str;
        to_string(str, val);
        return str;
    }

    inline AZStd::string to_string(const AZ::Aabb& val)
    {
        AZStd::string str;
        to_string(str, val);
        return str;
    }

    inline AZStd::string to_string(const AZ::Color& val)
    {
        AZStd::string str;
        to_string(str, val);
        return str;
    }
} // namespace AZStd
