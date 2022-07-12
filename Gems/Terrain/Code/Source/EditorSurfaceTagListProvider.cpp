/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorSurfaceTagListProvider.h>
#include <AzCore/std/sort.h>


namespace Terrain
{
    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> BuildSelectableTagList(const EditorSurfaceTagListProvider* tagListProvider,
        const SurfaceData::SurfaceTag& currentTag)
    {
        AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> availableTags = SurfaceData::SurfaceTag::GetRegisteredTags();

        AZStd::unordered_set<SurfaceData::SurfaceTag> tagsInUse;

        if (tagListProvider)
        {
            tagsInUse = AZStd::move(tagListProvider->GetSurfaceTagsInUse());

            // Filter out all tags in use from the list of registered tags
            AZStd::erase_if(availableTags, [&tagsInUse](const auto& tag)-> bool
            {
                return tagsInUse.contains(SurfaceData::SurfaceTag(tag.first));
            });
        }

        // Insert the current tag back if it was removed via tagsInUse
        availableTags.emplace_back(AZ::u32(currentTag), currentTag.GetDisplayName());

        // Sorting for consistency
        AZStd::sort(availableTags.begin(), availableTags.end(), [](const auto& lhs, const auto& rhs) {return lhs.second < rhs.second; });

        return AZStd::move(availableTags);
    }
}
