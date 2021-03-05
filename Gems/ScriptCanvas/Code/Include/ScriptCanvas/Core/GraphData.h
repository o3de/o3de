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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_map.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptEvents/ScriptEventsAsset.h>

namespace AZ
{
    class Entity;
}

namespace ScriptCanvas
{
    //! Structure for maintaining GraphData
    struct GraphData
    {
        AZ_TYPE_INFO(GraphData, "{ADCB5EB5-8D3F-42ED-8F65-EAB58A82C381}");
        AZ_CLASS_ALLOCATOR(GraphData, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        GraphData() = default;
        GraphData(const GraphData&) = default;
        GraphData& operator=(const GraphData&) = default;
        GraphData(GraphData&&);
        GraphData& operator=(GraphData&&);

        void BuildEndpointMap();
        void Clear(bool deleteData = false);
        void LoadDependentAssets();

        using NodeContainer = AZStd::unordered_set<AZ::Entity*>;
        using ConnectionContainer = AZStd::vector<AZ::Entity*>;
        using DependentScriptEvent = AZStd::vector<AZStd::pair<AZ::EntityId, ScriptEvents::ScriptEventsAssetPtr>>;

        NodeContainer m_nodes;
        ConnectionContainer m_connections;
        DependentScriptEvent m_scriptEventAssets;

        using DependentAssets = AZStd::unordered_map<AZ::Data::AssetId, AZStd::pair<AZ::EntityId, AZ::Data::AssetType>>; // DEPRECATED
        DependentAssets m_dependentAssets; // DEPRECATED

        // An endpoint(NodeId, SlotId Pair) is represents one end of a potential connection
        // The endpoint map is lookup table for all endpoints connected on the opposite end of the key value endpoint
        AZStd::unordered_multimap<Endpoint, Endpoint> m_endpointMap; ///< Endpoint map built at edit time based on active connections

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    };
}
