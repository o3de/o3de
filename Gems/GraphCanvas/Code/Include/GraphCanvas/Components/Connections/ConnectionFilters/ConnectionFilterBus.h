/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    struct Connectability;

    class ConnectionFilter
    {
    public:
        AZ_RTTI(ConnectionFilter, "{E8319FDC-DDC5-40DD-A601-5E8C41B019A8}");
        virtual ~ConnectionFilter() = default;

        void SetEntityId(const AZ::EntityId& entityId)
        {
            m_entityId = entityId;
        }

        const AZ::EntityId& GetEntityId() const
        {
            return m_entityId;
        }

        virtual bool CanConnectWith(const Endpoint& endpoint, const ConnectionMoveType& moveType) const = 0;

    private:

        AZ::EntityId m_entityId;
    };

    //! Requests that are serviced by objects that want to filter slot connections based on a set of predicates
    //! connections can either be filtered for inclusion or exclusion
    class ConnectionFilterRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Add a connection filter to the given slot.
        //! Params: ConnectionFilter* the filter to be added, ownership is taken by the Slot.
        virtual void AddFilter(ConnectionFilter*) = 0;
        virtual bool CanConnectWith(const Endpoint& endpoint, const ConnectionMoveType& moveType) const = 0;
    };

    using ConnectionFilterRequestBus = AZ::EBus<ConnectionFilterRequests>;

}
