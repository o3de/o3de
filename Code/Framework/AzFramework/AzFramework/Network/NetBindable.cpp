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

#include <AzFramework/Network/NetBindable.h>
#include <AzFramework/Network/NetBindingHandlerBus.h>
#include <AzFramework/Network/NetSystemBus.h>
#include <AzFramework/Network/NetworkContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AzFramework
{
    ////////////////
    // NetBindable
    ////////////////

    NetBindable::NetBindable()
        : m_isSyncEnabled(true)
    {
    }

    NetBindable::~NetBindable()
    {
        if (m_chunk)
        {
            // NetBindable is a base class for handlers for replica chunks, so we have to clear the handler since this object is about to go away
            m_chunk->SetHandler(nullptr);
            m_chunk = nullptr;
        }
    }

    GridMate::ReplicaChunkPtr NetBindable::GetNetworkBinding()
    {
        NetworkContext* netContext = nullptr;
        NetSystemRequestBus::BroadcastResult(netContext, &NetSystemRequestBus::Events::GetNetworkContext);
        AZ_Assert(netContext, "Cannot bind objects to the network with no NetworkContext");
        if (netContext)
        {
            m_chunk = netContext->CreateReplicaChunk(azrtti_typeid(this));
            netContext->Bind(this, m_chunk, NetworkContextBindMode::Authoritative);
            return m_chunk;
        }

        return nullptr;
    }

    void NetBindable::SetNetworkBinding (GridMate::ReplicaChunkPtr chunk)
    {
        m_chunk = chunk;

        NetworkContext* netContext = nullptr;
        NetSystemRequestBus::BroadcastResult(netContext, &NetSystemRequestBus::Events::GetNetworkContext);
        AZ_Assert(netContext, "Cannot bind objects to the network with no NetworkContext");
        if (netContext)
        {
            netContext->Bind(this, m_chunk, NetworkContextBindMode::NonAuthoritative);
        }
    }

    void NetBindable::UnbindFromNetwork()
    {
        if (m_chunk)
        {
            // NetworkContext-reflected chunks need access to the handler when they are being destroyed, so we won't null handler in here
            m_chunk = nullptr;
        }
    }

    void NetBindable::NetInit()
    {
        NetworkContext* netContext = nullptr;
        NetSystemRequestBus::BroadcastResult(netContext, &NetSystemRequestBus::Events::GetNetworkContext);
        AZ_Assert(netContext, "Cannot bind objects to the network with no NetworkContext");
        if (netContext)
        {
            netContext->Bind(this, nullptr, NetworkContextBindMode::NonAuthoritative);
        }
    }

    void NetBindable::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<NetBindable>()
                ->Field("m_isSyncEnabled", &NetBindable::m_isSyncEnabled);

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<NetBindable>(
                    "Network Bindable", "Network-bindable components are synchronized over the network.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Networking")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &NetBindable::m_isSyncEnabled, "Bind To network", "Enable binding to the network.");
            }
        }
    }
}
