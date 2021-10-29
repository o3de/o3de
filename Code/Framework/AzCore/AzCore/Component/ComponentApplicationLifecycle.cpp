/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>

namespace AZ::ComponentApplicationLifecycle
{
    bool ValidateEvent(AZ::SettingsRegistryInterface& settingsRegistry, AZStd::string_view eventName)
    {
        using FixedValueString = SettingsRegistryInterface::FixedValueString;
        using Type = SettingsRegistryInterface::Type;
        FixedValueString eventRegistrationKey{ ApplicationLifecycleEventRegistrationKey };
        eventRegistrationKey += '/';
        eventRegistrationKey += eventName;
        return settingsRegistry.GetType(eventRegistrationKey) == Type::Object;
    }

    bool SignalEvent(AZ::SettingsRegistryInterface& settingsRegistry, AZStd::string_view eventName, AZStd::string_view eventValue)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        using Format = AZ::SettingsRegistryInterface::Format;

        if (!ValidateEvent(settingsRegistry, eventName))
        {
            AZ_Warning("ComponentApplicationLifecycle", false, R"(Cannot signal event %.*s. Name does is not a field of object "%.*s".)"
                R"( Please make sure the entry exists in the '<engine-root>/Registry/application_lifecycle_events.setreg")"
                " or in *.setreg within the project", AZ_STRING_ARG(eventName), AZ_STRING_ARG(ApplicationLifecycleEventRegistrationKey));
            return false;
        }
        auto eventRegistrationKey = FixedValueString::format("%.*s/%.*s", AZ_STRING_ARG(ApplicationLifecycleEventRegistrationKey),
            AZ_STRING_ARG(eventName));

        return settingsRegistry.MergeSettings(eventValue, Format::JsonMergePatch, eventRegistrationKey);
    }

    bool RegisterEvent(AZ::SettingsRegistryInterface& settingsRegistry, AZStd::string_view eventName)
    {
        using FixedValueString = SettingsRegistryInterface::FixedValueString;
        using Format = AZ::SettingsRegistryInterface::Format;

        if (!ValidateEvent(settingsRegistry, eventName))
        {
            FixedValueString eventRegistrationKey{ ApplicationLifecycleEventRegistrationKey };
            eventRegistrationKey += '/';
            eventRegistrationKey += eventName;
            return settingsRegistry.MergeSettings(R"({})", Format::JsonMergePatch, eventRegistrationKey);
        }

        return true;
    }

    bool RegisterHandler(AZ::SettingsRegistryInterface& settingsRegistry, AZ::SettingsRegistryInterface::NotifyEventHandler& handler,
        AZ::SettingsRegistryInterface::NotifyCallback callback, AZStd::string_view eventName)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        using Type = AZ::SettingsRegistryInterface::Type;
        using NotifyEventHandler = AZ::SettingsRegistryInterface::NotifyEventHandler;

        if (!ValidateEvent(settingsRegistry, eventName))
        {
            AZ_Warning("ComponentApplicationLifecycle", false, R"(Cannot register event %.*s. Name does is not a field of object "%.*s".)"
                R"( Please make sure the entry exists in the '<engine-root>/Registry/application_lifecycle_events.setreg")"
                " or in *.setreg within the project", AZ_STRING_ARG(eventName), AZ_STRING_ARG(ApplicationLifecycleEventRegistrationKey));
            return false;
        }
        auto eventNameRegistrationKey = FixedValueString::format("%.*s/%.*s", AZ_STRING_ARG(ApplicationLifecycleEventRegistrationKey),
            AZ_STRING_ARG(eventName));
        auto lifecycleCallback = [callback = AZStd::move(callback), eventNameRegistrationKey](AZStd::string_view path, Type type)
        {
            if (path == eventNameRegistrationKey)
            {
                callback(path, type);
            }
        };

        handler = NotifyEventHandler(AZStd::move(lifecycleCallback));
        settingsRegistry.RegisterNotifier(handler);

        return true;
    }
}
