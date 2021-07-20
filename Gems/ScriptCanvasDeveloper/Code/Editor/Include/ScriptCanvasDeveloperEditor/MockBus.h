/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace ScriptCanvasDeveloper
{
    namespace Nodes
    {
        class MockDescriptorRequests : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::EntityId;
            
            virtual AZ::EntityId GetGraphCanvasNodeId() const = 0;
        };
        
        using MockDescriptorRequestBus = AZ::EBus<MockDescriptorRequests>;
        
        class MockDescriptorNotifications : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::EntityId;
            
            virtual void OnGraphCanvasNodeSetup(const AZ::EntityId& graphCanvasNodeId) = 0;
        };
        
        using MockDescriptorNotificationBus = AZ::EBus<MockDescriptorNotifications>;
    }
}
