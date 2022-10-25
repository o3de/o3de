/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LyShineEditorSettings.h"

#include <AzCore/Settings/SettingsRegistry.h>

namespace LyShine
{

    namespace Internal
    {
        template<typename T>
        void SetRegistry(const AZStd::string_view setting, T&& value)
        {
            if (auto* registry = AZ::SettingsRegistry::Get())
            {
                registry->Set(setting, AZStd::forward<T>(value));
            }
        }

        template<typename T>
        AZStd::remove_cvref_t<T> GetRegistry(const AZStd::string_view setting, T&& defaultValue)
        {
            AZStd::remove_cvref_t<T> value = AZStd::forward<T>(defaultValue);
            if (const auto* registry = AZ::SettingsRegistry::Get())
            {
                T potentialValue;
                if (registry->Get(potentialValue, setting))
                {
                    value = AZStd::move(potentialValue);
                }
            }

            return value;
        }
    }

    ViewportInteraction::InteractionMode GetInteractionMode()
    {
        return static_cast<ViewportInteraction::InteractionMode>(
            Internal::GetRegistry(InteractionModeSetting, aznumeric_cast<AZ::u64>(ViewportInteraction::InteractionMode::SELECTION)));
    }

    void SetInteractionMode(ViewportInteraction::InteractionMode mode)
    {
        Internal::SetRegistry(InteractionModeSetting, aznumeric_cast<AZ::u64>(mode));
    }

    ViewportInteraction::CoordinateSystem GetCoordinateSystem()
    {
        return static_cast<ViewportInteraction::CoordinateSystem>(
            Internal::GetRegistry(CoordinateSystemSetting, aznumeric_cast<AZ::u64>(ViewportInteraction::CoordinateSystem::LOCAL)));
    }

    void SetCoordinateSystem(ViewportInteraction::CoordinateSystem system)
    {
        Internal::SetRegistry(CoordinateSystemSetting, aznumeric_cast<AZ::u64>(system));
    }
}
