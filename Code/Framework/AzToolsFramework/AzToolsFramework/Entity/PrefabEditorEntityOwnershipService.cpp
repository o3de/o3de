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
#include <AzCore/Serialization/Utils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Spawnable/RootSpawnableInterface.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipService.h>
#include <AzToolsFramework/Prefab/EditorPrefabComponent.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabLoader.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabUndoHelpers.h>

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
        m_prefabSystemComponent = AZ::Interface<Prefab::PrefabSystemComponentInterface>::Get();
        AZ_Assert(m_prefabSystemComponent != nullptr,
            "Couldn't get prefab system component, it's a requirement for PrefabEntityOwnership system to work");

        m_loaderInterface = AZ::Interface<Prefab::PrefabLoaderInterface>::Get();
        AZ_Assert(m_loaderInterface != nullptr,
            "Couldn't get prefab loader interface, it's a requirement for PrefabEntityOwnership system to work");

        m_rootInstance = AZStd::unique_ptr<Prefab::Instance>(m_prefabSystemComponent->CreatePrefab({}, {}, "newLevel.prefab"));
        m_sliceOwnershipService.BusConnect(m_entityContextId);
        m_sliceOwnershipService.m_shouldAssertForLegacySlicesUsage = m_shouldAssertForLegacySlicesUsage;
        m_editorSliceOwnershipService.BusConnect();
        m_editorSliceOwnershipService.m_shouldAssertForLegacySlicesUsage = m_shouldAssertForLegacySlicesUsage;
    }

    bool PrefabEditorEntityOwnershipService::IsInitialized()
    {
        return m_rootInstance != nullptr;
    }

    void PrefabEditorEntityOwnershipService::Destroy()
    {
        StopPlayInEditor();
        m_editorSliceOwnershipService.BusDisconnect();
        m_sliceOwnershipService.BusDisconnect();

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
        ScopedUndoBatch undoBatch("Undo adding entity");
        Prefab::PrefabDom instanceDomBeforeUpdate;
        Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(*m_rootInstance, instanceDomBeforeUpdate);

        m_rootInstance->AddEntity(*entity);
        HandleEntitiesAdded({ entity });
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformInterface::SetParent, m_rootInstance->m_containerEntity->GetId());

        Prefab::PrefabUndoHelpers::UpdatePrefabInstance(
            *m_rootInstance, "Undo adding entity", instanceDomBeforeUpdate, undoBatch.GetUndoBatch());
    }

    void PrefabEditorEntityOwnershipService::AddEntities(const EntityList& entities)
    {
        AZ_Assert(IsInitialized(), "Tried to add entities without initializing the Entity Ownership Service");
        ScopedUndoBatch undoBatch("Undo adding entities");
        Prefab::PrefabDom instanceDomBeforeUpdate;
        Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(*m_rootInstance, instanceDomBeforeUpdate);

        for (AZ::Entity* entity : entities)
        {
            m_rootInstance->AddEntity(*entity);
        }

        HandleEntitiesAdded(entities);

        for (AZ::Entity* entity : entities)
        {
            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformInterface::SetParent, m_rootInstance->m_containerEntity->GetId());
        }

        Prefab::PrefabUndoHelpers::UpdatePrefabInstance(
            *m_rootInstance, "Undo adding entities", instanceDomBeforeUpdate, undoBatch.GetUndoBatch());
    }

    bool PrefabEditorEntityOwnershipService::DestroyEntity(AZ::Entity* entity)
    {
        return DestroyEntityById(entity->GetId());
    }

    bool PrefabEditorEntityOwnershipService::DestroyEntityById(AZ::EntityId entityId)
    {
        AZ_Assert(IsInitialized(), "Tried to destroy an entity without initializing the Entity Ownership Service");
        AZ_Assert(m_entitiesRemovedCallback, "Callback function for DestroyEntityById has not been set.");
        OnEntityRemoved(entityId);
        return true;
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
        }
        else
        {
            m_rootInstance->m_containerEntity->AddComponent(aznew Prefab::EditorPrefabComponent());
            HandleEntitiesAdded({ m_rootInstance->m_containerEntity.get() });

            AzToolsFramework::Prefab::PrefabDom dom;
            bool success = AzToolsFramework::Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(*m_rootInstance, dom);
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

        AZStd::unique_ptr<Prefab::Instance> instantiatedPrefabInstance =
            m_prefabSystemComponent->InstantiatePrefab(filePath, instanceToParentUnder);

        if (instantiatedPrefabInstance)
        {
            Prefab::Instance& addedInstance = instanceToParentUnder->get().AddInstance(
                AZStd::move(instantiatedPrefabInstance));
            HandleEntitiesAdded({addedInstance.m_containerEntity.get()});
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

    const AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& PrefabEditorEntityOwnershipService::GetPlayInEditorAssetData()
    {
        return m_playInEditorData.m_assets;
    }

    void PrefabEditorEntityOwnershipService::OnEntityRemoved(AZ::EntityId entityId)
    {
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

    void PrefabEditorEntityOwnershipService::LoadReferencedAssets(AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets)
    {
        // Start our loads on all assets by calling GetAsset from the AssetManager
        for (AZ::Data::Asset<AZ::Data::AssetData>& asset : referencedAssets)
        {
            if (!asset.GetId().IsValid())
            {
                AZ_Error("Prefab", false, "Invalid asset found referenced in scene while entering game mode");
                continue;
            }

            const AZ::Data::AssetLoadBehavior loadBehavior = asset.GetAutoLoadBehavior();

            if (loadBehavior == AZ::Data::AssetLoadBehavior::NoLoad)
            {
                continue;
            }

            AZ::Data::AssetId assetId = asset.GetId();
            AZ::Data::AssetType assetType = asset.GetType();

            asset = AZ::Data::AssetManager::Instance().GetAsset(assetId, assetType, loadBehavior);

            if (!asset.GetId().IsValid())
            {
                AZ_Error("Prefab", false, "Invalid asset found referenced in scene while entering game mode");
                continue;
            }
        }

        // For all Preload assets we block until they're ready
        // We do this as a seperate pass so that we don't interrupt queuing up all other asset loads
        for (AZ::Data::Asset<AZ::Data::AssetData>& asset : referencedAssets)
        {
            if (!asset.GetId().IsValid())
            {
                AZ_Error("Prefab", false, "Invalid asset found referenced in scene while entering game mode");
                continue;
            }

            const AZ::Data::AssetLoadBehavior loadBehavior = asset.GetAutoLoadBehavior();

            if (loadBehavior != AZ::Data::AssetLoadBehavior::PreLoad)
            {
                continue;
            }

            asset.BlockUntilLoadComplete();

            if (asset.IsError())
            {
                AZ_Error("Prefab", false, "Asset with id %s failed to preload while entering game mode",
                    asset.GetId().ToString<AZStd::string>().c_str());

                continue;
            }
        }
    }

    void PrefabEditorEntityOwnershipService::StartPlayInEditor()
    {
        // This is a workaround until the replacement for GameEntityContext is done
        AzFramework::GameEntityContextEventBus::Broadcast(&AzFramework::GameEntityContextEventBus::Events::OnPreGameEntitiesStarted);

        if (m_rootInstance && !m_playInEditorData.m_isEnabled)
        {
            // Construct the runtime entities and products 
            Prefab::TemplateReference templateReference = m_prefabSystemComponent->FindTemplate(m_rootInstance->GetTemplateId());
            if (templateReference.has_value())
            {
                bool converterLoaded = m_playInEditorData.m_converter.IsLoaded();
                if (!converterLoaded)
                {
                    converterLoaded = m_playInEditorData.m_converter.LoadStackProfile("PlayInEditor");
                }
                if (converterLoaded)
                {
                    // Use a random uuid as this is only a temporary source.
                    AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext context(AZ::Uuid::CreateRandom());
                    Prefab::PrefabDom copy;
                    copy.CopyFrom(templateReference->get().GetPrefabDom(), copy.GetAllocator(), false);
                    context.AddPrefab(DefaultMainSpawnableName, AZStd::move(copy));
                    m_playInEditorData.m_converter.ProcessPrefab(context);
                    if (context.HasCompletedSuccessfully() && !context.GetProcessedObjects().empty())
                    {
                        static constexpr size_t NoRootSpawnable = AZStd::numeric_limits<size_t>::max();
                        size_t rootSpawnableIndex = NoRootSpawnable;

                        // Create temporary assets from the processed data.
                        for (auto& product : context.GetProcessedObjects())
                        {
                            if (product.GetAssetType() == AZ::AzTypeInfo<AzFramework::Spawnable>::Uuid() &&
                                product.GetId() ==
                                    AZStd::string::format("%s.%s", DefaultMainSpawnableName, AzFramework::Spawnable::FileExtension))
                            {
                                rootSpawnableIndex = m_playInEditorData.m_assets.size();
                            }

                            AZ::Data::AssetInfo info;
                            info.m_assetId = product.GetAsset().GetId();
                            info.m_assetType = product.GetAssetType();
                            info.m_relativePath = product.GetId();

                            AZ::Data::AssetCatalogRequestBus::Broadcast(
                                &AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, info.m_assetId, info);
                            m_playInEditorData.m_assets.emplace_back(product.ReleaseAsset().release(), AZ::Data::AssetLoadBehavior::Default);

                            // Ensure the product asset is registered with the AssetManager
                            // Hold on to the returned asset to keep ref count alive until we assign it the latest data
                            AZ::Data::Asset<AZ::Data::AssetData> asset =
                                AZ::Data::AssetManager::Instance().FindOrCreateAsset(info.m_assetId, info.m_assetType, AZ::Data::AssetLoadBehavior::Default);

                            // Update the asset registered in the AssetManager with the data of our product from the Prefab Processor
                            AZ::Data::AssetManager::Instance().AssignAssetData(m_playInEditorData.m_assets.back());
                        }

                        for (auto& product : context.GetProcessedObjects())
                        {
                            LoadReferencedAssets(product.GetReferencedAssets());
                        }

                        // make sure that PRE_NOTIFY assets get their notify before we activate, so that we can preserve the order of 
                        // (load asset) -> (notify) -> (init) -> (activate)
                        AZ::Data::AssetManager::Instance().DispatchEvents();

                        if (rootSpawnableIndex != NoRootSpawnable)
                        {
                            m_playInEditorData.m_entities.Reset(m_playInEditorData.m_assets[rootSpawnableIndex]);
                            m_playInEditorData.m_entities.SpawnAllEntities();
                        }
                        else
                        {
                            AZ_Error("Prefab", false,
                                "Processing of the level prefab failed to produce a root spawnable while entering game mode. "
                                "Unable to fully enter game mode.");
                            return;
                        }

                        // This is a workaround until the replacement for GameEntityContext is done
                        AzFramework::GameEntityContextEventBus::Broadcast(
                            &AzFramework::GameEntityContextEventBus::Events::OnGameEntitiesStarted);
                    }
                    else
                    {
                        AZ_Error("Prefab", false,
                            "Failed to convert the prefab into assets. "
                            "Confirm that the 'PlayInEditor' prefab processor stack is capable of producing a useable product asset.");
                        return;
                    }
                }
                else
                {
                    AZ_Error("Prefab", false, "Failed to create a prefab processing stack from key 'PlayInEditor'.");
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
            }
        }

        m_playInEditorData.m_isEnabled = true;
    }

    void PrefabEditorEntityOwnershipService::StopPlayInEditor()
    {
        if (m_rootInstance && m_playInEditorData.m_isEnabled)
        {
            AZ_Assert(m_playInEditorData.m_entities.IsSet(),
                "Invalid Game Mode Entities Container encountered after play-in-editor stopped. "
                "Confirm that the container was initialized correctly");

            m_playInEditorData.m_entities.DespawnAllEntities();
            m_playInEditorData.m_entities.Alert(
                [assets = AZStd::move(m_playInEditorData.m_assets),
                 deactivatedEntities = AZStd::move(m_playInEditorData.m_deactivatedEntities)]
                ([[maybe_unused]]uint32_t generation) mutable
                {
                    auto end = deactivatedEntities.rend();
                    for (auto it = deactivatedEntities.rbegin(); it != end; ++it)
                    {
                        AZ_Assert(*it, "Invalid entity added to list for re-activation after play-in-editor stopped.");
                        (*it)->Activate();
                    }

                    for (auto& asset : assets)
                    {
                        if (asset)
                        {
                            // Explicitly release because this needs to happen before the asset is unregistered.
                            asset.Release();
                            AZ::Data::AssetCatalogRequestBus::Broadcast(
                                &AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset.GetId());
                        }
                    }
                    AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::GarbageCollect);

                    // This is a workaround until the replacement for GameEntityContext is done
                    AzFramework::GameEntityContextEventBus::Broadcast(&AzFramework::GameEntityContextEventBus::Events::OnGameEntitiesReset);
                });
            m_playInEditorData.m_entities.Clear();
        }

        // Game entity cleanup is queued onto the next tick via the DespawnEntities call.
        // To avoid both game entities and Editor entities active at the same time
        // we flush the tick queue to ensure the game entities are cleared first.
        // The Alert callback that follows the DespawnEntities call will then reactivate the editor entities
        // This should be considered temporary as a move to a less rigid event sequence that supports async entity clean up
        // is the desired direction forward.
        AZ::TickBus::ExecuteQueuedEvents();

        m_playInEditorData.m_isEnabled = false;
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

    //////////////////////////////////////////////////////////////////////////

    AzFramework::SliceInstantiationTicket UnimplementedSliceEditorEntityOwnershipService::InstantiateEditorSlice(
        const AZ::Data::Asset<AZ::Data::AssetData>&, const AZ::Transform&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return AzFramework::SliceInstantiationTicket();
    }

    AZ::SliceComponent::SliceInstanceAddress UnimplementedSliceEditorEntityOwnershipService::CloneEditorSliceInstance(
        AZ::SliceComponent::SliceInstanceAddress, AZ::SliceComponent::EntityIdToEntityIdMap&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return {};
    }

    AZ::SliceComponent::SliceInstanceAddress UnimplementedSliceEditorEntityOwnershipService::CloneSubSliceInstance(
        const AZ::SliceComponent::SliceInstanceAddress&, const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>&,
        const AZ::SliceComponent::SliceInstanceAddress&, AZ::SliceComponent::EntityIdToEntityIdMap*)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return {};
    }

    AZ::SliceComponent::SliceInstanceAddress UnimplementedSliceEditorEntityOwnershipService::PromoteEditorEntitiesIntoSlice(
        const AZ::Data::Asset<AZ::SliceAsset>&, const AZ::SliceComponent::EntityIdToEntityIdMap&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return {};
    }

    void UnimplementedSliceEditorEntityOwnershipService::DetachSliceEntities(const EntityIdList&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    void UnimplementedSliceEditorEntityOwnershipService::DetachSliceInstances(const AZ::SliceComponent::SliceInstanceAddressSet&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    void UnimplementedSliceEditorEntityOwnershipService::DetachSubsliceInstances(const AZ::SliceComponent::SliceInstanceEntityIdRemapList&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    void UnimplementedSliceEditorEntityOwnershipService::RestoreSliceEntity(
        AZ::Entity*, const AZ::SliceComponent::EntityRestoreInfo&, SliceEntityRestoreType)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    void UnimplementedSliceEditorEntityOwnershipService::ResetEntitiesToSliceDefaults(EntityIdList)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    AZ::SliceComponent* UnimplementedSliceEditorEntityOwnershipService::GetEditorRootSlice()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return nullptr;
    }
}
