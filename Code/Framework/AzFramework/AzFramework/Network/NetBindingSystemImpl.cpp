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

#include <AzFramework/Network/NetBindingSystemImpl.h>
#include <AzFramework/Network/NetBindingComponent.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Entity/SliceGameEntityOwnershipServiceBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Slice/SliceAsset.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/ReplicaFunctions.h>

//#define Extra_Tracing
#undef  Extra_Tracing

#if defined(Extra_Tracing)
#include <AzCore/Debug/Timer.h>
#define AZ_ExtraTracePrintf(window, ...)     AZ::Debug::Trace::Instance().Printf(window, __VA_ARGS__);
#else
#define AZ_ExtraTracePrintf(window, ...)
#endif

namespace AzFramework
{
    const AZStd::chrono::milliseconds NetBindingSystemImpl::s_sliceBindingTimeout = AZStd::chrono::milliseconds(5000);

    namespace
    {
        NetBindingHandlerInterface* GetNetBindingHandler(AZ::Entity* entity)
        {
            NetBindingHandlerInterface* handler = nullptr;
            for (AZ::Component* component : entity->GetComponents())
            {
                handler = azrtti_cast<NetBindingHandlerInterface*>(component);
                if (handler)
                {
                    break;
                }
            }
            return handler;
        }
    }

    NetBindingSliceInstantiationHandler::~NetBindingSliceInstantiationHandler()
    {
        // m_bindRequests in NetBindingSystemImpl could be cleaned before slice instantiation finished
        if (m_state == State::Spawning)
        {
            AzFramework::SliceInstantiationResultBus::Handler::BusDisconnect();
            SliceGameEntityOwnershipServiceRequestBus::Broadcast(
                &SliceGameEntityOwnershipServiceRequests::CancelDynamicSliceInstantiation, m_ticket
            );
        }

        for (AZ::Entity* entity : m_boundEntities)
        {
            AZ_ExtraTracePrintf("NetBindingSystemImpl", "Cleanup - deleting %llu\n", entity->GetId());
            EBUS_EVENT(GameEntityContextRequestBus, DestroyGameEntity, entity->GetId());
        }
    }

    void NetBindingSliceInstantiationHandler::InstantiateEntities()
    {
        if (m_sliceAssetId.IsValid())
        {
            AZ_ExtraTracePrintf("NetBindingSystemImpl", "InstantiateEntities sliceid %s\n",
                m_sliceInstanceId.ToString<AZStd::string>(false, false).c_str());

            if (AZ::Data::AssetManager::IsReady())
            {
                auto remapFunc = [bindingQueue=m_bindingQueue](AZ::EntityId originalId, bool /*isEntityId*/, const AZStd::function<AZ::EntityId()>&) -> AZ::EntityId
                {
                    auto iter = bindingQueue.find(originalId);
                    if (iter != bindingQueue.end())
                    {
                        return iter->second.m_desiredRuntimeEntityId;
                    }
                    return AZ::Entity::MakeId();
                };

                AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::DynamicSliceAsset>(m_sliceAssetId, AZ::Data::AssetLoadBehavior::Default);

                SliceGameEntityOwnershipServiceRequestBus::BroadcastResult(m_ticket,
                    &SliceGameEntityOwnershipServiceRequests::InstantiateDynamicSlice, asset, AZ::Transform::Identity(), remapFunc);
                SliceInstantiationResultBus::Handler::BusConnect(m_ticket);

                m_state = State::Spawning;
            }
            else
            {
                AZ_Warning("NetBindingSystemImpl", false, "AssetManager was not ready when attempting to instantiate sliceid %s\n",
                    m_sliceInstanceId.ToString<AZStd::string>(false, false).c_str());
                InstantiationFailureCleanup();
            }
        }
    }

    bool NetBindingSliceInstantiationHandler::IsInstantiated() const
    {
        return m_state == State::Spawned;
    }

    bool NetBindingSliceInstantiationHandler::IsANewSliceRequest() const
    {
        return m_state == State::NewRequest && m_sliceAssetId.IsValid() && !m_ticket.IsValid();
    }

    bool NetBindingSliceInstantiationHandler::IsBindingComplete() const
    {
        return !SliceInstantiationResultBus::Handler::BusIsConnected() && m_bindingQueue.empty();
    }

    bool NetBindingSliceInstantiationHandler::HasActiveEntities() const
    {
        for (const AZ::Entity* entity : m_boundEntities)
        {
            if (entity->GetState() == AZ::Entity::State::Active)
            {
                return true;
            }
        }

        return false;
    }

    void NetBindingSliceInstantiationHandler::OnSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        const auto& entityMapping = sliceAddress.GetInstance()->GetEntityIdToBaseMap();

