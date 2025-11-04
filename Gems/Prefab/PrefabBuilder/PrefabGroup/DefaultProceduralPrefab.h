
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <PrefabGroup/PrefabGroup.h>
#include <PrefabGroup/PrefabGroupBus.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <AzCore/Component/EntityId.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>

namespace AZ::SceneAPI
{
    namespace SceneData
    {
        class MeshGroup;
    }

    using PrefabGroup = AZ::SceneAPI::SceneData::PrefabGroup;
    using Scene = AZ::SceneAPI::Containers::Scene;

    //! Handler for the Prefab Group event logic
    class DefaultProceduralPrefabGroup
        : protected PrefabGroupEventBus::Handler
    {
    public:
        AZ_RTTI(DefaultProceduralPrefabGroup, "{6BAAB306-01EE-42E8-AAFE-C9EE0BF4CFDF}");

        DefaultProceduralPrefabGroup();
        virtual ~DefaultProceduralPrefabGroup();

        static void Reflect(ReflectContext* context);

        // PrefabGroupEventBus::Handler
        AZStd::optional<ManifestUpdates> GeneratePrefabGroupManifestUpdates(const Scene& scene) const override;
        AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>> GenerateDefaultPrefabMeshGroups(const Scene& scene) const override;

    protected:
        // this stores the data related with nodes that will translate to entities in the prefab group
        struct NodeDataForEntity
        {
            Containers::SceneGraph::NodeIndex m_meshIndex = {};
            Containers::SceneGraph::NodeIndex m_transformIndex = {};
            Containers::SceneGraph::NodeIndex m_propertyMapIndex = {};
        };

        using NodeDataMapEntry = AZStd::pair<Containers::SceneGraph::NodeIndex, NodeDataForEntity>;
        using NodeDataMap = AZStd::unordered_map<Containers::SceneGraph::NodeIndex, NodeDataForEntity>; // NodeIndex -> NodeDataForEntity
        using ManifestUpdates = AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>>;

        using NodeEntityMap = AZStd::unordered_map<Containers::SceneGraph::NodeIndex, AZStd::pair<AZ::EntityId, AzToolsFramework::Prefab::EntityAlias>>;
        using EntityIdMap = AZStd::unordered_map<AZ::EntityId, AzToolsFramework::Prefab::EntityAlias>;

        AZStd::shared_ptr<SceneData::MeshGroup> BuildMeshGroupForNode(
            const Scene& scene,
            const NodeDataForEntity& nodeData,
            const NodeDataMap& nodeDataMap) const;

        NodeDataMap CalculateNodeDataMap(const Containers::Scene& scene) const;

        bool AddEditorMaterialComponent(
            const AZ::EntityId& entityId,
            const DataTypes::ICustomPropertyData& propertyData) const;

        bool AddEditorMeshComponent(
            const AZ::EntityId& entityId,
            const AZStd::string& relativeSourcePath,
            const AZStd::string& meshGroupName,
            const AZStd::string& sourceFileExtension) const;

        bool CreateMeshGroupAndComponents(
            ManifestUpdates& manifestUpdates,
            AZ::EntityId entityId,
            const NodeDataForEntity& nodeData,
            const NodeDataMap& nodeDataMap,
            const Containers::Scene& scene,
            const AZStd::string& relativeSourcePath) const;

        NodeEntityMap CreateNodeEntityMap(
            ManifestUpdates& manifestUpdates,
            const NodeDataMap& nodeDataMap,
            const Containers::Scene& scene,
            const AZStd::string& relativeSourcePath) const;

        EntityIdMap FixUpEntityParenting(
            const NodeEntityMap& nodeEntityMap,
            const Containers::SceneGraph& graph,
            const NodeDataMap& nodeDataMap)  const;

        bool CreatePrefabGroupManifestUpdates(
            ManifestUpdates& manifestUpdates,
            const Containers::Scene& scene,
            const EntityIdMap& entities,
            const AZStd::string& filenameOnly,
            const AZStd::string& relativeSourcePath) const;
    };
}

namespace AZStd
{
    template<>
    struct hash<AZ::SceneAPI::Containers::SceneGraph::NodeIndex>
    {
        inline size_t operator()(const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex) const
        {
            size_t hashValue{ 0 };
            hash_combine(hashValue, nodeIndex.AsNumber());
            return hashValue;
        }
    };

    template<>
    struct hash<AZ::SceneAPI::PrefabGroupRequests::ManifestUpdates>
    {
        inline size_t operator()(const AZ::SceneAPI::PrefabGroupRequests::ManifestUpdates& updates) const
        {
            size_t hashValue{ 0 };
            hash_combine(hashValue, updates.size());
            return hashValue;
        }
    };
}

