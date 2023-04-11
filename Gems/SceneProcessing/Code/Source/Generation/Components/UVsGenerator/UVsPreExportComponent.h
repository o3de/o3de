/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    class ComponentDescriptor;
}

namespace AZ::SceneGenerationComponents
{
    inline constexpr const char* s_UVsPreExportComponentTypeId = "{64F79C1E-CED6-42A9-8229-6607F788C731}";

    //! This function will be called by the module class to get the descriptor.  Doing it this way saves
    //! it from having to actually see the entire component declaration here, it can all be in the implementation file.
    AZ::ComponentDescriptor* CreateUVsPreExportComponentDescriptor();
} // namespace AZ::SceneGenerationComponents
