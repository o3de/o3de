/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/ScriptCanvas/ScriptCanvasAttributes.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include "StringMethods.h"

#include <ScriptCanvas/Core/Attributes.h>

#define LUA_BACKEND

namespace ScriptCanvas
{
    bool StringMethods::EndsWith(const AZStd::string& sourceString, const AZStd::string& patternString, bool caseSensitive)
    {
        return AzFramework::StringFunc::EndsWith(sourceString.c_str(), patternString.c_str(), caseSensitive);
    }

    size_t StringMethods::Find(const AZStd::string& sourceString, const AZStd::string& patternString, bool searchFromEnd, bool caseSensitive)
    {
#if defined(LUA_BACKEND)
        AZ_Warning("ScriptCanvas", sourceString.size() <= k_LuaNpos && patternString.size() <= k_LuaNpos,
            "Source or Pattern string is too long, lua may lose precision on the position value.");
        size_t findPosition = AzFramework::StringFunc::Find(sourceString.c_str(), patternString.c_str(), 0, searchFromEnd, caseSensitive);
        return findPosition > k_LuaNpos ? k_LuaNpos : findPosition;
#else
        return AzFramework::StringFunc::Find(sourceString.c_str(), patternString.c_str(), 0, searchFromEnd, caseSensitive);
#endif
    }

    bool StringMethods::IsValidFindPosition(size_t findPosition)
    {
#if defined(LUA_BACKEND)
        return findPosition != k_LuaNpos;
#else
        return findPosition != AZStd::string::npos;
#endif
    }

    AZStd::string StringMethods::Join(const AZStd::vector<AZStd::string>& sourceArray, const AZStd::string& separatorString)
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

    AZStd::string StringMethods::Replace(AZStd::string& sourceString, const AZStd::string& replaceString, const AZStd::string& withString, bool caseSensitive)
    {
        AzFramework::StringFunc::Replace(sourceString, replaceString.c_str(), withString.c_str(), caseSensitive);
        return sourceString;
    }

    AZStd::vector<AZStd::string> StringMethods::Split(const AZStd::string& sourceString, const AZStd::string& delimiterString)
    {
        AZStd::vector<AZStd::string> stringArray;

        AzFramework::StringFunc::Tokenize(sourceString.c_str(), stringArray, delimiterString.c_str(), false, false);

        return stringArray;
    }

    bool StringMethods::StartsWith(const AZStd::string& sourceString, const AZStd::string& patternString, bool caseSensitive)
    {
        return AzFramework::StringFunc::StartsWith(sourceString.c_str(), patternString.c_str(), caseSensitive);
    }

    void StringMethods::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<StringMethods>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<StringMethods>("String", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "");
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            AZ::BranchOnResultInfo booleanResultInfo;

            AZ::BranchOnResultInfo nonBooleanResultInfo;
            nonBooleanResultInfo.m_returnResultInBranches = true;
            nonBooleanResultInfo.m_nonBooleanResultCheckName = "Is Valid Find Position";

            behaviorContext->Class<StringMethods>("String")
                ->Method("Is Valid Find Position", &IsValidFindPosition)
                ->Method("Contains String", &Find,
                    { { {"Source", "The string to search in."}, {"Pattern", "The substring to search for."}, {"Search From End", "Start the match checking from the end of a string."}, { "Case Sensitive", "Take into account the case of the string when searching." } } })
                    ->Attribute(AZ::ScriptCanvasAttributes::BranchOnResult, nonBooleanResultInfo)
                ->Method("Starts With", &StartsWith,
                    { { {"Source", "The string to search in."}, {"Pattern", "The substring to search for."}, {"Case Sensitive", "Take into account the case of the string when searching."} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::BranchOnResult, booleanResultInfo)
                ->Method("Ends With", &EndsWith,
                    { { {"Source", "The string to search in."}, {"Pattern", "The substring to search for."}, {"Case Sensitive", "Take into account the case of the string when searching."} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::BranchOnResult, booleanResultInfo)
                ->Method("Split", &Split,
                    { { {"Source", "The string to search in."}, {"Delimiters", "The characters that can be used as delimiters."} } })
                ->Method("Join", &Join,
                    { { {"String Array", "The array of strings to join."}, {"Separator", "Will use this string when concatenating the strings from the array."} } })
                ->Method("Replace String", &Replace,
                    { { {"Source", "The string to search in."}, {"Replace", "The substring to search for."}, {"With", "The string to replace the substring with."}, {"Case Sensitive", "Take into account the case of the string when searching."} } });
        }
    }
}
