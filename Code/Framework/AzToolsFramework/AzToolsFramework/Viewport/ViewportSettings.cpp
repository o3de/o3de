/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Viewport/ViewportSettings.h>

namespace AzToolsFramework
{
    constexpr AZStd::string_view FlipManipulatorAxesTowardsViewSetting = "/Amazon/Preferences/Editor/FlipManipulatorAxesTowardsView";

    bool FlipManipulatorAxesTowardsView()
    {
        return GetRegistry(FlipManipulatorAxesTowardsViewSetting, true);
    }

    void SetFlipManipulatorAxesTowardsView(const bool enabled)
    {
        SetRegistry(FlipManipulatorAxesTowardsViewSetting, enabled);
    }
} // namespace AzToolsFramework
