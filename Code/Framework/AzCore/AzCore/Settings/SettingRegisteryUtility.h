/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ::SettingsRegistryUtils
{
    template<typename T>
    inline void SetRegistry(SettingsRegistryInterface* registry, const AZStd::string_view setting, T&& value)
    {
        if (registry)
        {
            registry->Set(setting, AZStd::forward<T>(value));
        }
    }

    template<typename T>
    inline AZStd::remove_cvref_t<T> GetRegistry(SettingsRegistryInterface* registry, const AZStd::string_view setting, T&& defaultValue)
    {
        AZStd::remove_cvref_t<T> value = AZStd::forward<T>(defaultValue);
        if (registry)
        {
            T potentialValue;
            if (registry->Get(potentialValue, setting))
            {
                value = AZStd::move(potentialValue);
            }
        }

        return value;
    }
} // namespace AZ::SettingsRegistryUtils
