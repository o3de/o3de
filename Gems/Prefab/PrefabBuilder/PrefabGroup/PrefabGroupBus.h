
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
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <AzCore/Component/EntityId.h>

namespace AZ::SceneAPI::DataTypes
{
    class ICustomPropertyData;
}

namespace AZ::SceneAPI::SceneData
{
    class PrefabGroup;
}

namespace AZ::SceneAPI::Containers
{
    class Scene;
}

namespace AZ::SceneAPI
{
    using PrefabGroup = AZ::SceneAPI::SceneData::PrefabGroup;
    using Scene = AZ::SceneAPI::Containers::Scene;

    //! Events that handle Prefab Group logic.
    //! The behavior context will reflect this EBus so that it can be used in Python and C++ code.
    class PrefabGroupEvents
        : public AZ::EBusTraits
    {
    public:
        AZ_RTTI(PrefabGroupEvents, "{2AF2819A-59DA-4469-863A-E90D0AEF1646}");

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        PrefabGroupEvents() = default;
        virtual ~PrefabGroupEvents() = default;

        using ManifestUpdates = AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>>;

        virtual AZStd::optional<ManifestUpdates> GeneratePrefabGroupManifestUpdates(const Scene& scene) const = 0;
    };
    using PrefabGroupEventBus = AZ::EBus<PrefabGroupEvents>;

    //! Notifications during the default Prefab Group construction so that other scene builders can contribute the entity-component prefab.
    //! The behavior context will reflect this EBus so that it can be used in Python and C++ code.
    class PrefabGroupNotifications
        : public AZ::EBusTraits
    {
        AZ_RTTI(PrefabGroupNotifications, "{BD88ADC3-B72F-43DD-B279-A44E39CD612F}");

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        PrefabGroupNotifications() = default;
        virtual ~PrefabGroupNotifications() = default;

        //! This is sent when the Prefab Group logic is finished creating an entity but allows other scene builders to extend the entity
        virtual void OnUpdatePrefabEntity(const AZ::EntityId& prefabEntity) const = 0;
    };
    using PrefabGroupNotificationBus = AZ::EBus<PrefabGroupNotifications>;

    //! Handler for the Prefab Group event logic
    class PrefabGroupEventHandler
        : protected PrefabGroupEventBus::Handler
    {
    public:
        AZ_RTTI(PrefabGroupEventHandler, "{6BAAB306-01EE-42E8-AAFE-C9EE0BF4CFDF}");

        static void Reflect(ReflectContext* context);

        AZStd::optional<ManifestUpdates> GeneratePrefabGroupManifestUpdates(const Scene& scene) const override;

    protected:
        // this stores the data related with MeshData nodes
        struct MeshNodeData
        {
            Containers::SceneGraph::NodeIndex m_meshIndex = {};
            Containers::SceneGraph::NodeIndex m_transformIndex = {};
            Containers::SceneGraph::NodeIndex m_propertyMapIndex = {};
        };

        using MeshDataMapEntry = AZStd::pair<Containers::SceneGraph::NodeIndex, MeshNodeData>;
        using MeshDataMap = AZStd::unordered_map<Containers::SceneGraph::NodeIndex, MeshNodeData>; // MeshData Index -> MeshNodeData
        using NodeEntityMap = AZStd::unordered_map<Containers::SceneGraph::NodeIndex, AZ::EntityId>; // MeshData Index -> EntityId
        using EntityIdList = AZStd::vector<AZ::EntityId>;

        MeshDataMap CalculateMeshTransformMap(const Containers::Scene& scene) const;

        bool AddEditorMaterialComponent(
            const AZ::EntityId& entityId,
            const DataTypes::ICustomPropertyData& propertyData) const;

        bool AddEditorMeshComponent(
            const AZ::EntityId& entityId,
            const AZStd::string& relativeSourcePath,
            const AZStd::string& meshGroupName) const;

        NodeEntityMap CreateMeshGroups(
            ManifestUpdates& manifestUpdates,
            const MeshDataMap& meshDataMap,
            const Containers::Scene& scene,
            const AZStd::string& relativeSourcePath) const;

        EntityIdList FixUpEntityParenting(
            const NodeEntityMap& nodeEntityMap,
            const Containers::SceneGraph& graph,
            const MeshDataMap& meshDataMap) const;

        bool CreatePrefabGroupManifestUpdates(
            ManifestUpdates& manifestUpdates,
            const Containers::Scene& scene,
            const EntityIdList& entities,
            const AZStd::string& filenameOnly,
            const AZStd::string& relativeSourcePath) const;
    };
}
