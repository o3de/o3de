/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QColor>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    struct ExtenderSlotConfiguration
        : public SlotConfiguration
    {
    public:
        AZ_RTTI(ExtenderSlotConfiguration, "{E60B3B88-6D9E-497D-8F78-9280BCF289F9}", SlotConfiguration);
        AZ_CLASS_ALLOCATOR(ExtenderSlotConfiguration, AZ::SystemAllocator);

        ExtenderId m_extenderId;
    };

    class ExtenderSlotRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void TriggerExtension() = 0;
        virtual Endpoint ExtendForConnectionProposal(const ConnectionId& connectionId, const Endpoint& endpoint) = 0;
    };

    using ExtenderSlotRequestBus = AZ::EBus<ExtenderSlotRequests>;
    
    class ExtenderSlotNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
    };
    
    using ExtenderSlotNotificationBus = AZ::EBus<ExtenderSlotNotifications>;
}
