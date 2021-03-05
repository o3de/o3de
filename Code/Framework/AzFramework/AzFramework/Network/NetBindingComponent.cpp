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

#include <AzFramework/Network/NetBindingComponent.h>
#include <AzFramework/Network/NetBindable.h>
#include <AzFramework/Network/NetBindingSystemBus.h>
#include <AzFramework/Network/NetBindingComponentChunk.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>

namespace AzFramework
{
    void NetBindingComponent::Reflect(AZ::ReflectContext* reflection)
    {
        NetBindable::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<NetBindingComponent, AZ::Component>()
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<NetBindingComponent>(
                    "Network Binding", "The Network Binding component marks an entity as able to be replicated across the network")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Networking")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/NetBinding.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/NetBinding.png")
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-network-binding.html")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c));
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);

        if (behaviorContext)
        {
            behaviorContext->EBus<NetBindingHandlerBus>("NetBindingHandlerBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                ->Event("IsEntityBoundToNetwork", &NetBindingHandlerBus::Events::IsEntityBoundToNetwork)
                ->Event("IsEntityAuthoritative", &NetBindingHandlerBus::Events::IsEntityAuthoritative)

                // Desired, but currently unsupported events.
                // Seems to be an unsupported type(AZ::u16)
                //->Event("SetReplicaPriority", &NetBindingHandlerBus::Events::SetReplicaPriority)
                //->Event("GetReplicaPriority", &NetBindingHandlerBus::Events::GetReplicaPriority)
            ;
        }

        // We also need to register the chunk type, and this would be a good time to do so.
        if (!GridMate::ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(GridMate::ReplicaChunkClassId(NetBindingComponentChunk::GetChunkName())))
        {
            GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<AzFramework::NetBindingComponentChunk>();
        }
    }

    NetBindingComponent::NetBindingComponent()
        : m_isLevelSliceEntity(false)
    {
    }

    void NetBindingComponent::Activate()
    {
        NetBindingHandlerBus::Handler::BusConnect(GetEntityId());

        if (!IsEntityBoundToNetwork())
        {
            bool shouldBind = false;
            NetBindingSystemBus::BroadcastResult( shouldBind, &NetBindingSystemBus::Events::ShouldBindToNetwork);
            if (shouldBind)
            {
                BindToNetwork(nullptr);
            }
            else
            {
                /*
                 * This is the Editor path. We still need to call NetBindable::NetInit() in order
                 * to initialize NetworkContext Fields and RPCs, so that they behave as
                 * authoritative in game editor mode. Without this call RPCs callbacks won't invoke inside the Editor.
                 * For example:
                 *
                 * static void Reflect(...)
                 * {
                 *  NetworkContext->Class<MyNetworkComponent>()->RPC("my rpc", &MyNetworkComponent::m_myRpc);
                 * }
                 * ...
                 * m_myRpc(); // <--- will not invoke the callback inside the Editor unless NetInit() is called below.
                 */
                for (Component* component : GetEntity()->GetComponents())
                {
                    if (NetBindable* netBindable = azrtti_cast<NetBindable*>(component))
                    {
                        netBindable->NetInit();
                    }
                }
            }
        }
    }

    void NetBindingComponent::Deactivate()
    {
        NetBindingHandlerBus::Handler::BusDisconnect();
        if (IsEntityBoundToNetwork())
        {
            static_cast<NetBindingComponentChunk*>(m_chunk.get())->SetBinding(nullptr);
            if (m_chunk->IsMaster())
            {
                m_chunk->GetReplica()->Destroy();
            }
            m_chunk = nullptr;
        }
    }

    bool NetBindingComponent::IsEntityBoundToNetwork()
    {
        return m_chunk && m_chunk->GetReplica();
    }

    bool NetBindingComponent::IsEntityAuthoritative()
    {
        return !m_chunk || m_chunk->IsMaster();
    }

    void NetBindingComponent::BindToNetwork(GridMate::ReplicaPtr bindTo)
    {
        AZ_Assert(!IsEntityBoundToNetwork(), "We shouldn't be bound to the network if the network is just starting!");

        if (bindTo)
        {
            NetBindingComponentChunkPtr bindingChunk = bindTo->FindReplicaChunk<NetBindingComponentChunk>();
            AZ_Assert(bindingChunk, "Can't find NetBindingComponentChunk!");
            m_chunk = bindingChunk;
            bindingChunk->SetBinding(this);

            GridMate::Replica* replica = bindingChunk->GetReplica();
            size_t nChunks = replica->GetNumChunks();
            size_t nBindings = bindingChunk->m_bindMap.Get().size();
            AZ_Assert(nChunks == nBindings, "Number of chunks received is not the same as the size of the bind map!");
            nBindings = AZ::GetMin(nBindings, nChunks);
            for (size_t i = 0; i < nBindings; ++i)
            {
                AZ::ComponentId bindToId = bindingChunk->m_bindMap.Get()[i];
                if (bindToId != AZ::InvalidComponentId)
                {
                    AZ::Component* component = GetEntity()->FindComponent(bindToId);
                    NetBindable* netBindable = azrtti_cast<NetBindable*>(component);
                    AZ_Assert(netBindable, "Can't find net bindable component with id %llu to be bound to chunk type %s!", bindToId, replica->GetChunkByIndex(i)->GetDescriptor()->GetChunkName());
                    if (netBindable && netBindable->IsSyncEnabled())
                    {
                        netBindable->SetNetworkBinding(replica->GetChunkByIndex(i));
                    }
                }
            }
        }
        else
        {
            GridMate::ReplicaPtr replica = GridMate::Replica::CreateReplica(GetEntity()->GetName().c_str());
            NetBindingComponentChunk* chunk = GridMate::CreateReplicaChunk<NetBindingComponentChunk>();
            m_chunk = chunk;
            chunk->SetBinding(this);
            replica->AttachReplicaChunk(chunk);

            chunk->m_bindMap.Modify([&](AZStd::vector<AZ::ComponentId>& bindMap)
            {
                // Mark the chunks already in the replica as non-components.
                bindMap.resize(replica->GetNumChunks(), AZ::InvalidComponentId);

                // Collect the bindings and add the to the replica
                AZ::Entity* entity = GetEntity();
                for (Component* component : entity->GetComponents())
                {
                    NetBindable* netBindable = azrtti_cast<NetBindable*>(component);
                    if (netBindable && netBindable->IsSyncEnabled())
                    {
                        GridMate::ReplicaChunkPtr bindingChunk = netBindable->GetNetworkBinding();
                        if (bindingChunk)
                        {
                            bindMap.push_back(component->GetId());
                            replica->AttachReplicaChunk(bindingChunk);
                        }
                    }
                }
                return true;
            });

            // Add replica to session replica manager (may be deferred)
            NetBindingSystemBus::Broadcast( &NetBindingSystemBus::Events::AddReplicaMaster, GetEntity(), replica);
        }
    }

    void NetBindingComponent::UnbindFromNetwork()
    {
        if (m_chunk)
        {
            for (Component* component : GetEntity()->GetComponents())
            {
                NetBindable* netBindable = azrtti_cast<NetBindable*>(component);
                if (netBindable && netBindable->IsSyncEnabled())
                {
                    netBindable->UnbindFromNetwork();
                }
            }

            NetBindingComponentChunkPtr chunk = static_cast<NetBindingComponentChunk*>(m_chunk.get());
            chunk->SetBinding(nullptr);
            m_chunk = nullptr;
            if (chunk->IsProxy())
            {
                EntityContextId contextId = EntityContextId::CreateNull();
                EntityIdContextQueryBus::EventResult( contextId, GetEntityId(), &EntityIdContextQueryBus::Events::GetOwningContextId);
                if (contextId.IsNull())
                {
                    delete GetEntity();
                }
                else if (!IsLevelSliceEntity())
                {
                    NetBindingSystemBus::Broadcast( &NetBindingSystemBus::Events::UnbindGameEntity, GetEntityId(), m_sliceInstanceId);
                }
            }
        }
    }

    void NetBindingComponent::MarkAsLevelSliceEntity()
    {
        AZ_Assert(!IsEntityBoundToNetwork(), "MarkAsLevelSliceEntity() has to be called before the entity is bound to the network!");
        m_isLevelSliceEntity = true;
    }

    void NetBindingComponent::SetSliceInstanceId(const AZ::SliceComponent::SliceInstanceId& sliceInstanceId)
    {
        m_sliceInstanceId = sliceInstanceId;
    }

    void NetBindingComponent::RequestEntityChangeOwnership(GridMate::PeerId peerId)
    {
        if (m_chunk && m_chunk->GetReplica())
        {
            m_chunk->GetReplica()->RequestChangeOwnership(peerId);
        }
    }

    void NetBindingComponent::SetReplicaPriority(GridMate::ReplicaPriority replicaPriority)
    {
        if (m_chunk)
        {
            m_chunk->SetPriority(replicaPriority);
        }
    }

    GridMate::ReplicaPriority NetBindingComponent::GetReplicaPriority() const
    {
        if (m_chunk && m_chunk->GetReplica())
        {
            return m_chunk->GetReplica()->GetPriority();
        }
        else
        {
            AZ_Error("NetBindingComponent",false,"Trying to gather ReplicaPriority without having a Replica.");
            return GridMate::k_replicaPriorityLowest;
        }
    }

    bool NetBindingComponent::IsLevelSliceEntity() const
    {
        return m_isLevelSliceEntity;
    }
}   // namespace AzFramework

