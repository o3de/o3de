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
#include <AzCore/Utils/TypeHash.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabUndo.h>
#include <AzToolsFramework/Prefab/PrefabUndoHelpers.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void PrefabPublicHandler::RegisterPrefabPublicHandlerInterface()
        {
            m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
            AZ_Assert(m_instanceEntityMapperInterface, "PrefabPublicHandler - Could not retrieve instance of InstanceEntityMapperInterface");

            m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "PrefabPublicHandler - Could not retrieve instance of InstanceToTemplateInterface");

            m_prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
            AZ_Assert(m_prefabLoaderInterface, "Could not get PrefabLoaderInterface on PrefabPublicHandler construction.");

            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "Could not get PrefabSystemComponentInterface on PrefabPublicHandler construction.");

            m_prefabUndoCache.Initialize();

            AZ::Interface<PrefabPublicInterface>::Register(this);
        }

        void PrefabPublicHandler::UnregisterPrefabPublicHandlerInterface()
        {
            AZ::Interface<PrefabPublicInterface>::Unregister(this);

            m_prefabUndoCache.Destroy();
        }

        PrefabOperationResult PrefabPublicHandler::CreatePrefab(const AZStd::vector<AZ::EntityId>& entityIds, AZ::IO::PathView filePath)
        {
            EntityList inputEntityList, topLevelEntities;
            AZ::EntityId commonRootEntityId;
            InstanceOptionalReference commonRootEntityOwningInstance;
            PrefabOperationResult findCommonRootOutcome = FindCommonRootOwningInstance(
                entityIds, inputEntityList, topLevelEntities, commonRootEntityId, commonRootEntityOwningInstance);
            if (!findCommonRootOutcome.IsSuccess())
            {
                return findCommonRootOutcome;
            }

            InstanceOptionalReference instanceToCreate;
            {
                // Initialize Undo Batch object
                ScopedUndoBatch undoBatch("Create Prefab");

                PrefabDom commonRootInstanceDomBeforeCreate;
                m_instanceToTemplateInterface->GenerateDomForInstance(
                    commonRootInstanceDomBeforeCreate, commonRootEntityOwningInstance->get());

                AZStd::vector<AZ::Entity*> entities;
                AZStd::vector<AZStd::unique_ptr<Instance>> instances;

                // Retrieve all entities affected and identify Instances
                if (!RetrieveAndSortPrefabEntitiesAndInstances(inputEntityList, commonRootEntityOwningInstance->get(), entities, instances))
                {
                    return AZ::Failure(
                        AZStd::string("Could not create a new prefab out of the entities provided - invalid selection."));
                }

                // When we create a prefab with other prefab instances, we have to remove the existing links between the source and 
                // target templates of the other instances.
                for (auto& nestedInstance : instances)
                {
                    RemoveLink(nestedInstance, commonRootEntityOwningInstance->get().GetTemplateId(), undoBatch.GetUndoBatch());
                }

                PrefabUndoHelpers::UpdatePrefabInstance(
                    commonRootEntityOwningInstance->get(), "Update prefab instance", commonRootInstanceDomBeforeCreate,
                    undoBatch.GetUndoBatch());

                auto prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
                if (!prefabEditorEntityOwnershipInterface)
                {
                    return AZ::Failure(AZStd::string("Could not create a new prefab out of the entities provided - internal error "
                                                     "(PrefabEditorEntityOwnershipInterface unavailable)."));
                }

                // Create the Prefab
                instanceToCreate = prefabEditorEntityOwnershipInterface->CreatePrefab(
                    entities, AZStd::move(instances), filePath, commonRootEntityOwningInstance);

                if (!instanceToCreate)
                {
                    return AZ::Failure(AZStd::string("Could not create a new prefab out of the entities provided - internal error "
                                                     "(A null instance is returned)."));
                }

                AZ::EntityId containerEntityId = instanceToCreate->get().GetContainerEntityId();
                AZ::Entity* containerEntity = GetEntityById(containerEntityId);

                {
                    // Generate the transform for the container entity out of the top level entities, and set it
                    // This step needs to be done before anything is parented to the container, else children position will be wrong
                    Prefab::PrefabDom containerEntityDomBefore;
                    m_instanceToTemplateInterface->GenerateDomForEntity(containerEntityDomBefore, *containerEntity);

                    AZ::Vector3 containerEntityTranslation(AZ::Vector3::CreateZero());
                    AZ::Quaternion containerEntityRotation(AZ::Quaternion::CreateZero());

                    // Set container entity to be child of common root
                    AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetParent, commonRootEntityId);

                    // Set the transform (translation, rotation) of the container entity
                    GenerateContainerEntityTransform(topLevelEntities, containerEntityTranslation, containerEntityRotation);
                    AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetLocalTranslation, containerEntityTranslation);
                    AZ::TransformBus::Event(
                        containerEntityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, containerEntityRotation);

                    PrefabDom containerEntityDomAfter;
                    m_instanceToTemplateInterface->GenerateDomForEntity(containerEntityDomAfter, *containerEntity);

                    PrefabDom patch;
                    m_instanceToTemplateInterface->GeneratePatch(patch, containerEntityDomBefore, containerEntityDomAfter);
                    m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(patch, containerEntityId);

                    // Update the cache - this prevents these changes from being stored in the regular undo/redo nodes
                    m_prefabUndoCache.Store(containerEntityId, AZStd::move(containerEntityDomAfter), commonRootEntityId);

                    // Create a link between the templates of the newly created instance and the instance it's being parented under.
                    CreateLink(
                        instanceToCreate->get(), commonRootEntityOwningInstance->get().GetTemplateId(),
                        undoBatch.GetUndoBatch(), patch);
                }

                // Parent the entities to the container entity. Parenting the container entities of the instances passed to createPrefab
                // will be done during the creation of links below.
                for (AZ::Entity* topLevelEntity : entities)
                {
                    AZ::TransformBus::Event(topLevelEntity->GetId(), &AZ::TransformBus::Events::SetParent, containerEntityId);
                }

                // Update the template of the instance since the entities are modified since the template creation.
                Prefab::PrefabDom serializedInstance;
                if (Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(instanceToCreate->get(), serializedInstance))
                {
                    m_prefabSystemComponentInterface->UpdatePrefabTemplate(instanceToCreate->get().GetTemplateId(), serializedInstance);
                }

                instanceToCreate->get().GetNestedInstances([&](AZStd::unique_ptr<Instance>& nestedInstance) {
                    // These link creations shouldn't be undone because that would put the template in a non-usable state if a user
                    // chooses to instantiate the template after undoing the creation.
                    PrefabDom emptyPatch;
                    CreateLink(*nestedInstance, instanceToCreate->get().GetTemplateId(), undoBatch.GetUndoBatch(), emptyPatch, false);
                });

                for (AZ::Entity* topLevelEntity : topLevelEntities)
                {
                    AZ::EntityId topLevelEntityId = topLevelEntity->GetId();
                    if (topLevelEntityId.IsValid())
                    {
                        m_prefabUndoCache.UpdateCache(topLevelEntity->GetId());

                        // Parenting entities would mark entities as dirty. But we want to unmark the top level entities as dirty because
                        // if we don't, the template created would be updated and cause issues with undo operation followed by instantiation.
                        ToolsApplicationRequests::Bus::Broadcast(
                            &ToolsApplicationRequests::Bus::Events::RemoveDirtyEntity, topLevelEntity->GetId());
                    }
                }
                
                // Select Container Entity
                {
                    auto selectionUndo = aznew SelectionCommand({containerEntityId}, "Select Prefab Container Entity");
                    selectionUndo->SetParent(undoBatch.GetUndoBatch());
                    ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::RunRedoSeparately, selectionUndo);
                }
            }

            // Save Template to file
            m_prefabLoaderInterface->SaveTemplate(instanceToCreate->get().GetTemplateId());
            
            return AZ::Success();
        }

        PrefabOperationResult PrefabPublicHandler::InstantiatePrefab(
            AZStd::string_view filePath, AZ::EntityId parent, const AZ::Vector3& position)
        {
            auto prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
            if (!prefabEditorEntityOwnershipInterface)
            {
                return AZ::Failure(AZStd::string("Could not instantiate prefab - internal error "
                                                 "(PrefabEditorEntityOwnershipInterface unavailable)."));
            }

            InstanceOptionalReference instanceToParentUnder;

            // Get parent entity and owning instance
            if (parent.IsValid())
            {
                instanceToParentUnder = m_instanceEntityMapperInterface->FindOwningInstance(parent);
            }

            if (!instanceToParentUnder.has_value())
            {
                instanceToParentUnder = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
                parent = instanceToParentUnder->get().GetContainerEntityId();
            }

            //Detect whether this instantiation would produce a cyclical dependency
            auto relativePath = m_prefabLoaderInterface->GetRelativePathToProject(filePath);
            Prefab::TemplateId templateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(relativePath);

            // If the template isn't currently loaded, there's no way for it to be in the hierarchy so we just skip the check.
            if (templateId != Prefab::InvalidTemplateId && IsPrefabInInstanceAncestorHierarchy(templateId, instanceToParentUnder->get()))
            {
                return AZ::Failure(
                    AZStd::string::format(
                        "Instantiate Prefab operation aborted - Cyclical dependency detected\n(%s depends on %s).",
                        relativePath.Native().c_str(),
                        instanceToParentUnder->get().GetTemplateSourcePath().Native().c_str()
                    )
                );
            }
            
            {
                // Initialize Undo Batch object
                ScopedUndoBatch undoBatch("Instantiate Prefab");

                PrefabDom instanceToParentUnderDomBeforeCreate;
                m_instanceToTemplateInterface->GenerateDomForInstance(instanceToParentUnderDomBeforeCreate, instanceToParentUnder->get());

                // Instantiate the Prefab
                auto instanceToCreate = prefabEditorEntityOwnershipInterface->InstantiatePrefab(relativePath, instanceToParentUnder);

                if (!instanceToCreate)
                {
                    return AZ::Failure(AZStd::string("Could not instantiate the prefab provided - internal error "
                                                     "(A null instance is returned)."));
                }

                PrefabUndoHelpers::UpdatePrefabInstance(
                    instanceToParentUnder->get(), "Update prefab instance", instanceToParentUnderDomBeforeCreate, undoBatch.GetUndoBatch());

                PrefabDom emptyPatch;
                CreateLink(instanceToCreate->get(), instanceToParentUnder->get().GetTemplateId(), undoBatch.GetUndoBatch(), emptyPatch);
                AZ::EntityId containerEntityId = instanceToCreate->get().GetContainerEntityId();

                // Apply position
                AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetWorldTranslation, position);
            }

            return AZ::Success();
        }

        PrefabOperationResult PrefabPublicHandler::FindCommonRootOwningInstance(
            const AZStd::vector<AZ::EntityId>& entityIds, EntityList& inputEntityList, EntityList& topLevelEntities,
            AZ::EntityId& commonRootEntityId, InstanceOptionalReference& commonRootEntityOwningInstance)
        {
            // Retrieve entityList from entityIds
            inputEntityList = EntityIdListToEntityList(entityIds);

            // Remove Level Container Entity if it's part of the list
            AZ::EntityId levelEntityId = GetLevelInstanceContainerEntityId();
            if (levelEntityId.IsValid())
            {
                AZ::Entity* levelEntity = GetEntityById(levelEntityId);
                if (levelEntity)
                {
                    auto levelEntityIter = AZStd::find(inputEntityList.begin(), inputEntityList.end(), levelEntity);
                    if (levelEntityIter != inputEntityList.end())
                    {
                        inputEntityList.erase(levelEntityIter);
                    }
                }
            }

            // Find common root and top level entities
            bool entitiesHaveCommonRoot = false;

            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                entitiesHaveCommonRoot, &AzToolsFramework::ToolsApplicationRequests::FindCommonRootInactive, inputEntityList,
                commonRootEntityId, &topLevelEntities);

            // Bail if entities don't share a common root
            if (!entitiesHaveCommonRoot)
            {
                return AZ::Failure(AZStd::string("Failed to create a prefab: Provided entities do not share a common root."));
            }

            // Retrieve the owning instance of the common root entity, which will be our new instance's parent instance.
            commonRootEntityOwningInstance = GetOwnerInstanceByEntityId(commonRootEntityId);
            AZ_Assert(
                commonRootEntityOwningInstance.has_value(),
                "Failed to create prefab : Couldn't get a valid owning instance for the common root entity of the enities provided");
            return AZ::Success();
        }

        bool PrefabPublicHandler::IsPrefabInInstanceAncestorHierarchy(TemplateId prefabTemplateId, InstanceOptionalConstReference instance)
        {
            InstanceOptionalConstReference currentInstance = instance;

            while (currentInstance.has_value())
            {
                if (currentInstance->get().GetTemplateId() == prefabTemplateId)
                {
                    return true;
                }

                currentInstance = currentInstance->get().GetParentInstance();
            }

            return false;
        }

        void PrefabPublicHandler::CreateLink(
            Instance& sourceInstance, TemplateId targetTemplateId,
            UndoSystem::URSequencePoint* undoBatch, PrefabDom& patch, const bool isUndoRedoSupportNeeded)
        {
            LinkId linkId;
            if (isUndoRedoSupportNeeded)
            {
                linkId = PrefabUndoHelpers::CreateLink(
                    sourceInstance.GetTemplateId(), targetTemplateId, AZStd::move(patch), sourceInstance.GetInstanceAlias(), undoBatch);
            }
            else
            {
                linkId = m_prefabSystemComponentInterface->CreateLink(
                    targetTemplateId, sourceInstance.GetTemplateId(), sourceInstance.GetInstanceAlias(), patch,
                    InvalidLinkId);
                m_prefabSystemComponentInterface->PropagateTemplateChanges(targetTemplateId);
            }

            sourceInstance.SetLinkId(linkId);
        }

        void PrefabPublicHandler::RemoveLink(
            AZStd::unique_ptr<Instance>& sourceInstance, TemplateId targetTemplateId, UndoSystem::URSequencePoint* undoBatch)
        {
            LinkReference nestedInstanceLink = m_prefabSystemComponentInterface->FindLink(sourceInstance->GetLinkId());
            AZ_Assert(
                nestedInstanceLink.has_value(),
                "A valid link was not found for one of the instances provided as input for the CreatePrefab operation.");    

            PrefabDomReference nestedInstanceLinkDom = nestedInstanceLink->get().GetLinkDom();
            AZ_Assert(
                nestedInstanceLinkDom.has_value(),
                "A valid DOM was not found for the link corresponding to one of the instances provided as input for the "
                "CreatePrefab operation.");

            PrefabDomValueReference nestedInstanceLinkPatches =
                PrefabDomUtils::FindPrefabDomValue(nestedInstanceLinkDom->get(), PrefabDomUtils::PatchesName);
            AZ_Assert(
                nestedInstanceLinkPatches.has_value(),
                "A valid DOM for patches was not found for the link corresponding to one of the instances provided as input for the "
                "CreatePrefab operation.");

            PrefabDom patchesCopyForUndoSupport;
            patchesCopyForUndoSupport.CopyFrom(nestedInstanceLinkPatches->get(), patchesCopyForUndoSupport.GetAllocator());
            PrefabUndoHelpers::RemoveLink(
                sourceInstance->GetTemplateId(), targetTemplateId, sourceInstance->GetInstanceAlias(), sourceInstance->GetLinkId(),
                AZStd::move(patchesCopyForUndoSupport), undoBatch);
        }

        PrefabOperationResult PrefabPublicHandler::SavePrefab(AZ::IO::Path filePath)
        {
            auto templateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(filePath.c_str());

            if (templateId == InvalidTemplateId)
            {
                return AZ::Failure(
                    AZStd::string("SavePrefab - Path error. Path could be invalid, or the prefab may not be loaded in this level."));
            }

            if (!m_prefabLoaderInterface->SaveTemplate(templateId))
            {
                return AZ::Failure(AZStd::string("Could not save prefab - internal error (Json write operation failure)."));
            }

            return AZ::Success();
        }

        PrefabEntityResult PrefabPublicHandler::CreateEntity(AZ::EntityId parentId, const AZ::Vector3& position)
        {
            InstanceOptionalReference owningInstanceOfParentEntity = GetOwnerInstanceByEntityId(parentId);
            if (!owningInstanceOfParentEntity)
            {
                return AZ::Failure(AZStd::string::format(
                    "Cannot add entity because the owning instance of parent entity with id '%llu' could not be found.",
                    static_cast<AZ::u64>(parentId)));
            }
            
            EntityAlias entityAlias = Instance::GenerateEntityAlias();

            AliasPath absoluteEntityPath = owningInstanceOfParentEntity->get().GetAbsoluteInstanceAliasPath();
            absoluteEntityPath.Append(entityAlias);

            AZ::EntityId entityId = InstanceEntityIdMapper::GenerateEntityIdForAliasPath(absoluteEntityPath);
            AZStd::string entityName = AZStd::string::format("Entity%llu", static_cast<AZ::u64>(m_newEntityCounter++));

            AZ::Entity* entity = aznew AZ::Entity(entityId, entityName.c_str());
            
            Instance& entityOwningInstance = owningInstanceOfParentEntity->get();

            PrefabDom instanceDomBeforeUpdate;
            m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBeforeUpdate, entityOwningInstance);

            ScopedUndoBatch undoBatch("Add Entity");

            entityOwningInstance.AddEntity(*entity, entityAlias);

            EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequestBus::Events::HandleEntitiesAdded, EntityList{entity});

            AZ::Transform transform = AZ::Transform::CreateIdentity();
            transform.SetTranslation(position);

            EntityOptionalReference owningInstanceContainerEntity = entityOwningInstance.GetContainerEntity();
            if (owningInstanceContainerEntity && !parentId.IsValid())
            {
                parentId = owningInstanceContainerEntity->get().GetId();
            }

            if (parentId.IsValid())
            {
                AZ::TransformBus::Event(entityId, &AZ::TransformInterface::SetParent, parentId);
                AZ::TransformBus::Event(entityId, &AZ::TransformInterface::SetLocalTM, transform);
            }
            else
            {
                AZ::TransformBus::Event(entityId, &AZ::TransformInterface::SetWorldTM, transform);
            }
            

            // Select the new entity (and deselect others).
            AzToolsFramework::EntityIdList selection = {entityId};

            SelectionCommand* selectionCommand = aznew SelectionCommand(selection, "");
            selectionCommand->SetParent(undoBatch.GetUndoBatch());

            ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selection);

            PrefabUndoHelpers::UpdatePrefabInstance(
                entityOwningInstance, "Undo adding entity", instanceDomBeforeUpdate, undoBatch.GetUndoBatch());

            return AZ::Success(entityId);
        }

        void PrefabPublicHandler::GenerateUndoNodesForEntityChangeAndUpdateCache(
            AZ::EntityId entityId, UndoSystem::URSequencePoint* parentUndoBatch)
        {
            // Create Undo node on entities if they belong to an instance
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            if (!owningInstance.has_value())
            {
                return;
            }

            AZ::Entity* entity = GetEntityById(entityId);
            if (!entity)
            {
                m_prefabUndoCache.PurgeCache(entityId);
                return;
            }

            PrefabDom beforeState;
            AZ::EntityId beforeParentId;
            m_prefabUndoCache.Retrieve(entityId, beforeState, beforeParentId);

            PrefabDom afterState;
            AZ::EntityId afterParentId;
            AZ::TransformBus::EventResult(afterParentId, entityId, &AZ::TransformBus::Events::GetParentId);

            m_instanceToTemplateInterface->GenerateDomForEntity(afterState, *entity);

            PrefabDom patch;
            m_instanceToTemplateInterface->GeneratePatch(patch, beforeState, afterState);

            if (patch.IsArray() && !patch.Empty() && beforeState.IsObject())
            {
                if (IsInstanceContainerEntity(entityId) && !IsLevelInstanceContainerEntity(entityId))
                {
                    m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(patch, entityId);
                    GenerateUndoNodeForContainerOverride(parentUndoBatch, entityId, patch, owningInstance->get().GetLinkId());
                }
                else
                {
                    GenerateUndoNodeForEntityChange(parentUndoBatch, entityId, beforeState, afterState);
                }
            }

            if (beforeParentId != afterParentId)
            {
                // If the entity parent changed, verify if the owning instance changed too
                InstanceOptionalReference beforeOwningInstance = m_instanceEntityMapperInterface->FindOwningInstance(beforeParentId);
                InstanceOptionalReference afterOwningInstance = m_instanceEntityMapperInterface->FindOwningInstance(afterParentId);

                // TODO - See if we need to error out / do something special if either instance can't be found.
                if (beforeOwningInstance.has_value() && afterOwningInstance.has_value() &&
                    (&beforeOwningInstance->get() != &afterOwningInstance->get()))
                {
                    // TODO - potentially move this to its own function?

                    EntityList entities;
                    AZStd::vector<AZStd::unique_ptr<Instance>> instances;
                    AZStd::unordered_map<Instance*, PrefabDom> nestedInstanceLinkPatches;

                    // --- HANDLE PREVIOUS INSTANCE ---
                    {
                        // Get the previous state of the old instance for undo/redo purposes
                        PrefabDom beforeInstanceDomBeforeRemoval;
                        m_instanceToTemplateInterface->GenerateDomForInstance(beforeInstanceDomBeforeRemoval, beforeOwningInstance->get());

                        // Retrieve all descendants of this entity, be them entities of container entities
                        // Note that this will detach entities and instances from beforeInstance
                        RetrieveAndSortPrefabEntitiesAndInstances({entity}, beforeOwningInstance->get(), entities, instances);

                        // Create the Update node
                        PrefabUndoHelpers::UpdatePrefabInstance(
                            beforeOwningInstance->get(), "Update prior prefab instance", beforeInstanceDomBeforeRemoval, parentUndoBatch);

                        for (auto& nestedInstance : instances)
                        {
                            auto linkRef = m_prefabSystemComponentInterface->FindLink(nestedInstance->GetLinkId());

                            if (linkRef.has_value())
                            {
                                PrefabDom oldLinkPatches;
                                oldLinkPatches.CopyFrom(linkRef->get().GetLinkDom(), oldLinkPatches.GetAllocator());

                                nestedInstanceLinkPatches.emplace(nestedInstance.get(), AZStd::move(oldLinkPatches));

                                RemoveLink(nestedInstance, beforeOwningInstance->get().GetTemplateId(), parentUndoBatch);
                            }
                        }
                    }

                    // --- HANDLE NEW INSTANCE ---
                    {
                        // Get the previous state of the old instance for undo/redo purposes
                        PrefabDom afterInstanceDomBeforeAdd;
                        m_instanceToTemplateInterface->GenerateDomForInstance(afterInstanceDomBeforeAdd, afterOwningInstance->get());

                        // Add all entities
                        for (AZ::Entity* nestedEntity : entities)
                        {
                            afterOwningInstance->get().AddEntity(*nestedEntity);
                        }

                        // Add all instances, and recreate the links
                        for (AZStd::unique_ptr<Instance>& instance : instances)
                        {
                            PrefabDom previousPatch;

                            // Retrieve the previous patch if it exists
                            if (nestedInstanceLinkPatches.contains(instance.get()))
                            {
                                previousPatch = AZStd::move(nestedInstanceLinkPatches[instance.get()]);
                            }

                            // Get direct pointer as AddInstance will move the unique one.
                            Instance* instancePtr = instance.get();

                            afterOwningInstance->get().AddInstance(AZStd::move(instance));

                            // Add a new link with the old dom
                            CreateLink(*instancePtr, afterOwningInstance->get().GetTemplateId(), parentUndoBatch, previousPatch);
                        }

                        // Create the Update node
                        PrefabUndoHelpers::UpdatePrefabInstance(
                            afterOwningInstance->get(), "Update new prefab instance", afterInstanceDomBeforeAdd, parentUndoBatch);
                    }
                }
            }

            // Update the cache
            m_prefabUndoCache.Store(entityId, AZStd::move(afterState), afterParentId);
        }

        void PrefabPublicHandler::GenerateUndoNodeForContainerOverride(
            UndoSystem::URSequencePoint* undoBatch, AZ::EntityId entityId, const PrefabDom& patch, const LinkId linkId)
        {
            // Save these changes as patches to the link
            PrefabUndoLinkUpdate* linkUpdate = aznew PrefabUndoLinkUpdate(AZStd::to_string(static_cast<AZ::u64>(entityId)));
            linkUpdate->SetParent(undoBatch);
            linkUpdate->Capture(patch, linkId);

            linkUpdate->Redo();
        }

        void PrefabPublicHandler::GenerateUndoNodeForEntityChange(
            UndoSystem::URSequencePoint* undoBatch, AZ::EntityId entityId, PrefabDom& beforeState, PrefabDom& afterState)
        {
            // Update the state of the entity
            PrefabUndoEntityUpdate* state = aznew PrefabUndoEntityUpdate(AZStd::to_string(static_cast<AZ::u64>(entityId)));
            state->SetParent(undoBatch);
            state->Capture(beforeState, afterState, entityId);

            state->Redo();
        }

        bool PrefabPublicHandler::IsInstanceContainerEntity(AZ::EntityId entityId) const
        {
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            return owningInstance && (owningInstance->get().GetContainerEntityId() == entityId);
        }

        bool PrefabPublicHandler::IsLevelInstanceContainerEntity(AZ::EntityId entityId) const
        {
            // Get owning instance
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);

            // Get level root instance
            auto prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
            if (!prefabEditorEntityOwnershipInterface)
            {
                AZ_Assert(
                    false,
                    "Could not get owning instance of common root entity :"
                    "PrefabEditorEntityOwnershipInterface unavailable.");
            }
            InstanceOptionalReference levelInstance = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
            
            return owningInstance
                && levelInstance
                && (&owningInstance->get() == &levelInstance->get())
                && (owningInstance->get().GetContainerEntityId() == entityId);
        }

        AZ::EntityId PrefabPublicHandler::GetInstanceContainerEntityId(AZ::EntityId entityId) const
        {
            AZ::Entity* entity = GetEntityById(entityId);
            if (entity)
            {
                InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entity->GetId());
                if (owningInstance)
                {
                    return owningInstance->get().GetContainerEntityId();
                }
            }

            return AZ::EntityId();
        }

        AZ::EntityId PrefabPublicHandler::GetLevelInstanceContainerEntityId() const
        {
            auto prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
            if (!prefabEditorEntityOwnershipInterface)
            {
                AZ_Assert(
                    false,
                    "Could not get owning instance of common root entity :"
                    "PrefabEditorEntityOwnershipInterface unavailable.");
                return AZ::EntityId();
            }

            auto rootInstance = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();

            if (!rootInstance.has_value())
            {
                return AZ::EntityId();
            }

            return rootInstance->get().GetContainerEntityId();
        }

        AZ::IO::Path PrefabPublicHandler::GetOwningInstancePrefabPath(AZ::EntityId entityId) const
        {
            AZ::IO::Path path;
            InstanceOptionalReference instance = GetOwnerInstanceByEntityId(entityId);

            if (instance.has_value())
            {
                path = instance->get().GetTemplateSourcePath();
            }

            return path;
        }

        PrefabRequestResult PrefabPublicHandler::HasUnsavedChanges(AZ::IO::Path prefabFilePath) const
        {
            auto templateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(prefabFilePath.c_str());

            if (templateId == InvalidTemplateId)
            {
                return AZ::Failure(AZStd::string("HasUnsavedChanges - Path error. Path could be invalid, or the prefab may not be loaded in this level."));
            }

            return AZ::Success(m_prefabSystemComponentInterface->IsTemplateDirty(templateId));
        }

        PrefabOperationResult PrefabPublicHandler::DeleteEntitiesInInstance(const EntityIdList& entityIds)
        {
            return DeleteFromInstance(entityIds, false);
        }

        PrefabOperationResult PrefabPublicHandler::DeleteEntitiesAndAllDescendantsInInstance(const EntityIdList& entityIds)
        {
            return DeleteFromInstance(entityIds, true);
        }

        PrefabOperationResult PrefabPublicHandler::DeleteFromInstance(const EntityIdList& entityIds, bool deleteDescendants)
        {
            if (entityIds.empty())
            {
                return AZ::Success();
            }

            if (!EntitiesBelongToSameInstance(entityIds))
            {
                return AZ::Failure(AZStd::string("Cannot delete multiple entities belonging to different instances with one operation."));
            }

            AZ::EntityId firstEntityIdToDelete = entityIds[0];
            InstanceOptionalReference commonOwningInstance = GetOwnerInstanceByEntityId(firstEntityIdToDelete);

            // If the first entity id is a container entity id, then we need to mark its parent as the common owning instance because you
            // cannot delete an instance from itself.
            if (commonOwningInstance->get().GetContainerEntityId() == firstEntityIdToDelete)
            {
                commonOwningInstance = commonOwningInstance->get().GetParentInstance();
            }

            // Retrieve entityList from entityIds
            EntityList inputEntityList = EntityIdListToEntityList(entityIds);

            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            UndoSystem::URSequencePoint* currentUndoBatch = nullptr;
            ToolsApplicationRequests::Bus::BroadcastResult(currentUndoBatch, &ToolsApplicationRequests::Bus::Events::GetCurrentUndoBatch);

            bool createdUndo = false;
            if (!currentUndoBatch)
            {
                createdUndo = true;
                ToolsApplicationRequests::Bus::BroadcastResult(
                    currentUndoBatch, &ToolsApplicationRequests::Bus::Events::BeginUndoBatch, "Delete Selected");
                AZ_Assert(currentUndoBatch, "Failed to create new undo batch.");
            }

            // In order to undo DeleteSelected, we have to create a selection command which selects the current selection
            // and then add the deletion as children.
            // Commands always execute themselves first and then their children (when going forwards)
            // and do the opposite when going backwards.
            EntityIdList selectedEntities;
            ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);
            SelectionCommand* selCommand = aznew SelectionCommand(selectedEntities, "Delete Entities");

            // We insert a "deselect all" command before we delete the entities. This ensures the delete operations aren't changing
            // selection state, which triggers expensive UI updates. By deselecting up front, we are able to do those expensive
            // UI updates once at the start instead of once for each entity.
            {
                EntityIdList deselection;
                SelectionCommand* deselectAllCommand = aznew SelectionCommand(deselection, "Deselect Entities");
                deselectAllCommand->SetParent(selCommand);
            }

            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "Internal::DeleteEntities:UndoCaptureAndPurgeEntities");

                Prefab::PrefabDom instanceDomBefore;
                m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBefore, commonOwningInstance->get());

                if (deleteDescendants)
                {
                    AZStd::vector<AZ::Entity*> entities;
                    AZStd::vector<AZStd::unique_ptr<Instance>> instances;

                    bool success = RetrieveAndSortPrefabEntitiesAndInstances(inputEntityList, commonOwningInstance->get(), entities, instances);

                    if (!success)
                    {
                        return AZ::Failure(AZStd::string("DeleteEntitiesAndAllDescendantsInInstance"));
                    }

                    for (AZ::Entity* entity : entities)
                    {
                        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, entity->GetId());
                    }

                    for (auto& nestedInstance : instances)
                    {
                        RemoveLink(nestedInstance, commonOwningInstance->get().GetTemplateId(), currentUndoBatch);
                        nestedInstance.reset();
                    }
                }
                else
                {
                    for (AZ::EntityId entityId : entityIds)
                    {
                        InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
                        // If this is the container entity, it actually represents the instance so get its owner
                        if (owningInstance->get().GetContainerEntityId() == entityId)
                        {
                            auto instancePtr = commonOwningInstance->get().DetachNestedInstance(owningInstance->get().GetInstanceAlias());
                            RemoveLink(instancePtr, commonOwningInstance->get().GetTemplateId(), currentUndoBatch);
                        }
                        else
                        {
                            commonOwningInstance->get().DetachEntity(entityId);
                            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, entityId);
                        }
                    }
                }

                Prefab::PrefabDom instanceDomAfter;
                m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomAfter, commonOwningInstance->get());

                PrefabUndoInstance* command = aznew PrefabUndoInstance("Instance deletion");
                command->Capture(instanceDomBefore, instanceDomAfter, commonOwningInstance->get().GetTemplateId());
                command->SetParent(selCommand);
            }

            selCommand->SetParent(currentUndoBatch);
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "Internal::DeleteEntities:RunRedo");
                selCommand->RunRedo();
            }

            if (createdUndo)
            {
                ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::EndUndoBatch);
            }

            return AZ::Success();
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

        InstanceOptionalReference PrefabPublicHandler::GetOwnerInstanceByEntityId(AZ::EntityId entityId) const
        {
            if (entityId.IsValid())
            {
                return m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            }

            // If the entityId is invalid, then the owning instance would be the root prefab instance of the
            // PrefabEditorEntityOwnershipService.
            auto prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
            if (!prefabEditorEntityOwnershipInterface)
            {
                AZ_Assert(false, "Could not get owning instance of common root entity :"
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
            const EntityList& inputEntities, Instance& commonRootEntityOwningInstance,
            EntityList& outEntities, AZStd::vector<AZStd::unique_ptr<Instance>>& outInstances) const
        {
            if (inputEntities.size() == 0)
            {
                return false;
            }

            AZStd::queue<AZ::Entity*> entityQueue;

            for (auto inputEntity : inputEntities)
            {
                if (inputEntity && !IsLevelInstanceContainerEntity(inputEntity->GetId()))
                {
                    entityQueue.push(inputEntity);
                }
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
                AZ_Assert(
                    owningInstance.has_value(),
                    "An error occurred while retrieving entities and prefab instances : "
                    "Owning instance of entity with id '%llu' couldn't be found",
                    entity->GetId());

                // Check if this entity is owned by the same instance owning the root.
                if (&owningInstance->get() == &commonRootEntityOwningInstance)
                {
                    // If it's the same instance, we can add this entity to the new instance entities.
                    int priorEntitiesSize = entities.size();
                    
                    entities.insert(entity);

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
            outEntities.reserve(entities.size());

            for (AZ::Entity* entity : entities)
            {
                outEntities.emplace_back(commonRootEntityOwningInstance.DetachEntity(entity->GetId()).release());
            }

            outInstances.clear();
            outInstances.reserve(instances.size());
            for (Instance* instancePtr : instances)
            {
                outInstances.push_back(AZStd::move(commonRootEntityOwningInstance.DetachNestedInstance(instancePtr->GetInstanceAlias())));
            }

            return (outEntities.size() + outInstances.size()) > 0;
        }

        bool PrefabPublicHandler::EntitiesBelongToSameInstance(const EntityIdList& entityIds) const
        {
            if (entityIds.size() <= 1)
            {
                return true;
            }

            InstanceOptionalReference sharedInstance = AZStd::nullopt;

            for (AZ::EntityId entityId : entityIds)
            {
                InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);

                if (!owningInstance.has_value())
                {
                    AZ_Assert(
                        false,
                        "An error occurred in function EntitiesBelongToSameInstance: "
                        "Owning instance of entity with id '%llu' couldn't be found",
                        entityId);
                    return false;
                }

                // If this is a container entity, it actually represents a child instance so get its owner.
                // The only exception in the level root instance. We leave it as is to streamline operations.
                if (owningInstance->get().GetContainerEntityId() == entityId && !IsLevelInstanceContainerEntity(entityId))
                {
                    owningInstance = owningInstance->get().GetParentInstance();
                }

                if (!sharedInstance.has_value())
                {
                    sharedInstance = owningInstance;
                }
                else
                {
                    if (&sharedInstance->get() != &owningInstance->get())
                    {
                        return false;
                    }
                }
            }

            return true;
        }
    }
}
