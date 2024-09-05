/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Spawnable/RootSpawnableInterface.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipService.h>
#include <AzToolsFramework/Prefab/EditorPrefabComponent.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutorInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabLoader.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabUndoHelpers.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabConverterStackProfileNames.h>

namespace AzToolsFramework
{
    PrefabEditorEntityOwnershipService::PrefabEditorEntityOwnershipService(const AzFramework::EntityContextId& entityContextId,
        AZ::SerializeContext* serializeContext)
        : m_entityContextId(entityContextId)
        , m_serializeContext(serializeContext)
    {
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            m_shouldAssertForLegacySlicesUsage, &AzFramework::ApplicationRequests::ShouldAssertForLegacySlicesUsage);
        AZ::Interface<PrefabEditorEntityOwnershipInterface>::Register(this);
    }

    PrefabEditorEntityOwnershipService::~PrefabEditorEntityOwnershipService()
    {
        AZ::Interface<PrefabEditorEntityOwnershipInterface>::Unregister(this);
    }

    void PrefabEditorEntityOwnershipService::Initialize()
    {
        m_prefabFocusInterface = AZ::Interface<Prefab::PrefabFocusInterface>::Get();
        AZ_Assert(m_prefabFocusInterface != nullptr,
            "Couldn't get prefab focus interface, it's a requirement for PrefabEntityOwnership system to work");

        m_instanceEntityMapperInterface = AZ::Interface<Prefab::InstanceEntityMapperInterface>::Get();
        AZ_Assert(m_instanceEntityMapperInterface,
            "Couldn't get instance entity mapper interface. It's a requirement for PrefabEntityOwnership to work");

        m_prefabSystemComponent = AZ::Interface<Prefab::PrefabSystemComponentInterface>::Get();
        AZ_Assert(m_prefabSystemComponent != nullptr,
            "Couldn't get prefab system component, it's a requirement for PrefabEntityOwnership system to work");

        m_loaderInterface = AZ::Interface<Prefab::PrefabLoaderInterface>::Get();
        AZ_Assert(m_loaderInterface != nullptr,
            "Couldn't get prefab loader interface, it's a requirement for PrefabEntityOwnership system to work");

        m_rootInstance =
            AZStd::unique_ptr<Prefab::Instance>(m_prefabSystemComponent->CreatePrefab(AzToolsFramework::EntityList{}, {}, "newLevel.prefab"));
        m_sliceOwnershipService.BusConnect(m_entityContextId);
        m_sliceOwnershipService.m_shouldAssertForLegacySlicesUsage = m_shouldAssertForLegacySlicesUsage;
    }

    bool PrefabEditorEntityOwnershipService::IsInitialized()
    {
        return m_rootInstance != nullptr;
    }

    void PrefabEditorEntityOwnershipService::Destroy()
    {
        StopPlayInEditor();

        if (m_rootInstance != nullptr)
        {
            m_rootInstance.reset();
            m_prefabSystemComponent->RemoveAllTemplates();
        }
    }

    void PrefabEditorEntityOwnershipService::Reset()
    {
        m_isRootPrefabAssigned = false;

        if (m_rootInstance)
        {
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequestBus::Events::ClearDirtyEntities);
            Prefab::TemplateId templateId = m_rootInstance->GetTemplateId();
            m_rootInstance->Reset();
            if (templateId != Prefab::InvalidTemplateId)
            {
                m_rootInstance->SetTemplateId(Prefab::InvalidTemplateId);
                m_prefabSystemComponent->RemoveAllTemplates();
            }
            m_rootInstance->SetContainerEntityName("Level");
        }

        AzFramework::EntityOwnershipServiceNotificationBus::Event(
            m_entityContextId, &AzFramework::EntityOwnershipServiceNotificationBus::Events::OnEntityOwnershipServiceReset);
    }

    void PrefabEditorEntityOwnershipService::AddEntity(AZ::Entity* entity)
    {
        AZ_Assert(IsInitialized(), "Tried to add an entity without initializing the Entity Ownership Service");

        // Setup undo node.
        ScopedUndoBatch undoBatch("Add entity");

        // Determine which prefab instance should own this entity.
        Prefab::InstanceOptionalReference newOwningInstance = m_prefabFocusInterface->GetFocusedPrefabInstance(m_entityContextId);
        if (!newOwningInstance.has_value())
        {
            AZ_Assert(false, "Entity Ownership Service could not retrieve currently focused prefab.");
            return;
        }

        Prefab::PrefabDom instanceDomBeforeUpdate;
        Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(newOwningInstance->get(), instanceDomBeforeUpdate);

        newOwningInstance->get().AddEntity(*entity);
        HandleEntitiesAdded({ entity });
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformInterface::SetParent, newOwningInstance->get().GetContainerEntityId());

        Prefab::PrefabUndoHelpers::UpdatePrefabInstance(
            newOwningInstance->get(), "Add entity", instanceDomBeforeUpdate, undoBatch.GetUndoBatch());
    }

    void PrefabEditorEntityOwnershipService::AddEntities(const EntityList& entities)
    {
        AZ_Assert(IsInitialized(), "Tried to add entities without initializing the Entity Ownership Service");

        // Setup undo node.
        ScopedUndoBatch undoBatch("Add entities");

        // Determine which prefab instance should own these entities.
        Prefab::InstanceOptionalReference newOwningInstance = m_prefabFocusInterface->GetFocusedPrefabInstance(m_entityContextId);
        if (!newOwningInstance.has_value())
        {
            AZ_Assert(false, "Entity Ownership Service could not retrieve currently focused prefab.");
            return;
        }

        Prefab::PrefabDom instanceDomBeforeUpdate;
        Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(newOwningInstance->get(), instanceDomBeforeUpdate);

        for (AZ::Entity* entity : entities)
        {
            newOwningInstance->get().AddEntity(*entity);
        }

        HandleEntitiesAdded(entities);

        for (AZ::Entity* entity : entities)
        {
            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformInterface::SetParent, newOwningInstance->get().m_containerEntity->GetId());
        }

        Prefab::PrefabUndoHelpers::UpdatePrefabInstance(
            newOwningInstance->get(), "Undo adding entities", instanceDomBeforeUpdate, undoBatch.GetUndoBatch());
    }

    bool PrefabEditorEntityOwnershipService::DestroyEntity(AZ::Entity* entity)
    {
        AZ_Assert(entity, "Tried to destroy a null entity");
        if (!entity)
        {
            return false;
        }

        return DestroyEntityById(entity->GetId());
    }

    bool PrefabEditorEntityOwnershipService::DestroyEntityById(AZ::EntityId entityId)
    {
        AZ_Assert(IsInitialized(), "Tried to destroy an entity without initializing the Entity Ownership Service");
        OnEntityRemoved(entityId);
        bool destroyResult = false;
        if (auto owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId); owningInstance.has_value())
        {
            if (owningInstance->get().GetContainerEntityId() == entityId)
            {
                destroyResult = owningInstance->get().DestroyContainerEntity();
            }
            else
            {
                destroyResult = owningInstance->get().DestroyEntity(entityId);
            }
        }
        return destroyResult;
    }

    void PrefabEditorEntityOwnershipService::GetNonPrefabEntities(EntityList& entities)
    {
        m_rootInstance->GetEntities(
            [&entities](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                entities.emplace_back(entity.get());
                return true;
            });
    }

    bool PrefabEditorEntityOwnershipService::GetAllEntities(EntityList& entities)
    {
        m_rootInstance->GetAllEntitiesInHierarchy(
            [&entities](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                entities.emplace_back(entity.get());
                return true;
            });

        return true;
    }

    void PrefabEditorEntityOwnershipService::InstantiateAllPrefabs()
    {
    }

    void PrefabEditorEntityOwnershipService::HandleEntitiesAdded(const EntityList& entities)
    {
        AZ_Assert(m_entitiesAddedCallback, "Callback function for AddEntity has not been set.");
        for (const AZ::Entity* entity : entities)
        {
            AzFramework::SliceEntityRequestBus::MultiHandler::BusConnect(entity->GetId());
        }
        m_entitiesAddedCallback(entities);
    }

    bool PrefabEditorEntityOwnershipService::LoadFromStream(AZ::IO::GenericStream& stream, AZStd::string_view filename)
    {
        Reset();

        const size_t bufSize = stream.GetLength();
        AZStd::unique_ptr<char[]> buf(new char[bufSize]);
        AZ::IO::SizeType bytes = stream.Read(bufSize, buf.get());

        Prefab::TemplateId templateId = m_loaderInterface->LoadTemplateFromString(AZStd::string_view(buf.get(), bytes), filename);
        if (templateId == Prefab::InvalidTemplateId)
        {
            AZ_Error("Prefab", false, "Couldn't load prefab content from '%.*s'", AZ_STRING_ARG(filename));
            return false;
        }

        m_rootInstance->SetTemplateId(templateId);
        m_rootInstance->SetTemplateSourcePath(m_loaderInterface->GenerateRelativePath(filename));
        m_rootInstance->SetContainerEntityName("Level");

        auto instanceUpdateExecutorInterface = AZ::Interface<Prefab::InstanceUpdateExecutorInterface>::Get();
        if (!instanceUpdateExecutorInterface)
        {
            AZ_Assert(false,
                "InstanceUpdateExecutorInterface is unavailable for PrefabEditorEntityOwnershipService::LoadFromStream.");
            return false;
        }
        instanceUpdateExecutorInterface->QueueRootPrefabLoadedNotificationForNextPropagation();

        m_prefabSystemComponent->PropagateTemplateChanges(templateId);
        m_isRootPrefabAssigned = true;

        return true;
    }

    bool PrefabEditorEntityOwnershipService::LoadFromStream(
        [[maybe_unused]] AZ::IO::GenericStream& stream,
        [[maybe_unused]] bool remapIds,
        [[maybe_unused]] EntityIdToEntityIdMap* idRemapTable,
        [[maybe_unused]] const AZ::ObjectStream::FilterDescriptor& filterDesc)
    {
        return false;
    }

    bool PrefabEditorEntityOwnershipService::SaveToStream(AZ::IO::GenericStream& stream, AZStd::string_view filename)
    {
        AZ::IO::Path relativePath = m_loaderInterface->GenerateRelativePath(filename);

        m_rootInstance->SetTemplateSourcePath(relativePath);
        m_prefabSystemComponent->UpdateTemplateFilePath(m_rootInstance->GetTemplateId(), relativePath);

        AZStd::string out;
        if (!m_loaderInterface->SaveTemplateToString(m_rootInstance->GetTemplateId(), out))
        {
            return false;
        }

        const size_t bytesToWrite = out.size();
        const size_t bytesWritten = stream.Write(bytesToWrite, out.data());
        if(bytesWritten != bytesToWrite)
        {
            return false;
        }
        m_prefabSystemComponent->SetTemplateDirtyFlag(m_rootInstance->GetTemplateId(), false);
        return true;
    }

    void PrefabEditorEntityOwnershipService::CreateNewLevelPrefab(AZStd::string_view filename, const AZStd::string& templateFilename)
    {
        AZ::IO::Path relativePath = m_loaderInterface->GenerateRelativePath(filename);
        AzToolsFramework::Prefab::TemplateId templateId = m_prefabSystemComponent->GetTemplateIdFromFilePath(relativePath);

        m_rootInstance->SetTemplateSourcePath(relativePath);

        AZStd::string watchFolder;
        AZ::Data::AssetInfo assetInfo;
        bool sourceInfoFound = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, templateFilename.c_str(),
            assetInfo, watchFolder);

        if (sourceInfoFound)
        {
            AZStd::string fullPath;
            AZ::StringFunc::Path::Join(watchFolder.c_str(), assetInfo.m_relativePath.c_str(), fullPath);

            // Get the default prefab and copy the Dom over to the new template being saved
            Prefab::TemplateId defaultId = m_loaderInterface->LoadTemplateFromFile(fullPath.c_str());
            Prefab::PrefabDom& dom = m_prefabSystemComponent->FindTemplateDom(defaultId);

            Prefab::PrefabDom levelDefaultDom;
            levelDefaultDom.CopyFrom(dom, levelDefaultDom.GetAllocator());

            Prefab::PrefabDomPath sourcePath("/Source");
            sourcePath.Set(levelDefaultDom, relativePath.c_str());

            templateId = m_prefabSystemComponent->AddTemplate(relativePath, AZStd::move(levelDefaultDom));

            // Initialize the container entity if it is in constructed state. This will make the entity queryable before the first
            // time it gets deserialized.
            if (m_rootInstance->m_containerEntity && m_rootInstance->m_containerEntity->GetState() == AZ::Entity::State::Constructed)
            {
                m_rootInstance->m_containerEntity->Init();
            }
        }
        else
        {
            m_rootInstance->m_containerEntity->AddComponent(aznew Prefab::EditorPrefabComponent());
            HandleEntitiesAdded({ m_rootInstance->m_containerEntity.get() });

            AzToolsFramework::Prefab::PrefabDom dom;
            bool success = AzToolsFramework::Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(*m_rootInstance, dom,
                AzToolsFramework::Prefab::PrefabDomUtils::StoreFlags::StripLinkIds);
            if (!success)
            {
                AZ_Error(
                    "Prefab", false, "Failed to convert current root instance into a DOM when saving file '%.*s'", AZ_STRING_ARG(filename));
                return;
            }
            templateId = m_prefabSystemComponent->AddTemplate(relativePath, std::move(dom));
        }

        if (templateId == AzToolsFramework::Prefab::InvalidTemplateId)
        {
            AZ_Error("Prefab", false, "Couldn't create new template id '%i' when creating new level '%.*s'", templateId, AZ_STRING_ARG(filename));
            return;
        }

        Prefab::TemplateId prevTemplateId = m_rootInstance->GetTemplateId();
        m_rootInstance->SetTemplateId(templateId);

        if (prevTemplateId != Prefab::InvalidTemplateId && templateId != prevTemplateId)
        {
            // Make sure we only have one level template loaded at a time
            m_prefabSystemComponent->RemoveTemplate(prevTemplateId);
        }

        auto instanceUpdateExecutorInterface = AZ::Interface<Prefab::InstanceUpdateExecutorInterface>::Get();
        if (!instanceUpdateExecutorInterface)
        {
            AZ_Assert(false,
                "InstanceUpdateExecutorInterface is unavailable for PrefabEditorEntityOwnershipService::CreateNewLevelPrefab.");
            return;
        }
        instanceUpdateExecutorInterface->QueueRootPrefabLoadedNotificationForNextPropagation();

        m_prefabSystemComponent->PropagateTemplateChanges(templateId);
        m_isRootPrefabAssigned = true;
    }

    bool PrefabEditorEntityOwnershipService::IsRootPrefabAssigned() const
    {
        return m_isRootPrefabAssigned;
    }

    Prefab::InstanceOptionalReference PrefabEditorEntityOwnershipService::CreatePrefab(
        const AZStd::vector<AZ::Entity*>& entities, AZStd::vector<AZStd::unique_ptr<Prefab::Instance>>&& nestedPrefabInstances,
        AZ::IO::PathView filePath, Prefab::InstanceOptionalReference instanceToParentUnder)
    {
        if (!instanceToParentUnder)
        {
            instanceToParentUnder = *m_rootInstance;
        }

        AZStd::unique_ptr<Prefab::Instance> createdPrefabInstance = m_prefabSystemComponent->CreatePrefab(
            entities, AZStd::move(nestedPrefabInstances), filePath, nullptr, instanceToParentUnder, false);

        if (createdPrefabInstance)
        {
            Prefab::Instance& addedInstance = instanceToParentUnder->get().AddInstance(
                AZStd::move(createdPrefabInstance));
            AZ::Entity* containerEntity = addedInstance.m_containerEntity.get();
            containerEntity->AddComponent(aznew Prefab::EditorPrefabComponent());
            HandleEntitiesAdded({containerEntity});
            HandleEntitiesAdded(entities);
            return addedInstance;
        }

        return AZStd::nullopt;
    }

    Prefab::InstanceOptionalReference PrefabEditorEntityOwnershipService::InstantiatePrefab(
        AZ::IO::PathView filePath, Prefab::InstanceOptionalReference instanceToParentUnder)
    {
        if (!instanceToParentUnder)
        {
            instanceToParentUnder = *m_rootInstance;
        }

        AZStd::unique_ptr<Prefab::Instance> instantiatedPrefabInstance = m_prefabSystemComponent->InstantiatePrefab(
            filePath, instanceToParentUnder,
            [this](const EntityList& entities)
            {
                HandleEntitiesAdded(entities); // includes container entity
            });

        if (instantiatedPrefabInstance)
        {
            Prefab::Instance& addedInstance = instanceToParentUnder->get().AddInstance(
                AZStd::move(instantiatedPrefabInstance));
            return addedInstance;
        }

        return AZStd::nullopt;
    }

    Prefab::InstanceOptionalReference PrefabEditorEntityOwnershipService::GetRootPrefabInstance()
    {
        AZ_Assert(m_rootInstance, "A valid root prefab instance couldn't be found in PrefabEditorEntityOwnershipService.");
        if (m_rootInstance)
        {
            return *m_rootInstance;
        }

        return AZStd::nullopt;
    }

    Prefab::TemplateId PrefabEditorEntityOwnershipService::GetRootPrefabTemplateId()
    {
        AZ_Assert(m_rootInstance, "A valid root prefab instance couldn't be found in PrefabEditorEntityOwnershipService.");
        return m_rootInstance ? m_rootInstance->GetTemplateId() : Prefab::InvalidTemplateId;
    }

    const AzFramework::InMemorySpawnableAssetContainer::SpawnableAssets& PrefabEditorEntityOwnershipService::GetPlayInEditorAssetData() const
    {
        return m_playInEditorData.m_assetsCache.GetAssetContainerConst().GetAllInMemorySpawnableAssets();
    }

    void PrefabEditorEntityOwnershipService::OnEntityRemoved(AZ::EntityId entityId)
    {
        AZ_Assert(m_entitiesRemovedCallback, "Callback function for entity removal has not been set.");
        AzFramework::SliceEntityRequestBus::MultiHandler::BusDisconnect(entityId);
        m_entitiesRemovedCallback({ entityId });
    }

    void PrefabEditorEntityOwnershipService::SetEntitiesAddedCallback(OnEntitiesAddedCallback onEntitiesAddedCallback)
    {
        m_entitiesAddedCallback = AZStd::move(onEntitiesAddedCallback);
    }

    void PrefabEditorEntityOwnershipService::SetEntitiesRemovedCallback(OnEntitiesRemovedCallback onEntitiesRemovedCallback)
    {
        m_entitiesRemovedCallback = AZStd::move(onEntitiesRemovedCallback);
    }

    void PrefabEditorEntityOwnershipService::SetValidateEntitiesCallback(ValidateEntitiesCallback validateEntitiesCallback)
    {
        m_validateEntitiesCallback = AZStd::move(validateEntitiesCallback);
    }

    void PrefabEditorEntityOwnershipService::HandleEntityBeingDestroyed(const AZ::EntityId& entityId)
    {
        AZ_Assert(IsInitialized(), "Tried to destroy an entity without initializing the Entity Ownership Service");
        OnEntityRemoved(entityId);
        if (auto owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId); owningInstance.has_value())
        {
            // Entities removed through the application (as in via manual 'delete'),
            // should be detached from the owning instance, but not again deleted.
            if (owningInstance->get().GetContainerEntityId() == entityId)
            {
                owningInstance->get().DetachContainerEntity().release();
            }
            else
            {
                owningInstance->get().DetachEntity(entityId).release();
            }
        }
    }

    void PrefabEditorEntityOwnershipService::StartPlayInEditor()
    {
        // This is a workaround until the replacement for GameEntityContext is done
        AzFramework::GameEntityContextEventBus::Broadcast(&AzFramework::GameEntityContextEventBus::Events::OnPreGameEntitiesStarted);
        m_gameModeEvent.Signal(GameModeState::Started);

        if (m_rootInstance && !m_playInEditorData.m_isEnabled)
        {
            // Construct the runtime entities and products
            bool readyToCreateRootSpawnable = m_playInEditorData.m_assetsCache.IsActivated();
            if (!readyToCreateRootSpawnable && !m_playInEditorData.m_assetsCache.Activate(Prefab::PrefabConversionUtils::PlayInEditor))

            {
                AZ_Error("Prefab", false, "Failed to create a prefab processing stack from key '%.*s'.", AZ_STRING_ARG(Prefab::PrefabConversionUtils::PlayInEditor));
                return;
            }

            auto createRootSpawnableResult = m_playInEditorData.m_assetsCache.CreateInMemorySpawnableAsset(m_rootInstance->GetTemplateId(), AzFramework::Spawnable::DefaultMainSpawnableName, true);
            if (createRootSpawnableResult.IsSuccess())
            {
                // make sure that PRE_NOTIFY assets get their notify before we activate, so that we can preserve the order of
                // (load asset) -> (notify) -> (init) -> (activate)
                AZ::Data::AssetManager::Instance().DispatchEvents();

                AZ::Data::Asset<AzFramework::Spawnable> rootSpawnable = createRootSpawnableResult.GetValue().get();
                m_playInEditorData.m_entities.Reset(rootSpawnable);

                AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
                auto spawnCompleteCB = [rootSpawnable](
                                           [[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId,
                                           [[maybe_unused]] AzFramework::SpawnableConstEntityContainerView view)
                {
                    AzFramework::RootSpawnableNotificationBus::Broadcast(
                        &AzFramework::RootSpawnableNotificationBus::Events::OnRootSpawnableReady, rootSpawnable, 0);
                };

                optionalArgs.m_completionCallback = AZStd::move(spawnCompleteCB);
                m_playInEditorData.m_entities.SpawnAllEntities(AZStd::move(optionalArgs));

                // This is a workaround until the replacement for GameEntityContext is done
                AzFramework::GameEntityContextEventBus::Broadcast(
                    &AzFramework::GameEntityContextEventBus::Events::OnGameEntitiesStarted);
            }
            else
            {
                AZ_Error("Prefab", false, "Failed to create the root spawnable due to the error: '%.*s'", AZ_STRING_ARG(createRootSpawnableResult.GetError()));
                return;
            }

            m_rootInstance->GetAllEntitiesInHierarchy([this](AZStd::unique_ptr<AZ::Entity>& entity)
            {
                AZ_Assert(entity, "Invalid entity found in root instance while starting play in editor.");
                if (entity->GetState() == AZ::Entity::State::Active)
                {
                    entity->Deactivate();
                    m_playInEditorData.m_deactivatedEntities.push_back(entity.get());
                }
                return true;
            });

            m_playInEditorData.m_isEnabled = true;
        }
    }

    void PrefabEditorEntityOwnershipService::StopPlayInEditor()
    {
        if (m_rootInstance && m_playInEditorData.m_isEnabled)
        {
            AZ_Assert(
                m_playInEditorData.m_entities.IsSet(),
                "Invalid Game Mode Entities Container encountered after play-in-editor stopped. "
                "Confirm that the container was initialized correctly");

            m_playInEditorData.m_entities.DespawnAllEntities();
            m_playInEditorData.m_entities.Alert(
                [allSpawnableAssetData = m_playInEditorData.m_assetsCache.GetAssetContainer().MoveAllInMemorySpawnableAssets(),
                 deactivatedEntities = AZStd::move(m_playInEditorData.m_deactivatedEntities),
                 this]([[maybe_unused]] uint32_t generation) mutable
                {
                    auto end = deactivatedEntities.rend();
                    for (auto it = deactivatedEntities.rbegin(); it != end; ++it)
                    {
                        AZ_Assert(*it, "Invalid entity added to list for re-activation after play-in-editor stopped.");
                        (*it)->Activate();
                    }

                    for (auto& [spawnableName, spawnableAssetData] : allSpawnableAssetData)
                    {
                        for (auto& asset : spawnableAssetData.m_assets)
                        {
                            asset.Release();
                            AZ::Data::AssetCatalogRequestBus::Broadcast(
                                &AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset.GetId());
                        }
                    }
                    AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::GarbageCollect);

                    // This is a workaround until the replacement for GameEntityContext is done
                    AzFramework::GameEntityContextEventBus::Broadcast(&AzFramework::GameEntityContextEventBus::Events::OnGameEntitiesReset);
                    m_gameModeEvent.Signal(GameModeState::Stopped);
                });
            m_playInEditorData.m_entities.Clear();

            // Game entity cleanup is queued onto the next tick via the DespawnEntities call.
            // To avoid both game entities and Editor entities active at the same time
            // we flush the tick queue and the spawnable queue to ensure the game entities are cleared first.
            // The Alert callback that follows the DespawnEntities call will then reactivate the editor entities
            // This should be considered temporary as a move to a less rigid event sequence that supports async entity clean up
            // is the desired direction forward.
            AZ::TickBus::ExecuteQueuedEvents();
            auto* rootSpawnableInterface = AzFramework::RootSpawnableInterface::Get();
            if (rootSpawnableInterface)
            {
                rootSpawnableInterface->ProcessSpawnableQueue();
                AzFramework::RootSpawnableNotificationBus::ExecuteQueuedEvents();
            }
        }

        m_playInEditorData.m_isEnabled = false;
    }

    bool PrefabEditorEntityOwnershipService::IsValidRootAliasPath(Prefab::RootAliasPath rootAliasPath) const
    {
        return GetInstanceReferenceFromRootAliasPath(rootAliasPath) != AZStd::nullopt;
    }

    Prefab::InstanceOptionalReference PrefabEditorEntityOwnershipService::GetInstanceReferenceFromRootAliasPath(
        Prefab::RootAliasPath rootAliasPath) const
    {
        Prefab::InstanceOptionalReference instance = *m_rootInstance;

        for (const auto& pathElement : rootAliasPath)
        {
            if (pathElement.Native() == rootAliasPath.begin()->Native())
            {
                // If the root is not the root Instance, the rootAliasPath is invalid.
                if (pathElement.Native() != instance->get().GetInstanceAlias())
                {
                    return Prefab::InstanceOptionalReference();
                }
            }
            else
            {
                // If the instance alias can't be found, the rootAliasPath is invalid.
                instance = instance->get().FindNestedInstance(pathElement.Native());
                if (!instance.has_value())
                {
                    return Prefab::InstanceOptionalReference();
                }
            }
        }

        return instance;
    }

    bool PrefabEditorEntityOwnershipService::GetInstancesInRootAliasPath(
        Prefab::RootAliasPath rootAliasPath, const AZStd::function<bool(const Prefab::InstanceOptionalReference)>& callback) const
    {
        if (!IsValidRootAliasPath(rootAliasPath))
        {
            return false;
        }

        Prefab::InstanceOptionalReference instance;

        for (const auto& pathElement : rootAliasPath)
        {
            if (!instance.has_value())
            {
                instance = *m_rootInstance;
            }
            else
            {
                instance = instance->get().FindNestedInstance(pathElement.Native());
            }

            if(callback(instance))
            {
                return true;
            }
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    // Slice Buses implementation with Assert(false), this will exist only during Slice->Prefab
    // development to pinpoint and replace specific calls to Slice system

    AZ::SliceComponent::SliceInstanceAddress PrefabEditorEntityOwnershipService::GetOwningSlice()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return AZ::SliceComponent::SliceInstanceAddress();
    }


    AZ::Data::AssetId UnimplementedSliceEntityOwnershipService::CurrentlyInstantiatingSlice()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return {};
    }

    bool UnimplementedSliceEntityOwnershipService::HandleRootEntityReloadedFromStream(
        AZ::Entity*, bool, AZ::SliceComponent::EntityIdToEntityIdMap*)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return false;
    }

    AZ::SliceComponent* UnimplementedSliceEntityOwnershipService::GetRootSlice()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return nullptr;
    }

    const AZ::SliceComponent::EntityIdToEntityIdMap& UnimplementedSliceEntityOwnershipService::GetLoadedEntityIdMap()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        static AZ::SliceComponent::EntityIdToEntityIdMap dummy;
        return dummy;
    }

    AZ::EntityId UnimplementedSliceEntityOwnershipService::FindLoadedEntityIdMapping(const AZ::EntityId&) const
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return AZ::EntityId();
    }

    AzFramework::SliceInstantiationTicket UnimplementedSliceEntityOwnershipService::InstantiateSlice(
        const AZ::Data::Asset<AZ::Data::AssetData>&,
        const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper&,
        const AZ::Data::AssetFilterCB&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return AzFramework::SliceInstantiationTicket();
    }

    AZ::SliceComponent::SliceInstanceAddress UnimplementedSliceEntityOwnershipService::CloneSliceInstance(
        AZ::SliceComponent::SliceInstanceAddress, AZ::SliceComponent::EntityIdToEntityIdMap&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return {};
    }

    void UnimplementedSliceEntityOwnershipService::CancelSliceInstantiation(const AzFramework::SliceInstantiationTicket&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    AzFramework::SliceInstantiationTicket UnimplementedSliceEntityOwnershipService::GenerateSliceInstantiationTicket()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return AzFramework::SliceInstantiationTicket();
    }

    void UnimplementedSliceEntityOwnershipService::SetIsDynamic(bool)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    const AzFramework::RootSliceAsset& UnimplementedSliceEntityOwnershipService::GetRootAsset() const
    {
        static AzFramework::RootSliceAsset dummy;
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return dummy;
    }
}
