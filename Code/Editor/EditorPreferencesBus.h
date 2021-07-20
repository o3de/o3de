/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

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
