/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace PhysX
{
    /// Class that keeps track of unique strings (case insensitive) in groups.
    class UniqueStringContainer
    {
    public:
        struct CaseInsensitiveStringHash
        {
            AZ_TYPE_INFO(UniqueStringContainer::CaseInsensitiveStringHash, "{EB80F2A1-2DEB-47CC-ABF7-592F492C20A9}");

            size_t operator()(const AZStd::string& str) const
            {
                AZStd::string lowerStr = str;
                AZStd::to_lower(lowerStr.begin(), lowerStr.end());
                return AZStd::hash<AZStd::string>{}(lowerStr);
            }
        };

        struct CaseInsensitiveStringEqual
        {
            AZ_TYPE_INFO(UniqueStringContainer::CaseInsensitiveStringEqual, "{6ADEA1D9-27B8-4C7A-913D-EC8191F1B6A9}");

            bool operator()(const AZStd::string& arg0, const AZStd::string& arg1) const
            {
                return AZ::StringFunc::Equal(arg0, arg1, false/*bCaseSensitive*/);
            }
        };

        using StringSet = AZStd::unordered_set<AZStd::string, CaseInsensitiveStringHash, CaseInsensitiveStringEqual>;

        /// Add a unique string to a group of unique strings.
        void AddString(AZ::Crc32 stringGroupId, const AZStd::string& stringIn);

        /// Returns a modified version of the given input string that would be unique in the specified string group.
        AZStd::string GetUniqueString(AZ::Crc32 stringGroupId
            , const AZStd::string& stringIn
            , AZ::u64 maxStringLength
            , const StringSet& forbiddenStrings) const;

        /// Checks if a string would be unique in the identified string group.
        bool IsStringUnique(AZ::Crc32 stringGroupId, const AZStd::string& stringIn) const;

        /// Removes a string from the identified group of unique strings.
        void RemoveString(AZ::Crc32 stringGroupId, const AZStd::string& stringIn);

    private:
        using StringGroups = AZStd::unordered_map<AZ::Crc32, StringSet>;
        StringGroups m_stringGroups; ///< Collection of groups of unique strings, each group identified by an ID.
    };
} // namespace PhysX
