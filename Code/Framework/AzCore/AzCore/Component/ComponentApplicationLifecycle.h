/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/string/string_view.h>

namespace AZ::ComponentApplicationLifecycle
{
    //! Root Key where lifecycle events should be registered under
    inline constexpr AZStd::string_view ApplicationLifecycleEventRegistrationKey = "/O3DE/Application/LifecycleEvents";


    //! Validates that the event @eventName is stored in the array at ApplicationLifecycleEventRegistrationKey
    //! @param settingsRegistry registry where @eventName will be searched
    //! @param eventName name of key that validated that exists as an element in the ApplicationLifecycleEventRegistrationKey array
    //! @return true if the @eventName was found in the ApplicationLifecycleEventRegistrationKey array
    bool ValidateEvent(AZ::SettingsRegistryInterface& settingsRegistry, AZStd::string_view eventName);

    //! Wrapper around setting a value underneath the ApplicationLifecycleEventRegistrationKey
    //! It validates if the @eventName is is part of the ApplicationLifecycleEventRegistrationKey array 
    //! It then appends the @eventName to the ApplicationLifecycleEventRegistrationKey merges the @eventValue into
    //! the SettingsRegistry at that key
    //! NOTE: This function should only be invoked from ComponentApplication and its derived classes
    //! @param settingsRegistry registry where eventName should be set
    //! @param eventName name of key underneath the ApplicationLifecycleEventRegistrationKey to signal
    //! @param eventValue JSON Object that will be merged into the SettingsRegistry at <ApplicationLifecycleEventRootKey>/<eventName>
    //! @return true if the eventValue was successfully merged at the <ApplicationLifecycleEventRootKey>/<eventName>
    bool SignalEvent(AZ::SettingsRegistryInterface& settingsRegistry, AZStd::string_view eventName, AZStd::string_view eventValue);

    //! Register that the event @eventName is stored in the array at ApplicationLifecycleEventRegistrationKey
    //! @param settingsRegistry registry where @eventName will be searched
    //! @param eventName name of key that will be stored in the ApplicationLifecycleEventRegistrationKey array
    //! @return true if the event passed validation or the eventName was stored in the ApplicationLifecycleEventRegistrationKey array
    bool RegisterEvent(AZ::SettingsRegistryInterface& settingsRegistry, AZStd::string_view eventName);

    //! Wrapper around registering the NotifyEventHandler with the SettingsRegistry for the specified event
    //! It validates if the @eventName is is part of the ApplicationLifecycleEventRegistrationKey array and if
    //! so moves the @callback into @handler and then registers the handler with the SettingsRegistry NotifyEvent
    //! @param settingsRegistry registry where handler will be registered
    //! @param handler handler where callback will be moved into and then registered with the SettingsRegistry
    //!        if the specified @eventName passes validation
    //! @param callback will be moved into the handler if the specified @eventName is valid
    //! @param eventName name of key underneath the ApplicationLifecycleEventRegistrationKey to register
    //! @param autoRegisterEvent automatically register this event if it hasn't been registered yet. This is useful
    //!         when registering a handler before the settings registry has been loaded.
    //! @return true if the handler was registered with the SettingsRegistry NotifyEvent
    bool RegisterHandler(AZ::SettingsRegistryInterface& settingsRegistry, AZ::SettingsRegistryInterface::NotifyEventHandler& handler,
        AZ::SettingsRegistryInterface::NotifyCallback callback, AZStd::string_view eventName, bool autoRegisterEvent = false);
}