        const AZ::SliceComponent::EntityList& sliceEntities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;
        for (AZ::Entity *sliceEntity : sliceEntities)
        {
            auto it = entityMapping.find(sliceEntity->GetId());
            AZ_Assert(it != entityMapping.end(), "Failed to retrieve static entity id for a slice entity!");
            const AZ::EntityId staticEntityId = it->second;

            auto itBindRecord = m_bindingQueue.find(staticEntityId);
            if (itBindRecord != m_bindingQueue.end())
            {
                AZ_Assert(GetNetBindingHandler(sliceEntity), "Slice entity matched the static id of replicated entity, but there is no valid NetBindingHandlerInterface on it!");

                itBindRecord->second.m_actualRuntimeEntityId = sliceEntity->GetId();
            }
            else if (GetNetBindingHandler(sliceEntity))
            {
                AZ_ExtraTracePrintf("NetBindingSystemImpl", "OnSlicePreInstantiate late bindRequest, slice %s, staticid %llu, spawned %llu\n",
                    m_sliceInstanceId.ToString<AZStd::string>(false, false).c_str(),
                    static_cast<AZ::u64>(staticEntityId),
                    static_cast<AZ::u64>(sliceEntity->GetId()));

                BindRequest& request = m_bindingQueue[staticEntityId];
                request.m_desiredRuntimeEntityId = staticEntityId;
                request.m_actualRuntimeEntityId = sliceEntity->GetId();
                request.m_requestTime = m_bindTime;
                request.m_state = BindRequest::State::PlaceholderBind;
            }

            sliceEntity->SetRuntimeActiveByDefault(false);
        }
    }

    void NetBindingSliceInstantiationHandler::OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        SliceInstantiationResultBus::Handler::BusDisconnect();

        CloseEntityMap(sliceAddress.GetInstance()->GetEntityIdMap());

        const AZ::SliceComponent::EntityList sliceEntities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;
        for (AZ::Entity *sliceEntity : sliceEntities)
        {
            auto it = sliceAddress.GetInstance()->GetEntityIdToBaseMap().find(sliceEntity->GetId());
            AZ_Assert(it != sliceAddress.GetInstance()->GetEntityIdToBaseMap().end(), "Failed to retrieve static entity id for a slice entity!");
            const AZ::EntityId staticEntityId = it->second;
            const auto itUnbound = m_bindingQueue.find(staticEntityId);
            if (itUnbound == m_bindingQueue.end())
            {
                /*
                 * Remove entities that aren't meant to be net bounded.
                 */
                if (!GetNetBindingHandler(sliceEntity))
                {
                    EBUS_EVENT(GameEntityContextRequestBus, DestroyGameEntity, sliceEntity->GetId());
                    continue;
                }
            }

            AZ_ExtraTracePrintf("NetBindingSystemImpl", "Adding %llu \n", sliceEntity->GetId());
            m_boundEntities.push_back(sliceEntity);
        }

        m_state = State::Spawned;
    }

    void NetBindingSliceInstantiationHandler::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
    {
        SliceInstantiationResultBus::Handler::BusDisconnect();

        AZ_UNUSED(sliceAssetId);
        AZ_TracePrintf("NetBindingSystemImpl", "Failed to instantiate a slice %s!", sliceAssetId.ToString<AZStd::string>().c_str());

        InstantiationFailureCleanup();
    }

    void NetBindingSliceInstantiationHandler::InstantiationFailureCleanup()
    {
        m_boundEntities.clear();
        m_bindingQueue.clear();

        // With m_bindingQueue empty, this slice instance handler will be removed on the next tick of NetBindingSystemImpl
        m_state = State::Failed;
    }

    void NetBindingSliceInstantiationHandler::UseCacheFor(BindRequest& request, const AZ::EntityId& staticEntityId)
    {
        AZ_Warning("NetBindingSystemImpl", !m_staticToRuntimeEntityMap.empty(), "An empty slice, really? static %llu",
            static_cast<AZ::u64>(staticEntityId));

        const auto actualRuntimeIter = m_staticToRuntimeEntityMap.find(staticEntityId);
        if (actualRuntimeIter == m_staticToRuntimeEntityMap.end())
        {
            AZ_Warning("NetBindingSystemImpl", false, "Wrong mapping, expected cache to have entity %llu for slice %s \n",
                static_cast<AZ::u64>(staticEntityId),
                m_sliceInstanceId.ToString<AZStd::string>(false, false).c_str());

#if defined(Extra_Tracing)
            for (auto& item: m_staticToRuntimeEntityMap)
            {
                AZ_UNUSED(item);
                AZ_ExtraTracePrintf("NetBindingSystemImpl", "mapping had %llu to %llu \n",
                    static_cast<AZ::u64>(item.first),
                    static_cast<AZ::u64>(item.second));
            }
#endif

            return;
        }

        const AZ::EntityId actualRuntimeEntityId = actualRuntimeIter->second;
        const auto itCache = AZStd::find_if(m_boundEntities.begin(), m_boundEntities.end(), [&actualRuntimeEntityId](AZ::Entity* entity) {
            return entity->GetId() == actualRuntimeEntityId;
        });

        if (itCache != m_boundEntities.end())
        {
            AZ_ExtraTracePrintf("NetBindingSystemImpl", "OnSlicePreInstantiate late bindRequest, slice %s, staticid %llu, spawned %llu\n",
                m_sliceInstanceId.ToString<AZStd::string>(false, false).c_str(),
                static_cast<AZ::u64>(staticEntityId),
                static_cast<AZ::u64>(actualRuntimeEntityId));

            request.m_actualRuntimeEntityId = actualRuntimeEntityId;
            request.m_desiredRuntimeEntityId = staticEntityId;
        }
        else
        {
            AZ_Warning("NetBindingSystemImpl", false, "Expected cache to have entity %llu for slice %s \n",
                static_cast<AZ::u64>(request.m_desiredRuntimeEntityId),
                m_sliceInstanceId.ToString<AZStd::string>(false, false).c_str());
        }
    }

    void NetBindingSliceInstantiationHandler::CloseEntityMap(
        const AZ::SliceComponent::EntityIdToEntityIdMap& staticToRuntimeMap)
    {
        m_staticToRuntimeEntityMap.clear();
        for (auto& item : staticToRuntimeMap)
        {
            m_staticToRuntimeEntityMap[item.first] = item.second;
        }
    }

    NetBindingSystemContextData::NetBindingSystemContextData()
        : m_bindingContextSequence("BindingContextSequence", UnspecifiedNetBindingContextSequence)
    {
    }

    void NetBindingSystemContextData::OnReplicaActivate(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
        NetBindingSystemImpl* system = static_cast<NetBindingSystemImpl*>(NetBindingSystemBus::FindFirstHandler());
        AZ_Assert(system, "NetBindingSystemContextData requires a valid NetBindingSystemComponent to function!");
        system->OnContextDataActivated(this);
    }

    void NetBindingSystemContextData::OnReplicaDeactivate(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
        NetBindingSystemImpl* system = static_cast<NetBindingSystemImpl*>(NetBindingSystemBus::FindFirstHandler());
        if (system)
        {
            system->OnContextDataDeactivated(this);
        }
    }


    NetBindingSystemImpl::NetBindingSystemImpl()
        : m_bindingSession(nullptr)
        , m_currentBindingContextSequence(UnspecifiedNetBindingContextSequence)
        , m_isAuthoritativeRootSliceLoad(false)
        , m_overrideRootSliceLoadAuthoritative(false)
    {
    }

    NetBindingSystemImpl::~NetBindingSystemImpl()
    {
    }

    void NetBindingSystemImpl::Init()
    {
        NetBindingSystemBus::Handler::BusConnect();
        NetBindingSystemEventsBus::Handler::BusConnect();

        // Start listening for game context events
        EntityContextId gameContextId = EntityContextId::CreateNull();
        EBUS_EVENT_RESULT(gameContextId, GameEntityContextRequestBus, GetGameEntityContextId);
        if (!gameContextId.IsNull())
        {
            EntityContextEventBus::Handler::BusConnect(gameContextId);
        }
    }

    void NetBindingSystemImpl::Shutdown()
    {
        EntityContextEventBus::Handler::BusDisconnect();
        NetBindingSystemEventsBus::Handler::BusDisconnect();
        NetBindingSystemBus::Handler::BusDisconnect();

        m_contextData.reset();
    }

    bool NetBindingSystemImpl::ShouldBindToNetwork()
    {
        return m_contextData && m_contextData->ShouldBindToNetwork();
    }

    NetBindingContextSequence NetBindingSystemImpl::GetCurrentContextSequence()
    {
        return m_currentBindingContextSequence;
    }

    bool NetBindingSystemImpl::ReadyToAddReplica() const
    {
        return m_bindingSession && m_bindingSession->GetReplicaMgr();
    }

    void NetBindingSystemImpl::AddReplicaMaster(AZ::Entity* entity, GridMate::ReplicaPtr replica)
    {
        bool addReplica = ShouldBindToNetwork();
        AZ_Assert(addReplica, "Entities shouldn't be binding to the network right now!");
        if (addReplica)
        {
            if (ReadyToAddReplica())
            {
                m_bindingSession->GetReplicaMgr()->AddMaster(replica);
            }
            else
            {
            m_addMasterRequests.push_back(AZStd::make_pair(entity->GetId(), replica));
        }
    }
    }


    AZ::EntityId NetBindingSystemImpl::GetStaticIdFromEntityId(AZ::EntityId entityId)
    {
        AZ::EntityId staticId = entityId; // if no static id mapping is found, then the static id is the same as the runtime id

        // If entity came from a slice, try to get the mapping from it
        AZ::SliceComponent::SliceInstanceAddress sliceInfo;
        SliceEntityRequestBus::EventResult(sliceInfo, entityId, &SliceEntityRequestBus::Events::GetOwningSlice);
        AZ::SliceComponent::SliceInstance* sliceInstance = sliceInfo.GetInstance();
        if (sliceInstance)
        {
            const auto it = sliceInstance->GetEntityIdToBaseMap().find(entityId);
            if (it != sliceInstance->GetEntityIdToBaseMap().end())
            {
                staticId = it->second;
            }
        }

        return staticId;
    }

    AZ::EntityId NetBindingSystemImpl::GetEntityIdFromStaticId(AZ::EntityId staticEntityId)
    {
        AZ::EntityId runtimeId = AZ::EntityId();

        // if we can find an entity with the static id, then the static id is the same as the runtime id.
        AZ::Entity* entity = nullptr;
        EBUS_EVENT(AZ::ComponentApplicationBus, FindEntity, staticEntityId);
        if (entity)
        {
            runtimeId = staticEntityId;
        }

        return runtimeId;
    }

    void NetBindingSystemImpl::SpawnEntityFromSlice(GridMate::ReplicaId bindTo, const NetBindingSliceContext& bindToContext)
    {
        auto& sliceQueue = m_bindRequests[bindToContext.m_contextSequence];

        const bool slicePresent = sliceQueue.find(bindToContext.m_sliceInstanceId) != sliceQueue.end();

        auto iterSliceRequest = sliceQueue.insert_key(bindToContext.m_sliceInstanceId);
        NetBindingSliceInstantiationHandler& sliceHandler = iterSliceRequest.first->second;
        sliceHandler.m_sliceAssetId = bindToContext.m_sliceAssetId;
        sliceHandler.m_sliceInstanceId = bindToContext.m_sliceInstanceId;

        BindRequest& request = sliceHandler.m_bindingQueue[bindToContext.m_staticEntityId];

        if (!slicePresent)
        {
            request.m_state = BindRequest::State::FirstBindInSlice;
        }
        else
        {
            request.m_state = BindRequest::State::LateBind;
        }

        AZ_ExtraTracePrintf("NetBindingSystemImpl", "SpawnEntityFromSlice late, slice %s, static %llu, desired %llu, state %d \n",
            bindToContext.m_sliceInstanceId.ToString<AZStd::string>(false, false).c_str(),
            static_cast<AZ::u64>(bindToContext.m_staticEntityId),
            static_cast<AZ::u64>(bindToContext.m_runtimeEntityId),
            request.m_state);

        sliceHandler.m_bindTime = Now();

        request.m_bindTo = bindTo;
        request.m_desiredRuntimeEntityId = bindToContext.m_runtimeEntityId;
        request.m_requestTime = Now();

        if (sliceHandler.IsInstantiated())
        {
            // The slice has been instantiated now, thus we have to use the cache to populated the request with the entity.
            sliceHandler.UseCacheFor(request, bindToContext.m_staticEntityId);
        }
    }

    void NetBindingSystemImpl::SpawnEntityFromStream(AZ::IO::GenericStream& spawnData, AZ::EntityId useEntityId, GridMate::ReplicaId bindTo, NetBindingContextSequence addToContext)
    {
        auto& requestQueue = m_spawnRequests[addToContext];
        requestQueue.push_back();
        SpawnRequest& request = requestQueue.back();
        request.m_bindTo = bindTo;
        request.m_useEntityId = useEntityId;
        request.m_spawnDataBuffer.resize_no_construct(spawnData.GetLength());
        spawnData.Read(request.m_spawnDataBuffer.size(), request.m_spawnDataBuffer.data());
    }

    void NetBindingSystemImpl::OnNetworkSessionActivated(GridMate::GridSession* session)
    {
        AZ_Assert(!m_bindingSession, "We already have an active session! Was the previous session deactivated?");
        if (!m_bindingSession)
        {
            m_bindingSession = session;

            if (m_bindingSession->IsHost())
            {
                GridMate::Replica* replica = CreateSystemReplica();
                session->GetReplicaMgr()->AddMaster(replica);
            }
        }
    }

    void NetBindingSystemImpl::OnNetworkSessionDeactivated(GridMate::GridSession* session)
    {
        if (session == m_bindingSession)
        {
            m_bindingSession = nullptr;
        }
    }

    void NetBindingSystemImpl::UnbindGameEntity(AZ::EntityId entityId, const AZ::SliceComponent::SliceInstanceId& sliceInstanceId)
    {
        if (!m_bindRequests.empty())
        {
            const auto itCurrentContextQueue = m_bindRequests.lower_bound(GetCurrentContextSequence());

            if (itCurrentContextQueue != m_bindRequests.end())
            {
                if (itCurrentContextQueue->first == GetCurrentContextSequence())
                {
                    const auto itSliceHandler = itCurrentContextQueue->second.find(sliceInstanceId);
                    if (itSliceHandler != itCurrentContextQueue->second.end())
                    {
                        NetBindingSliceInstantiationHandler& sliceHandler = itSliceHandler->second;
                        for (AZ::Entity* entity : sliceHandler.m_boundEntities)
                        {
                            if (entity->GetId() == entityId)
                            {
                                entity->Deactivate();
                                return;
                            }
                        }

                        // clean any relevant bind requests as well
                        const auto bindQueueItem = sliceHandler.m_bindingQueue.find(entityId);
                        if (bindQueueItem != sliceHandler.m_bindingQueue.end())
                        {
                            sliceHandler.m_bindingQueue.erase(bindQueueItem);
                            return;
                        }
                    }
                }
            }
        }

        AZ_ExtraTracePrintf("NetBindingSystemImpl", "Not in cache - deleting %llu \n", entityId);
        EBUS_EVENT(GameEntityContextRequestBus, DestroyGameEntity, entityId);
    }

    void NetBindingSystemImpl::OnEntityContextReset()
    {
        const bool isContextOwner = m_contextData && m_contextData->IsMaster() && m_bindingSession && m_bindingSession->IsHost();
        if (isContextOwner)
        {
            ++m_currentBindingContextSequence;
            NetBindingSystemContextData* context = static_cast<NetBindingSystemContextData*>(m_contextData.get());
            context->m_bindingContextSequence.Set(m_currentBindingContextSequence);
        }
    }

    bool NetBindingSystemImpl::IsAuthoritateLoad() const
    {
        if (m_overrideRootSliceLoadAuthoritative)
        {
            return m_isAuthoritativeRootSliceLoad;
        }

        return !m_bindingSession || m_bindingSession->IsHost();
    }

    void NetBindingSystemImpl::UpdateClock(float deltaTime)
    {
        m_currentTime += AZStd::chrono::milliseconds(aznumeric_cast<int>(deltaTime * AZStd::milli::den));
    }

    AZStd::chrono::system_clock::time_point NetBindingSystemImpl::Now() const
    {
        return m_currentTime;
    }

    void NetBindingSystemImpl::OnEntityContextLoadedFromStream(const AZ::SliceComponent::EntityList& contextEntities)
    {
        const bool isAuthoritativeLoad = IsAuthoritateLoad();

        for (AZ::Entity* entity : contextEntities)
        {
            NetBindingHandlerInterface* netBinder = GetNetBindingHandler(entity);
            if (netBinder)
            {
                netBinder->MarkAsLevelSliceEntity();
            }

            if (!isAuthoritativeLoad && netBinder)
            {
                entity->SetRuntimeActiveByDefault(false);

                auto& slicesQueue = m_bindRequests[GetCurrentContextSequence()];
                auto& sliceHandler = slicesQueue[UnspecifiedSliceInstanceId];
                BindRequest& request = sliceHandler.m_bindingQueue[entity->GetId()];
                request.m_actualRuntimeEntityId = entity->GetId();
                request.m_requestTime = Now();
            }
        }
    }

    void NetBindingSystemImpl::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        AZ_UNUSED(time);

        UpdateClock(deltaTime);
        UpdateContextSequence();

