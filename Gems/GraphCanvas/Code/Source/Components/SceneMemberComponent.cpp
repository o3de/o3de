
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>

#include <Components/SceneMemberComponent.h>
#include <Components/PersistentIdComponent.h>

namespace GraphCanvas
{
    /////////////////////////
    // SceneMemberComponent
    /////////////////////////

    void SceneMemberComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<SceneMemberComponent, AZ::Component>()
                ->Version(1)
                ->Field("IsGroupable", &SceneMemberComponent::m_isGroupable)
            ;
        }
    }


    SceneMemberComponent::SceneMemberComponent()
        : m_isGroupable(false)
    {
    }

    SceneMemberComponent::SceneMemberComponent(bool isGroupable)
        : m_isGroupable(isGroupable)
    {
    }

    void SceneMemberComponent::Init()
    {
        AZ::EntityBus::Handler::BusConnect(GetEntityId());
    }

    void SceneMemberComponent::Activate()
    {
        SceneMemberRequestBus::Handler::BusConnect(GetEntityId());

        if (m_isGroupable)
        {
            GroupableSceneMemberRequestBus::Handler::BusConnect(GetEntityId());
        }
    }

    void SceneMemberComponent::Deactivate()
    {
        GroupableSceneMemberRequestBus::Handler::BusDisconnect();
        SceneMemberRequestBus::Handler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();
    }

    void SceneMemberComponent::SetScene(const AZ::EntityId& sceneId)
    {
        if (m_sceneId != sceneId)
        {
            AZ_Warning("Graph Canvas", !m_sceneId.IsValid(), "Trying to change a SceneMember's scene without removing it from the previous scene.");
            if (m_sceneId.IsValid())
            {
                ClearScene(m_sceneId);
            }

            m_sceneId = sceneId;

            SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneSet, sceneId);
        }
    }

    void SceneMemberComponent::ClearScene(const AZ::EntityId& sceneId)
    {
        AZ_Warning("Graph Canvas", m_sceneId == sceneId, "Trying to remove a SceneMember from a scene it is not a part of.");

        if (m_sceneId == sceneId)
        {
            SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnRemovedFromScene, sceneId);
            m_sceneId.SetInvalid();
        }
    }

    void SceneMemberComponent::SignalMemberSetupComplete()
    {
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnMemberSetupComplete);
    }

    AZ::EntityId SceneMemberComponent::GetScene() const
    {
        return m_sceneId;
    }

    void SceneMemberComponent::OnEntityExists(const AZ::EntityId& /*entityId*/)
    {
        AZ::EntityBus::Handler::BusDisconnect();

        // Temporary version conversion added in 1.xx to add a PersistentId onto the SceneMembers.
        // Remove after a few revisions with warnings about resaving graphs.
        if (AZ::EntityUtils::FindFirstDerivedComponent<PersistentIdComponent>(GetEntityId()) == nullptr)
        {
            AZ::Entity* selfEntity = GetEntity();
            if (selfEntity)
            {
                selfEntity->CreateComponent<PersistentIdComponent>();
            }
        }
    }

    bool SceneMemberComponent::IsGrouped() const
    {
        return m_groupId.IsValid();
    }

    const AZ::EntityId& SceneMemberComponent::GetGroupId() const
    {
        return m_groupId;
    }

    void SceneMemberComponent::RegisterToGroup(const AZ::EntityId& groupId)
    {
        if (m_groupId.IsValid())
        {
            AZ_Assert(false, "Trying to register an element to two groups at the same time.");
        }
        else
        {
            m_groupId = groupId;
            GroupableSceneMemberNotificationBus::Event(GetEntityId(), &GroupableSceneMemberNotifications::OnGroupChanged);
        }
    }

    void SceneMemberComponent::UnregisterFromGroup(const AZ::EntityId& groupId)
    {
        if (m_groupId == groupId)
        {
            m_groupId.SetInvalid();
            GroupableSceneMemberNotificationBus::Event(GetEntityId(), &GroupableSceneMemberNotifications::OnGroupChanged);
        }
    }

    void SceneMemberComponent::RemoveFromGroup()
    {
        if (m_groupId.IsValid())
        {
            NodeGroupRequestBus::Event(m_groupId, &NodeGroupRequests::RemoveElementFromGroup, GetEntityId());
        }
    }
}
