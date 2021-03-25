/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    class StringMethods
    {
    public:
        static const size_t k_LuaNpos = UINT_MAX;

        AZ_TYPE_INFO(StringMethods, "{4A70FD56-10A8-460E-B822-3EF03F1EF7A0}");

        static bool EndsWith(const AZStd::string& sourceString, const AZStd::string& patternString, bool caseSensitive);

        static size_t Find(const AZStd::string& sourceString, const AZStd::string& patternString, bool searchFromEnd, bool caseSensitive);

        static bool IsValidFindPosition(size_t findPosition);

        static AZStd::string Join(const AZStd::vector<AZStd::string>& sourceArray, const AZStd::string& separatorString);

        static AZStd::string Replace(AZStd::string& sourceString, const AZStd::string& replaceString, const AZStd::string& withString, bool caseSensitive);

        static AZStd::vector<AZStd::string> Split(const AZStd::string& sourceString, const AZStd::string& delimiterString);

        static bool StartsWith(const AZStd::string& sourceString, const AZStd::string& patternString, bool caseSensitive);

        static void Reflect(AZ::ReflectContext* context);
    };
}
