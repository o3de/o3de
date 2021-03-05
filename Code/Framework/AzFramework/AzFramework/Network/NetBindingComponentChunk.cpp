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

#include <AzFramework/Network/NetBindingComponentChunk.h>
#include <AzFramework/Network/NetBindingComponent.h>
#include <AzFramework/Network/NetBindingSystemBus.h>
#include <AzFramework/Network/NetBindingEventsBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Slice/SliceEntityBus.h>
#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/UuidMarshal.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/IO/ByteContainerStream.h>

namespace AzFramework
{
    NetBindingComponentChunk::SpawnInfo::SpawnInfo()
        : m_runtimeEntityId(AZ::EntityId::InvalidEntityId)
        , m_owningContextId(UnspecifiedNetBindingContextSequence)
        , m_staticEntityId(AZ::EntityId::InvalidEntityId)
        , m_sliceInstanceId(UnspecifiedSliceInstanceId)
        , m_sliceAssetId(UnspecifiedSliceInstanceId, 0)
    {
    }

    bool NetBindingComponentChunk::SpawnInfo::operator==(const SpawnInfo& rhs)
    {
        return m_owningContextId == rhs.m_owningContextId
            && m_runtimeEntityId == rhs.m_runtimeEntityId
            && m_staticEntityId == rhs.m_staticEntityId
            && m_serializedState == rhs.m_serializedState
            && m_sliceAssetId == rhs.m_sliceAssetId;
    }

    bool NetBindingComponentChunk::SpawnInfo::ContainsSerializedState() const
    {
        return !m_serializedState.empty();
    }

    void NetBindingComponentChunk::SpawnInfo::Marshaler::Marshal(GridMate::WriteBuffer& wb, const SpawnInfo& data)
    {
        wb.Write(data.m_owningContextId, GridMate::VlqU32Marshaler());
        wb.Write(data.m_runtimeEntityId);

        bool useSerializedState = data.ContainsSerializedState();
        wb.Write(useSerializedState);
        if (useSerializedState)
        {
            wb.Write(data.m_serializedState);
        }
        else
        {
            wb.Write(data.m_sliceAssetId);
            wb.Write(data.m_staticEntityId);
            wb.Write(data.m_sliceInstanceId);
        }
    }

    void NetBindingComponentChunk::SpawnInfo::Marshaler::Unmarshal(SpawnInfo& data, GridMate::ReadBuffer& rb)
    {
        rb.Read(data.m_owningContextId, GridMate::VlqU32Marshaler());
        rb.Read(data.m_runtimeEntityId);

        bool hasSerializedState = false;
        rb.Read(hasSerializedState);
        if (hasSerializedState)
        {
            rb.Read(data.m_serializedState);
        }
        else
        {
            rb.Read(data.m_sliceAssetId);
            rb.Read(data.m_staticEntityId);
            rb.Read(data.m_sliceInstanceId);
        }
    }

    NetBindingComponentChunk::NetBindingComponentChunk()
        : m_bindingComponent(nullptr)
        , m_spawnInfo("SpawnInfo")
        , m_bindMap("ComponentBindMap")
    {
        m_spawnInfo.SetMaxIdleTime(0.f);
        m_bindMap.SetMaxIdleTime(0.f);
    }

