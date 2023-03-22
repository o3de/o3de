/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Pipeline/PhysicsPrefabProcessor.h>

#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/Spawnable/SpawnableUtils.h>
#include <Source/ArticulationLinkComponent.h>
#include <Source/BaseColliderComponent.h>
#include <Source/BoxColliderComponent.h>
#include <Source/CapsuleColliderComponent.h>
#include <Source/MeshColliderComponent.h>
#include <Source/SphereColliderComponent.h>

namespace PhysX
{
    static void EntityDataToArticulationLinkData(AZ::Entity* entity, ArticulationLinkData* linkData)
    {
        linkData->m_entityId = entity->GetId();

        auto* transformComponent = entity->FindComponent<AzFramework::TransformComponent>();
        linkData->m_relativeTransform = transformComponent->GetLocalTM();

        BaseColliderComponent* baseColliderComponent = entity->FindComponent<BaseColliderComponent>();
        if (!baseColliderComponent)
        {
            baseColliderComponent = entity->FindComponent<MeshColliderComponent>();
        }

        if (!baseColliderComponent)
        {
            baseColliderComponent = entity->FindComponent<CapsuleColliderComponent>();
        }

        if (!baseColliderComponent)
        {
            baseColliderComponent = entity->FindComponent<BoxColliderComponent>();
        }

        if (!baseColliderComponent)
        {
            baseColliderComponent = entity->FindComponent<SphereColliderComponent>();
        }

        if (baseColliderComponent)
        {
            AzPhysics::ShapeColliderPair shapeColliderPair = baseColliderComponent->GetShapeConfigurations()[0];
            linkData->m_colliderConfiguration = *(shapeColliderPair.first);
            linkData->m_shapeConfiguration = shapeColliderPair.second;
        }

        // TODO: pack joints data here.
    }

    PhysicsPrefabProcessor::PhysicsPrefabProcessor()
        : m_postProcessHandler(
              [this](const AZStd::string& prefabName, AzFramework::Spawnable& spawnable)
              {
                  PostProcessSpawnable(prefabName, spawnable);
              })
    {
    }

    void PhysicsPrefabProcessor::Process(AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context)
    {
        // This handler will be called at the end of the prefab processing pipeline when the final spawnable is constructed.
        context.AddPrefabSpawnablePostProcessEventHandler(m_postProcessHandler);
    }

    void PhysicsPrefabProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<PhysicsPrefabProcessor, PrefabProcessor>()->Version(1);
        }
    }

    void PhysicsPrefabProcessor::BuildArticulationLinksData(
        ArticulationsGraph& graph,
        AZStd::vector<AZStd::shared_ptr<ArticulationLinkData>>& links,
        ArticulationNode* current)
    {
        AZ::Entity* currentEntity = current->m_entity;
        ArticulationLinkComponent* articulationComponent = current->m_articulationComponent;

        auto thisLinkData = AZStd::make_shared<ArticulationLinkData>();

        // Pack the data from this entity into ArticulationLinkData.
        // This includes the information about collision shapes, collider configuration, joints, debug data etc.
        EntityDataToArticulationLinkData(currentEntity, thisLinkData.get());

        // If this is not the root entity, then insert the link data into the vector of child links.
        if (graph.m_articulationRoots.count(thisLinkData->m_entityId) == 0)
        {
            links.push_back(thisLinkData);
        }
        else
        {
            // Root link data lives in the component itself since there can only be 1 root
            articulationComponent->m_articulationLinkData = thisLinkData;
        }

        // Recursively call this data gathering routing for all children.
        for (ArticulationNode* childNode : current->m_children)
        {
            BuildArticulationLinksData(graph, thisLinkData->m_childLinks, childNode);
        }
    }

    void PhysicsPrefabProcessor::ProcessHierarchy(ArticulationsGraph& graph, AZ::EntityId rootId)
    {
        auto rootNodeIter = graph.m_nodes.find(rootId);
        AZ_Assert(rootNodeIter != graph.m_nodes.end(), "Articulation root not found in the graph");

        ArticulationNode* rootNode = rootNodeIter->second.get();
        ArticulationLinkComponent* articulationComponent = rootNode->m_articulationComponent;
        ArticulationLinkData* articulationLinkData = articulationComponent->m_articulationLinkData.get();

        BuildArticulationLinksData(graph, articulationLinkData->m_childLinks, rootNode);
    }

    void PhysicsPrefabProcessor::ProcessArticulationHierarchies(ArticulationsGraph& graph)
    {
        for (auto entityId : graph.m_articulationRoots)
        {
            ProcessHierarchy(graph, entityId);
        }
    }

    void PhysicsPrefabProcessor::PostProcessSpawnable(const AZStd::string& /*prefabName*/, AzFramework::Spawnable& spawnable)
    {
        // Here we build the graph of all articulations in the spawnable. There may be multiple articulations in the same spawnable.
        ArticulationsGraph graph;

        AzFramework::Spawnable::EntityList& entityList = spawnable.GetEntities();
        for (AZStd::unique_ptr<AZ::Entity>& entity : entityList)
        {
            auto* articulationComponent = entity->FindComponent<ArticulationLinkComponent>();
            if (!articulationComponent)
            {
                // We only process entities with Articulation Link Component.
                continue;
            }

            // Create a graph node for this articulation link.
            auto articulationLink = AZStd::make_unique<ArticulationNode>();

            // Set relevant data to the node.
            const AZ::EntityId entityId = entity->GetId();
            articulationLink->m_entity = entity.get();

            articulationLink->m_articulationComponent = articulationComponent;

            auto* transformComponent = entity->FindComponent<AzFramework::TransformComponent>();
            articulationLink->m_transformComponent = transformComponent;

            // Here we detect if the current link is a root one
            // or we have already processed its parent as an articulation link.
            // This logic is possible because spawnables have all entities sorted in order from parent to child.

            const AZ::EntityId parentId = transformComponent->GetParentId();
            if (graph.m_nodes.find(parentId) == graph.m_nodes.end())
            {
                // Root link IDs are stored separately and will be entry points for the later processing.
                graph.m_articulationRoots.insert(entityId);
            }
            else
            {
                graph.m_nodes[parentId]->m_children.emplace_back(articulationLink.get());
            }

            graph.m_nodes[entityId] = AZStd::move(articulationLink);
        }

        // Now we process the entire graph of articulations.
        ProcessArticulationHierarchies(graph);
    }
}
