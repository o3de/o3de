/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_set.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/Connections/ConnectionFilters/ConnectionFilterBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace GraphCanvas
{
    enum class ConnectionFilterType : AZ::u32
    {
        Include = 0,
        Exclude,
        Invalid
    };
    
    class SlotTypeFilter
        : public ConnectionFilter
    {
        friend class SlotConnectionFilterComponent;
    public:
        AZ_RTTI(SlotTypeFilter, "{210FB521-041E-4932-BC7F-C91079125F68}", ConnectionFilter);
        AZ_CLASS_ALLOCATOR(SlotTypeFilter, AZ::SystemAllocator);

        SlotTypeFilter()
            : m_filterType(ConnectionFilterType::Invalid)
        {
        }

        SlotTypeFilter(ConnectionFilterType filterType)
            : m_filterType(filterType)
        {
        }
        
        void AddSlotType(SlotType slotType)
        {
            m_slotTypes.insert(slotType);
        }
        
        bool CanConnectWith(const Endpoint& endpoint, const ConnectionMoveType& moveType) const override
        {
            AZ_UNUSED(moveType);

            SlotType connectingSlotType = SlotTypes::Invalid;
            SlotRequestBus::EventResult(connectingSlotType, endpoint.GetSlotId(), &SlotRequests::GetSlotType);
            AZ_Assert(connectingSlotType != SlotGroups::Invalid, "Slot %s is in an invalid slot type. Connections to it are disabled", endpoint.GetSlotId().ToString().c_str());
            
            bool canConnect = false;
            
            if (connectingSlotType != SlotTypes::Invalid)
            {
                bool isInFilter = m_slotTypes.count(connectingSlotType) != 0;
                
                switch (m_filterType)
                {
                case ConnectionFilterType::Include:
                    canConnect = isInFilter;
                    break;
                case ConnectionFilterType::Exclude:
                    canConnect = !isInFilter;
                    break;
                }
            }
            
            return canConnect;
        }
        
    private:
    
        AZStd::unordered_set<SlotType> m_slotTypes;
    
        ConnectionFilterType m_filterType;
    };
    
    class ConnectionTypeFilter
        : public ConnectionFilter
    {
        friend class SlotConnectionFilterComponent;
    public:
        AZ_RTTI(ConnectionTypeFilter, "{57D65203-51AB-47A8-A7D2-248AFF92E058}", ConnectionFilter);
        AZ_CLASS_ALLOCATOR(ConnectionTypeFilter, AZ::SystemAllocator);

        ConnectionTypeFilter()
            : m_filterType(ConnectionFilterType::Invalid)
        {
        }

        ConnectionTypeFilter(ConnectionFilterType filterType)
            : m_filterType(filterType)
        {
        }
        
        void AddConnectionType(ConnectionType connectionType)
        {
            m_connectionTypes.insert(connectionType);
        }
        
        bool CanConnectWith(const Endpoint& endpoint, const ConnectionMoveType& moveType) const override
        {
            AZ_UNUSED(moveType);

            ConnectionType connectionType = ConnectionType::CT_Invalid;
            SlotRequestBus::EventResult(connectionType, endpoint.GetSlotId(), &SlotRequests::GetConnectionType);
            AZ_Assert(connectionType != ConnectionType::CT_Invalid, "Slot %s is in an invalid slot type. Connections to it are disabled", endpoint.GetSlotId().ToString().c_str())
            
            bool canConnect = false;
            
            if (connectionType != ConnectionType::CT_Invalid)
            {            
                bool isInFilter = m_connectionTypes.count(connectionType) != 0;
                
                switch (m_filterType)
                {
                case ConnectionFilterType::Include:
                    canConnect = isInFilter;
                    break;
                case ConnectionFilterType::Exclude:
                    canConnect = !isInFilter;
                    break;
                }
            }
            
            return canConnect;
        }
        
    private:
   
        AZStd::unordered_set<ConnectionType> m_connectionTypes;
        ConnectionFilterType m_filterType;
    };
}
