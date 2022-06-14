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
    class Matrix3x4;
    class Matrix4x4;
    class Quaternion;
    class Transform;
    class Vector2;
    class Vector3;
    class Vector4;
} // namespace AZ

namespace AZStd
{
    namespace MathStringFormat
    {
        inline constexpr const char* Vector2DefaultFormat = "%.8f,%.8f";
        inline constexpr const char* Vector2ScriptFormat = "(x=%.7f,y=%.7f)";

        inline constexpr const char* Vector3DefaultFormat = "%.8f,%.8f,%.8f";
        inline constexpr const char* Vector3ScriptFormat = "(x=%.7f,y=%.7f,z=%.7f)";
        
        inline constexpr const char* Vector4DefaultFormat = "%.8f,%.8f,%.8f,%.8f";
        inline constexpr const char* Vector4ScriptFormat = "(x=%.7f,y=%.7f,z=%.7f,w=%.7f)";
        
        inline constexpr const char* QuaternionDefaultFormat = "%.8f,%.8f,%.8f,%.8f";
        inline constexpr const char* QuaternionScriptFormat = "(x=%.7f,y=%.7f,z=%.7f,w=%.7f)";

        inline constexpr const char* Matrix3x3DefaultFormat = "%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f";
        inline constexpr const char* Matrix3x3ScriptFormat = "(x=%.7f,y=%.7f,z=%.7f),(x=%.7f,y=%.7f,z=%.7f),(x=%.7f,y=%.7f,z=%.7f)";
        
        inline constexpr const char* Matrix4x4DefaultFormat = "%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f";
        inline constexpr const char* Matrix4x4ScriptFormat = "(x=%.7f,y=%.7f,z=%.7f,w=%.7f),(x=%.7f,y=%.7f,z=%.7f,w=%.7f),(x=%.7f,y=%.7f,z=%.7f,w=%.7f)";

        inline constexpr const char* Matrix3x4DefaultFormat = "%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f";
        inline constexpr const char* Matrix3x4ScriptFormat = "(x=%.7f,y=%.7f,z=%.7f),(x=%.7f,y=%.7f,z=%.7f),(x=%.7f,y=%.7f,z=%.7f)";

        inline constexpr const char* ColorDefaultFormat = "R:%d, G:%d, B:%d A:%d";
        inline constexpr const char* ColorScriptFormat = "(r=%.7f,g=%.7f,b=%.7f,a=%.7f)";
    }; // namespace MathStringFormat

    //! Prints a Vector2 with precision to 8 decimal places.
    void to_string(string& str, const AZ::Vector2& value, const char* format = MathStringFormat::Vector2DefaultFormat);

    //! Prints a Vector3 with precision to 8 decimal places.
    void to_string(string& str, const AZ::Vector3& value, const char* format = MathStringFormat::Vector3DefaultFormat);

    //! Prints a Vector4 with precision to 8 decimal places.
    void to_string(string& str, const AZ::Vector4& value, const char* format = MathStringFormat::Vector4DefaultFormat);

    //! Prints a Quaternion with precision to 8 decimal places.
    void to_string(string& str, const AZ::Quaternion& value, const char* format = MathStringFormat::QuaternionDefaultFormat);

    //! Prints a 3x3 matrix in row major order over three lines with precision to 8 decimal places.
    void to_string(string& str, const AZ::Matrix3x3& value, const char* format = MathStringFormat::Matrix3x3DefaultFormat);

    //! Prints a 4x4 matrix in row major order over four lines with precision to 8 decimal places.
    void to_string(string& str, const AZ::Matrix4x4& value, const char* format = MathStringFormat::Matrix4x4DefaultFormat);

    //! Prints a 3x4 matrix in row major order over four lines with precision to 8 decimal places.
    void to_string(string& str, const AZ::Matrix3x4& value, const char* format = MathStringFormat::Matrix3x4ScriptFormat);

    //! Prints a transform as a 3x4 matrix in row major order over four lines with precision to 8 decimal places.
    void to_string(string& str, const AZ::Transform& value);

    //! Prints an AABB as a pair of Vector3s with precision to 8 decimal places.
    void to_string(string& str, const AZ::Aabb& value);

    //! Prints a Color as four unsigned ints representing RGBA.
    void to_string(string& str, const AZ::Color& value, const char* format = MathStringFormat::ColorDefaultFormat);

    inline AZStd::string to_string(const AZ::Vector2& val, const char* format = MathStringFormat::Vector2DefaultFormat)
    {
        AZStd::string str;
        to_string(str, val, format);
        return str;
    }

    inline AZStd::string to_string(const AZ::Vector3& val, const char* format = MathStringFormat::Vector3DefaultFormat)
    {
        AZStd::string str;
        to_string(str, val, format);
        return str;
    }

    inline AZStd::string to_string(const AZ::Vector4& val, const char* format = MathStringFormat::Vector4DefaultFormat)
    {
        AZStd::string str;
        to_string(str, val, format);
        return str;
    }

    inline AZStd::string to_string(const AZ::Quaternion& val, const char* format = MathStringFormat::QuaternionDefaultFormat)
    {
        AZStd::string str;
        to_string(str, val, format);
        return str;
    }

    inline AZStd::string to_string(const AZ::Matrix3x3& val, const char* format = MathStringFormat::Matrix3x3DefaultFormat)
    {
        AZStd::string str;
        to_string(str, val, format);
        return str;
    }

    inline AZStd::string to_string(const AZ::Matrix4x4& val, const char* format = MathStringFormat::Matrix4x4DefaultFormat)
    {
        AZStd::string str;
        to_string(str, val, format);
        return str;
    }
    
    inline AZStd::string to_string(const AZ::Matrix3x4& val, const char* format = MathStringFormat::Matrix3x4DefaultFormat)
    {
        AZStd::string str;
        to_string(str, val, format);
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

    inline AZStd::string to_string(const AZ::Color& val, const char* format = MathStringFormat::ColorDefaultFormat)
    {
        AZStd::string str;
        to_string(str, val, format);
        return str;
    }
} // namespace AZStd
