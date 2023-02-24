/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>

namespace AzToolsFramework::Prefab
{
    extern const AZStd::string_view EnablePrefabOverridesUxKey;
    extern const AZStd::string_view InspectorOverrideManagementKey;
    extern const AZStd::string_view HotReloadToggleKey;

    bool IsHotReloadingEnabled();
    bool IsPrefabOverridesUxEnabled();
    bool IsInspectorOverrideManagementEnabled();

} // namespace AzToolsFramework::Prefab
