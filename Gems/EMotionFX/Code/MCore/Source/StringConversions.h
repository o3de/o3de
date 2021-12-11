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
#include <AzCore/Math/MathStringConversions.h>
#include <AzFramework/StringFunc/StringFunc.h>

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

    inline AZStd::string to_string(bool val) { AZStd::string str; to_string(str, val); return str; }
}
