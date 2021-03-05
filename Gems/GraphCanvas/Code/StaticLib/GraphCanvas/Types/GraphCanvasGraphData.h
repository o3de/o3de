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

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>

#include <GraphCanvas/Types/Endpoint.h>

namespace GraphCanvas
{
    //! Data structure that fully represents a GraphCanvas scene
    //! This structure can be used to create new scenes as well as serialize scenes to streams
    struct GraphData
    {
        AZ_TYPE_INFO(GraphData, "{B2E32DB8-B436-41D0-8DF4-98515D936653}");
        AZ_CLASS_ALLOCATOR(GraphData, AZ::SystemAllocator, 0);

        void Clear();
        void CollectItemIds(AZStd::unordered_set<AZ::EntityId>& itemIds) const;
        void CollectEntities(AZStd::unordered_set<AZ::Entity*>& entities) const;

        AZStd::unordered_set<AZ::Entity*> m_nodes;
        AZStd::unordered_set<AZ::Entity*> m_connections;
        AZStd::unordered_set<AZ::Entity*> m_bookmarkAnchors;
        AZStd::any m_userData; ///< AZStd::any object which can be used to serialize any type of user data(i.e could be used by another canvas system for serializing out its data).

        AZStd::unordered_multimap<Endpoint, Endpoint> m_endpointMap; ///< Endpoint map built at edit time based on active connections
    };
} // namespace GraphCanvas