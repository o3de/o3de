/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StringFunctions.h"
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace ScriptCanvas
{
    namespace StringFunctions
    {
        AZStd::string ToLower(AZStd::string sourceString)
        {
            AZStd::to_lower(sourceString.begin(), sourceString.end());
            return sourceString;
        }

        AZStd::string ToUpper(AZStd::string sourceString)
        {
            AZStd::to_upper(sourceString.begin(), sourceString.end());
            return sourceString;
        }

        AZStd::string Substring(AZStd::string sourceString, AZ::u32 index, AZ::u32 length)
        {
            length = AZ::GetClamp<AZ::u32>(length, 0, aznumeric_cast<AZ::u32>(sourceString.size()));

            if (length == 0 || index >= sourceString.size())
            {
                return {};
            }

            return sourceString.substr(index, length);
        }

        AZStd::string Join(const AZStd::vector<AZStd::string>& sourceArray, const AZStd::string& separatorString)
        {
            AZStd::string result;
            size_t index = 0;
            const size_t length = sourceArray.size();
            for (auto string : sourceArray)
            {
                if (index < length - 1)
                {
                    result.append(AZStd::string::format("%s%s", string.c_str(), separatorString.c_str()));
                }
                else
                {
                    result.append(string);
                }
                ++index;
            }
            return result;
        }

        AZStd::string ReplaceString(
            AZStd::string& sourceString, const AZStd::string& replaceString, const AZStd::string& withString, bool caseSensitive)
        {
            AzFramework::StringFunc::Replace(sourceString, replaceString.c_str(), withString.c_str(), caseSensitive);
            return sourceString;
        }

        AZStd::vector<AZStd::string> Split(const AZStd::string& sourceString, const AZStd::string& delimiterString)
        {
            AZStd::vector<AZStd::string> stringArray;

            AzFramework::StringFunc::Tokenize(sourceString.c_str(), stringArray, delimiterString.c_str(), false, false);

            return stringArray;
        }
    } // namespace StringFunctions
} // namespace ScriptCanvas
