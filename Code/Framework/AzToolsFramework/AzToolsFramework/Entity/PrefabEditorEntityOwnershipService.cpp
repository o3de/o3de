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
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
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

        m_rootInstance = AZStd::unique_ptr<Prefab::Instance>(m_prefabSystemComponent->CreatePrefab({}, {}, "NewLevel.prefab"));

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
            // Need to save off the template id to remove the template after the instance is deleted.
            Prefab::TemplateId templateId = m_rootInstance->GetTemplateId();
            m_rootInstance.reset();
            if (templateId != Prefab::InvalidTemplateId)
            {
                // Remove the template here so that if we're in a Deactivate/Activate cycle, it can recreate the template/rootInstance
                // correctly
                m_prefabSystemComponent->RemoveTemplate(templateId);
            }
        }
    }

    void PrefabEditorEntityOwnershipService::Reset()
    {
        Prefab::TemplateId templateId = m_rootInstance->GetTemplateId();
        if (templateId != Prefab::InvalidTemplateId)
        {
            m_rootInstance->SetTemplateId(Prefab::InvalidTemplateId);
            m_prefabSystemComponent->RemoveTemplate(templateId);
        }
        m_rootInstance->Reset();
        m_rootInstance->SetContainerEntityName("Level");

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
        m_rootInstance->GetEntities(entities, false);
    }

    bool PrefabEditorEntityOwnershipService::GetAllEntities(EntityList& entities)
    {
        m_rootInstance->GetEntities(entities, true);
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
        // Make loading from stream to behave the same in terms of filesize as regular loading of prefabs
        // This may need to be revisited in the future for supporting higher sizes along with prefab loading
        if (stream.GetLength() > Prefab::MaxPrefabFileSize)
        {
            AZ_Error("Prefab", false, "'%.*s' prefab content is bigger than the max supported size (%f MB)", AZ_STRING_ARG(filename), Prefab::MaxPrefabFileSize / (1024.f * 1024.f));
            return false;
        }
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
        m_rootInstance->SetTemplateSourcePath(m_loaderInterface->GetRelativePathToProject(filename));
        m_rootInstance->SetContainerEntityName("Level");
        m_prefabSystemComponent->PropagateTemplateChanges(templateId);

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
        AZ::IO::Path relativePath = m_loaderInterface->GetRelativePathToProject(filename);
        AzToolsFramework::Prefab::TemplateId templateId = m_prefabSystemComponent->GetTemplateIdFromFilePath(relativePath);

        m_rootInstance->SetTemplateSourcePath(relativePath);

        if (templateId == AzToolsFramework::Prefab::InvalidTemplateId)
        {
            m_rootInstance->m_containerEntity->AddComponent(aznew Prefab::EditorPrefabComponent());
            HandleEntitiesAdded({ m_rootInstance->m_containerEntity.get() });

            AzToolsFramework::Prefab::PrefabDom dom;
            bool success = AzToolsFramework::Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(*m_rootInstance, dom);
            if (!success)
            {
                AZ_Error("Prefab", false, "Failed to convert current root instance into a DOM when saving file '%.*s'", AZ_STRING_ARG(filename));
                return false;
            }
            templateId = m_prefabSystemComponent->AddTemplate(relativePath, AZStd::move(dom));

            if (templateId == AzToolsFramework::Prefab::InvalidTemplateId)
            {
                AZ_Error("Prefab", false, "Couldn't add new template id '%i' when saving file '%.*s'", templateId, AZ_STRING_ARG(filename));
                return false;
            }
        }

        Prefab::TemplateId prevTemplateId = m_rootInstance->GetTemplateId();
        m_rootInstance->SetTemplateId(templateId);

        if (prevTemplateId != Prefab::InvalidTemplateId && templateId != prevTemplateId)
        {
            // Make sure we only have one level template loaded at a time
            m_prefabSystemComponent->RemoveTemplate(prevTemplateId);
        }

        AZStd::string out;
        if (m_loaderInterface->SaveTemplateToString(m_rootInstance->GetTemplateId(), out))
        {
            const size_t bytesToWrite = out.size();
            const size_t bytesWritten = stream.Write(bytesToWrite, out.data());
            return bytesWritten == bytesToWrite;
        }
        return false;
    }

    void PrefabEditorEntityOwnershipService::CreateNewLevelPrefab(AZStd::string_view filename)
    {
        AZ::IO::Path relativePath = m_loaderInterface->GetRelativePathToProject(filename);
        AzToolsFramework::Prefab::TemplateId templateId = m_prefabSystemComponent->GetTemplateIdFromFilePath(relativePath);

        m_rootInstance->SetTemplateSourcePath(relativePath);

        AZStd::string watchFolder;
        AZ::Data::AssetInfo assetInfo;
        bool sourceInfoFound = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, DefaultLevelTemplateName,
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
            sourcePath.Set(levelDefaultDom, assetInfo.m_relativePath.c_str());

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
    }

    Prefab::InstanceOptionalReference PrefabEditorEntityOwnershipService::CreatePrefab(
        const AZStd::vector<AZ::Entity*>& entities, AZStd::vector<AZStd::unique_ptr<Prefab::Instance>>&& nestedPrefabInstances,
        AZ::IO::PathView filePath, Prefab::InstanceOptionalReference instanceToParentUnder)
    {
        AZStd::unique_ptr<Prefab::Instance> createdPrefabInstance =
            m_prefabSystemComponent->CreatePrefab(entities, AZStd::move(nestedPrefabInstances), filePath, nullptr, false);

        if (createdPrefabInstance)
        {
            if (!instanceToParentUnder)
            {
                instanceToParentUnder = *m_rootInstance;
            }

            Prefab::Instance& addedInstance = instanceToParentUnder->get().AddInstance(AZStd::move(createdPrefabInstance));
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
        AZStd::unique_ptr<Prefab::Instance> createdPrefabInstance = m_prefabSystemComponent->InstantiatePrefab(filePath);

        if (createdPrefabInstance)
        {
            if (!instanceToParentUnder)
            {
                instanceToParentUnder = *m_rootInstance;
            }

            Prefab::Instance& addedInstance = instanceToParentUnder->get().AddInstance(AZStd::move(createdPrefabInstance));
            HandleEntitiesAdded({addedInstance.m_containerEntity.get()});
            return addedInstance;
        }

        return AZStd::nullopt;
    }

    Prefab::InstanceOptionalReference PrefabEditorEntityOwnershipService::GetRootPrefabInstance()
    {
        AZ_Assert(m_rootInstance, "A valid root prefab instance couldn't be found in PrefabEditorEntityOwnershipService.");
        return *m_rootInstance;
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
                    if (context.HasCompletedSuccessfully())
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

                            LoadReferencedAssets(product.GetReferencedAssets());

                            AZ::Data::AssetInfo info;
                            info.m_assetId = product.GetAsset().GetId();
                            info.m_assetType = product.GetAssetType();
                            info.m_relativePath = product.GetId();

                            AZ::Data::AssetCatalogRequestBus::Broadcast(
                                &AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, info.m_assetId, info);
                            m_playInEditorData.m_assets.emplace_back(product.ReleaseAsset().release(), AZ::Data::AssetLoadBehavior::Default);
                        }

                        // make sure that PRE_NOTIFY assets get their notify before we activate, so that we can preserve the order of 
                        // (load asset) -> (notify) -> (init) -> (activate)
                        AZ::Data::AssetManager::Instance().DispatchEvents();

                        if (rootSpawnableIndex != NoRootSpawnable)
                        {
                            m_playInEditorData.m_entities.Reset(m_playInEditorData.m_assets[rootSpawnableIndex]);
                            m_playInEditorData.m_entities.SpawnAllEntities();
                        }

                        // This is a workaround until the replacement for GameEntityContext is done
                        AzFramework::GameEntityContextEventBus::Broadcast(
                            &AzFramework::GameEntityContextEventBus::Events::OnGameEntitiesStarted);
                    }
                    else
                    {
                        AZ_Error("Prefab", false, "Failed to convert the prefab into assets.");
                        return;
                    }
                }
                else
                {
                    AZ_Error("Prefab", false, "Failed to create a prefab processing stack from key 'PlayInEditor'.");
                    return;
                }

                m_rootInstance->GetNestedEntities([this](AZStd::unique_ptr<AZ::Entity>& entity)
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
            auto end = m_playInEditorData.m_deactivatedEntities.rend();
            for (auto it = m_playInEditorData.m_deactivatedEntities.rbegin(); it != end; ++it)
            {
                AZ_Assert(*it, "Invalid entity added to list for re-activation after play-in-editor stopped.");
                (*it)->Activate();
            }
            m_playInEditorData.m_deactivatedEntities.clear();

            m_playInEditorData.m_entities.DespawnAllEntities();
            m_playInEditorData.m_entities.Alert(
                [assets = AZStd::move(m_playInEditorData.m_assets)]([[maybe_unused]]uint32_t generation) mutable
                {
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
                });
            m_playInEditorData.m_entities.Clear();

            // This is a workaround until the replacement for GameEntityContext is done
            AzFramework::GameEntityContextEventBus::Broadcast(&AzFramework::GameEntityContextEventBus::Events::OnGameEntitiesReset);
        }

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
