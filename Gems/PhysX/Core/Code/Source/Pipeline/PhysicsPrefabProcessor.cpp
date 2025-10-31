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
    static void EntityDataToArticulationLinkData(
        AZ::Entity* entity, ArticulationLinkData* linkData, ArticulationLinkData* parentLinkData)
    {
        auto* transformComponent = entity->FindComponent<AzFramework::TransformComponent>();
        linkData->m_localTransform = transformComponent->GetLocalTM();

        const auto& components = entity->GetComponents();
        for (const auto& component : components)
        {
            if (auto* baseColliderComponent = azdynamic_cast<BaseColliderComponent*>(component))
            {
                AzPhysics::ShapeColliderPairList shapeColliderPairList = baseColliderComponent->GetShapeConfigurations();
                AZ_Assert(!shapeColliderPairList.empty(), "Collider component with no shape configurations");
                linkData->m_shapeColliderConfigurationList.insert(
                    linkData->m_shapeColliderConfigurationList.end(), shapeColliderPairList.begin(), shapeColliderPairList.end());
            }
        }

        auto* articulationLinkComponent = entity->FindComponent<ArticulationLinkComponent>();
        AZ_Assert(articulationLinkComponent, "Entity being proceessed for articulation has not articulation link component.");
        linkData->m_articulationLinkConfiguration = articulationLinkComponent->m_config;
        linkData->m_articulationLinkConfiguration.m_entityId = entity->GetId();
        linkData->m_articulationLinkConfiguration.m_debugName = entity->GetName();

        // If the entity has a parent then it's not a root articulation and we fill the joint information.
        if (parentLinkData)
        {
            linkData->m_jointFollowerLocalFrame = AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromEulerAnglesDegrees(articulationLinkComponent->m_config.m_localRotation),
                articulationLinkComponent->m_config.m_localPosition);

            if (articulationLinkComponent->m_config.m_autoCalculateLeadFrame)
            {
                linkData->m_jointLeadLocalFrame =
                    linkData->m_localTransform * linkData->m_jointFollowerLocalFrame;
            }
            else
            {
                linkData->m_jointLeadLocalFrame = AZ::Transform::CreateFromQuaternionAndTranslation(
                    AZ::Quaternion::CreateFromEulerAnglesDegrees(articulationLinkComponent->m_config.m_LeadLocalRotation),
                    articulationLinkComponent->m_config.m_leadLocalPosition);
            }
        }
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
        ArticulationNode* currentNode,
        AZStd::shared_ptr<ArticulationLinkData> parentLinkData)
    {
        auto thisLinkData = AZStd::make_shared<ArticulationLinkData>();

        // Pack the data from this entity into ArticulationLinkData.
        // This includes the information about collision shapes, collider configuration, joints, debug data etc.
        EntityDataToArticulationLinkData(currentNode->m_entity, thisLinkData.get(), parentLinkData.get());

        if (parentLinkData)
        {
            // When it is not the root node (parent link data is valid),
            // insert this link data into the parent's child links.
            parentLinkData->m_childLinks.push_back(thisLinkData);
        }
        else
        {
            // Root link data lives in the component itself since there can only be 1 root
            currentNode->m_articulationComponent->m_articulationLinkData = thisLinkData;
        }

        // Recursively call this data gathering routing for all children.
        for (ArticulationNode* childNode : currentNode->m_children)
        {
            BuildArticulationLinksData(childNode, thisLinkData);
        }
    }

    void PhysicsPrefabProcessor::ProcessArticulationHierarchies(ArticulationsGraph& graph)
    {
        for (auto rootEntityId : graph.m_articulationRoots)
        {
            auto rootNodeIter = graph.m_nodes.find(rootEntityId);
            AZ_Assert(rootNodeIter != graph.m_nodes.end(), "Articulation root not found in the graph");
            AZ_Assert(rootNodeIter->second.get(), "Invalid Articulation root node in the graph");
            BuildArticulationLinksData(rootNodeIter->second.get());
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
            auto articulationNode = AZStd::make_unique<ArticulationNode>();

            // Set relevant data to the node.
            const AZ::EntityId entityId = entity->GetId();
            articulationNode->m_entity = entity.get();

            articulationNode->m_articulationComponent = articulationComponent;

            auto* transformComponent = entity->FindComponent<AzFramework::TransformComponent>();
            articulationNode->m_transformComponent = transformComponent;

            // Here we detect if the current node is a root one
            // or we have already processed its parent as an articulation node.
            // This logic is possible because spawnables have all entities sorted in order from parent to child.

            const AZ::EntityId parentId = transformComponent->GetParentId();
            if (graph.m_nodes.find(parentId) == graph.m_nodes.end())
            {
                // Root link IDs are stored separately and will be entry points for the later processing.
                graph.m_articulationRoots.insert(entityId);
            }
            else
            {
                graph.m_nodes[parentId]->m_children.emplace_back(articulationNode.get());
            }

            graph.m_nodes[entityId] = AZStd::move(articulationNode);
        }

        // Now we process the entire graph of articulations.
        ProcessArticulationHierarchies(graph);
    }
}
