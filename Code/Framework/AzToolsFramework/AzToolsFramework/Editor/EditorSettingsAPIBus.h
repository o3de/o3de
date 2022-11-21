/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/any.h>
#include <AzCore/std/string/string.h>

namespace AzToolsFramework
{
    // For Console window background
    enum class ConsoleColorTheme
    {
        Light,
        Dark
    };

    //! Editor Settings API Bus
    class EditorSettingsAPIRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~EditorSettingsAPIRequests() = default;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual AZStd::vector<AZStd::string> BuildSettingsList() = 0;

        using SettingOutcome = AZ::Outcome<AZStd::any, AZStd::string>;

        virtual SettingOutcome GetValue(const AZStd::string_view path) = 0;
        virtual SettingOutcome SetValue(const AZStd::string_view path, const AZStd::any& value) = 0;
        virtual ConsoleColorTheme GetConsoleColorTheme() const = 0;
        virtual AZ::u64 GetMaxNumberOfItemsShownInSearchView() const = 0;
        virtual void SaveSettingsRegistryFile() = 0; 
    };

    using EditorSettingsAPIBus = AZ::EBus<EditorSettingsAPIRequests>;

    //! Allows handlers to be notified when settings are changed to refresh accordingly
    class EditorPreferencesNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Notifies about changes in the Editor Preferences
        virtual void OnEditorPreferencesChanged() {}
    };

    using EditorPreferencesNotificationBus = AZ::EBus<EditorPreferencesNotifications>;
}
