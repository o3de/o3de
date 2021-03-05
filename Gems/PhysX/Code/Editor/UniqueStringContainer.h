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

#include <AzCore/base.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

namespace PhysX
{
    /// Class that keeps track of unique strings in groups.
    class UniqueStringContainer
    {
    public:
        /// Add a unique string to a group of unique strings.
        void AddString(AZ::Crc32 stringGroupId, const AZStd::string& stringIn);

        /// Returns a modified version of the given input string that would be unique in the specified string group.
        AZStd::string GetUniqueString(AZ::Crc32 stringGroupId
            , const AZStd::string& stringIn
            , AZ::u64 maxStringLength
            , const AZStd::unordered_set<AZStd::string>& forbiddenStrings) const;

        /// Checks if a string would be unique in the identified string group.
        bool IsStringUnique(AZ::Crc32 stringGroupId, const AZStd::string& stringIn) const;

        /// Removes a string from the identified group of unique strings.
        void RemoveString(AZ::Crc32 stringGroupId, const AZStd::string& stringIn);

    private:
        using StringGroups = AZStd::unordered_map<AZ::Crc32, AZStd::unordered_set<AZStd::string>>;
        StringGroups m_stringGroups; ///< Collection of groups of unique strings, each group identified by an ID.
    };
}
