/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzToolsFramework::Prefab
{
    //! Checks if hot reloading for prefab files is enabled.
    bool IsHotReloadingEnabled();

    //! Checks if override visualization and authoring workflows are enabled in Entity Outliner.
    bool IsOutlinerOverrideManagementEnabled();

    //! Checks if override visualization and authoring workflows are enabled in DPE Entity Inspector.
    //! Note that this feature does not work if DPE flag is disabled.
    bool IsInspectorOverrideManagementEnabled();

} // namespace AzToolsFramework::Prefab
