/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #pragma once
 
#include <AzCore/EBus/EBus.h>
#include <Rendering/HairGlobalSettings.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            class HairGlobalSettingsNotifications
                : public AZ::EBusTraits
            {
            public:
                // EBusTraits
                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
                static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

                virtual void OnHairGlobalSettingsChanged(const HairGlobalSettings& hairGlobalSettings) = 0;
            };

            typedef AZ::EBus<HairGlobalSettingsNotifications> HairGlobalSettingsNotificationBus;

            class HairGlobalSettingsRequests
                : public AZ::EBusTraits
            {
            public:
                // EBusTraits
                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
                static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

                virtual void GetHairGlobalSettings(HairGlobalSettings& hairGlobalSettings) = 0;
                virtual void SetHairGlobalSettings(const HairGlobalSettings& hairGlobalSettings) = 0;
            };

            typedef AZ::EBus<HairGlobalSettingsRequests> HairGlobalSettingsRequestBus;
        } //namespace Hair
    } // namespace Render
} // namespace AZ
