/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/Utils/TypeHash.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/EditorPrefabComponent.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicHandler.h>
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

            m_prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            AZ_Assert(m_prefabFocusInterface, "Could not get PrefabFocusInterface on PrefabPublicHandler construction.");

            m_prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
            AZ_Assert(m_prefabFocusPublicInterface, "Could not get PrefabFocusPublicInterface on PrefabPublicHandler construction.");

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

        CreatePrefabResult PrefabPublicHandler::CreatePrefabInMemory(const EntityIdList& entityIds, AZ::IO::PathView filePath)
        {
            EntityList inputEntityList, topLevelEntities;
            AZ::EntityId commonRootEntityId;
            InstanceOptionalReference commonRootEntityOwningInstance;
            PrefabOperationResult findCommonRootOutcome = FindCommonRootOwningInstance(
                entityIds, inputEntityList, topLevelEntities, commonRootEntityId, commonRootEntityOwningInstance);
            if (!findCommonRootOutcome.IsSuccess())
            {
                return AZ::Failure(findCommonRootOutcome.TakeError());
            }

            AZ::EntityId containerEntityId;

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
                PrefabOperationResult retrieveEntitiesAndInstancesOutcome = RetrieveAndSortPrefabEntitiesAndInstances(
                    inputEntityList, commonRootEntityOwningInstance->get(), entities, instances);
                if (!retrieveEntitiesAndInstancesOutcome.IsSuccess())
                {
                    return AZ::Failure(retrieveEntitiesAndInstancesOutcome.TakeError());
                }

                AZStd::unordered_map<AZ::EntityId, AZStd::string> oldEntityAliases;

                // Detach the retrieved entities
                for (AZ::Entity* entity : entities)
                {
                    AZ::EntityId entityId = entity->GetId();
                    oldEntityAliases.emplace(entityId, commonRootEntityOwningInstance->get().GetEntityAlias(entityId)->get());
                    commonRootEntityOwningInstance->get().DetachEntity(entity->GetId()).release();
                }

                // When we create a prefab with other prefab instances, we have to remove the existing links between the source and 
                // target templates of the other instances.
                for (auto& nestedInstance : instances)
                {
                    AZStd::unique_ptr<Instance> outInstance = commonRootEntityOwningInstance->get().DetachNestedInstance(nestedInstance->GetInstanceAlias());

                    LinkId detachingInstanceLinkId = nestedInstance->GetLinkId();
                    auto linkRef = m_prefabSystemComponentInterface->FindLink(detachingInstanceLinkId);
                    AZ_Assert(linkRef.has_value(), "Unable to find link with id '%llu' during prefab creation.", detachingInstanceLinkId);

                    PrefabDomValueReference linkPatches = linkRef->get().GetLinkPatches();
                    AZ_Assert(
                        linkPatches.has_value(), "Unable to get patches on link with id '%llu' during prefab creation.",
                        detachingInstanceLinkId);

                    PrefabDom linkPatchesCopy;
                    linkPatchesCopy.CopyFrom(linkPatches->get(), linkPatchesCopy.GetAllocator());
                    nestedInstanceLinkPatchesMap.emplace(nestedInstance, AZStd::move(linkPatchesCopy));

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
                AZ_Assert(filePath.IsAbsolute(), "CreatePrefabInMemory requires an absolute file path.");

                instanceToCreate = prefabEditorEntityOwnershipInterface->CreatePrefab(
                    entities, AZStd::move(instancePtrs), m_prefabLoaderInterface->GenerateRelativePath(filePath),
                    commonRootEntityOwningInstance);

                if (!instanceToCreate)
                {
                    return AZ::Failure(AZStd::string("Could not create a new prefab out of the entities provided - internal error "
                        "(A null instance is returned)."));
                }

                containerEntityId = instanceToCreate->get().GetContainerEntityId();

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
                        UpdateLinkPatchesWithNewEntityAliases(previousPatch, oldEntityAliases, instanceToCreate->get());
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

                        // We won't parent this undo node to the undo batch so that the newly created template and link will remain
                        // unaffected by undo actions. This is needed so that any future instantiations of the template will work.
                        PrefabUndoLinkUpdate linkUpdate = PrefabUndoLinkUpdate(AZStd::to_string(static_cast<AZ::u64>(nestedInstanceContainerEntityId)));
                        linkUpdate.Capture(reparentPatch, nestedInstance->GetLinkId());
                        linkUpdate.Redo();
                    }
                });

                // Create a link between the templates of the newly created instance and the instance it's being parented under.
                CreateLink(
                    instanceToCreate->get(), commonRootEntityOwningInstance->get().GetTemplateId(), undoBatch.GetUndoBatch(),
                    AZStd::move(patch));

                // Reset the transform of the container entity so that the new values aren't saved in the new prefab's dom.
                // The new values were saved in the link, so propagation will apply them correctly.
                {
                    AZ::Entity* containerEntity = GetEntityById(containerEntityId);

                    PrefabDom containerBeforeReset;
                    m_instanceToTemplateInterface->GenerateDomForEntity(containerBeforeReset, *containerEntity);

                    AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetParent, AZ::EntityId());
                    AZ::TransformBus::Event(containerEntityId, &AZ::TransformBus::Events::SetLocalTM, AZ::Transform::CreateIdentity());

                    PrefabDom containerAfterReset;
                    m_instanceToTemplateInterface->GenerateDomForEntity(containerAfterReset, *containerEntity);

                    // Update the state of the entity
                    auto templateId = instanceToCreate->get().GetTemplateId();

                    PrefabDom transformPatch;
                    m_instanceToTemplateInterface->GeneratePatch(transformPatch, containerBeforeReset, containerAfterReset);
                    m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(transformPatch, containerEntityId);

                    m_instanceToTemplateInterface->PatchTemplate(transformPatch, templateId);
                }

                // This clears any entities marked as dirty due to reparenting of entities during the process of creating a prefab.
                // We are doing this so that the changes in those entities are not queued up twice for propagation.
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequestBus::Events::ClearDirtyEntities);

                // Select Container Entity
                {
                    const EntityIdList selectedEntities = EntityIdList{ containerEntityId };
                    auto selectionUndo = aznew SelectionCommand(selectedEntities, "Select Prefab Container Entity");
                    selectionUndo->SetParent(undoBatch.GetUndoBatch());
                    ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::SetSelectedEntities, selectedEntities);
                }
            }

            return AZ::Success(containerEntityId);
        }

        CreatePrefabResult PrefabPublicHandler::CreatePrefabInDisk(const EntityIdList& entityIds, AZ::IO::PathView filePath)
        {
            auto result = CreatePrefabInMemory(entityIds, filePath);
            if (result.IsSuccess())
            {
                // Save Template to file
                auto relativePath = m_prefabLoaderInterface->GenerateRelativePath(filePath);
                Prefab::TemplateId templateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(relativePath);
                if (!m_prefabLoaderInterface->SaveTemplateToFile(templateId, filePath))
                {
                    AZStd::string_view filePathString(filePath);
                    return AZ::Failure(AZStd::string::format(
                        "Could not save the newly created prefab to file path %.*s - internal error ",
                        AZ_STRING_ARG(filePathString)));
                }
            }
            
            return result;
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
            m_prefabUndoCache.Store(containerEntityId, AZStd::move(containerEntityDomAfter), parentEntityId);

            return AZStd::move(patch);
        }

        InstantiatePrefabResult PrefabPublicHandler::InstantiatePrefab(
            AZStd::string_view filePath, AZ::EntityId parent, const AZ::Vector3& position)
        {
            auto prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
            if (!prefabEditorEntityOwnershipInterface)
            {
                return AZ::Failure(AZStd::string("Could not instantiate prefab - internal error "
                                                 "(PrefabEditorEntityOwnershipInterface unavailable)."));
            }
            if (!prefabEditorEntityOwnershipInterface->IsRootPrefabAssigned())
            {
                return AZ::Failure(AZStd::string("Could not instantiate prefab - no root prefab assigned. "
                "Currently, prefabs can only be instantiated inside a level"));
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
            auto relativePath = m_prefabLoaderInterface->GenerateRelativePath(filePath);
            Prefab::TemplateId templateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(relativePath);

            if (templateId == InvalidTemplateId)
            {
                // Load the template from the file
                templateId = m_prefabLoaderInterface->LoadTemplateFromFile(filePath);
                AZ_Assert(templateId != InvalidTemplateId, "Template with source path %.*s couldn't be loaded correctly.", AZ_STRING_ARG(filePath));
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

            AZ::EntityId containerEntityId;
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
                containerEntityId = instanceToCreate->get().GetContainerEntityId();
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

                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequestBus::Events::ClearDirtyEntities);
            }

            return AZ::Success(containerEntityId);
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
            // If the parent is invalid, parent to the container of the currently focused prefab.
            if (!parentId.IsValid())
            {
                AzFramework::EntityContextId editorEntityContextId = AzToolsFramework::GetEntityContextId();
                parentId = m_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(editorEntityContextId);
            }

            // If the parent entity isn't owned by a prefab instance, bail.
            InstanceOptionalReference owningInstanceOfParentEntity = GetOwnerInstanceByEntityId(parentId);
            if (!owningInstanceOfParentEntity)
            {
                return AZ::Failure(AZStd::string::format(
                    "Cannot add entity because the owning instance of parent entity with id '%llu' could not be found.",
                    static_cast<AZ::u64>(parentId)));
            }

            // If the parent entity is a closed container, bail.
            if (auto containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get(); !containerEntityInterface->IsContainerOpen(parentId))
            {
                return AZ::Failure(AZStd::string::format(
                    "Cannot add entity because the parent entity (id '%llu') is a closed container entity.",
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

        PrefabOperationResult PrefabPublicHandler::GenerateUndoNodesForEntityChangeAndUpdateCache(
            AZ::EntityId entityId, UndoSystem::URSequencePoint* parentUndoBatch)
        {
            // Create Undo node on entities if they belong to an instance
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            if (!owningInstance.has_value())
            {
                return AZ::Success();
            }

            AZ::Entity* entity = GetEntityById(entityId);
            if (!entity)
            {
                m_prefabUndoCache.PurgeCache(entityId);
                return AZ::Success();
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
            m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(patch, entityId);

            if (patch.IsArray() && !patch.Empty() && beforeState.IsObject())
            {
                bool isInstanceContainerEntity = IsInstanceContainerEntity(entityId) && !IsLevelInstanceContainerEntity(entityId);
                bool isNewParentOwnedByDifferentInstance = false;

                if (beforeParentId != afterParentId)
                {
                    // If the entity parent changed, verify if the owning instance changed too
                    InstanceOptionalReference beforeOwningInstance = m_instanceEntityMapperInterface->FindOwningInstance(beforeParentId);
                    InstanceOptionalReference afterOwningInstance = m_instanceEntityMapperInterface->FindOwningInstance(afterParentId);

                    if (beforeOwningInstance.has_value() && afterOwningInstance.has_value() &&
                        (&beforeOwningInstance->get() != &afterOwningInstance->get()))
                    {
                        isNewParentOwnedByDifferentInstance = true;

                        // Detect loops. Assert if an instance has been reparented in such a way to generate circular dependencies.
                        AZStd::vector<Instance*> instancesInvolved;

                        if (isInstanceContainerEntity)
                        {
                            instancesInvolved.push_back(&owningInstance->get());
                        }
                        else
                        {
                            // Retrieve all nested instances that are part of the subtree under the current entity.
                            EntityList entities;
                            PrefabOperationResult retrieveEntitiesAndInstancesOutcome = RetrieveAndSortPrefabEntitiesAndInstances(
                                { entity }, beforeOwningInstance->get(), entities, instancesInvolved);
                            if (!retrieveEntitiesAndInstancesOutcome.IsSuccess())
                            {
                                return retrieveEntitiesAndInstancesOutcome;
                            }
                        }

                        for (Instance* instance : instancesInvolved)
                        {
                            const PrefabDom& templateDom =
                                m_prefabSystemComponentInterface->FindTemplateDom(instance->GetTemplateId());
                            AZStd::unordered_set<AZ::IO::Path> templatePaths;
                            PrefabDomUtils::GetTemplateSourcePaths(templateDom, templatePaths);

                            if (IsCyclicalDependencyFound(afterOwningInstance->get(), templatePaths))
                            {
                                // Cancel the operation by restoring the previous parent
                                AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetParent, beforeParentId);
                                m_prefabUndoCache.UpdateCache(entityId);

                                // Skip the creation of an undo node
                                return AZ::Failure(AZStd::string::format(
                                    "Reparent Prefab operation aborted - Cyclical dependency detected\n(%s depends on %s).",
                                    instance->GetTemplateSourcePath().Native().c_str(),
                                    afterOwningInstance->get().GetTemplateSourcePath().Native().c_str()));
                            }
                        }
                    }
                }

                if (isInstanceContainerEntity)
                {
                    if (isNewParentOwnedByDifferentInstance)
                    {
                        Internal_HandleInstanceChange(parentUndoBatch, entity, beforeParentId, afterParentId);

                        PrefabDom afterStateafterReparenting;
                        m_instanceToTemplateInterface->GenerateDomForEntity(afterStateafterReparenting, *entity);

                        PrefabDom newPatch;
                        m_instanceToTemplateInterface->GeneratePatch(newPatch, afterState, afterStateafterReparenting);
                        m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(newPatch, entityId);

                        InstanceOptionalReference owningInstanceAfterReparenting =
                            m_instanceEntityMapperInterface->FindOwningInstance(entityId);

                        Internal_HandleContainerOverride(
                            parentUndoBatch, entityId, newPatch, owningInstanceAfterReparenting->get().GetLinkId());
                    }
                    else
                    {
                        Internal_HandleContainerOverride(
                            parentUndoBatch, entityId, patch, owningInstance->get().GetLinkId(), owningInstance->get().GetParentInstance());
                    }
                }
                else
                {
                    Internal_HandleEntityChange(parentUndoBatch, entityId, beforeState, afterState, owningInstance);

                    if (isNewParentOwnedByDifferentInstance)
                    {
                        Internal_HandleInstanceChange(parentUndoBatch, entity, beforeParentId, afterParentId);
                    }
                }
            }

            m_prefabUndoCache.UpdateCache(entityId);

            return AZ::Success();
        }

        void PrefabPublicHandler::Internal_HandleContainerOverride(
            UndoSystem::URSequencePoint* undoBatch, AZ::EntityId entityId, const PrefabDom& patch,
            const LinkId linkId, InstanceOptionalReference parentInstance)
        {
            // Save these changes as patches to the link
            PrefabUndoLinkUpdate* linkUpdate = aznew PrefabUndoLinkUpdate(AZStd::to_string(static_cast<AZ::u64>(entityId)));
            linkUpdate->SetParent(undoBatch);
            linkUpdate->Capture(patch, linkId);

            linkUpdate->Redo(parentInstance);
        }

        void PrefabPublicHandler::Internal_HandleEntityChange(
            UndoSystem::URSequencePoint* undoBatch, AZ::EntityId entityId, PrefabDom& beforeState,
            PrefabDom& afterState, InstanceOptionalReference instance)
        {
            // Update the state of the entity
            PrefabUndoEntityUpdate* state = aznew PrefabUndoEntityUpdate(AZStd::to_string(static_cast<AZ::u64>(entityId)));
            state->SetParent(undoBatch);
            state->Capture(beforeState, afterState, entityId);

            state->Redo(instance);
        }

        void PrefabPublicHandler::Internal_HandleInstanceChange(
            UndoSystem::URSequencePoint* undoBatch, AZ::Entity* entity, AZ::EntityId beforeParentId, AZ::EntityId afterParentId)
        {
            // If the entity parent changed, verify if the owning instance changed too
            InstanceOptionalReference beforeOwningInstance = m_instanceEntityMapperInterface->FindOwningInstance(beforeParentId);
            InstanceOptionalReference afterOwningInstance = m_instanceEntityMapperInterface->FindOwningInstance(afterParentId);

            EntityList entities;
            AZStd::vector<Instance*> instances;

            // Retrieve all descendant entities and instances of this entity that belonged to the same owning instance.
            PrefabOperationResult retrieveEntitiesAndInstancesOutcome = RetrieveAndSortPrefabEntitiesAndInstances(
                { entity }, beforeOwningInstance->get(), entities, instances);
            AZ_Error("Prefab", retrieveEntitiesAndInstancesOutcome.IsSuccess(), retrieveEntitiesAndInstancesOutcome.GetError().data());

            AZStd::vector<AZStd::unique_ptr<Instance>> instanceUniquePtrs;
            AZStd::vector<AZStd::pair<Instance*, PrefabDom>> instancePatches;

            // Remove Entities and Instances from the prior instance
            {
                // Remove Instances
                for (Instance* nestedInstance : instances)
                {
                    auto linkRef = m_prefabSystemComponentInterface->FindLink(nestedInstance->GetLinkId());

                    PrefabDom oldLinkPatches;

                    if (linkRef.has_value())
                    {
                        auto patches = linkRef->get().GetLinkPatches();
                        if (patches.has_value())
                        {
                            oldLinkPatches.CopyFrom(patches->get(), oldLinkPatches.GetAllocator());
                        }
                    }

                    auto nestedInstanceUniquePtr = beforeOwningInstance->get().DetachNestedInstance(nestedInstance->GetInstanceAlias());
                    RemoveLink(nestedInstanceUniquePtr, beforeOwningInstance->get().GetTemplateId(), undoBatch);

                    instancePatches.emplace_back(AZStd::make_pair(nestedInstanceUniquePtr.get(), AZStd::move(oldLinkPatches)));
                    instanceUniquePtrs.emplace_back(AZStd::move(nestedInstanceUniquePtr));
                }

                // Get the previous state of the prior instance for undo/redo purposes
                PrefabDom beforeInstanceDomBeforeRemoval;
                m_instanceToTemplateInterface->GenerateDomForInstance(beforeInstanceDomBeforeRemoval, beforeOwningInstance->get());

                // Remove Entities
                for (AZ::Entity* nestedEntity : entities)
                {
                    beforeOwningInstance->get().DetachEntity(nestedEntity->GetId()).release();
                }

                // Create the Update node for the prior owning instance
                // Instance removal will be taken care of from the RemoveLink function for undo/redo purposes
                PrefabUndoHelpers::UpdatePrefabInstance(
                    beforeOwningInstance->get(), "Update prior prefab instance", beforeInstanceDomBeforeRemoval, undoBatch);
            }

            // Add Entities and Instances to new instance
            {
                // Add Instances
                for (auto& instanceUniquePtr : instanceUniquePtrs)
                {
                    afterOwningInstance->get().AddInstance(AZStd::move(instanceUniquePtr));
                }

                // Create Links
                for (auto& instanceInfo : instancePatches)
                {
                    // Add a new link with the old dom
                    CreateLink(
                        *instanceInfo.first, afterOwningInstance->get().GetTemplateId(), undoBatch,
                        AZStd::move(instanceInfo.second));
                }

                // Get the previous state of the new instance for undo/redo purposes
                PrefabDom afterInstanceDomBeforeAdd;
                m_instanceToTemplateInterface->GenerateDomForInstance(afterInstanceDomBeforeAdd, afterOwningInstance->get());

                // Add Entities
                for (AZ::Entity* nestedEntity : entities)
                {
                    afterOwningInstance->get().AddEntity(*nestedEntity);
                }

                // Create the Update node for the new owning instance
                PrefabUndoHelpers::UpdatePrefabInstance(
                    afterOwningInstance->get(), "Update new prefab instance", afterInstanceDomBeforeAdd, undoBatch);
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

        DuplicatePrefabResult PrefabPublicHandler::DuplicateEntitiesInInstance(const EntityIdList& entityIds)
        {
            if (entityIds.empty())
            {
                return AZ::Failure(AZStd::string("No entities to duplicate."));
            }

            const EntityIdList entityIdsNoFocusContainer = GenerateEntityIdListWithoutFocusedInstanceContainer(entityIds);
            if (entityIdsNoFocusContainer.empty())
            {
                return AZ::Failure(AZStd::string("No entities to duplicate because only instance selected is the container entity of the focused instance."));
            }

            if (!EntitiesBelongToSameInstance(entityIdsNoFocusContainer))
            {
                return AZ::Failure(AZStd::string("Cannot duplicate multiple entities belonging to different instances with one operation."
                    "Change your selection to contain entities in the same instance."));
            }

            // We've already verified the entities are all owned by the same instance,
            // so we can just retrieve our instance from the first entity in the list.
            AZ::EntityId firstEntityIdToDuplicate = entityIdsNoFocusContainer[0];
            InstanceOptionalReference commonOwningInstance = GetOwnerInstanceByEntityId(firstEntityIdToDuplicate);
            if (!commonOwningInstance.has_value())
            {
                return AZ::Failure(AZStd::string("Failed to duplicate : Couldn't get a valid owning instance for the common root entity of the entities provided."));
            }

            // If the first entity id is a container entity id, then we need to mark its parent as the common owning instance
            // This is because containers, despite representing the nested instance in the parent, are owned by the child.
            if (commonOwningInstance->get().GetContainerEntityId() == firstEntityIdToDuplicate)
            {
                commonOwningInstance = commonOwningInstance->get().GetParentInstance();
            }
            if (!commonOwningInstance.has_value())
            {
                return AZ::Failure(AZStd::string("Failed to duplicate : Couldn't get a valid owning instance for the common root entity of the entities provided."));
            }

            // This will cull out any entities that have ancestors in the list, since we will end up duplicating
            // the full nested hierarchy with what is returned from RetrieveAndSortPrefabEntitiesAndInstances
            AzToolsFramework::EntityIdSet duplicationSet = AzToolsFramework::GetCulledEntityHierarchy(entityIdsNoFocusContainer);

            AZ_PROFILE_FUNCTION(AzToolsFramework);

            ScopedUndoBatch undoBatch("Duplicate Entities");

            EntityIdList duplicatedEntityAndInstanceIds;
            {
                AZ_PROFILE_SCOPE(AzToolsFramework, "DuplicateEntitiesInInstance::UndoCaptureAndDuplicateEntities");

                AZStd::vector<AZ::Entity*> entities;
                AZStd::vector<Instance*> instances;

                EntityList inputEntityList = EntityIdSetToEntityList(duplicationSet);
                PrefabOperationResult retrieveEntitiesAndInstancesOutcome =
                    RetrieveAndSortPrefabEntitiesAndInstances(inputEntityList, commonOwningInstance->get(), entities, instances);

                if (!retrieveEntitiesAndInstancesOutcome.IsSuccess())
                {
                    return AZ::Failure(retrieveEntitiesAndInstancesOutcome.TakeError());
                }

                // Take a snapshot of the instance DOM before we manipulate it
                PrefabDom instanceDomBefore;
                m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBefore, commonOwningInstance->get());

                // Make a copy of our before instance DOM where we will add our duplicated entities and/or instances
                PrefabDom instanceDomAfter;
                instanceDomAfter.CopyFrom(instanceDomBefore, instanceDomAfter.GetAllocator());

                // Duplicate any nested entities and instances as requested
                AZStd::unordered_map<InstanceAlias, Instance*> newInstanceAliasToOldInstanceMap;
                AZStd::unordered_map<EntityAlias, EntityAlias> duplicateEntityAliasMap;
                DuplicateNestedEntitiesInInstance(commonOwningInstance->get(),
                    entities, instanceDomAfter, duplicatedEntityAndInstanceIds, duplicateEntityAliasMap);

                PrefabUndoInstance* command = aznew PrefabUndoInstance("Entity/Instance duplication", false);
                command->SetParent(undoBatch.GetUndoBatch());
                command->Capture(instanceDomBefore, instanceDomAfter, commonOwningInstance->get().GetTemplateId());
                command->Redo();

                DuplicateNestedInstancesInInstance(commonOwningInstance->get(),
                    instances, instanceDomAfter, duplicatedEntityAndInstanceIds, newInstanceAliasToOldInstanceMap);

                // Create links for our duplicated instances (if any were duplicated)
                for (auto [newInstanceAlias, oldInstance] : newInstanceAliasToOldInstanceMap)
                {
                    LinkId oldLinkId = oldInstance->GetLinkId();
                    auto linkRef = m_prefabSystemComponentInterface->FindLink(oldLinkId);
                    AZ_Assert(
                        linkRef.has_value(), "Unable to find link with id '%llu' during instance duplication.",
                        oldLinkId);

                    PrefabDomValueReference linkPatches = linkRef->get().GetLinkPatches();
                    AZ_Assert(
                        linkPatches.has_value(), "Link with id '%llu' is missing patches.",
                        oldLinkId);

                    PrefabDom linkPatchesCopy;
                    linkPatchesCopy.CopyFrom(linkPatches->get(), linkPatchesCopy.GetAllocator());

                    // If the instance was duplicated as part of an ancestor's nested hierarchy, the container's parent patch
                    // will need to be refreshed to point to the new duplicated parent entity
                    auto oldInstanceContainerEntityId = oldInstance->GetContainerEntityId();
                    AZ_Assert(oldInstanceContainerEntityId.IsValid(), "Instance returned invalid Container Entity Id");

                    AZ::EntityId previousParentEntityId;
                    AZ::TransformBus::EventResult(previousParentEntityId, oldInstanceContainerEntityId, &AZ::TransformBus::Events::GetParentId);

                    if (previousParentEntityId.IsValid() && AZStd::find(duplicatedEntityAndInstanceIds.begin(), duplicatedEntityAndInstanceIds.end(), previousParentEntityId))
                    {
                        auto oldParentAlias = commonOwningInstance->get().GetEntityAlias(previousParentEntityId);
                        if (oldParentAlias.has_value() && duplicateEntityAliasMap.contains(oldParentAlias->get()))
                        {
                            // Get the dom into a QString for search/replace purposes
                            rapidjson::StringBuffer buffer;
                            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                            linkPatchesCopy.Accept(writer);

                            QString linkPatchesString(buffer.GetString());

                            ReplaceOldAliases(linkPatchesString, oldParentAlias->get(), duplicateEntityAliasMap[oldParentAlias->get()]);

                            linkPatchesCopy.Parse(linkPatchesString.toUtf8().constData());
                        }
                    }

                    PrefabUndoHelpers::CreateLink(
                        oldInstance->GetTemplateId(), commonOwningInstance->get().GetTemplateId(),
                        AZStd::move(linkPatchesCopy), newInstanceAlias, undoBatch.GetUndoBatch());
                }

                // Select the duplicated entities/instances
                auto selectionUndo = aznew SelectionCommand(duplicatedEntityAndInstanceIds, "Select Duplicated Entities/Instances");
                selectionUndo->SetParent(undoBatch.GetUndoBatch());
                ToolsApplicationRequestBus::Broadcast(
                    &ToolsApplicationRequestBus::Events::SetSelectedEntities, duplicatedEntityAndInstanceIds);
            }

            return AZ::Success(AZStd::move(duplicatedEntityAndInstanceIds));
        }

        PrefabOperationResult PrefabPublicHandler::DeleteFromInstance(const EntityIdList& entityIds, bool deleteDescendants)
        {
            // Remove the container entity of the focused prefab from the list, if it is included.
            const EntityIdList entityIdsNoFocusContainer = GenerateEntityIdListWithoutFocusedInstanceContainer(entityIds);

            if (entityIdsNoFocusContainer.empty())
            {
                return AZ::Success();
            }

            // All entities in this list need to belong to the same prefab instance for the operation to be valid.
            if (!EntitiesBelongToSameInstance(entityIdsNoFocusContainer))
            {
                return AZ::Failure(AZStd::string("Cannot delete multiple entities belonging to different instances with one operation."));
            }

            AZ::EntityId firstEntityIdToDelete = entityIdsNoFocusContainer[0];
            InstanceOptionalReference commonOwningInstance = GetOwnerInstanceByEntityId(firstEntityIdToDelete);

            // If the first entity id is a container entity id, then we need to mark its parent as the common owning instance because you
            // cannot delete an instance from itself.
            if (commonOwningInstance->get().GetContainerEntityId() == firstEntityIdToDelete)
            {
                commonOwningInstance = commonOwningInstance->get().GetParentInstance();
            }

            // We only allow explicit deletions for entities inside the currently focused prefab.
            AzFramework::EntityContextId editorEntityContextId = AzToolsFramework::GetEntityContextId();
            if (&m_prefabFocusInterface->GetFocusedPrefabInstance(editorEntityContextId)->get() != &commonOwningInstance->get())
            {
                return AZ::Failure(AZStd::string("Cannot delete entities belonging to an instance that is not being edited."));
            }

            // Retrieve entityList from entityIds
            EntityList inputEntityList = EntityIdListToEntityList(entityIdsNoFocusContainer);

            AZ_PROFILE_FUNCTION(AzToolsFramework);

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
                AZ_PROFILE_SCOPE(AzToolsFramework, "Internal::DeleteEntities:UndoCaptureAndPurgeEntities");

                Prefab::PrefabDom instanceDomBefore;
                m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBefore, commonOwningInstance->get());

                if (deleteDescendants)
                {
                    AZStd::vector<AZ::Entity*> entities;
                    AZStd::vector<Instance*> instances;

                    PrefabOperationResult retrieveEntitiesAndInstancesOutcome =
                        RetrieveAndSortPrefabEntitiesAndInstances(inputEntityList, commonOwningInstance->get(), entities, instances);

                    if (!retrieveEntitiesAndInstancesOutcome.IsSuccess())
                    {
                        return AZStd::move(retrieveEntitiesAndInstancesOutcome);
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
                    for (AZ::EntityId entityId : entityIdsNoFocusContainer)
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
                AZ_PROFILE_SCOPE(AzToolsFramework, "Internal::DeleteEntities:RunRedo");
                selCommand->RunRedo();
            }

            return AZ::Success();
        }

        PrefabOperationResult PrefabPublicHandler::DetachPrefab(const AZ::EntityId& containerEntityId)
        {
            if (!containerEntityId.IsValid())
            {
                return AZ::Failure(AZStd::string("Cannot detach Prefab Instance with invalid container entity."));
            }

            auto editorEntityContextId = AzFramework::EntityContextId::CreateNull();
            EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

            if (containerEntityId == m_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(editorEntityContextId))
            {
                return AZ::Failure(AZStd::string("Cannot detach focused Prefab Instance."));
            }

            InstanceOptionalReference owningInstance = GetOwnerInstanceByEntityId(containerEntityId);
            if (owningInstance->get().GetContainerEntityId() != containerEntityId)
            {
                return AZ::Failure(AZStd::string("Input entity should be its owning Instance's container entity."));
            }

            AZ_PROFILE_FUNCTION(AzToolsFramework);

            {
                AZ_PROFILE_SCOPE(AzToolsFramework, "Internal::DetachPrefab:UndoCapture");

                ScopedUndoBatch undoBatch("Detach Prefab");

                InstanceOptionalReference getParentInstanceResult = owningInstance->get().GetParentInstance();
                AZ_Assert(getParentInstanceResult.has_value(), "Can't get parent Instance from Instance of given container entity.");

                auto& parentInstance = getParentInstanceResult->get();
                const auto parentTemplateId = parentInstance.GetTemplateId();

                {
                    auto instancePtr = parentInstance.DetachNestedInstance(owningInstance->get().GetInstanceAlias());
                    AZ_Assert(instancePtr, "Can't detach selected Instance from its parent Instance.");

                    RemoveLink(instancePtr, parentTemplateId, undoBatch.GetUndoBatch());

                    Prefab::PrefabDom instanceDomBefore;
                    m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBefore, parentInstance);

                    AZStd::unordered_map<AZ::EntityId, AZStd::string> oldEntityAliases;
                    oldEntityAliases.emplace(containerEntityId, instancePtr->GetEntityAlias(containerEntityId)->get());

                    auto containerEntityPtr = instancePtr->DetachContainerEntity();
                    auto& containerEntity = *containerEntityPtr.release();
                    auto editorPrefabComponent = containerEntity.FindComponent<EditorPrefabComponent>();
                    containerEntity.Deactivate();
                    [[maybe_unused]] const bool editorPrefabComponentRemoved = containerEntity.RemoveComponent(editorPrefabComponent);
                    AZ_Assert(editorPrefabComponentRemoved, "Remove EditorPrefabComponent failed.");
                    delete editorPrefabComponent;
                    containerEntity.Activate();

                    [[maybe_unused]] const bool containerEntityAdded = parentInstance.AddEntity(containerEntity);
                    AZ_Assert(containerEntityAdded, "Add target Instance's container entity to its parent Instance failed.");

                    EntityIdList entityIds;
                    entityIds.emplace_back(containerEntity.GetId());

                    instancePtr->GetEntities(
                        [&](AZStd::unique_ptr<AZ::Entity>& entityPtr)
                    {
                        oldEntityAliases.emplace(entityPtr->GetId(), instancePtr->GetEntityAlias(entityPtr->GetId())->get());
                        return true;
                    });

                    instancePtr->DetachEntities(
                        [&](AZStd::unique_ptr<AZ::Entity> entityPtr)
                    {
                        auto& entity = *entityPtr.release();
                        [[maybe_unused]] const bool entityAdded = parentInstance.AddEntity(entity);
                        AZ_Assert(entityAdded, "Add target Instance's entity to its parent Instance failed.");

                        entityIds.emplace_back(entity.GetId());
                    });

                    Prefab::PrefabDom instanceDomAfter;
                    m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomAfter, parentInstance);

                    PrefabUndoInstance* command = aznew PrefabUndoInstance("Instance detachment", false);
                    command->Capture(instanceDomBefore, instanceDomAfter, parentTemplateId);
                    command->SetParent(undoBatch.GetUndoBatch());
                    {
                        AZ_PROFILE_SCOPE(AzToolsFramework, "Internal::DetachPrefab:RunRedo");
                        command->RunRedo();
                    }

                    instancePtr->DetachNestedInstances(
                        [&](AZStd::unique_ptr<Instance> detachedNestedInstance)
                    {
                        PrefabDom& nestedInstanceTemplateDom =
                            m_prefabSystemComponentInterface->FindTemplateDom(detachedNestedInstance->GetTemplateId());

                        Instance& nestedInstanceUnderNewParent = parentInstance.AddInstance(AZStd::move(detachedNestedInstance));
                        
                        PrefabDom nestedInstanceDomUnderNewParent;
                        m_instanceToTemplateInterface->GenerateDomForInstance(
                            nestedInstanceDomUnderNewParent, nestedInstanceUnderNewParent);
                        PrefabDom reparentPatch;
                        m_instanceToTemplateInterface->GeneratePatch(
                            reparentPatch, nestedInstanceTemplateDom, nestedInstanceDomUnderNewParent);
                        
                        CreateLink(nestedInstanceUnderNewParent, parentTemplateId, undoBatch.GetUndoBatch(), AZStd::move(reparentPatch), true);
                    });
                }

                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequestBus::Events::ClearDirtyEntities);
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

        PrefabOperationResult PrefabPublicHandler::RetrieveAndSortPrefabEntitiesAndInstances(
            const EntityList& inputEntities,
            Instance& commonRootEntityOwningInstance,
            EntityList& outEntities,
            AZStd::vector<Instance*>& outInstances) const
        {
            if (inputEntities.size() == 0)
            {
                return AZ::Failure(
                    AZStd::string("An empty list of input entities is provided to retrieve the prefab entities and instances."));
            }

            AZStd::queue<AZ::Entity*> entityQueue;

            auto editorEntityContextId = AzFramework::EntityContextId::CreateNull();
            EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

            AZ::EntityId focusedPrefabContainerEntityId =
                m_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(editorEntityContextId);
            for (auto inputEntity : inputEntities)
            {
                if (inputEntity && inputEntity->GetId() != focusedPrefabContainerEntityId)
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
                    "Owning instance of entity with name '%s' and id '%llu' couldn't be found",
                    entity->GetName().c_str(), static_cast<AZ::u64>(entity->GetId()));

                // Check if this entity is owned by the same instance owning the root.
                if (&owningInstance->get() == &commonRootEntityOwningInstance)
                {
                    // If it's the same instance, we can add this entity to the new instance entities.
                    size_t priorEntitiesSize = entities.size();
                    
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
                        return AZ::Failure(AZStd::string::format(
                            "Entity with name '%s' and id '%llu' has an owning instance that doesn't belong to the instance "
                            "hierarchy of the selected entities.",
                            entity->GetName().c_str(), static_cast<AZ::u64>(entity->GetId())));
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

            if ((outEntities.size() + outInstances.size()) == 0)
            {
                return AZ::Failure(
                    AZStd::string("An empty list of entities and prefab instances were retrieved from the selected entities"));
            }
            return AZ::Success();
        }

        EntityIdList PrefabPublicHandler::GenerateEntityIdListWithoutFocusedInstanceContainer(
            const EntityIdList& entityIds) const
        {
            EntityIdList outEntityIds(entityIds);

            AzFramework::EntityContextId editorEntityContextId = AzToolsFramework::GetEntityContextId();
            AZ::EntityId focusedInstanceContainerEntityId = m_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(editorEntityContextId);

            if (auto iter = AZStd::find(outEntityIds.begin(), outEntityIds.end(), focusedInstanceContainerEntityId); iter != outEntityIds.end())
            {
                outEntityIds.erase(iter);
            }
            
            return outEntityIds;
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

        void PrefabPublicHandler::DuplicateNestedEntitiesInInstance(Instance& commonOwningInstance,
            const AZStd::vector<AZ::Entity*>& entities, PrefabDom& domToAddDuplicatedEntitiesUnder,
            EntityIdList& duplicatedEntityIds, AZStd::unordered_map<EntityAlias, EntityAlias>& oldAliasToNewAliasMap)
        {
            if (entities.empty())
            {
                return;
            }

            AZStd::unordered_map<EntityAlias, QString> aliasToEntityDomMap;

            for (AZ::Entity* entity : entities)
            {
                EntityAliasOptionalReference oldAliasRef = commonOwningInstance.GetEntityAlias(entity->GetId());
                AZ_Assert(oldAliasRef.has_value(), "No alias found for Entity in the DOM");
                EntityAlias oldAlias = oldAliasRef.value();

                // Give this the outer allocator so that the memory reference will be valid when
                // it gets used for AddMember
                PrefabDom entityDomBefore(&domToAddDuplicatedEntitiesUnder.GetAllocator());
                m_instanceToTemplateInterface->GenerateDomForEntity(entityDomBefore, *entity);

                // Keep track of the old alias <-> new alias mapping for this duplicated entity
                // so we can fixup references later
                EntityAlias newEntityAlias = Instance::GenerateEntityAlias();
                oldAliasToNewAliasMap.emplace(oldAlias, newEntityAlias);

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                entityDomBefore.Accept(writer);

                // Store our duplicated Entity DOM with its new alias as a string
                // so that we can fixup entity alias references before adding it
                // to the Entities member of our instance DOM
                QString entityDomString(buffer.GetString());
                aliasToEntityDomMap.emplace(newEntityAlias, entityDomString);
            }

            auto entitiesIter = domToAddDuplicatedEntitiesUnder.FindMember(PrefabDomUtils::EntitiesName);
            AZ_Assert(entitiesIter != domToAddDuplicatedEntitiesUnder.MemberEnd(), "Instance DOM missing the Entities member.");

            // Now that all the duplicated Entity DOMs have been created, we need to iterate
            // through them and replace any previous EntityAlias references with the new ones.
            // These are more than just parent entity references for nested entities, this will
            // also cover any EntityId references that were made in the components between them.
            for (auto [newEntityAlias, newEntityDomString] : aliasToEntityDomMap)
            {
                // Replace all of the old alias references with the new ones
                for (auto [oldAlias, newAlias] : oldAliasToNewAliasMap)
                {
                    ReplaceOldAliases(newEntityDomString, oldAlias, newAlias);
                }

                // Create the new Entity DOM from parsing the JSON string
                PrefabDom entityDomAfter(&domToAddDuplicatedEntitiesUnder.GetAllocator());
                entityDomAfter.Parse(newEntityDomString.toUtf8().constData());

                // Add the new Entity DOM to the Entities member of the instance
                rapidjson::Value aliasName(newEntityAlias.c_str(), static_cast<rapidjson::SizeType>(newEntityAlias.length()), domToAddDuplicatedEntitiesUnder.GetAllocator());
                entitiesIter->value.AddMember(AZStd::move(aliasName), entityDomAfter, domToAddDuplicatedEntitiesUnder.GetAllocator());
            }

            for (auto aliasMapIter : oldAliasToNewAliasMap)
            {
                EntityAlias newEntityAlias = aliasMapIter.second;

                AliasPath absoluteEntityPath = commonOwningInstance.GetAbsoluteInstanceAliasPath();
                absoluteEntityPath.Append(newEntityAlias);

                AZ::EntityId newEntityId = InstanceEntityIdMapper::GenerateEntityIdForAliasPath(absoluteEntityPath);
                duplicatedEntityIds.push_back(newEntityId);
            }
        }

        void PrefabPublicHandler::DuplicateNestedInstancesInInstance(Instance& commonOwningInstance,
            const AZStd::vector<Instance*>& instances, PrefabDom& domToAddDuplicatedInstancesUnder,
            EntityIdList& duplicatedEntityIds, AZStd::unordered_map<InstanceAlias, Instance*>& newInstanceAliasToOldInstanceMap)
        {
            if (instances.empty())
            {
                return;
            }

            AZStd::unordered_map<InstanceAlias, InstanceAlias> oldInstanceAliasToNewInstanceAliasMap;
            AZStd::unordered_map<InstanceAlias, QString> aliasToInstanceDomMap;

            for (auto instance : instances)
            {
                PrefabDom nestedInstanceDomBefore;
                m_instanceToTemplateInterface->GenerateDomForInstance(nestedInstanceDomBefore, *instance);

                // Keep track of the old alias <-> new alias mapping for this duplicated instance
                // so we can fixup references later
                InstanceAlias oldAlias = instance->GetInstanceAlias();
                InstanceAlias newInstanceAlias = Instance::GenerateInstanceAlias();
                oldInstanceAliasToNewInstanceAliasMap.emplace(oldAlias, newInstanceAlias);

                // Keep track of our new instance alias with the Instance it was duplicated from,
                // so that after all instances are duplicated, we can go back and create links for them
                newInstanceAliasToOldInstanceMap.emplace(newInstanceAlias, instance);

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                nestedInstanceDomBefore.Accept(writer);

                // Store our duplicated Instance DOM with its new alias as a string
                // so that we can fixup instance alias references before adding it
                // to the Instances member of our instance DOM
                QString instanceDomString(buffer.GetString());
                aliasToInstanceDomMap.emplace(newInstanceAlias, instanceDomString);
            }

            auto instancesIter = domToAddDuplicatedInstancesUnder.FindMember(PrefabDomUtils::InstancesName);
            AZ_Assert(instancesIter != domToAddDuplicatedInstancesUnder.MemberEnd(), "Instance DOM missing the Instances member.");

            // Now that all the duplicated Instance DOMs have been created, we need to iterate
            // through them and replace any previous InstanceAlias references with the new ones.
            for (auto [newInstanceAlias, newInstanceDomString]: aliasToInstanceDomMap)
            {
                // Replace all of the old alias references with the new ones
                for (auto [oldAlias, newAlias] : oldInstanceAliasToNewInstanceAliasMap)
                {
                    ReplaceOldAliases(newInstanceDomString, oldAlias, newAlias);
                }

                // Create the new Instance DOM from parsing the JSON string
                PrefabDom nestedInstanceDomAfter(&domToAddDuplicatedInstancesUnder.GetAllocator());
                nestedInstanceDomAfter.Parse(newInstanceDomString.toUtf8().constData());

                // Add the new Instance DOM to the Instances member of the instance
                rapidjson::Value aliasName(newInstanceAlias.c_str(), static_cast<rapidjson::SizeType>(newInstanceAlias.length()), domToAddDuplicatedInstancesUnder.GetAllocator());
                instancesIter->value.AddMember(AZStd::move(aliasName), nestedInstanceDomAfter, domToAddDuplicatedInstancesUnder.GetAllocator());
            }

            for (auto aliasMapIter : oldInstanceAliasToNewInstanceAliasMap)
            {
                InstanceAlias newInstanceAlias = aliasMapIter.second;

                AliasPath absoluteInstancePath = commonOwningInstance.GetAbsoluteInstanceAliasPath();
                absoluteInstancePath.Append(newInstanceAlias);
                absoluteInstancePath.Append(PrefabDomUtils::ContainerEntityName);

                AZ::EntityId newEntityId = InstanceEntityIdMapper::GenerateEntityIdForAliasPath(absoluteInstancePath);
                duplicatedEntityIds.push_back(newEntityId);
            }
        }

        void PrefabPublicHandler::ReplaceOldAliases(QString& stringToReplace, AZStd::string_view oldAlias, AZStd::string_view newAlias)
        {
            // Replace all of the old alias references with the new ones
            // We bookend the aliases with \" and also with a / as an extra precaution to prevent
            // inadvertently replacing a matching string vs. where an actual EntityId is expected
            // This will cover both cases where an alias could be used in a normal entity vs. an instance
            QString oldAliasQuotes = QString("\"%1\"").arg(oldAlias.data());
            QString newAliasQuotes = QString("\"%1\"").arg(newAlias.data());

            stringToReplace.replace(oldAliasQuotes, newAliasQuotes);

            QString oldAliasPathRef = QString("/%1").arg(oldAlias.data());
            QString newAliasPathRef = QString("/%1").arg(newAlias.data());

            stringToReplace.replace(oldAliasPathRef, newAliasPathRef);
        }

        void PrefabPublicHandler::UpdateLinkPatchesWithNewEntityAliases(
            PrefabDom& linkPatch,
            const AZStd::unordered_map<AZ::EntityId, AZStd::string>& oldEntityAliases,
            Instance& newParent)
        {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            linkPatch.Accept(writer);
            QString previousPatchString(buffer.GetString());

            for (const auto& [entityId, oldEntityAlias] : oldEntityAliases)
            {
                EntityAliasOptionalReference newEntityAlias = newParent.GetEntityAlias(entityId);
                AZ_Assert(
                    newEntityAlias.has_value(),
                    "Could not fetch entity alias for entity with id '%llu' during prefab creation.",
                    static_cast<AZ::u64>(entityId));

                ReplaceOldAliases(previousPatchString, oldEntityAlias, newEntityAlias->get());
            }

            linkPatch.Parse(previousPatchString.toUtf8().constData());
        }

    } // namespace Prefab
} // namespace AzToolsFramework
