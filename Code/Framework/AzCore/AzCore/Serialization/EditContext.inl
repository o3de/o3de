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

#include <AzCore/RTTI/AttributeReader.h>

namespace AZ
{
    namespace Edit
    {
        template <typename TagContainer>
        bool SystemComponentTagsMatchesAtLeastOneTag(
            const AZ::SerializeContext::ClassData* classData,
            const TagContainer& requiredTags,
            bool defaultVal)
        {
            if (!classData)
            {
                return defaultVal;
            }
    
            AZ::Edit::Attribute* attribute = FindAttribute(AZ::Edit::Attributes::SystemComponentTags, classData->m_attributes);
            if (!attribute)
            {
                return defaultVal;
            }

            if (requiredTags.empty())
            {
                return false;
            }

            AZStd::vector<AZ::Crc32> tags;
            AZ::AttributeReader reader(nullptr, attribute);
            AZ::Crc32 tag;
            if (reader.Read<AZ::Crc32>(tag))
            {
                tags.emplace_back(tag);
            }
            else
            {
                AZ_Verify(reader.Read<AZStd::vector<AZ::Crc32>>(tags), "Attribute \"AZ::Edit::Attributes::SystemComponentTags\" must be of type AZ::Crc32 or AZStd::vector<AZ::Crc32>");
            }
    
            for (const AZ::Crc32& requiredTag : requiredTags)
            {
                if (AZStd::find(tags.begin(), tags.end(), requiredTag) != tags.end())
                {
                    return true;
                }
            }
    
            return false;
        }
    } // namespace Edit
} // namespace AZ
