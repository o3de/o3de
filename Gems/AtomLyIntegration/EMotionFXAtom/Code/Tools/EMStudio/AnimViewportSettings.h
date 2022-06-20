/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Viewport/ModularViewportCameraController.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace EMStudio::ViewportUtil
{
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

    float CameraRotateSmoothness();
    float CameraTranslateSmoothness();
    bool CameraRotateSmoothingEnabled();
    bool CameraTranslateSmoothingEnabled();
    float CameraDefaultOrbitDistance();

    AzFramework::TranslateCameraInputChannelIds TranslateCameraInputChannelIds();
    AzFramework::InputChannelId RotateCameraInputChannelId();
    AzFramework::InputChannelId OrbitCameraInputChannelId();
    AzFramework::InputChannelId OrbitLookCameraInputChannelId();
    AzFramework::InputChannelId OrbitDollyCameraInputChannelId();
    AzFramework::InputChannelId PanCameraInputChannelId();
} // namespace EMStudio::ViewportUtil
