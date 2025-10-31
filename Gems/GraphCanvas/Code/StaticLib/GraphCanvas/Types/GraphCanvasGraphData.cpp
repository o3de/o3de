/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/SerializeContext.h>

#include <GraphCanvas/Types/GraphCanvasGraphData.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Types/Endpoint.h>

namespace GraphCanvas
{
    // Implement the GraphCanvas Endpoint RTTI and functions in a cpp file
    AZ_CLASS_ALLOCATOR_IMPL(Endpoint, AZ::SystemAllocator);
    AZ_TYPE_INFO_WITH_NAME_IMPL(Endpoint, "Endpoint", "{4AF80E61-8E0A-43F3-A560-769C925A113B}");

    void Endpoint::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Endpoint>()
                ->Version(1)
                ->Field("nodeId", &Endpoint::m_nodeId)
                ->Field("slotId", &Endpoint::m_slotId)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Endpoint>("Endpoint", "Endpoint")
                    ->DataElement(0, &Endpoint::m_nodeId, "Node Id", "Node Id portion of endpoint")
                    ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::DontGatherReference | AZ::Edit::SliceFlags::NotPushable)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                    ;
            }
        }
    }
    //////////////
    // GraphData
    //////////////

    void GraphData::Clear()
    {
        m_nodes.clear();
        m_connections.clear();
        m_bookmarkAnchors.clear();
        m_userData.clear();

        m_endpointMap.clear();
    }

    void GraphData::CollectItemIds(AZStd::unordered_set<AZ::EntityId>& itemIds) const
    {
        for (auto&& container : { m_nodes, m_connections, m_bookmarkAnchors })
        {
            for (AZ::Entity* item : container)
            {
                itemIds.insert(item->GetId());
            }
        }
    }

    void GraphData::CollectEntities(AZStd::unordered_set<AZ::Entity*>& entities) const
    {
        for (auto&& container : { m_nodes, m_connections, m_bookmarkAnchors })
        {
            for (AZ::Entity* entity : container)
            {
                entities.insert(entity);
            }
        }
    }
} // namespace GraphCanvas
