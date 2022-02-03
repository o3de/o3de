/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>

#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/FocusMode/FocusModeNotificationBus.h>
#include <AzToolsFramework/FocusMode/FocusModeSystemComponent.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    bool IsInFocusSubTree(AZ::EntityId entityId, AZ::EntityId focusRootId)
    {
        if (entityId == AZ::EntityId())
        {
            return false;
        }

        if (entityId == focusRootId)
        {
            return true;
        }

        AZ::EntityId parentId;
        AZ::TransformBus::EventResult(parentId, entityId, &AZ::TransformInterface::GetParentId);

        return IsInFocusSubTree(parentId, focusRootId);
    }

    void FocusModeSystemComponent::Init()
    {
    }

    void FocusModeSystemComponent::Activate()
    {
        AZ::Interface<FocusModeInterface>::Register(this);
        EditorEntityInfoNotificationBus::Handler::BusConnect();
        Prefab::PrefabPublicNotificationBus::Handler::BusConnect();
    }

    void FocusModeSystemComponent::Deactivate()
    {
        Prefab::PrefabPublicNotificationBus::Handler::BusDisconnect();
        EditorEntityInfoNotificationBus::Handler::BusDisconnect();
        AZ::Interface<FocusModeInterface>::Unregister(this);
    }

    void FocusModeSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FocusModeSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void FocusModeSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("EditorFocusMode"));
    }

    void FocusModeSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void FocusModeSystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    void FocusModeSystemComponent::SetFocusRoot(AZ::EntityId entityId)
    {
        if (m_focusRoot == entityId)
        {
            return;
        }

        if (auto tracker = AZ::Interface<ViewportEditorModeTrackerInterface>::Get())
        {
            if (!m_focusRoot.IsValid() && entityId.IsValid())
            {
                tracker->ActivateMode({ GetEntityContextId() }, ViewportEditorMode::Focus);
            }
            else if (m_focusRoot.IsValid() && !entityId.IsValid())
            {
                tracker->DeactivateMode({ GetEntityContextId() }, ViewportEditorMode::Focus);
            }
        }

        AZ::EntityId previousFocusEntityId = m_focusRoot;
        m_focusRoot = entityId;

        RefreshFocusedEntityIdList();

        FocusModeNotificationBus::Broadcast(&FocusModeNotifications::OnEditorFocusChanged, previousFocusEntityId, m_focusRoot);
    }

    void FocusModeSystemComponent::ClearFocusRoot([[maybe_unused]] AzFramework::EntityContextId entityContextId)
    {
        SetFocusRoot(AZ::EntityId());
    }

    AZ::EntityId FocusModeSystemComponent::GetFocusRoot([[maybe_unused]] AzFramework::EntityContextId entityContextId)
    {
        return m_focusRoot;
    }

    EntityIdList FocusModeSystemComponent::GetFocusedEntities([[maybe_unused]] AzFramework::EntityContextId entityContextId)
    {
        return m_focusedEntityIdList;
    }

    bool FocusModeSystemComponent::IsInFocusSubTree(AZ::EntityId entityId) const
    {
        if (m_focusRoot == AZ::EntityId())
        {
            return true;
        }

        return AzToolsFramework::IsInFocusSubTree(entityId, m_focusRoot);
    }

    void FocusModeSystemComponent::OnEntityInfoUpdatedAddChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
    {
        // If the parent's entityId is in the list, add the child.
        if (auto iter = AZStd::find(m_focusedEntityIdList.begin(), m_focusedEntityIdList.end(), parentId);
            iter != m_focusedEntityIdList.end())
        {
            m_focusedEntityIdList.push_back(childId);
        }
    }

    void FocusModeSystemComponent::OnEntityInfoUpdatedRemoveChildEnd([[maybe_unused]] AZ::EntityId parentId, AZ::EntityId childId)
    {
        // If the removed entityId is in the list, remove it.
        if (auto iter = AZStd::find(m_focusedEntityIdList.begin(), m_focusedEntityIdList.end(), childId);
            iter != m_focusedEntityIdList.end())
        {
            m_focusedEntityIdList.erase(iter);
        }
    }

    void FocusModeSystemComponent::OnPrefabInstancePropagationEnd()
    {
        // Can't rely on any of the entities in the list to still exist, refresh the whole thing.
        RefreshFocusedEntityIdList();
    }

    void FocusModeSystemComponent::RefreshFocusedEntityIdList()
    {
        m_focusedEntityIdList.clear();

        AZStd::queue<AZ::EntityId> entityIdQueue;
        entityIdQueue.push(m_focusRoot);

        while (!entityIdQueue.empty())
        {
            AZ::EntityId entityId = entityIdQueue.front();
            entityIdQueue.pop();

            m_focusedEntityIdList.push_back(entityId);

            EntityIdList children;
            EditorEntityInfoRequestBus::EventResult(children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);

            for (AZ::EntityId childEntityId : children)
            {
                entityIdQueue.push(childEntityId);
            }
        }
    }

} // namespace AzToolsFramework
