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
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/writer.h>
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

#include <QString>

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
                AZStd::vector<AZStd::unique_ptr<Instance>> instancePtrs;
                AZStd::vector<Instance*> instances;
                AZStd::unordered_map<Instance*, PrefabDom> nestedInstanceLinkPatchesMap;

                // Retrieve all entities affected and identify Instances
                if (!RetrieveAndSortPrefabEntitiesAndInstances(inputEntityList, commonRootEntityOwningInstance->get(), entities, instances))
                {
                    return AZ::Failure(
                        AZStd::string("Could not create a new prefab out of the entities provided - invalid selection."));
                }

                // Detach the retrieved entities
                for (AZ::Entity* entity : entities)
                {
                    commonRootEntityOwningInstance->get().DetachEntity(entity->GetId()).release();
                }

                // When we create a prefab with other prefab instances, we have to remove the existing links between the source and 
                // target templates of the other instances.
                for (auto& nestedInstance : instances)
                {
                    AZStd::unique_ptr<Instance> outInstance = commonRootEntityOwningInstance->get().DetachNestedInstance(nestedInstance->GetInstanceAlias());

                    auto linkRef = m_prefabSystemComponentInterface->FindLink(nestedInstance->GetLinkId());

                    if (linkRef.has_value())
                    {
                        PrefabDom oldLinkPatches;
                        oldLinkPatches.CopyFrom(linkRef->get().GetLinkDom(), oldLinkPatches.GetAllocator());

                        nestedInstanceLinkPatchesMap.emplace(nestedInstance, AZStd::move(oldLinkPatches));
                    }
                    
                    RemoveLink(outInstance, commonRootEntityOwningInstance->get().GetTemplateId(), undoBatch.GetUndoBatch());

                    instancePtrs.emplace_back(AZStd::move(outInstance));
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
                    entities, AZStd::move(instancePtrs), filePath, commonRootEntityOwningInstance);

                if (!instanceToCreate)
                {
                    return AZ::Failure(AZStd::string("Could not create a new prefab out of the entities provided - internal error "
                                                     "(A null instance is returned)."));
                }

                AZ::EntityId containerEntityId = instanceToCreate->get().GetContainerEntityId();

                // Apply the correct transform to the container for the new instance, and store the patch for use when creating the link.
                PrefabDom patch = ApplyContainerTransformAndGeneratePatch(containerEntityId, commonRootEntityId, topLevelEntities);

                // Parent the non-container top level entities to the container entity.
                // Parenting the top level container entities will be done during the creation of links.
                for (AZ::Entity* topLevelEntity : topLevelEntities)
                {
                    if (!IsInstanceContainerEntity(topLevelEntity->GetId()))
                    {
                        AZ::TransformBus::Event(topLevelEntity->GetId(), &AZ::TransformBus::Events::SetParent, containerEntityId);
                    }
                }

                // Update the template of the instance since the entities are modified since the template creation.
                Prefab::PrefabDom serializedInstance;
                if (Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(instanceToCreate->get(), serializedInstance))
                {
                    m_prefabSystemComponentInterface->UpdatePrefabTemplate(instanceToCreate->get().GetTemplateId(), serializedInstance);
                }

                instanceToCreate->get().GetNestedInstances([&](AZStd::unique_ptr<Instance>& nestedInstance) {
                    AZ_Assert(nestedInstance, "Invalid nested instance found in the new prefab created.");

                    EntityOptionalReference nestedInstanceContainerEntity = nestedInstance->GetContainerEntity();
                    AZ_Assert(
                        nestedInstanceContainerEntity, "Invalid container entity found for the nested instance used in prefab creation.");

                    AZ::EntityId nestedInstanceContainerEntityId = nestedInstanceContainerEntity->get().GetId();
                    PrefabDom previousPatch;

                    // Retrieve the previous patch if it exists
                    if (nestedInstanceLinkPatchesMap.contains(nestedInstance.get()))
                    {
                        previousPatch = AZStd::move(nestedInstanceLinkPatchesMap[nestedInstance.get()]);
                    }

                    // These link creations shouldn't be undone because that would put the template in a non-usable state if a user
                    // chooses to instantiate the template after undoing the creation.
                    CreateLink(*nestedInstance, instanceToCreate->get().GetTemplateId(), undoBatch.GetUndoBatch(), AZStd::move(previousPatch), false);

                    // If this nested instance's container is a top level entity in the new prefab, re-parent it and apply the change.
                    if (AZStd::find(topLevelEntities.begin(), topLevelEntities.end(), &nestedInstanceContainerEntity->get()) != topLevelEntities.end())
                    {
                        Prefab::PrefabDom containerEntityDomBefore;
                        m_instanceToTemplateInterface->GenerateDomForEntity(containerEntityDomBefore, *nestedInstanceContainerEntity);

                        AZ::TransformBus::Event(nestedInstanceContainerEntityId, &AZ::TransformBus::Events::SetParent, containerEntityId);

                        PrefabDom containerEntityDomAfter;
                        m_instanceToTemplateInterface->GenerateDomForEntity(containerEntityDomAfter, *nestedInstanceContainerEntity);

                        PrefabDom reparentPatch;
                        m_instanceToTemplateInterface->GeneratePatch(reparentPatch, containerEntityDomBefore, containerEntityDomAfter);
                        m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(reparentPatch, nestedInstanceContainerEntityId);

                        // Update the cache - this prevents these changes from being stored in the regular undo/redo nodes as a separate step
                        m_prefabUndoCache.Store(nestedInstanceContainerEntityId, AZStd::move(containerEntityDomAfter));

                        // Save these changes as patches to the link
                        PrefabUndoLinkUpdate* linkUpdate = aznew PrefabUndoLinkUpdate(AZStd::to_string(static_cast<AZ::u64>(nestedInstanceContainerEntityId)));
                        linkUpdate->SetParent(undoBatch.GetUndoBatch());
                        linkUpdate->Capture(reparentPatch, nestedInstance->GetLinkId());

                        linkUpdate->Redo();
                    }
                });

                // Create a link between the templates of the newly created instance and the instance it's being parented under.
                CreateLink(
                    instanceToCreate->get(), commonRootEntityOwningInstance->get().GetTemplateId(), undoBatch.GetUndoBatch(),
                    AZStd::move(patch));

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

        PrefabDom PrefabPublicHandler::ApplyContainerTransformAndGeneratePatch(AZ::EntityId containerEntityId, AZ::EntityId parentEntityId, const EntityList& childEntities)
        {
            AZ::Entity* containerEntity = GetEntityById(containerEntityId);
            AZ_Assert(containerEntity, "Invalid container entity passed to ApplyContainerTransformAndGeneratePatch.");

            // Generate the transform for the container entity out of the top level entities, and set it
            // This step needs to be done before anything is parented to the container, else children position will be wrong
            Prefab::PrefabDom containerEntityDomBefore;
            m_instanceToTemplateInterface->GenerateDomForEntity(containerEntityDomBefore, *containerEntity);

            AZ::Vector3 containerEntityTranslation(AZ::Vector3::CreateZero());
            AZ::Quaternion containerEntityRotation(AZ::Quaternion::CreateZero());

            // Set container entity to be child of common root
            AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetParent, parentEntityId);

            // Set the transform (translation, rotation) of the container entity
            GenerateContainerEntityTransform(childEntities, containerEntityTranslation, containerEntityRotation);
            AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetLocalTranslation, containerEntityTranslation);
            AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, containerEntityRotation);

            PrefabDom containerEntityDomAfter;
            m_instanceToTemplateInterface->GenerateDomForEntity(containerEntityDomAfter, *containerEntity);

            PrefabDom patch;
            m_instanceToTemplateInterface->GeneratePatch(patch, containerEntityDomBefore, containerEntityDomAfter);
            m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(patch, containerEntityId);

            // Update the cache - this prevents these changes from being stored in the regular undo/redo nodes
            m_prefabUndoCache.Store(containerEntityId, AZStd::move(containerEntityDomAfter));

            return AZStd::move(patch);
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

            if (templateId == InvalidTemplateId)
            {
                // Load the template from the file
                templateId = m_prefabLoaderInterface->LoadTemplateFromFile(filePath);
                AZ_Assert(templateId != InvalidTemplateId, "Template with source path %s couldn't be loaded correctly.", filePath);
            }

            const PrefabDom& templateDom = m_prefabSystemComponentInterface->FindTemplateDom(templateId);
            AZStd::unordered_set<AZ::IO::Path> templatePaths;
            PrefabDomUtils::GetTemplateSourcePaths(templateDom, templatePaths);

            if (IsCyclicalDependencyFound(instanceToParentUnder->get(), templatePaths))
            {
                return AZ::Failure(AZStd::string::format(
                    "Instantiate Prefab operation aborted - Cyclical dependency detected\n(%s depends on %s).",
                    relativePath.Native().c_str(), instanceToParentUnder->get().GetTemplateSourcePath().Native().c_str()));
            }

            {
                // Initialize Undo Batch object
                ScopedUndoBatch undoBatch("Instantiate Prefab");

                // Instantiate the Prefab
                PrefabDom instanceToParentUnderDomBeforeCreate;
                m_instanceToTemplateInterface->GenerateDomForInstance(instanceToParentUnderDomBeforeCreate, instanceToParentUnder->get());

                auto instanceToCreate = prefabEditorEntityOwnershipInterface->InstantiatePrefab(relativePath, instanceToParentUnder);

                if (!instanceToCreate)
                {
                    return AZ::Failure(AZStd::string("Could not instantiate the prefab provided - internal error "
                                                     "(A null instance is returned)."));
                }

                PrefabUndoHelpers::UpdatePrefabInstance(
                    instanceToParentUnder->get(), "Update prefab instance", instanceToParentUnderDomBeforeCreate, undoBatch.GetUndoBatch());

                // Create Link with correct container patches
                AZ::EntityId containerEntityId = instanceToCreate->get().GetContainerEntityId();
                AZ::Entity* containerEntity = GetEntityById(containerEntityId);
                AZ_Assert(containerEntity, "Invalid container entity detected in InstantiatePrefab.");

                Prefab::PrefabDom containerEntityDomBefore;
                m_instanceToTemplateInterface->GenerateDomForEntity(containerEntityDomBefore, *containerEntity);

                // Set container entity's parent
                AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetParent, parent);

                // Set the position of the container entity
                AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetWorldTranslation, position);

                PrefabDom containerEntityDomAfter;
                m_instanceToTemplateInterface->GenerateDomForEntity(containerEntityDomAfter, *containerEntity);

                // Generate patch to be stored in the link
                PrefabDom patch;
                m_instanceToTemplateInterface->GeneratePatch(patch, containerEntityDomBefore, containerEntityDomAfter);
                m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(patch, containerEntityId);

                CreateLink(instanceToCreate->get(), instanceToParentUnder->get().GetTemplateId(), undoBatch.GetUndoBatch(), AZStd::move(patch));

                // Update the cache - this prevents these changes from being stored in the regular undo/redo nodes
                m_prefabUndoCache.Store(containerEntityId, AZStd::move(containerEntityDomAfter));
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

            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
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

        bool PrefabPublicHandler::IsCyclicalDependencyFound(
            InstanceOptionalConstReference instance, const AZStd::unordered_set<AZ::IO::Path>& templateSourcePaths)
        {
            InstanceOptionalConstReference currentInstance = instance;

            while (currentInstance.has_value())
            {
                if (templateSourcePaths.contains(currentInstance->get().GetTemplateSourcePath()))
                {
                    return true;
                }
                currentInstance = currentInstance->get().GetParentInstance();
            }

            return false;
        }

        void PrefabPublicHandler::CreateLink(
            Instance& sourceInstance, TemplateId targetTemplateId,
            UndoSystem::URSequencePoint* undoBatch, PrefabDom patch, const bool isUndoRedoSupportNeeded)
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

            if (owningInstance.has_value())
            {
                PrefabDom afterState;
                AZ::Entity* entity = GetEntityById(entityId);
                if (entity)
                {
                    PrefabDom beforeState;
                    m_prefabUndoCache.Retrieve(entityId, beforeState);

                    m_instanceToTemplateInterface->GenerateDomForEntity(afterState, *entity);

                    PrefabDom patch;
                    m_instanceToTemplateInterface->GeneratePatch(patch, beforeState, afterState);

                    if (patch.IsArray() && !patch.Empty() && beforeState.IsObject())
                    {
                        if (IsInstanceContainerEntity(entityId) && !IsLevelInstanceContainerEntity(entityId))
                        {
                            m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(patch, entityId);

                            // Save these changes as patches to the link
                            PrefabUndoLinkUpdate* linkUpdate =
                                aznew PrefabUndoLinkUpdate(AZStd::to_string(static_cast<AZ::u64>(entityId)));
                            linkUpdate->SetParent(parentUndoBatch);
                            linkUpdate->Capture(patch, owningInstance->get().GetLinkId());

                            linkUpdate->Redo();
                        }
                        else
                        {
                            // Update the state of the entity
                            PrefabUndoEntityUpdate* state = aznew PrefabUndoEntityUpdate(AZStd::to_string(static_cast<AZ::u64>(entityId)));
                            state->SetParent(parentUndoBatch);
                            state->Capture(beforeState, afterState, entityId);

                            state->Do(owningInstance);
                        }
                    }

                    // Update the cache
                    m_prefabUndoCache.Store(entityId, AZStd::move(afterState));
                }
                else
                {
                    m_prefabUndoCache.PurgeCache(entityId);
                }
            }
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

        PrefabOperationResult PrefabPublicHandler::DuplicateEntitiesInInstance(const EntityIdList& entityIds)
        {
            if (entityIds.empty())
            {
                return AZ::Failure(AZStd::string("No entities to duplicate."));
            }

            if (!EntitiesBelongToSameInstance(entityIds))
            {
                return AZ::Failure(AZStd::string("Cannot duplicate multiple "
                    "entities belonging to different instances with one operation."));
            }

            // We've already verified the entities are all owned by the same instance,
            // so we can just retrieve our instance from the first entity in the list.
            InstanceOptionalReference commonEntityOwningInstance = GetOwnerInstanceByEntityId(entityIds[0]);
            AZ_Assert(
                commonEntityOwningInstance.has_value(),
                "Failed to duplicate : Couldn't get a valid owning instance for the common root entity of the entities provided");

            // This will cull out any entities that have ancestors in the list, since we will end up duplicating
            // the full nested hierarchy with what is returned from RetrieveAndSortPrefabEntitiesAndInstances
            AzToolsFramework::EntityIdSet duplicationSet = AzToolsFramework::GetCulledEntityHierarchy(entityIds);

            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            ScopedUndoBatch undoBatch("Duplicate Entities");

            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "DuplicateEntitiesInInstance::UndoCaptureAndDuplicateEntities");

                // Take a snapshot of the instance DOM before we manipulate it
                Prefab::PrefabDom instanceDomBefore;
                m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBefore, commonEntityOwningInstance->get());

                AZStd::vector<AZ::Entity*> entities;
                AZStd::vector<Instance*> instances;

                // Gather all entities/instances in the hierarchy, but don't detach them because we are duplicating not deleting.
                EntityList inputEntityList = EntityIdSetToEntityList(duplicationSet);
                bool success = RetrieveAndSortPrefabEntitiesAndInstances(inputEntityList, commonEntityOwningInstance->get(), entities, instances);

                if (!success)
                {
                    return AZ::Failure(AZStd::string("Failed to retrieve entities and instances from the given list of entity ids for duplication"));
                }

                // Make a copy of our before instance DOM where we will add our duplicated entities
                Prefab::PrefabDom instanceDomAfter;
                instanceDomAfter.CopyFrom(instanceDomBefore, instanceDomAfter.GetAllocator());

                AZStd::unordered_map<EntityAlias, EntityAlias> oldAliasToNewAliasMap;
                AZStd::unordered_map<EntityAlias, QString> aliasToEntityDomMap;

                for (AZ::Entity* entity : entities)
                {
                    EntityAliasOptionalReference oldAliasRef = commonEntityOwningInstance->get().GetEntityAlias(entity->GetId());
                    AZ_Assert(oldAliasRef.has_value(), "No alias found for Entity in the DOM");
                    EntityAlias oldAlias = oldAliasRef.value();

                    // Give this the outer allocator so that the memory reference will be valid when
                    // it gets used for AddMember
                    Prefab::PrefabDom entityDomBefore(&instanceDomAfter.GetAllocator());
                    m_instanceToTemplateInterface->GenerateDomForEntity(entityDomBefore, *entity);

                    // Keep track of the old alias <-> new alias mapping for this duplicated entity
                    // so we can fixup references later
                    EntityAlias newEntityAlias = Instance::GenerateEntityAlias();
                    oldAliasToNewAliasMap.insert(AZStd::make_pair(oldAlias, newEntityAlias));

                    rapidjson::StringBuffer buffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                    entityDomBefore.Accept(writer);

                    // Store our duplicated Entity DOM with its new alias as a string
                    // so that we can fixup entity alias references before adding it
                    // to the Entities member of our instance DOM
                    QString entityDomString(buffer.GetString());
                    aliasToEntityDomMap.insert(AZStd::make_pair(newEntityAlias, entityDomString));
                }

                auto entitiesIter = instanceDomAfter.FindMember(PrefabDomUtils::EntitiesName);
                AZ_Assert(entitiesIter != instanceDomAfter.MemberEnd(), "Instance DOM missing the Entities member.");

                // Now that all the duplicated Entity DOMs have been created, we need to iterate
                // through them and replace any previous EntityAlias references with the new ones.
                // These are more than just parent entity references for nested entities, this will
                // also cover any EntityId references that were made in the components between them.
                for (auto aliasEntityPair : aliasToEntityDomMap)
                {
                    EntityAlias newEntityAlias = aliasEntityPair.first;
                    QString newEntityDomString = aliasEntityPair.second;

                    // Replace all of the old alias references with the new ones
                    // We bookend the aliases with \" and also with a / as an extra precaution to prevent
                    // inadvertently replacing a matching string vs. where an actual EntityId is expected
                    // This will cover both cases where an alias could be used in a normal entity vs. an instance
                    for (auto aliasMapIter : oldAliasToNewAliasMap)
                    {
                        QString oldAliasQuotes = QString("\"%1\"").arg(aliasMapIter.first.c_str());
                        QString newAliasQuotes = QString("\"%1\"").arg(aliasMapIter.second.c_str());

                        newEntityDomString.replace(oldAliasQuotes, newAliasQuotes);

                        QString oldAliasPathRef = QString("/%1").arg(aliasMapIter.first.c_str());
                        QString newAliasPathRef = QString("/%1").arg(aliasMapIter.second.c_str());

                        newEntityDomString.replace(oldAliasPathRef, newAliasPathRef);
                    }

                    // Create the new Entity DOM from parsing the JSON string
                    Prefab::PrefabDom entityDomAfter(&instanceDomAfter.GetAllocator());
                    entityDomAfter.Parse(newEntityDomString.toUtf8().constData());

                    // Add the new Entity DOM to the Entities member of the instance
                    rapidjson::Value aliasName(newEntityAlias.c_str(), newEntityAlias.length(), instanceDomAfter.GetAllocator());
                    entitiesIter->value.AddMember(AZStd::move(aliasName), entityDomAfter, instanceDomAfter.GetAllocator());
                }

                PrefabUndoInstance* command = aznew PrefabUndoInstance("Entity duplication");
                command->SetParent(undoBatch.GetUndoBatch());
                command->Capture(instanceDomBefore, instanceDomAfter, commonEntityOwningInstance->get().GetTemplateId());
                command->RunRedo();

                EntityIdList duplicatedEntityIds;
                for (auto aliasMapIter : oldAliasToNewAliasMap)
                {
                    EntityAlias newEntityAlias = aliasMapIter.second;

                    AliasPath absoluteEntityPath = commonEntityOwningInstance->get().GetAbsoluteInstanceAliasPath();
                    absoluteEntityPath.Append(newEntityAlias);

                    AZ::EntityId newEntityId = InstanceEntityIdMapper::GenerateEntityIdForAliasPath(absoluteEntityPath);
                    duplicatedEntityIds.push_back(newEntityId);
                }

                // Select the duplicated entities
                auto selectionUndo = aznew SelectionCommand(duplicatedEntityIds, "Select Duplicated Entities");
                selectionUndo->SetParent(undoBatch.GetUndoBatch());
                ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::RunRedoSeparately, selectionUndo);
            }

            return AZ::Success();
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

            ScopedUndoBatch undoBatch("Delete Selected");

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
                    AZStd::vector<Instance*> instances;

                    bool success = RetrieveAndSortPrefabEntitiesAndInstances(inputEntityList, commonOwningInstance->get(), entities, instances);

                    if (!success)
                    {
                        return AZ::Failure(AZStd::string("DeleteEntitiesAndAllDescendantsInInstance"));
                    }

                    for (AZ::Entity* entity : entities)
                    {
                        commonOwningInstance->get().DetachEntity(entity->GetId()).release();
                        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, entity->GetId());
                    }

                    for (auto& nestedInstance : instances)
                    {
                        AZStd::unique_ptr<Instance> outInstance = commonOwningInstance->get().DetachNestedInstance(nestedInstance->GetInstanceAlias());
                        RemoveLink(outInstance, commonOwningInstance->get().GetTemplateId(), undoBatch.GetUndoBatch());
                        outInstance.reset();
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
                            RemoveLink(instancePtr, commonOwningInstance->get().GetTemplateId(), undoBatch.GetUndoBatch());
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

            selCommand->SetParent(undoBatch.GetUndoBatch());
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "Internal::DeleteEntities:RunRedo");
                selCommand->RunRedo();
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
            EntityList& outEntities, AZStd::vector<Instance*>& outInstances) const
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
                outEntities.emplace_back(entity);
            }

            outInstances.clear();
            outInstances.reserve(instances.size());
            for (Instance* instancePtr : instances)
            {
                outInstances.push_back(instancePtr);
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
    } // namespace Prefab
} // namespace AzToolsFramework
