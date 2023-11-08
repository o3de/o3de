/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace ScriptCanvas
{
    namespace StringFunctions
    {
        AZStd::string ToLower(AZStd::string sourceString);

        AZStd::string ToUpper(AZStd::string sourceString);

        AZStd::string Substring(AZStd::string sourceString, AZ::u32 index, AZ::u32 length);

        bool IsValidFindPosition(size_t findPosition);

        size_t ContainsString(const AZStd::string& sourceString, const AZStd::string& patternString, bool searchFromEnd, bool caseSensitive);

        bool StartsWith(const AZStd::string& sourceString, const AZStd::string& patternString, bool caseSensitive);

        bool EndsWith(const AZStd::string& sourceString, const AZStd::string& patternString, bool caseSensitive);

        AZStd::string Join(const AZStd::vector<AZStd::string>& sourceArray, const AZStd::string& separatorString);

        AZStd::string ReplaceString(AZStd::string& sourceString, const AZStd::string& replaceString, const AZStd::string& withString, bool caseSensitive);

        AZStd::vector<AZStd::string> Split(const AZStd::string& sourceString, const AZStd::string& delimiterString);
    } // namespace StringFunctions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/String/StringFunctions.generated.h>