    void NetBindingComponentChunk::OnReplicaActivate(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
        if (IsMaster())
        {
            // Get and store entity spawn data
            AZ_Assert(m_bindingComponent, "Entity binding is invalid!");

            m_spawnInfo.Modify([&](SpawnInfo& spawnInfo)
                {
                    spawnInfo.m_runtimeEntityId = static_cast<AZ::u64>(m_bindingComponent->GetEntity()->GetId());

                    bool isProceduralEntity = true;
                    AZ::SliceComponent::SliceInstanceAddress sliceInfo;

                    EntityContextId contextId = EntityContextId::CreateNull();
                    const AZ::EntityId bindingComponentEntityId = m_bindingComponent->GetEntityId();
                    EntityIdContextQueryBus::EventResult(contextId, bindingComponentEntityId,
                        &EntityIdContextQueryBus::Events::GetOwningContextId);
                    if (!contextId.IsNull())
                    {
                        EBUS_EVENT_RESULT(spawnInfo.m_owningContextId, NetBindingSystemBus, GetCurrentContextSequence);
                        SliceEntityRequestBus::EventResult(sliceInfo, bindingComponentEntityId,
                            &SliceEntityRequestBus::Events::GetOwningSlice);
                        bool isDynamicSliceEntity = sliceInfo.IsValid();

                        isProceduralEntity = !m_bindingComponent->IsLevelSliceEntity() && !isDynamicSliceEntity;
                    }

                    if (isProceduralEntity)
                    {
                        // write cloning info
                        AZ::SerializeContext* sc = nullptr;
                        EBUS_EVENT_RESULT(sc, AZ::ComponentApplicationBus, GetSerializeContext);
                        AZ_Assert(sc, "Can't find SerializeContext!");
                        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> spawnDataStream(&spawnInfo.m_serializedState);
                        AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&spawnDataStream, *sc, AZ::DataStream::ST_BINARY);
                        objStream->WriteClass(m_bindingComponent->GetEntity());
                        objStream->Finalize();
                    }
                    else
                    {
                        // write slice info
                        if (sliceInfo.IsValid())
                        {
                            AZ::Data::AssetId sliceAssetId = sliceInfo.GetReference()->GetSliceAsset().GetId();
                            spawnInfo.m_sliceAssetId = AZStd::make_pair(sliceAssetId.m_guid, sliceAssetId.m_subId);
                        }
                        if (sliceInfo.GetInstance())
                        {
                            spawnInfo.m_sliceInstanceId = sliceInfo.GetInstance()->GetId();
                        }

                        AZ::EntityId staticEntityId;
                        EBUS_EVENT_RESULT(staticEntityId, NetBindingSystemBus, GetStaticIdFromEntityId, m_bindingComponent->GetEntity()->GetId());
                        spawnInfo.m_staticEntityId = static_cast<AZ::u64>(staticEntityId);
                    }

                    return true;
                });
        }
        else
        {
            AZ::EntityId runtimeEntityId(m_spawnInfo.Get().m_runtimeEntityId);
            NetBindingContextSequence owningContextId = m_spawnInfo.Get().m_owningContextId;

            //TODO Move to Filter Hook
            //      Reject and cancel sessions with duplicate MachineIds?
            //      Reject and cancel sessions with duplicate entity ID creation requests?
            //Check MachineId collision
            bool collision = AZ::Entity::GetProcessSignature() == (m_spawnInfo.Get().m_runtimeEntityId & 0xFFFFFFFF);
            AZ_Error("GridMate", !collision, "Replica received with duplicate Entity Machine IDs. Ignoring");

            if (!collision)
            {
                //Check EntityID collision
                AZ::Entity* entity = nullptr;
                EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, runtimeEntityId);

                /*
                 * Only false if no machine ID collision and no entity ID collision
                 * And the entity is already active, it's possible the entity already exists in deactivated state as a cache mechanism
                 */
                collision = (entity != nullptr) && (entity->GetState() == AZ::Entity::State::Active);
            }

            /**
             * Special case - static entities should not count as duplicates.
             * Static entities are loaded with the level and will be bounded here.
             */
            if (collision)
            {
                AZ::EntityId staticEntityId;
                EBUS_EVENT_RESULT(staticEntityId, NetBindingSystemBus, GetStaticIdFromEntityId, runtimeEntityId);
                if (staticEntityId == runtimeEntityId)
                {
                    collision = false;
                }
            }

            if (!collision)    //Ignore duplicate runtime entity IDs
            {
                if (m_spawnInfo.Get().ContainsSerializedState())
                {
                    // Spawn the entity from stream input data
                    AZ::IO::MemoryStream spawnData(m_spawnInfo.Get().m_serializedState.data(), m_spawnInfo.Get().m_serializedState.size());
                    EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromStream, spawnData, runtimeEntityId, GetReplicaId(), owningContextId);
                }
                else
                {
                    NetBindingSliceContext spawnContext;
                    spawnContext.m_contextSequence = owningContextId;
                    spawnContext.m_sliceAssetId = AZ::Data::AssetId(m_spawnInfo.Get().m_sliceAssetId.first, m_spawnInfo.Get().m_sliceAssetId.second);
                    spawnContext.m_runtimeEntityId = runtimeEntityId;
                    spawnContext.m_staticEntityId = AZ::EntityId(m_spawnInfo.Get().m_staticEntityId);
                    spawnContext.m_sliceInstanceId = m_spawnInfo.Get().m_sliceInstanceId;
                    EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, GetReplicaId(), spawnContext);
                }
            }
            else    //Fail early to prevent unnecessary spawning of duplicate entity IDs
            {
                //Misconfiguration or potential cheating/DoS?
                AZ_Warning("NetBinding", false, "Received duplicate Entity ID %llu. Ignoring.", runtimeEntityId);
            }
        }
    }

    void NetBindingComponentChunk::OnReplicaDeactivate(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
        if (m_bindingComponent)
        {
            m_bindingComponent->UnbindFromNetwork();
        }
    }

    bool NetBindingComponentChunk::AcceptChangeOwnership(GridMate::PeerId requestor, const GridMate::ReplicaContext& rc)
    {
        bool result = true;

        if (m_bindingComponent)
        {
            EBUS_EVENT_ID_RESULT(result, m_bindingComponent->GetEntityId(), NetBindingEventsBus, OnEntityAcceptChangeOwnership, requestor, rc);
        }

        return result;
    }

    void NetBindingComponentChunk::OnReplicaChangeOwnership(const GridMate::ReplicaContext& rc)
    {
        if (m_bindingComponent)
        {
            EBUS_EVENT_ID(m_bindingComponent->GetEntityId(), NetBindingEventsBus, OnEntityChangeOwnership, rc);
        }
    }
}   // namespace AzFramework
