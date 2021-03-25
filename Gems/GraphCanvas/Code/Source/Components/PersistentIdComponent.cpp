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
#include "precompiled.h"

#include <Components/PersistentIdComponent.h>

namespace GraphCanvas
{
    //////////////////////////
    // PersistentIdComponent
    //////////////////////////
    
    void PersistentIdComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        
        if (serializeContext)
        {
            serializeContext->Class<PersistentIdComponentSaveData, ComponentSaveData>()
                ->Version(1)
                ->Field("PersistentId", &PersistentIdComponentSaveData::m_persistentId)
            ;
            
            serializeContext->Class<PersistentIdComponent, AZ::Component>()
                ->Version(1)
                ->Field("SaveData", &PersistentIdComponent::m_saveData)
            ;
        }
    }
    
    PersistentIdComponent::PersistentIdComponent()
        : AZ::Component()
        , m_previousId(PersistentGraphMemberId::CreateNull())
    {

    }

    void PersistentIdComponent::Init()
    {
        m_previousId = PersistentGraphMemberId::CreateNull();
    }
    
    void PersistentIdComponent::Activate()
    {
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());

        m_saveData.Activate(GetEntityId());
    }
    
    void PersistentIdComponent::Deactivate()
    {
        PersistentIdRequestBus::Handler::BusDisconnect();
        PersistentMemberRequestBus::Handler::BusDisconnect();
    }
    
    void PersistentIdComponent::OnSceneSet(const AZ::EntityId& graphId)
    {
        if (!PersistentIdRequestBus::Handler::BusIsConnected())
        {
            PersistentIdRequestBus::Handler::BusConnect(m_saveData.m_persistentId);
            PersistentMemberRequestBus::Handler::BusConnect(GetEntityId());
        }
        else if (!graphId.IsValid())
        {
            PersistentIdRequestBus::Handler::BusDisconnect();
            PersistentMemberRequestBus::Handler::BusDisconnect();
        }
    }

    void PersistentIdComponent::OnSceneMemberDeserialized(const AZ::EntityId& /*graphId*/, const GraphSerialization& /*serializationTarget*/)
    {
        m_previousId = m_saveData.m_persistentId;
        m_saveData.RemapId();
    }

    AZ::EntityId PersistentIdComponent::MapToEntityId() const
    {
        return GetEntityId();
    }

    PersistentGraphMemberId PersistentIdComponent::GetPreviousGraphMemberId() const
    {
        return m_previousId;
    }

    PersistentGraphMemberId PersistentIdComponent::GetPersistentGraphMemberId() const
    {
        return m_saveData.m_persistentId;
    }
}
