/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Editor/ActionManagerUtils.h>

#include <AzCore/Settings/SettingsRegistry.h>

namespace AzToolsFramework
{
    static constexpr AZStd::string_view ActionManagerToggleKey = "/O3DE/ActionManager/EnableNewActionManager";

    bool IsNewActionManagerEnabled()
    {
        bool isNewActionManagerEnabled = false;

        // Retrieve new action manager setting
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(isNewActionManagerEnabled, ActionManagerToggleKey);
        }

        return isNewActionManagerEnabled;
    }

} // namespace AzToolsFramework
