/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Settings/SettingsRegistry.h>

//! @file - contains a few utility functions for dealing with the settings registry.

namespace AzToolsFramework
{
    //! Set a value in the Settings Registry.
    //! setting needs to be a fully formed path, like "O3DE/Editor/General/Something"
    template<typename T>
    void SetRegistry(const AZStd::string_view setting, T&& value)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(setting, AZStd::forward<T>(value));
        }
    }

    //! Get a value from the Settings Registry.
    //! setting needs to be a fully formed path, like "O3DE/Editor/General/Something"
    //! defaultValue is returned if the setting is not found.
    template<typename T>
    AZStd::remove_cvref_t<T> GetRegistry(const AZStd::string_view setting, T&& defaultValue)
    {
        AZStd::remove_cvref_t<T> value = AZStd::forward<T>(defaultValue);
        if (const auto* registry = AZ::SettingsRegistry::Get())
        {
            AZStd::remove_cvref_t<T> potentialValue;
            if (registry->Get(potentialValue, setting))
            {
                value = AZStd::move(potentialValue);
            }
        }
        return value;
    }

    //! Clear a value from the Settings Registry.
    //! setting needs to be a fully formed path, like "O3DE/Editor/General/Something"
    inline void ClearRegistry(const AZStd::string_view setting)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Remove(setting);
        }
    }
}
