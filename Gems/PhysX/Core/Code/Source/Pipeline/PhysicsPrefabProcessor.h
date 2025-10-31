/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessor.h>
#include <AzCore/Serialization/ObjectStream.h>


namespace AzFramework
{
    class TransformComponent;
}

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class PrefabProcessorContext;
}
   
namespace PhysX
{
    class ArticulationLinkComponent;
    struct ArticulationLinkData;

    class PhysicsPrefabProcessor final : public AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysicsPrefabProcessor, AZ::SystemAllocator);
        AZ_RTTI(
            PhysX::PhysicsPrefabProcessor,
            "{F6E1E453-6829-491E-8604-B7996331CDB5}",
            AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessor);

        PhysicsPrefabProcessor();
        ~PhysicsPrefabProcessor() override = default;

        static void Reflect(AZ::ReflectContext* context);

        // PrefabProcessor overrides...
        void Process(AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context) override;

    private:
        struct ArticulationNode
        {
            AZ::Entity* m_entity = nullptr;
            ArticulationLinkComponent* m_articulationComponent = nullptr;
            AzFramework::TransformComponent* m_transformComponent = nullptr;
            AZStd::vector<ArticulationNode*> m_children;
        };

        struct ArticulationsGraph
        {
            AZStd::unordered_map<AZ::EntityId, AZStd::unique_ptr<ArticulationNode>> m_nodes;
            AZStd::unordered_set<AZ::EntityId> m_articulationRoots;
        };

        void ProcessArticulationHierarchies(ArticulationsGraph& graph);
        void BuildArticulationLinksData(ArticulationNode* currentNode, AZStd::shared_ptr<ArticulationLinkData> parentLinkData = nullptr);

        void PostProcessSpawnable(const AZStd::string& prefabName, AzFramework::Spawnable& spawnable);

        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabSpawnablePostProcessEvent::Handler m_postProcessHandler;
    };
}
