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
            AZ_Warning("ComponentApplicationLifecycle", false, R"(Cannot signal event %.*s because it is not a field of object "%.*s".)"
                R"( Please make sure the entry exists in the '<engine-root>/Registry/application_lifecycle_events.setreg")"
                " or in *.setreg within the project", AZ_STRING_ARG(eventName), AZ_STRING_ARG(ApplicationLifecycleEventRegistrationKey));
            return false;
        }
        // The Settings Registry key used to signal the event is a transient runtime key which is separate from the registration key
        auto eventSignalKey = FixedValueString::format("%.*s/%.*s", AZ_STRING_ARG(ApplicationLifecycleEventSignalKey),
            AZ_STRING_ARG(eventName));

        return static_cast<bool>(settingsRegistry.MergeSettings(eventValue, Format::JsonMergePatch, eventSignalKey));
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
            return static_cast<bool>(settingsRegistry.MergeSettings(R"({})", Format::JsonMergePatch, eventRegistrationKey));
        }

        return true;
    }

    bool RegisterHandler(AZ::SettingsRegistryInterface& settingsRegistry, AZ::SettingsRegistryInterface::NotifyEventHandler& handler,
        AZ::SettingsRegistryInterface::NotifyCallback callback, AZStd::string_view eventName, bool autoRegisterEvent)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        using NotifyEventHandler = AZ::SettingsRegistryInterface::NotifyEventHandler;

        // Some systems may attempt to register a handler before the settings registry has been loaded
        // If so, this flag lets them automatically register an event if it hasn't yet been registered.
        // RegisterEvent calls validate event.
        if ((!autoRegisterEvent && !ValidateEvent(settingsRegistry, eventName)) ||
            (autoRegisterEvent && !RegisterEvent(settingsRegistry, eventName)))
        {
            AZ_Warning(
                "ComponentApplicationLifecycle", false,
                R"(Cannot register event %.*s. Name is not a field of object "%.*s".)"
                R"( Please make sure the entry exists in the '<engine-root>/Registry/application_lifecycle_events.setreg")"
                " or in *.setreg within the project", AZ_STRING_ARG(eventName), AZ_STRING_ARG(ApplicationLifecycleEventRegistrationKey));
            return false;
        }

        // The signal key is used for invoking the handler
        auto lifecycleCallback = [callback = AZStd::move(callback),
            eventNameSignalKey = FixedValueString::format("%.*s/%.*s",
                AZ_STRING_ARG(ApplicationLifecycleEventSignalKey), AZ_STRING_ARG(eventName))](
            const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            if (notifyEventArgs.m_jsonKeyPath == eventNameSignalKey)
            {
                callback(notifyEventArgs);
            }
        };

        handler = NotifyEventHandler(AZStd::move(lifecycleCallback));
        settingsRegistry.RegisterNotifier(handler);

        return true;
    }
}
