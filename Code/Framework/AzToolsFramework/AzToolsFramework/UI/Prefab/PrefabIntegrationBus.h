/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabInstanceContainerNotifications
            : public AZ::EBusTraits
        {
        public:
            virtual ~PrefabInstanceContainerNotifications() = default;

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            virtual void OnPrefabComponentActivate(AZ::EntityId entityId) = 0;
            virtual void OnPrefabComponentDeactivate(AZ::EntityId entityId) = 0;
        };
        using PrefabInstanceContainerNotificationBus = AZ::EBus<PrefabInstanceContainerNotifications>;
    }
}
