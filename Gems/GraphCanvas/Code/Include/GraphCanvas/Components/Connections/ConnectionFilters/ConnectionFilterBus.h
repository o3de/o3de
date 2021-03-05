/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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