/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

class QGraphicsLayout;
class QGraphicsLayoutItem;
class QGraphicsLinearLayout;
class QGraphicsWidget;

namespace GraphCanvas
{
    static const AZ::Crc32 NodeLayoutServiceCrc = AZ_CRC_CE("GraphCanvas_NodeLayoutService");
    static const AZ::Crc32 NodeSlotsServiceCrc = AZ_CRC_CE("GraphCanvas_NodeSlotsService");
    static const AZ::Crc32 NodeLayoutSupportServiceCrc = AZ_CRC_CE("GraphCanvas_NodeLayoutSupportService");

    //! NodeLayoutRequests
    //! Requests that are serviced by a node layout implementation.
    class NodeLayoutRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        
        //! Obtain the layout component as a \code QGraphicsLayout*. 
        virtual QGraphicsLayout* GetLayout() { return{}; }
    };

    using NodeLayoutRequestBus = AZ::EBus<NodeLayoutRequests>;

    //! NodeSlotRequestBus
    //! Used for making requests of a particular slot.
    class NodeSlotsRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual QGraphicsLayoutItem* GetGraphicsLayoutItem() = 0;

        virtual QGraphicsLinearLayout* GetLinearLayout(const SlotGroup& slotGroup) = 0;

        virtual QGraphicsWidget* GetSpacer(const SlotGroup& slotGroup) = 0;

    };

    using NodeSlotsRequestBus = AZ::EBus<NodeSlotsRequests>;
}
