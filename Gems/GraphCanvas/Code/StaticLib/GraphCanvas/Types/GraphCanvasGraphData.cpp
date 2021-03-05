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
#include <AzCore/Serialization/SerializeContext.h>

#include <GraphCanvas/Types/GraphCanvasGraphData.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Types/Endpoint.h>

namespace GraphCanvas
{
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