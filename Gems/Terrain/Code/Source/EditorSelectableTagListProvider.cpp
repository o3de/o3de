/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EditorSelectableTagListProvider.h>
#include <SurfaceData/SurfaceTag.h>

namespace Terrain
{
    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> EditorSelectableTagListProvider::BuildSelectableTagList() const
    {
        AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> availableTags = SurfaceData::SurfaceTag::GetRegisteredTags();

        AZStd::unordered_set<AZ::u32> tagsInUse = AZStd::move(GetSurfaceTagsInUse());

        // Filter out all tags in use from the list of registered tags
        availableTags.erase(std::remove_if(availableTags.begin(), availableTags.end(),
            [&tagsInUse](const auto& tag)-> bool
        {
            return tagsInUse.contains(tag.first);
        }), availableTags.end());

        return AZStd::move(availableTags);
    }
}
