/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#include <AzCore/std/string/string_view.h>

namespace AzToolsFramework::Prefab
{
    AZStd::string_view GetHotReloadToggleKey();
    
    //! Checks if hot reloading for prefab files is enabled.
    AZTF_API bool IsHotReloadingEnabled();

    //! Checks if override visualization and authoring workflows in Entity Outliner are enabled.
    AZTF_API bool IsOutlinerOverrideManagementEnabled();

    //! Checks if override visualization and authoring workflows in DPE Entity Inspector are enabled.
    //! Note that this feature does not work if DPE flag is disabled.
    AZTF_API bool IsInspectorOverrideManagementEnabled();

} // namespace AzToolsFramework::Prefab
