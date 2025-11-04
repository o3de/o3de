/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

class QGraphicsItem;

namespace GraphCanvas
{
    using GraphicsEffectId = AZ::EntityId;

    class GraphicsEffectRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = GraphicsEffectId;
        
        virtual QGraphicsItem* AsQGraphicsItem() = 0;

        virtual void PrepareGeometryChange() = 0;
        
        virtual void OnGraphicsEffectCancelled() = 0;

        // Mainly used in clearing the scene which enumerates over these interfaces.
        virtual GraphicsEffectId GetEffectId() const = 0;
    };
    
    using GraphicsEffectRequestBus = AZ::EBus<GraphicsEffectRequests>;
}

DECLARE_EBUS_EXTERN(GraphCanvas::GraphicsEffectRequests);