#if defined(Extra_Tracing)
        static AZ::Debug::Timer sTimer;
        sTimer.Stamp();
#endif
        ProcessBindRequests();
#if defined(Extra_Tracing)
        const float seconds = sTimer.StampAndGetDeltaTimeInSeconds();

        static float debugPeriod = 2.f;
        static float accumulator = 0;
        static float totalTimeTaken = 0;
        static AZ::u32 totalTicks = 0;
        accumulator += deltaTime;
        totalTimeTaken += seconds;
        totalTicks++;

        if (accumulator >= debugPeriod)
        {
            AZ_ExtraTracePrintf("NetBindingSystemImpl", "ProcessBindRequests() took %f sec \n", totalTicks > 0 ? totalTimeTaken / totalTicks : 0);

            accumulator -= debugPeriod;
            totalTimeTaken = 0;
            totalTicks = 0;
        }
#endif

        ProcessSpawnRequests();
    }

    int NetBindingSystemImpl::GetTickOrder()
    {
        return AZ::TICK_PLACEMENT + 1;
    }

    void NetBindingSystemImpl::UpdateContextSequence()
    {
        NetBindingSystemContextData* contextChunk = static_cast<NetBindingSystemContextData*>(m_contextData.get());
        if (m_currentBindingContextSequence != contextChunk->m_bindingContextSequence.Get())
        {
            m_currentBindingContextSequence = contextChunk->m_bindingContextSequence.Get();
        }
    }

    GridMate::Replica* NetBindingSystemImpl::CreateSystemReplica()
    {
        AZ_Assert(m_bindingSession->IsHost(), "CreateSystemReplica should only be called on the host!");
        GridMate::Replica* replica = GridMate::Replica::CreateReplica("NetBindingSystem");
        NetBindingSystemContextData* contextChunk = GridMate::CreateReplicaChunk<NetBindingSystemContextData>();
        replica->AttachReplicaChunk(contextChunk);

        return replica;
    }

    void NetBindingSystemImpl::OnContextDataActivated(GridMate::ReplicaChunkPtr contextData)
    {
        AZ_Assert(!m_contextData, "We already have our context!");
        m_contextData = contextData;

        // Make sure we always have the unspecified entry. This should also
        // be the lower_bound in the map and assuming it is always there
        // makes things simpler.
        m_spawnRequests.insert(UnspecifiedNetBindingContextSequence);
        m_bindRequests.insert(UnspecifiedNetBindingContextSequence);

        if (contextData->IsMaster())
        {
            ++m_currentBindingContextSequence;
            static_cast<NetBindingSystemContextData*>(contextData.get())->m_bindingContextSequence.Set(m_currentBindingContextSequence);
        }
        else
        {
            UpdateContextSequence();
        }
        AZ::TickBus::Handler::BusConnect();
        EBUS_EVENT(AzFramework::NetBindingHandlerBus, BindToNetwork, nullptr);
    }

    void NetBindingSystemImpl::OnContextDataDeactivated(GridMate::ReplicaChunkPtr contextData)
    {
        AZ_Assert(m_contextData == contextData, "This is not our context!");
        m_contextData = nullptr;

        AZ::TickBus::Handler::BusDisconnect();
        m_spawnRequests.clear();
        m_bindRequests.clear();
        m_addMasterRequests.clear();
        m_currentBindingContextSequence = UnspecifiedNetBindingContextSequence;
    }

    void NetBindingSystemImpl::ProcessSpawnRequests()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "NetBindingSystemComponent requires a valid SerializeContext in order to spawn entities!");
        const auto spawnFunc = [=](SpawnRequest& spawnData, AZ::EntityId useEntityId, bool addToContext)
        {
            AZ::Entity* proxyEntity = nullptr;
            AZ::ObjectStream::ClassReadyCB readyCB([&](void* classPtr, const AZ::Uuid& classId, AZ::SerializeContext* sc)
            {
                (void)classId;
                (void)sc;
                proxyEntity = static_cast<AZ::Entity*>(classPtr);
            });
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > stream(&spawnData.m_spawnDataBuffer);
            AZ::ObjectStream::LoadBlocking(&stream, *serializeContext, readyCB);

            AZ_Warning("NetBindingSystemImpl", proxyEntity, "Could not spawn entity from stream %llu", useEntityId);
            if (proxyEntity)
            {
                proxyEntity->SetId(useEntityId);
                if (!BindAndActivate(proxyEntity, spawnData.m_bindTo, addToContext, AZ::Uuid::CreateNull()))
                {
                    AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
                    AzFramework::EntityIdContextQueryBus::EventResult(
                        contextId, proxyEntity->GetId(), &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);

                    if (contextId.IsNull())
                    {
                        delete proxyEntity;
                    }
                    else
                    {
                        GameEntityContextRequestBus::Broadcast(
                            &GameEntityContextRequestBus::Events::DestroyGameEntity, proxyEntity->GetId());
                    }

                }
            }
        };

        if (!m_spawnRequests.empty())
        {
            SpawnRequestContextContainerType::iterator itContextQueue = m_spawnRequests.lower_bound(UnspecifiedNetBindingContextSequence);
            AZ_Assert(itContextQueue->first == UnspecifiedNetBindingContextSequence, "We should always have the unspecified (aka global entity) spawn queue!");//

            // Process requests for global entities (not part of any context)
            SpawnRequestContainerType& globalQueue = itContextQueue->second;
            for (SpawnRequest& request : globalQueue)
            {
                spawnFunc(request, request.m_useEntityId, false);
            }
            globalQueue.clear();

            if (GetCurrentContextSequence() != UnspecifiedNetBindingContextSequence)
            {
                ++itContextQueue;

                // Clear any obsolete requests (any contexts below the current context sequence)
                SpawnRequestContextContainerType::iterator itCurrentContextQueue = m_spawnRequests.lower_bound(GetCurrentContextSequence());
                if (itContextQueue != itCurrentContextQueue)
                {
                    m_spawnRequests.erase(itContextQueue, itCurrentContextQueue);
                }

                // Spawn any entities for the current context
                if (itCurrentContextQueue != m_spawnRequests.end())
                {
                    if (itCurrentContextQueue->first == GetCurrentContextSequence())
                    {
                        for (SpawnRequest& request : itCurrentContextQueue->second)
                        {
                            spawnFunc(request, request.m_useEntityId, true);
                        }
                        itCurrentContextQueue->second.clear();
                    }
                }
            }
        }
    }

    void NetBindingSystemImpl::ProcessBindRequests()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "NetBindingSystemComponent requires a valid SerializeContext in order to spawn entities!");

        if (!m_bindRequests.empty())
        {
            BindRequestContextContainerType::iterator itContextQueue = m_bindRequests.lower_bound(UnspecifiedNetBindingContextSequence);
            AZ_Assert(itContextQueue->first == UnspecifiedNetBindingContextSequence, "We should always have the unspecified/global spawn queue!");

            if (GetCurrentContextSequence() != UnspecifiedNetBindingContextSequence)
            {
                ++itContextQueue;

                // Clear any obsolete requests (any contexts below the current context sequence)
                BindRequestContextContainerType::iterator itCurrentContextQueue = m_bindRequests.lower_bound(GetCurrentContextSequence());
                if (itContextQueue != itCurrentContextQueue)
                {
                    m_bindRequests.erase(itContextQueue, itCurrentContextQueue);
                }

                // Spawn any proxy entities for the current context
                if (itCurrentContextQueue != m_bindRequests.end())
                {
                    if (itCurrentContextQueue->first == GetCurrentContextSequence())
                    {
                        for (auto itSliceHandler = itCurrentContextQueue->second.begin(); itSliceHandler != itCurrentContextQueue->second.end(); /*++itSliceHandler*/)
                        {
                            NetBindingSliceInstantiationHandler& sliceHandler = itSliceHandler->second;

                            // If this is a new slice request, instantiate it
                            if (sliceHandler.IsANewSliceRequest())
                            {
                                sliceHandler.InstantiateEntities();
                            }
                            /*
                             * A slice instance is kept alive for caching purposes. As we check each bind request for its readiness,
                             * we are also going to check if the slice instance itself has become inactive and needs to be removed.
                             */
                            bool mightBeInactiveSlice = true;
                            if (sliceHandler.m_bindingQueue.empty() && sliceHandler.HasActiveEntities())
                            {
                                // The slice instance is spawned and full bound.
                                mightBeInactiveSlice = false;
                            }

                            // If the entity is ready to be bound to the network, bind it.
                            // NOTE: It is possible for entities spawned from a slice containing multiple entities with net binding
                            // to never receive their replica counterpart, either because the replica was destroyed, or was interest
                            // filtered. We don't have a very good pipeline to prevent these slices from being authored, so if we
                            // encounter them, we will delete them after a timeout.
                            for (auto itRequest = sliceHandler.m_bindingQueue.begin(); itRequest != sliceHandler.m_bindingQueue.end(); /*++itRequest*/)
                            {
                                BindRequest& request = itRequest->second;

                                if (request.m_bindTo != GridMate::InvalidReplicaId && request.m_actualRuntimeEntityId.IsValid())
                                {
                                    AZ::Entity* proxyEntity = nullptr;
                                    EBUS_EVENT_RESULT(proxyEntity, AZ::ComponentApplicationBus, FindEntity, request.m_actualRuntimeEntityId);
                                    AZ_Warning("NetBindingSystemImpl", proxyEntity, "Could not find entity for binding %llu", request.m_actualRuntimeEntityId);
                                    if (proxyEntity)
                                    {
                                        AZ_ExtraTracePrintf("NetBindingSystemImpl", "BindAndActivate desired id %llu, actual %llu, slice %s \n",
                                            static_cast<AZ::u64>(request.m_desiredRuntimeEntityId),
                                            static_cast<AZ::u64>(request.m_actualRuntimeEntityId),
                                            sliceHandler.m_sliceInstanceId.ToString<AZStd::string>(false, false).c_str());

                                        BindAndActivate(proxyEntity, request.m_bindTo, false, sliceHandler.m_sliceInstanceId);
                                    }
                                    itRequest = sliceHandler.m_bindingQueue.erase(itRequest);

                                    // The slice instance is not fully bound. It may remain for a while for caching purposes.
                                    mightBeInactiveSlice = false;
                                }
                                else if (AZStd::chrono::milliseconds(Now() - request.m_requestTime) > s_sliceBindingTimeout)
                                {
                                    // If the real request never showed up, then no need for a trace
                                    if (request.m_state == BindRequest::State::FirstBindInSlice ||
                                        request.m_state == BindRequest::State::LateBind)
                                    {
                                        AZ_TracePrintf("NetBindingSystemImpl", "Entity with static id [%llu], slice [%s]\n is still unbound after %llu ms. Discarding unbound entity.\n",
                                            static_cast<AZ::u64>(request.m_actualRuntimeEntityId),
                                            sliceHandler.m_sliceInstanceId.ToString<AZStd::string>(false, false).c_str(),
                                            s_sliceBindingTimeout.count());
                                    }

                                    switch (sliceHandler.m_state)
                                    {
                                    case NetBindingSliceInstantiationHandler::State::NewRequest:
                                    case NetBindingSliceInstantiationHandler::State::Spawning:
                                        // The slice instance isn't ready yet. We will wait to consider the timing logic until it is ready.
                                        mightBeInactiveSlice = false;
                                        break;
                                    case NetBindingSliceInstantiationHandler::State::Spawned:
                                    case NetBindingSliceInstantiationHandler::State::Failed:
                                        // Now the timing logic for removing the slice instance becomes valid.
                                        mightBeInactiveSlice = true;
                                        break;
                                    default:
                                        break;
                                    }

                                    ++itRequest;
                                }
                                else
                                {
                                    mightBeInactiveSlice = false;
                                    ++itRequest;
                                }
                            }

                            if (mightBeInactiveSlice && !sliceHandler.HasActiveEntities())
                            {
                                AZ_ExtraTracePrintf("NetBindingSystemImpl", "Removing inactive slice %s \n",
                                    sliceHandler.m_sliceInstanceId.ToString<AZStd::string>(false, false).c_str());

                                itSliceHandler = itCurrentContextQueue->second.erase(itSliceHandler);
                            }
                            else
                            {
                                ++itSliceHandler;
                            }
                        }
                    }
                }
            }
        }

        // Spawn replicas for any local entities that are still valid
        for (auto& addRequest : m_addMasterRequests)
        {
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, addRequest.first);
            if (entity)
            {
                m_bindingSession->GetReplicaMgr()->AddMaster(addRequest.second);
            }
        }
        m_addMasterRequests.clear();
    }

    bool NetBindingSystemImpl::BindAndActivate(AZ::Entity* entity, GridMate::ReplicaId replicaId, bool addToContext,
        const AZ::SliceComponent::SliceInstanceId& sliceInstanceId)
    {
        bool success = false;

        if ( ShouldBindToNetwork() )
        {
            const GridMate::ReplicaPtr bindTo = m_contextData->GetReplicaManager()->FindReplica(replicaId);
            if (bindTo)
            {
                if (addToContext)
                {
                    EBUS_EVENT(GameEntityContextRequestBus, AddGameEntity, entity);
                }

                if (entity->GetState() == AZ::Entity::State::Constructed)
                {
                    entity->Init();
                }

                NetBindingHandlerInterface* binding = GetNetBindingHandler(entity);
                AZ_Warning("NetBindingSystemImpl", binding, "Can't find NetBindingComponent on entity %llu (%s)!", static_cast<AZ::u64>(entity->GetId()), entity->GetName().c_str());
                if (binding)
                {
                    binding->BindToNetwork(bindTo);
                    binding->SetSliceInstanceId(sliceInstanceId);

                    entity->Activate();
                    success = true;
                }
            }
            else
            {
                // NOTE: It is possible for entities spawned from a slice containing multiple entities with net binding
                // to never receive their replica counterpart, either because the replica was destroyed, or was interest
                // filtered.
                AZ_ExtraTracePrintf("NetBindingSystemImpl", "Failed to bind entity %llu - could not find replica %u", entity->GetId(), replicaId);
            }
        }

        return success;
    }

    void NetBindingSystemImpl::Reflect(AZ::ReflectContext* context)
    {
        if (context)
        {
            // We need to register the chunk type, and this would be a good time to do so.
            if (!GridMate::ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(GridMate::ReplicaChunkClassId(NetBindingSystemContextData::GetChunkName())))
            {
                GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<AzFramework::NetBindingSystemContextData>();
            }
        }
    }
} // namespace AzFramework
