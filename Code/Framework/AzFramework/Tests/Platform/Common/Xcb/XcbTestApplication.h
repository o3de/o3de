/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Application/Application.h>

namespace AzFramework
{
    class XcbTestApplication
        : public Application
    {
    public:
        XcbTestApplication(AZ::u64 enabledGamepadsCount, bool keyboardEnabled, bool motionEnabled, bool mouseEnabled, bool touchEnabled, bool virtualKeyboardEnabled)
        {
            auto* settingsRegistry = AZ::SettingsRegistry::Get();
            settingsRegistry->Set("/O3DE/InputSystem/GamepadsEnabled", enabledGamepadsCount);
            settingsRegistry->Set("/O3DE/InputSystem/KeyboardEnabled", keyboardEnabled);
            settingsRegistry->Set("/O3DE/InputSystem/MotionEnabled", motionEnabled);
            settingsRegistry->Set("/O3DE/InputSystem/MouseEnabled", mouseEnabled);
            settingsRegistry->Set("/O3DE/InputSystem/TouchEnabled", touchEnabled);
            settingsRegistry->Set("/O3DE/InputSystem/VirtualKeyboardEnabled", virtualKeyboardEnabled);
        }

        void Start(const Descriptor& descriptor = {}, const StartupParameters& startupParameters = {}) override
        {
            Application::Start(descriptor, startupParameters);
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }
    };
} // namespace AzFramework
