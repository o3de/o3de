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
#include <AzFramework/Network/InterestManagerComponent.h>

#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <GridMate/GridMate.h>
#include <GridMate/Replica/Interest/BitmaskInterestHandler.h>
#include <GridMate/Replica/Interest/InterestManager.h>
#include <GridMate/Replica/Interest/ProximityInterestHandler.h>

using namespace GridMate;

namespace AzFramework
{
    void InterestManagerComponent::Reflect(AZ::ReflectContext* context)
    {
        if (context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<InterestManagerComponent, AZ::Component>()
                    ->Version(1);

                AZ::EditContext* editContext = serializeContext->GetEditContext();

                if (editContext)
                {
                    editContext->Class<InterestManagerComponent>(
                        "InterestManagerComponent", "Interest manager instance")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b));
                }
            }

            // We need to register the chunk types for each handler here at reflect time
            if (!GridMate::ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(GridMate::ReplicaChunkClassId(ProximityInterestChunk::GetChunkName())))
            {
                GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<GridMate::ProximityInterestChunk>();
            }

            if (!GridMate::ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(GridMate::ReplicaChunkClassId(BitmaskInterestChunk::GetChunkName())))
            {
                GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<GridMate::BitmaskInterestChunk>();
            }
        }
    }

    void InterestManagerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("InterestManager", 0x79993873));
    }

    void InterestManagerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("InterestManager", 0x79993873));
    }



    InterestManagerComponent::InterestManagerComponent()
        : m_im(nullptr)
        , m_bitmaskHandler(nullptr)
        , m_proximityHandler(nullptr)
        , m_session(nullptr)
    {

    }

    void InterestManagerComponent::Activate()
    {
        InterestManagerRequestsBus::Handler::BusConnect();
        NetBindingSystemEventsBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void InterestManagerComponent::Deactivate()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        NetBindingSystemEventsBus::Handler::BusDisconnect();
        InterestManagerRequestsBus::Handler::BusDisconnect();

        ShutdownInterestManager();
    }

    void InterestManagerComponent::OnSystemTick()
    {
        if (m_im && m_im->IsReady())
        {
            m_im->Update();
        }
    }

    InterestManager* InterestManagerComponent::GetInterestManager()
    {
        return m_im.get();
    }

    BitmaskInterestHandler* InterestManagerComponent::GetBitmaskInterest()
    {
        return m_bitmaskHandler.get();
    }

    ProximityInterestHandler* InterestManagerComponent::GetProximityInterest()
    {
        return m_proximityHandler.get();
    }

    void InterestManagerComponent::OnNetworkSessionActivated(GridSession* session)
    {
        AZ_Assert(m_session == nullptr, "Already bound to the session");

        AZ_TracePrintf("AzFramework", "Interest manager hooked up to the session '%s'\n", session->GetId().c_str());

        m_session = session;
        m_session->GetReplicaMgr()->SetAutoBroadcast(false);

        InitInterestManager();
    }

    void InterestManagerComponent::OnNetworkSessionDeactivated(GridSession* session)
    {
        if (m_session && m_session == session)
        {
            AZ_TracePrintf("AzFramework", "Interest manager disconnected from the session '%s'\n", session ? session->GetId().c_str() : "nullptr");

            if (m_session->GetReplicaMgr())
            {
                m_session->GetReplicaMgr()->SetAutoBroadcast(true);
            }

            m_session = nullptr;
            ShutdownInterestManager();
        }
        else
        {
            AZ_Warning("AzFramework", false, "Interest manager was never active for session '%s'\n", session ? session->GetId().c_str() : "nullptr");
        }
    }

    void InterestManagerComponent::InitInterestManager()
    {
        AZ_Assert(m_im == nullptr, "Already initialized interest manager");
        m_im = AZStd::make_unique<InterestManager>();

        InterestManagerDesc desc;
        desc.m_rm = m_session->GetReplicaMgr();
        m_im->Init(desc);

        m_bitmaskHandler = AZStd::make_unique<BitmaskInterestHandler>();
        m_im->RegisterHandler(m_bitmaskHandler.get());

        m_proximityHandler = AZStd::make_unique<ProximityInterestHandler>();
        m_im->RegisterHandler(m_proximityHandler.get());

        InterestManagerEventsBus::Broadcast(
            &InterestManagerEventsBus::Events::OnInterestManagerActivate, m_im.get());
    }

    void InterestManagerComponent::ShutdownInterestManager()
    {
        if (m_im)
        {
            InterestManagerEventsBus::Broadcast(
                &InterestManagerEventsBus::Events::OnInterestManagerDeactivate, m_im.get());

            m_im->UnregisterHandler(m_bitmaskHandler.get());
            m_im->UnregisterHandler(m_proximityHandler.get());

            m_bitmaskHandler = nullptr;
            m_proximityHandler = nullptr;
            m_im = nullptr;
        }
    }
} // namespace AzFramework
