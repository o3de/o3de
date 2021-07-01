/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    class Matrix4x4;
    class Quaternion;
    class Transform;
    class Vector2;
    class Vector3;
    class Vector4;
}

namespace MCore
{
    AZStd::string GenerateUniqueString(const char* prefix, const AZStd::function<bool(const AZStd::string& value)>& validationFunction);
    AZStd::string ConstructStringSeparatedBySemicolons(const AZStd::vector<AZStd::string>& stringVec);

    class CharacterConstants
    {
    public:
        static const char space = ' ';
        static const char tab = '\t';
        static const char endLine = '\n';
        static const char comma = ',';
        static const char dot = '.';
        static const char backSlash = '\\';
        static const char forwardSlash = '/';
        static const char semiColon = ';';
        static const char colon = ':';
        static const char doubleQuote = '\"';
        static const char dash = '-';

        static const char* wordSeparators;
    };
}

namespace AZStd
{
    void to_string(string& str, bool value);
    void to_string(string& str, const AZ::Vector2& value);
    void to_string(string& str, const AZ::Vector3& value);
    void to_string(string& str, const AZ::Vector4& value);
    void to_string(string& str, const AZ::Quaternion& value);
    void to_string(string& str, const AZ::Matrix4x4& value);
    void to_string(string& str, const AZ::Transform& value);

    inline AZStd::string to_string(bool val) { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(const AZ::Vector2& val) { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(const AZ::Vector3& val) { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(const AZ::Vector4& val) { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(const AZ::Quaternion& val) { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(const AZ::Matrix4x4& val) { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(const AZ::Transform& val) { AZStd::string str; to_string(str, val); return str; }
}
