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
#include "PhysX_precompiled.h"

#include <AzCore/std/string/conversions.h>

#include "UniqueStringContainer.h"

namespace PhysX
{
    void UniqueStringContainer::AddString(AZ::Crc32 stringGroupId
        , const AZStd::string& stringIn)
    {
        StringGroups::iterator stringGroupsIter = m_stringGroups.find(stringGroupId);
        if (stringGroupsIter == m_stringGroups.end())
        {
            m_stringGroups.emplace(stringGroupId, AZStd::unordered_set<AZStd::string>());
        }
        m_stringGroups[stringGroupId].insert(stringIn);
    }

    AZStd::string UniqueStringContainer::GetUniqueString(AZ::Crc32 stringGroupId
        , const AZStd::string& stringIn
        , AZ::u64 maxStringLength
        , const AZStd::unordered_set<AZStd::string>& forbiddenStrings) const
    {
        StringGroups::const_iterator stringGroupsIter = m_stringGroups.find(stringGroupId);

        // Group for string does not yet exist. It will be unique if a new group that contains only this string is added.
        // String is not any of the forbidden string values.
        if (stringGroupsIter == m_stringGroups.end()
            && forbiddenStrings.find(stringIn) == forbiddenStrings.end()
            && stringIn.length() != 0)
        {
            return stringIn;
        }

        AZStd::string stringOut;
        const AZStd::unordered_set<AZStd::string>& stringGroup = (stringGroupsIter == m_stringGroups.end())? AZStd::unordered_set<AZStd::string>():stringGroupsIter->second;

        // Attempts to append a post-fix value, e.g. "_1" etc., to the original string so it is unique.
        // A unique post-fix index can be found by iterating total number of invalid string plus 1.
        const size_t totalNumInvalidStrings = stringGroup.size() + forbiddenStrings.size() + 1;
        for (size_t nameIndex = 1; nameIndex <= totalNumInvalidStrings; ++nameIndex)
        {
            AZStd::string postFix;
            to_string(postFix, nameIndex);
            postFix = "_" + postFix;

            size_t trimLength = (stringIn.length() + postFix.length()) - maxStringLength;
            if (trimLength > 0)
            {
                stringOut = stringIn.substr(0, stringIn.length() - trimLength) + postFix;
            }
            else
            {
                stringOut = stringIn + postFix;
            }

            if (stringGroup.find(stringOut) == stringGroup.end()
                && forbiddenStrings.find(stringOut) == forbiddenStrings.end())
            {
                break;
            }
        }
        return stringOut;
    }

    bool UniqueStringContainer::IsStringUnique(AZ::Crc32 stringGroupId
        , const AZStd::string& stringIn) const
    {
        StringGroups::const_iterator stringGroupsIter = m_stringGroups.find(stringGroupId);

        // String group does not yet exist. String would be unique if group is created for it.
        if (stringGroupsIter == m_stringGroups.end())
        {
            return true;
        }

        const AZStd::unordered_set<AZStd::string>& stringGroup = stringGroupsIter->second;
        return stringGroup.find(stringIn) == stringGroup.end();
    }

    void UniqueStringContainer::RemoveString(AZ::Crc32 stringGroupId
        , const AZStd::string& stringIn)
    {
        StringGroups::iterator stringGroupsIter = m_stringGroups.find(stringGroupId);
        if (stringGroupsIter == m_stringGroups.end())
        {
            AZ_Warning("AzToolsFramework"
                , false
                , "Could not remove string %s from unrecognized group"
                , stringIn.c_str());
            return;
        }
        stringGroupsIter->second.erase(stringIn);
    }
}
