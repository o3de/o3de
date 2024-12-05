/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace Render
    {
        class ShadowingDirectionalLightNotifications : public AZ::EBusTraits
        {
        public:
            // EBusTraits
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            virtual void OnShadowingDirectionalLightChanged(const DirectionalLightFeatureProcessorInterface::LightHandle& handle) = 0;
        };

        typedef AZ::EBus<ShadowingDirectionalLightNotifications> ShadowingDirectionalLightNotificationsBus;
    }
}