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
#include <AzCore/std/containers/unordered_set.h>
#include <SurfaceData/SurfaceTag.h>

namespace Terrain
{
    //! Interface for a class providing information about surface tags available for selecting in Editor components.
    class EditorSurfaceTagListProvider
    {
    public:
        //! Returns a set of all surface tags currently in use that won't be available for selecting.
        virtual AZStd::unordered_set<SurfaceData::SurfaceTag> GetSurfaceTagsInUse() const = 0;
    };

    //! Returns a list of available tags to be selected in the component.
    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> BuildSelectableTagList(
        const EditorSurfaceTagListProvider* tagListProvider,
        const SurfaceData::SurfaceTag& currentTag);
}
