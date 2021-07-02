/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <precompiled.h>

#include <AzCore/Component/ComponentApplicationBus.h>

#include <Components/Slots/SlotConnectionFilterComponent.h>

#include <GraphCanvas/Components/Connections/ConnectionFilters/ConnectionFilters.h>
#include <GraphCanvas/Components/Connections/ConnectionFilters/DataConnectionFilters.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>

namespace GraphCanvas
{
    //////////////////////////////////
    // SlotConnectionFilterComponent
    //////////////////////////////////

    void SlotConnectionFilterComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ConnectionFilter>()
            ->Version(1)
        ;

        serializeContext->Class<SlotTypeFilter, ConnectionFilter>()
            ->Version(2)
            ->Field("FilterType", &SlotTypeFilter::m_filterType)
            ->Field("Types", &SlotTypeFilter::m_slotTypes)
            ;

        serializeContext->Class<ConnectionTypeFilter, ConnectionFilter>()
            ->Version(1)
            ->Field("FilterType", &ConnectionTypeFilter::m_filterType)
            ->Field("Types", &ConnectionTypeFilter::m_connectionTypes)
            ;

        serializeContext->Class<SlotConnectionFilterComponent, AZ::Component>()
            ->Version(2)
            ->Field("m_filterSlotGroups", &SlotConnectionFilterComponent::m_filters)
            ;

        serializeContext->Class<DataSlotTypeFilter>()
            ->Version(1)
            ;
    }

    SlotConnectionFilterComponent::SlotConnectionFilterComponent()
    {
    }

    SlotConnectionFilterComponent::~SlotConnectionFilterComponent()
    {
        for (ConnectionFilter* filter : m_filters)
        {
            delete filter;
        }

        m_filters.clear();
    }

    void SlotConnectionFilterComponent::Activate()
    {
        ConnectionFilterRequestBus::Handler::BusConnect(GetEntityId());

        for (ConnectionFilter* filter : m_filters)
        {
            filter->SetEntityId(GetEntityId());
        }
    }

    void SlotConnectionFilterComponent::Deactivate()
    {
        ConnectionFilterRequestBus::Handler::BusDisconnect();
    }

    void SlotConnectionFilterComponent::AddFilter(ConnectionFilter* filter)
    {
        filter->SetEntityId(GetEntityId());
        m_filters.emplace_back(filter);
    }

    bool SlotConnectionFilterComponent::CanConnectWith(const Endpoint& endpoint, const ConnectionMoveType& moveType) const
    {
        if (GetEntityId() == endpoint.GetSlotId())
        {
            return false;
        }

        if (SlotRequestBus::FindFirstHandler(endpoint.GetSlotId()) == nullptr)
        {
            // Unable to find the other slot's entity.
            return false;
        }

        bool isConnected = false;
        SlotRequestBus::EventResult(isConnected, GetEntityId(), &SlotRequests::IsConnectedTo, endpoint);

        if (isConnected)
        {
            // Already connected
            return false;
        }

        return AZStd::all_of(m_filters.begin(), m_filters.end(), 
            [&](ConnectionFilter* filter) 
            { 
                return filter->CanConnectWith(endpoint, moveType);
            }
        );
    }
}
