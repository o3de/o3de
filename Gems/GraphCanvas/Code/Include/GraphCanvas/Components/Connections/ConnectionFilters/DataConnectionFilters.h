/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_set.h>

#include <GraphCanvas/Components/Connections/ConnectionFilters/ConnectionFilterBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>

namespace GraphCanvas
{    
    class DataSlotTypeFilter
        : public ConnectionFilter
    {
        friend class SlotConnectionFilter;
    public:
        AZ_RTTI(DataSlotTypeFilter, "{D625AE2F-5F71-461E-A553-554402A824BF}", ConnectionFilter);
        AZ_CLASS_ALLOCATOR(DataSlotTypeFilter, AZ::SystemAllocator);

        DataSlotTypeFilter()
        {
        }
        
        bool CanConnectWith(const Endpoint& endpoint, const ConnectionMoveType& moveType) const override
        {
            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);            

            DataSlotType sourceType = DataSlotType::Unknown;
            DataSlotType targetType = DataSlotType::Unknown;

            Endpoint sourceEndpoint;
            Endpoint targetEndpoint;

            // We want to look at the connection we are trying to create.
            // Since this runs on the target of the connections.
            // We need to look at this from the perspective of the thing asking us for the connection.
            ConnectionType connectionType = CT_None;
            SlotRequestBus::EventResult(connectionType, endpoint.GetSlotId(), &SlotRequests::GetConnectionType);

            if (connectionType == CT_Input)
            {
                sourceEndpoint.m_slotId = GetEntityId();
                SlotRequestBus::EventResult(sourceEndpoint.m_nodeId, GetEntityId(), &SlotRequests::GetNode);
                
                targetEndpoint = endpoint;
            }
            else if (connectionType == CT_Output)
            {
                sourceEndpoint = endpoint;

                targetEndpoint.m_slotId = GetEntityId();
                SlotRequestBus::EventResult(targetEndpoint.m_nodeId, GetEntityId(), &SlotRequests::GetNode);
            }
            else
            {
                return false;
            }

            DataSlotRequestBus::EventResult(sourceType, sourceEndpoint.GetSlotId(), &DataSlotRequests::GetDataSlotType);
            DataSlotRequestBus::EventResult(targetType, targetEndpoint.GetSlotId(), &DataSlotRequests::GetDataSlotType);

            bool acceptConnection = false;

            // We don't want to allow any connections to a reference pin.
            // But we do want to allow connections from a reference pin.
            if (sourceType == DataSlotType::Reference)
            {
                if (targetType == DataSlotType::Reference)
                {
                    acceptConnection = true;
                }                
            }
            else if (sourceType == DataSlotType::Value)
            {
                if (targetType == DataSlotType::Value)
                {
                    acceptConnection = true;
                }
            }

            if (!acceptConnection)
            {
                if (moveType == ConnectionMoveType::Source)
                {
                    if (targetType == DataSlotType::Reference)
                    {
                        bool hasConnections = false;
                        SlotRequestBus::EventResult(hasConnections, sourceEndpoint.GetSlotId(), &SlotRequests::HasConnections);

                        // Only want to try to convert to references when we have no connections
                        if (!hasConnections)
                        {
                            DataSlotRequestBus::EventResult(acceptConnection, sourceEndpoint.GetSlotId(), &DataSlotRequests::CanConvertToReference, false);
                        }
                    }
                    else if (targetType == DataSlotType::Value)
                    {
                        DataSlotRequestBus::EventResult(acceptConnection, sourceEndpoint.GetSlotId(), &DataSlotRequests::CanConvertToValue);
                    }
                }
                else if (moveType == ConnectionMoveType::Target)
                {
                    if (sourceType == DataSlotType::Reference)
                    {
                        bool hasConnections = false;
                        SlotRequestBus::EventResult(hasConnections, targetEndpoint.GetSlotId(), &SlotRequests::HasConnections);

                        // Only want to try to convert to references when we have no connections
                        if (!hasConnections)
                        {
                            DataSlotRequestBus::EventResult(acceptConnection, targetEndpoint.GetSlotId(), &DataSlotRequests::CanConvertToReference, false);
                        }
                    }
                    else if (sourceType == DataSlotType::Value)
                    {
                        DataSlotRequestBus::EventResult(acceptConnection, targetEndpoint.GetSlotId(), &DataSlotRequests::CanConvertToValue);
                    }
                }
            }

            return acceptConnection;
        }
    };
}
