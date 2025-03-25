/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Entity/SliceEntityOwnershipService.h>

namespace AzFramework
{
    SliceEntityOwnershipService::SliceEntityOwnershipService(const EntityContextId& entityContextId,
        AZ::SerializeContext* serializeContext)
        : m_entityContextId(entityContextId)
        , m_serializeContext(serializeContext)
        , m_nextSliceTicketId(0)
    {
        if (nullptr == serializeContext)
        {
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve application serialization context.");
        }
    }

    void SliceEntityOwnershipService::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SliceInstantiationTicket>()->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AzFramework::SliceInstantiationTicket>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "entity")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Method("Equal", &AzFramework::SliceInstantiationTicket::operator==)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ->Method("ToString", &SliceInstantiationTicket::ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("IsValid", &AzFramework::SliceInstantiationTicket::IsValid);
        }
    }

    void SliceEntityOwnershipService::Initialize()
    {
        if (!m_rootAsset)
        {
            m_rootAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), false);
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_rootAsset->GetId());
            SliceEntityOwnershipServiceRequestBus::Handler::BusConnect(m_entityContextId);
        }
        CreateRootSlice();
    }

    bool SliceEntityOwnershipService::IsInitialized()
    {
        return m_rootAsset ? true : false;
    }

    void SliceEntityOwnershipService::Destroy()
    {
        if (m_rootAsset)
        {
            SliceEntityOwnershipServiceRequestBus::Handler::BusDisconnect(m_entityContextId);
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(m_rootAsset->GetId());
            m_rootAsset.Reset();
        }
    }

    bool SliceEntityOwnershipService::CheckAndAssertRootComponentIsAvailable()
    {
        bool componentReady = m_rootAsset && m_rootAsset->GetComponent();
        if (!componentReady)
        {
            AZ_Assert(false, "SliceEntityOwnershipService - Attempt to use the root asset component, but slice has not yet been created.");
            return false;
        }
        return true;

    }

    void SliceEntityOwnershipService::Reset()
    {
        if (m_rootAsset)
        {
            EntityOwnershipServiceNotificationBus::Event(m_entityContextId, &EntityOwnershipServiceNotificationBus::Events::PrepareForEntityOwnershipServiceReset);
            while (!m_queuedSliceInstantiations.empty())
            {
                // clear out the remaining instantiations in a conservative manner, assuming that callbacks such as
                // OnSliceInstantiationFailed will call back into us and potentially mutate this list.
                const InstantiatingSliceInfo& instantiating = m_queuedSliceInstantiations.back();

                // 'instantiating' is deleted during this loop, so capture the asset Id and Ticket before we continue and destroy it.
                AZ::Data::AssetId idToNotify = instantiating.m_asset.GetId();
                AzFramework::SliceInstantiationTicket ticket = instantiating.m_ticket;

                // this will decrement the refcount of the asset, which could mean its invalid by the next line.
                // the above line also ensures that our list no longer contains this particular instantiation.
                // its important to do that, before calling any callbacks, because some listeners on the following functions
                // may call additional functions on this entity ownership service, and we could get into a situation
                // where we end up iterating over this list again (before returning from the below bus calls).
                m_queuedSliceInstantiations.pop_back();

                AZ::Data::AssetBus::MultiHandler::BusDisconnect(idToNotify);
                DispatchOnSliceInstantiationFailed(ticket, idToNotify, true);
            }

            EntityList entities = GetRootSliceEntities();

            for (AZ::Entity* entity : entities)
            {
                DestroyEntity(entity);
            }

            // Re-create fresh root slice asset.
            CreateRootSlice();

            EntityOwnershipServiceNotificationBus::Event(m_entityContextId, &EntityOwnershipServiceNotificationBus::Events::OnEntityOwnershipServiceReset);
        }
    }

    void SliceEntityOwnershipService::AddEntity(AZ::Entity* entity)
    {
        if (!CheckAndAssertRootComponentIsAvailable())
        {
            return;
        }
        m_rootAsset->GetComponent()->AddEntity(entity);
        HandleEntitiesAdded(EntityList{ entity });
    }

    void SliceEntityOwnershipService::AddEntities(const EntityList& entities)
    {
        if (!CheckAndAssertRootComponentIsAvailable())
        {
            return;
        }

        for (AZ::Entity* entity : entities)
        {
            AZ_Assert(!AzFramework::SliceEntityRequestBus::MultiHandler::BusIsConnectedId(entity->GetId()),
                "Entity already present.");
            GetRootAsset()->GetComponent()->AddEntity(entity);
        }

        HandleEntitiesAdded(entities);
    }

    bool SliceEntityOwnershipService::DestroyEntity(AZ::Entity* entity)
    {
        if (entity)
        {
            SliceEntityRequestBus::MultiHandler::BusDisconnect(entity->GetId());
            if (m_entitiesAddedCallback)
            {
                m_entitiesRemovedCallback({ entity->GetId() });
            }
            if (CheckAndAssertRootComponentIsAvailable())
            {
                return m_rootAsset->GetComponent()->RemoveEntity(entity);
            }
        }
        return false;
    }

    bool SliceEntityOwnershipService::DestroyEntityById(AZ::EntityId entityId)
    {
        if (!CheckAndAssertRootComponentIsAvailable())
        {
            return false;
        }

        AZ_Assert(m_entitiesRemovedCallback, "Callback function m_entitiesRemovedCallback for DestroyEntityById has not been set.");
        if (m_entitiesRemovedCallback)
        {
            m_entitiesRemovedCallback({ entityId });
        }

        // Note: This function should actually be destroying the entity, but some legacy slice code already
        // expects it to just detach the entity (such as RestoreSliceEntity_SliceEntityDeleted_SliceEntityRestored),
        // so the behavior is left unchanged.

        // Entities removed through the application (as in via manual 'delete'),
        // should be removed from the root slice, but not again deleted.
        return m_rootAsset->GetComponent()->RemoveEntity(entityId, false);
    }

    void SliceEntityOwnershipService::CreateRootSlice()
    {
        AZ_PROFILE_FUNCTION(AzFramework);

        if (!m_rootAsset || !m_rootAsset.Get())
        {
            AZ_Assert(false, "Root slice asset has not been created yet.");
            return;
        }
        
        CreateRootSlice(m_rootAsset.Get());
    }

    void SliceEntityOwnershipService::CreateRootSlice(AZ::SliceAsset* rootSliceAsset)
    {
        AZ_PROFILE_FUNCTION(AzFramework);

        if (!m_rootAsset || !m_rootAsset.Get())
        {
            AZ_Assert(false, "Root slice asset has not been created yet.");
            return;
        }

        AZ::Entity* rootEntity = new AZ::Entity();
        rootEntity->CreateComponent<AZ::SliceComponent>();

        // Manually create an asset to hold the root slice.
        rootSliceAsset->SetData(rootEntity, rootEntity->FindComponent<AZ::SliceComponent>());
        AZ::SliceComponent* rootSliceComponent = rootSliceAsset->GetComponent();
        rootSliceComponent->InitMetadata();
        rootSliceComponent->SetMyAsset(rootSliceAsset);
        rootSliceComponent->SetSerializeContext(m_serializeContext);
        rootSliceComponent->ListenForAssetChanges();

        // Root slice is always dynamic by default. Whether it's a "level",
        // or something else, it can be instantiated at runtime.
        rootSliceComponent->SetIsDynamic(true);

        // Make sure the root slice metadata entity is marked as persistent.
        AZ::Entity* metadataEntity = rootSliceComponent->GetMetadataEntity();

        if (metadataEntity)
        {
            AZ::SliceMetadataInfoComponent* infoComponent = metadataEntity->FindComponent<AZ::SliceMetadataInfoComponent>();

            if (infoComponent)
            {
                infoComponent->MarkAsPersistent(true);
            }
            HandleNewMetadataEntitiesCreated(*rootSliceComponent);
        }
    }

    AZ::SliceComponent* SliceEntityOwnershipService::GetRootSlice()
    {
        return m_rootAsset ? m_rootAsset->GetComponent() : nullptr;
    }

    EntityList SliceEntityOwnershipService::GetRootSliceEntities()
    {
        EntityList entities;

        CheckAndAssertRootComponentIsAvailable();

        const AZ::SliceComponent* rootSliceComponent = m_rootAsset->GetComponent();

        AZ_Assert(rootSliceComponent, "Root slice component has not been created.");
        if (!rootSliceComponent->IsInstantiated())
        {
            AZ_Assert(false, "Root slice has not been instantiated yet");
            return entities;
        }

        const EntityList& looseEntities = rootSliceComponent->GetNewEntities();
        
        entities.reserve(looseEntities.size());
        for (AZ::Entity* entity : looseEntities)
        {
            entities.push_back(entity);
        }

        const AZ::SliceComponent::SliceList& subSlices = rootSliceComponent->GetSlices();
        for (const AZ::SliceComponent::SliceReference& subSlice : subSlices)
        {
            for (const AZ::SliceComponent::SliceInstance& instance : subSlice.GetInstances())
            {
                for (AZ::Entity* entity : instance.GetInstantiated()->m_entities)
                {
                    entities.push_back(entity);
                }
            }
        }

        return entities;
    }

    bool SliceEntityOwnershipService::LoadFromStream(AZ::IO::GenericStream& stream, bool remapIds, EntityIdToEntityIdMap* idRemapTable, const AZ::ObjectStream::FilterDescriptor& filterDesc)
    {
        AZ_PROFILE_FUNCTION(AzFramework);

        if (!m_rootAsset)
        {
            AZ_Assert(false, "The entity ownership service has not been initialized.");
            return false;
        }

        AZ::Entity* newRootEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(stream, m_serializeContext, filterDesc);

        // Make sure that PRE_NOTIFY assets get their notify before we activate, so that we can preserve the order of 
        // (load asset) -> (notify) -> (init) -> (activate)
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // For other kinds of instantiations, like slice instantiations, becuase they use the queued slice instantiation mechanism,
        // they will always be instantiated after their asset is already ready.

        return HandleRootEntityReloadedFromStream(newRootEntity, remapIds, idRemapTable);
    }

    bool SliceEntityOwnershipService::HandleRootEntityReloadedFromStream(AZ::Entity* rootEntity, bool remapIds,
        AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable)
    {
        AZ_PROFILE_FUNCTION(AzFramework);

        if (!rootEntity)
        {
            return false;
        }

        // Flush asset database events after serialization, so all loaded asset statuses are updated.
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().DispatchEvents();
        }

        AZ::SliceComponent* newRootSlice = rootEntity->FindComponent<AZ::SliceComponent>();

        if (!newRootSlice)
        {
            AZ_Error("SliceEntityOwnershipService", false, "Loaded root entity is not a slice.");
            return false;
        }

        Reset();

        AZ::SliceAsset* rootSlice = m_rootAsset.Get();
        rootSlice->SetData(rootEntity, newRootSlice, true);
        newRootSlice->SetMyAsset(rootSlice);
        newRootSlice->SetSerializeContext(m_serializeContext);
        newRootSlice->ListenForAssetChanges();

        m_loadedEntityIdMap.clear();
        if (remapIds)
        {
            newRootSlice->GenerateNewEntityIds(&m_loadedEntityIdMap);
            if (idRemapTable)
            {
                *idRemapTable = m_loadedEntityIdMap;
            }
        }


        AZ::SliceComponent::EntityList entities;
        newRootSlice->GetEntities(entities);

        if (!remapIds)
        {
            for (AZ::Entity* entity : entities)
            {
                m_loadedEntityIdMap.emplace(entity->GetId(), entity->GetId());
            }
        }

        // Make sure the root slice metadata entity is marked as persistent.
        AZ::Entity* metadataEntity = newRootSlice->GetMetadataEntity();

        if (!metadataEntity)
        {
            AZ_Error("SliceEntityOwnershipService", false, "Root entity must have a metadata entity");
            return false;
        }

        AZ::SliceMetadataInfoComponent* infoComponent = metadataEntity->FindComponent<AZ::SliceMetadataInfoComponent>();

        if (!infoComponent)
        {
            AZ_Error("SliceEntityOwnershipService", false, "Root metadata entity must have a valid info component");
            return false;
        }

        infoComponent->MarkAsPersistent(true);

        EntityOwnershipServiceNotificationBus::Event(m_entityContextId,
            &EntityOwnershipServiceNotificationBus::Events::OnEntitiesReloadedFromStream, entities);

        HandleEntitiesAdded(entities);

        HandleNewMetadataEntitiesCreated(*newRootSlice);

        AZ::Data::AssetBus::MultiHandler::BusConnect(m_rootAsset->GetId());
        return true;
    }

    SliceInstantiationTicket SliceEntityOwnershipService::GenerateSliceInstantiationTicket()
    {
        return SliceInstantiationTicket(m_entityContextId, ++m_nextSliceTicketId);
    }

    void SliceEntityOwnershipService::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_rootAsset)
        {
            return;
        }

        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

        for (auto iter = m_queuedSliceInstantiations.begin(); iter != m_queuedSliceInstantiations.end(); )
        {
            const InstantiatingSliceInfo& instantiating = *iter;

            if (instantiating.m_asset.GetId() == asset.GetId())
            {
                // grab a refcount on the asset and copy the ticket, as 'instantiating' is about to be destroyed!
                AZ::Data::AssetId cachedId = instantiating.m_asset.GetId();
                SliceInstantiationTicket ticket = instantiating.m_ticket;

                AZStd::function<void()> notifyCallback =
                    [cachedId, ticket]() // capture these by value since we're about to leave the scope in which these variables exist.
                {
                    DispatchOnSliceInstantiationFailed(ticket, cachedId, false);
                };

                // Instantiation is queued against the tick bus. This ensures we're not holding the AssetBus lock
                // while the instantiation is handled, which may be costly.
                AZ::TickBus::QueueFunction(notifyCallback);

                iter = m_queuedSliceInstantiations.erase(iter); // this invalidates the instantiating data.
            }
            else
            {
                ++iter;
            }
        }
    }

    void SliceEntityOwnershipService::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> readyAsset)
    {
        AZ_PROFILE_FUNCTION(AzFramework);

        if (!readyAsset.GetAs<AZ::SliceAsset>())
        {
            AZ_Assert(readyAsset.GetAs<AZ::SliceAsset>(), "OnAssetReady : Asset %s (%s) is not a slice!", readyAsset.GetHint().c_str(), readyAsset.GetId().ToString<AZStd::string>().c_str());
            return;
        }

        if (readyAsset == m_rootAsset)
        {
            return;
        }

        AZ::Data::AssetBus::MultiHandler::BusDisconnect(readyAsset.GetId());

        // we intentionally capture readyAsset by value here, so that its refcount doesn't hit 0 by the time this call happens.
        AZStd::function<void()> instantiateCallback = [this, readyAsset]()
        {
            const AZ::Data::AssetId readyAssetId = readyAsset.GetId();
            for (auto iter = m_queuedSliceInstantiations.begin(); iter != m_queuedSliceInstantiations.end(); )
            {
                const InstantiatingSliceInfo& instantiating = *iter;

                if (instantiating.m_asset.GetId() == readyAssetId)
                {
                    // here we actually refcount / copy by value the internals of 'instantiating' since we will destroy it later
                    // but still wish to send bus messages based on ticket/asset.
                    AZ::Data::Asset<AZ::Data::AssetData> asset = instantiating.m_asset;
                    SliceInstantiationTicket ticket = instantiating.m_ticket;
                    m_instantiatingAssetId = instantiating.m_asset.GetId();
                    AZ::SliceComponent::SliceInstanceAddress instance = m_rootAsset->GetComponent()->
                        AddSlice(asset, instantiating.m_customMapper);

                    // Its important to remove this instantiation from the instantiation list
                    // as soon as possible, before we call these below notification functions, because they might result in our 
                    // own functions that search this list being called again.
                    iter = m_queuedSliceInstantiations.erase(iter);
                    // --------------------------- do not refer to 'instantiating' after the above call, it has been destroyed ------------

                    bool isSliceInstantiated = false;

                    if (instance.IsValid())
                    {
                        AZ_Assert(instance.GetInstance()->GetInstantiated(), "Failed to instantiate root slice!");

                        if (instance.GetInstance()->GetInstantiated() &&
                            m_validateEntitiesCallback(instance.GetInstance()->GetInstantiated()->m_entities))
                        {
                            SliceInstantiationResultBus::Event(ticket, &SliceInstantiationResultBus::Events::OnSlicePreInstantiate,
                                m_instantiatingAssetId, instance);

                            HandleEntitiesAdded(instance.GetInstance()->GetInstantiated()->m_entities);

                            SliceInstantiationResultBus::Event(ticket, &SliceInstantiationResultBus::Events::OnSliceInstantiated,
                                m_instantiatingAssetId, instance);

                            isSliceInstantiated = true;
                        }
                        else
                        {
                            // The slice has already been added to the root slice. But we are disallowing the
                            // instantiation. So we need to remove it
                            m_rootAsset->GetComponent()->RemoveSliceInstance(instance);
                        }
                    }

                    if (!isSliceInstantiated)
                    {
                        DispatchOnSliceInstantiationFailed(ticket, m_instantiatingAssetId, false);
                    }

                    // clear the Asset ID cache
                    m_instantiatingAssetId.SetInvalid();
                }
                else
                {
                    ++iter;
                }
            }
        };

        // Instantiation is queued against the tick bus. This ensures we're not holding the AssetBus lock
        // while the instantiation is handled, which may be costly. This also guarantees callers can
        // jump on the SliceInstantiationResultBus for their ticket before the events are fired.
        AZ::TickBus::QueueFunction(instantiateCallback);
    }

    void SliceEntityOwnershipService::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_PROFILE_FUNCTION(AzFramework);
        if (asset == m_rootAsset && asset.Get() != m_rootAsset.Get())
        {
            Reset();

            m_rootAsset = asset;

            auto* rootSliceComponent = m_rootAsset->GetComponent();

            // Because cloned components don't listen for changes by default as they are usually discarded,
            // we need to manually listen here - root is special in this way
            rootSliceComponent->ListenForAssetChanges();

            HandleNewMetadataEntitiesCreated(*m_rootAsset->GetComponent());

            AZ::SliceComponent::EntityList entities;
            m_rootAsset->GetComponent()->GetEntities(entities);

            HandleEntitiesAdded(entities);

            m_rootAsset->GetComponent()->ListenForDependentAssetChanges();
        }
    }

    SliceInstantiationTicket SliceEntityOwnershipService::InstantiateSlice(const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper, const AZ::Data::AssetFilterCB& assetLoadFilter)
    {
        if (asset.GetId().IsValid())
        {
            const SliceInstantiationTicket ticket = GenerateSliceInstantiationTicket();
            m_queuedSliceInstantiations.emplace_back(asset, ticket, customIdMapper);
            m_queuedSliceInstantiations.back().m_asset.QueueLoad(AZ::Data::AssetLoadParameters(assetLoadFilter));

            AZ::Data::AssetBus::MultiHandler::BusConnect(asset.GetId());

            return ticket;
        }

        return SliceInstantiationTicket();
    }

    void SliceEntityOwnershipService::CancelSliceInstantiation(const SliceInstantiationTicket& ticket)
    {
        auto iter = AZStd::find_if(m_queuedSliceInstantiations.begin(), m_queuedSliceInstantiations.end(),
            [ticket](const InstantiatingSliceInfo& instantiating)
        {
            return instantiating.m_ticket == ticket;
        });

        if (iter != m_queuedSliceInstantiations.end())
        {
            const AZ::Data::AssetId assetId = iter->m_asset.GetId();

            // Erase ticket, but stay connected to AssetBus in case asset is used by multiple tickets.
            m_queuedSliceInstantiations.erase(iter);

            // Clear the iterator so that code inserted after this point to operate on iter will raise issues.
            iter = m_queuedSliceInstantiations.end(); 

            // No need to queue this notification.
            // (It's queued in other circumstances, to avoid holding the AssetBus lock any longer than necessary)
            DispatchOnSliceInstantiationFailed(ticket, assetId, true);
        }
    }

    void SliceEntityOwnershipService::DispatchOnSliceInstantiationFailed(const SliceInstantiationTicket& ticket,
        const AZ::Data::AssetId& assetId, bool canceled)
    {
        SliceInstantiationResultBus::Event(ticket, &SliceInstantiationResultBus::Events::OnSliceInstantiationFailed, assetId);
        SliceInstantiationResultBus::Event(ticket, &SliceInstantiationResultBus::Events::OnSliceInstantiationFailedOrCanceled,
            assetId, canceled);
    }

    AZ::SliceComponent::SliceInstanceAddress SliceEntityOwnershipService::CloneSliceInstance(
        AZ::SliceComponent::SliceInstanceAddress sourceInstance, AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap)
    {
        AZ_PROFILE_FUNCTION(AzFramework);

        if (!sourceInstance.IsValid())
        {
            AZ_Assert(false, "Source slice instance is invalid.");
            return {};
        }

        AZ::SliceComponent::SliceInstance* newInstance = sourceInstance.GetReference()->CloneInstance(sourceInstance.GetInstance(),
            sourceToCloneEntityIdMap);

        return AZ::SliceComponent::SliceInstanceAddress(sourceInstance.GetReference(), newInstance);
    }

    AZ::SliceComponent::SliceInstanceAddress SliceEntityOwnershipService::GetOwningSlice()
    {
        const AZ::EntityId entityId = *SliceEntityRequestBus::GetCurrentBusId();
        return GetOwningSlice(entityId);
    }

    AZ::SliceComponent::SliceInstanceAddress SliceEntityOwnershipService::GetOwningSlice(AZ::EntityId entityId)
    {
        if (!CheckAndAssertRootComponentIsAvailable())
        {
            return {};
        }

        return m_rootAsset->GetComponent()->FindSlice(entityId);
    }

    const AZ::SliceComponent::EntityIdToEntityIdMap& SliceEntityOwnershipService::GetLoadedEntityIdMap()
    {
        return m_loadedEntityIdMap;
    }

    AZ::EntityId SliceEntityOwnershipService::FindLoadedEntityIdMapping(const AZ::EntityId& staticId) const
    {
        auto idIter = m_loadedEntityIdMap.find(staticId);

        if (idIter == m_loadedEntityIdMap.end())
        {
            return AZ::EntityId();
        }

        return idIter->second;
    }

    void SliceEntityOwnershipService::HandleEntitiesAdded(const EntityList& entities)
    {
        AZ_Assert(m_entitiesAddedCallback, "Callback function for AddEntity has not been set.");
        for (const AZ::Entity* entity : entities)
        {
            SliceEntityRequestBus::MultiHandler::BusConnect(entity->GetId());
        }

        m_entitiesAddedCallback(entities);
    }

    void SliceEntityOwnershipService::GetNonPrefabEntities(EntityList& entityList)
    {
        AZ::SliceComponent* rootSliceComponent = GetRootSlice();
        AZ_Error("SliceEntityOwnershipService", rootSliceComponent, "Root slice is not available.");
        if (rootSliceComponent)
        {
            const EntityList& newEntities = rootSliceComponent->GetNewEntities();
            entityList.insert(entityList.end(), newEntities.cbegin(), newEntities.cend());
        }
    }

    bool SliceEntityOwnershipService::GetAllEntities(EntityList& entityList)
    {
        AZ::SliceComponent* rootSliceComponent = GetRootSlice();
        if (rootSliceComponent)
        {
            return rootSliceComponent->GetEntities(entityList);
        }
        return false;
    }

    void SliceEntityOwnershipService::InstantiateAllPrefabs()
    {
        AZ::SliceComponent* rootSliceComponent = GetRootSlice();
        if (rootSliceComponent)
        {
            // Instantiating the root slice would in-turn instantiate all slices under it.
            rootSliceComponent->Instantiate();
        }
    }

    void SliceEntityOwnershipService::SetIsDynamic(bool isDynamic)
    {
        AZ::SliceComponent* rootSliceComponent = GetRootSlice();
        if (rootSliceComponent)
        {
            rootSliceComponent->SetIsDynamic(isDynamic);
        }
    }

    AZ::SerializeContext* SliceEntityOwnershipService::GetSerializeContext()
    {
        return m_serializeContext;
    }

    const RootSliceAsset& SliceEntityOwnershipService::GetRootAsset() const
    {
        return m_rootAsset;
    }

    void SliceEntityOwnershipService::SetEntitiesAddedCallback(OnEntitiesAddedCallback onEntitiesAddedCallback)
    {
        m_entitiesAddedCallback = AZStd::move(onEntitiesAddedCallback);
    }

    void SliceEntityOwnershipService::SetEntitiesRemovedCallback(OnEntitiesRemovedCallback onEntitiesRemovedCallback)
    {
        m_entitiesRemovedCallback = AZStd::move(onEntitiesRemovedCallback);
    }

    void SliceEntityOwnershipService::SetValidateEntitiesCallback(ValidateEntitiesCallback validateEntitiesCallback)
    {
        m_validateEntitiesCallback = AZStd::move(validateEntitiesCallback);
    }

    void SliceEntityOwnershipService::HandleEntityBeingDestroyed(const AZ::EntityId& entityId)
    {
        AZ_Assert(m_entitiesRemovedCallback, "Callback function for entity removal has not been set.");

        if (m_entitiesAddedCallback)
        {
            m_entitiesRemovedCallback({ entityId });
        }

        // Entities removed through the application (as in via manual 'delete'),
        // should be removed from the root slice, but not again deleted.
        if (CheckAndAssertRootComponentIsAvailable())
        {
            m_rootAsset->GetComponent()->RemoveEntity(entityId, false);
        }
    }

    AZ::Data::AssetId SliceEntityOwnershipService::CurrentlyInstantiatingSlice()
    {
        return m_instantiatingAssetId;
    }
}
