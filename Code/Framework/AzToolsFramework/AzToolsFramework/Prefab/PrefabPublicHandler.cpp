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

#include <AzToolsFramework/Prefab/PrefabPublicHandler.h>

#include <AzCore/Component/TransformBus.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabPublicHandler::PrefabPublicHandler()
        {
            m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
            AZ_Assert(m_instanceEntityMapperInterface, "PrefabPublicHandler - Could not retrieve instance of InstanceEntityMapperInterface");

            AZ::Interface<PrefabPublicInterface>::Register(this);
        }

        PrefabPublicHandler::~PrefabPublicHandler()
        {
            AZ::Interface<PrefabPublicInterface>::Unregister(this);
        }

        PrefabOperationResult PrefabPublicHandler::CreatePrefab(const AZStd::vector<AZ::EntityId>& entityIds, AZStd::string_view filePath)
        {
            // Retrieve entityList from entityIds
            EntityList inputEntityList;
            inputEntityList.reserve(entityIds.size());

            for (AZ::EntityId entityId : entityIds)
            {
                if (entityId.IsValid())
                {
                    inputEntityList.emplace_back(GetEntityById(entityId));
                }
            }

            // Find common root and top level entities
            bool entitiesHaveCommonRoot = false;
            AZ::EntityId commonRootEntityId;
            EntityList topLevelEntities;

            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                entitiesHaveCommonRoot,
                &AzToolsFramework::ToolsApplicationRequests::FindCommonRootInactive,
                inputEntityList,
                commonRootEntityId,
                &topLevelEntities
            );

            // Bail if entities don't share a common root
            if (!entitiesHaveCommonRoot)
            {
                return AZ::Failure(AZStd::string("Could not create a new prefab out of the entities provided - entities do not share a common root."));
            }

            AZ::Entity* commonRootEntity = nullptr;
            if (commonRootEntityId.IsValid())
            {
                commonRootEntity = GetEntityById(commonRootEntityId);
            }

            // Retrieve the owning instance of the common root entity, which will be our new instance's parent instance.
            InstanceOptionalReference commonRootEntityOwningInstance = GetCommonRootEntityOwningInstance(commonRootEntityId);
            AZ_Assert(commonRootEntityOwningInstance.has_value(), "Failed to create prefab : "
                "Couldn't get a valid owning instance for the common root entity of the enities provided");

            AZStd::vector<AZ::Entity*> entities;
            AZStd::vector<AZStd::unique_ptr<Instance>> instances;

            // Retrieve all entities affected and identify Instances
            if (!RetrieveAndSortPrefabEntitiesAndInstances(inputEntityList, commonRootEntityOwningInstance->get(), entities, instances))
            {
                return AZ::Failure(AZStd::string("Could not create a new prefab out of the entities provided - entities do not share a common root."));
            }

            auto prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
            if (!prefabEditorEntityOwnershipInterface)
            {
                return AZ::Failure(AZStd::string("Could not create a new prefab out of the entities provided - internal error "
                                                 "(PrefabEditorEntityOwnershipInterface unavailable)."));
            }

            InstanceOptionalReference instance = prefabEditorEntityOwnershipInterface->CreatePrefab(
                entities, AZStd::move(instances), filePath, commonRootEntityOwningInstance->get());

            if (!instance)
            {
                return AZ::Failure(AZStd::string("Could not create a new prefab out of the entities provided - internal error "
                                                 "(A null instance is returned)."));
            }

            auto prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
            if (prefabLoaderInterface == nullptr)
            {
                return AZ::Failure(AZStd::string("Could not create a new prefab out of the entities provided - internal error (PrefabLoaderInterface unavailable)."));
            }

            AZ::EntityId containerEntityId = instance->get().GetContainerEntityId();
            AZ::Vector3 containerEntityTranslation(AZ::Vector3::CreateZero());
            AZ::Quaternion containerEntityRotation(AZ::Quaternion::CreateZero());

            // Set the transform (translation, rotation) of the container entity
            GenerateContainerEntityTransform(topLevelEntities, containerEntityTranslation, containerEntityRotation);

            AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetLocalTranslation, containerEntityTranslation);
            AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, containerEntityRotation);

            // Set container entity to be child of common root
            AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetParent, commonRootEntityId);
            
            // Change top level entities to be parented to the container entity
            for (AZ::Entity* topLevelEntity : topLevelEntities)
            {
                AZ::TransformBus::Event(topLevelEntity->GetId(), &AZ::TransformBus::Events::SetParent, containerEntityId);
            }
            
            // Register container entity to PrefabUiHandler
            auto editorEntityUiInterface = AZ::Interface<EditorEntityUiInterface>::Get();

            if (editorEntityUiInterface != nullptr)
            {
                editorEntityUiInterface->RegisterEntity(containerEntityId, 1);
            }

            // Save Template
            prefabLoaderInterface->SaveTemplate(instance->get().GetTemplateId());

            return AZ::Success();
        }

        PrefabOperationResult PrefabPublicHandler::InstantiatePrefab(AZStd::string_view /*filePath*/, AZ::EntityId /*parent*/, AZ::Vector3 /*position*/)
        {
            return AZ::Failure(AZStd::string("Prefab - InstantiatePrefab is yet to be implemented."));
        }

        bool PrefabPublicHandler::IsInstanceContainerEntity(AZ::EntityId entityId)
        {
            AZ::Entity* entity = GetEntityById(entityId);
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entity->GetId());
            return owningInstance && (owningInstance->get().GetContainerEntityId() == entityId);
        }

        AZ::EntityId PrefabPublicHandler::GetInstanceContainerEntityId(AZ::EntityId entityId)
        {
            AZ::Entity* entity = GetEntityById(entityId);
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entity->GetId());
            if (owningInstance)
            {
                return owningInstance->get().GetContainerEntityId();
            }

            return AZ::EntityId();
        }

        void PrefabPublicHandler::GenerateContainerEntityTransform(const EntityList& topLevelEntities,
            AZ::Vector3& translation, AZ::Quaternion& rotation)
        {
            // --- Multiple top level entities
            // Translation is the average of all translations, with the minimum Z value.
            // Rotation is set to zero.
            if (topLevelEntities.size() > 1)
            {
                AZ::Vector3 translationSum = AZ::Vector3::CreateZero();
                float minZ = AZStd::numeric_limits<float>::max();
                int transformCount = 0;

                for (AZ::Entity* topLevelEntity : topLevelEntities)
                {
                    if (topLevelEntity != nullptr)
                    {
                        AzToolsFramework::Components::TransformComponent* transformComponent =
                            topLevelEntity->FindComponent<AzToolsFramework::Components::TransformComponent>();

                        if (transformComponent != nullptr)
                        {
                            ++transformCount;

                            auto currentTranslation = transformComponent->GetLocalTranslation();
                            translationSum += currentTranslation;
                            minZ = AZ::GetMin<float>(minZ, currentTranslation.GetZ());
                        }
                    }
                }

                if (transformCount > 0)
                {
                    translation = translationSum / aznumeric_cast<float>(transformCount);
                    translation.SetZ(minZ);

                    rotation = AZ::Quaternion::CreateZero();
                }

            }
            // --- Single top level entity
            // World Translation and Rotation are inherited, unchanged.
            else if (topLevelEntities.size() == 1)
            {
                AZ::Entity* topLevelEntity = topLevelEntities[0];
                if (topLevelEntity)
                {
                    AzToolsFramework::Components::TransformComponent* transformComponent =
                        topLevelEntity->FindComponent<AzToolsFramework::Components::TransformComponent>();

                    if (transformComponent)
                    {
                        translation = transformComponent->GetLocalTranslation();

                        rotation = transformComponent->GetLocalRotationQuaternion();
                    }
                }
            }
        }

        InstanceOptionalReference PrefabPublicHandler::GetCommonRootEntityOwningInstance(AZ::EntityId entityId)
        {
            if (entityId.IsValid())
            {
                return m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            }

            // If the commonRootEntity is invalid, then the owning instance would be the root prefab instance of the
            // PrefabEditorEntityOwnershipService.
            auto prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
            if (!prefabEditorEntityOwnershipInterface)
            {
                AZ_Assert(false, "Could not get owining instance of common root entity :"
                    "PrefabEditorEntityOwnershipInterface unavailable.");
            }
            return prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
        }

        Instance* PrefabPublicHandler::GetParentInstance(Instance* instance)
        {
            auto instanceRef = instance->GetParentInstance();

            if (instanceRef != AZStd::nullopt)
            {
                return &instanceRef->get();
            }

            return nullptr;
        }

        Instance* PrefabPublicHandler::GetAncestorOfInstanceThatIsChildOfRoot(const Instance* root, Instance* instance)
        {
            while (instance != nullptr)
            {
                Instance* parent = GetParentInstance(instance);

                if (parent == root)
                {
                    return instance;
                }

                instance = parent;
            }

            return nullptr;
        }

        bool PrefabPublicHandler::RetrieveAndSortPrefabEntitiesAndInstances(
            const EntityList& inputEntities, const Instance& commonRootEntityOwningInstance,
            EntityList& outEntities, AZStd::vector<AZStd::unique_ptr<Instance>>& outInstances) const
        {
            AZStd::queue<AZ::Entity*> entityQueue;

            for (auto inputEntity : inputEntities)
            {
                entityQueue.push(inputEntity);
            }

            // Support sets to easily identify if we're processing the same entity multiple times.
            AZStd::unordered_set<AZ::Entity*> entities;
            AZStd::unordered_set<Instance*> instances;

            while (!entityQueue.empty())
            {
                AZ::Entity* entity = entityQueue.front();
                entityQueue.pop();

                // Get this entity's owning instance.
                InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entity->GetId());
                AZ_Assert(owningInstance.has_value(), "An error occored while retrieving entities and prefab instances : "
                    "Owning instance of entity with id '%llu' couldn't be found", entity->GetId());

                // Check if this entity is owned by the same instance owning the root.
                if (&owningInstance->get() == &commonRootEntityOwningInstance)
                {
                    AZStd::unique_ptr<AZ::Entity> detachedEntity = owningInstance->get().DetachEntity(entity->GetId());
                    // If it's the same instance, we can add this entity to the new instance entities.
                    int priorEntitiesSize = entities.size();
                    
                    entities.insert(detachedEntity.release());

                    // If the size of entities increased, then it wasn't added before.
                    // In that case, add the children of this entity to the queue.
                    if (entities.size() > priorEntitiesSize)
                    {
                        EntityIdList childrenIds;
                        EditorEntityInfoRequestBus::EventResult(
                            childrenIds,
                            entity->GetId(),
                            &EditorEntityInfoRequests::GetChildren
                        );

                        for (AZ::EntityId childId : childrenIds)
                        {
                            AZ::Entity* child = GetEntityById(childId);
                            entityQueue.push(child);
                        }
                    }
                }
                else
                {
                    // The instances differ, so we should add the instance to the instances set,
                    // but only if it's a direct descendant of the root instance!
                    Instance* childInstance = GetAncestorOfInstanceThatIsChildOfRoot(&commonRootEntityOwningInstance, &owningInstance->get());

                    if (childInstance != nullptr)
                    {
                        instances.insert(childInstance);
                    }
                    else
                    {
                        // This can only happen if one entity does not share the common root!
                        return false;
                    }
                }
            }

            // Store results
            outEntities.clear();
            outEntities.resize(entities.size());
            AZStd::copy(entities.begin(), entities.end(), outEntities.begin());

            outInstances.clear();
            outInstances.reserve(instances.size());
            for (Instance* instancePtr : instances)
            {
                auto parentInstance = instancePtr->GetParentInstance();

                if (parentInstance.has_value())
                {
                    auto uniquePtr = parentInstance->get().DetachNestedInstance(instancePtr->GetInstanceAlias());
                    outInstances.push_back(AZStd::move(uniquePtr));
                }
            }

            return true;
        }
    }
}
