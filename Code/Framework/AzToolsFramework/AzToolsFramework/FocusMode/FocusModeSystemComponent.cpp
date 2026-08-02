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

    //! A null id addresses the active world; the editor context id addresses world 0 itself.
    void FocusModeSystemComponent::Init()
    {
    }

    void FocusModeSystemComponent::Activate()
    {
        AZ::Interface<FocusModeInterface>::Register(this);
        EditorEntityContextNotificationBus::Handler::BusConnect();
        EditorEntityInfoNotificationBus::Handler::BusConnect();
        Prefab::PrefabPublicNotificationBus::Handler::BusConnect();
    }

    void FocusModeSystemComponent::Deactivate()
    {
        Prefab::PrefabPublicNotificationBus::Handler::BusDisconnect();
        EditorEntityInfoNotificationBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
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
        const AzFramework::EntityContextId worldId =
            entityId.IsValid() ? GetEntityWorldId(entityId) : GetActiveWorldId();
        SetFocusRootForWorld(worldId, entityId);
    }

    void FocusModeSystemComponent::SetFocusRootForWorld(const AzFramework::EntityContextId& worldId, AZ::EntityId entityId)
    {
        WorldFocus& focus = m_worldFocus[worldId];
        const AZ::EntityId previousFocusEntityId = focus.m_focusRoot;
        focus.m_focusRoot = entityId;

        RefreshFocusedEntityIdList(focus);

        // Only trigger notifications if the focus root has changed.
        if (focus.m_focusRoot != previousFocusEntityId)
        {
            if (auto tracker = AZ::Interface<ViewportEditorModeTrackerInterface>::Get())
            {
                if (!previousFocusEntityId.IsValid() && focus.m_focusRoot.IsValid())
                {
                    tracker->ActivateMode({ worldId }, ViewportEditorMode::Focus);
                }
                else if (previousFocusEntityId.IsValid() && !focus.m_focusRoot.IsValid())
                {
                    tracker->DeactivateMode({ worldId }, ViewportEditorMode::Focus);
                }
            }

            FocusModeNotificationBus::Broadcast(
                &FocusModeNotifications::OnEditorFocusChanged, previousFocusEntityId, focus.m_focusRoot);
        }
    }

    void FocusModeSystemComponent::ClearFocusRoot(AzFramework::EntityContextId entityContextId)
    {
        SetFocusRootForWorld(ResolveWorldId(entityContextId), AZ::EntityId());
    }

    AZ::EntityId FocusModeSystemComponent::GetFocusRoot(AzFramework::EntityContextId entityContextId)
    {
        auto focusIt = m_worldFocus.find(ResolveWorldId(entityContextId));
        return focusIt != m_worldFocus.end() ? focusIt->second.m_focusRoot : AZ::EntityId();
    }

    const EntityIdList& FocusModeSystemComponent::GetFocusedEntities(AzFramework::EntityContextId entityContextId)
    {
        WorldFocus& focus = m_worldFocus[ResolveWorldId(entityContextId)];
        if (focus.m_focusedEntityIdList.empty())
        {
            RefreshFocusedEntityIdList(focus);
        }
        return focus.m_focusedEntityIdList;
    }

    bool FocusModeSystemComponent::IsInFocusSubTree(AZ::EntityId entityId) const
    {
        auto focusIt = m_worldFocus.find(GetEntityWorldId(entityId));
        const AZ::EntityId focusRoot = focusIt != m_worldFocus.end() ? focusIt->second.m_focusRoot : AZ::EntityId();

        // If the focus is on the root, all entities are in the focus subtree.
        return focusRoot.IsValid() ? AzToolsFramework::IsInFocusSubTree(entityId, focusRoot) : true;
    }

    bool FocusModeSystemComponent::IsFocusRoot(AZ::EntityId entityId) const
    {
        auto focusIt = m_worldFocus.find(GetEntityWorldId(entityId));
        const AZ::EntityId focusRoot = focusIt != m_worldFocus.end() ? focusIt->second.m_focusRoot : AZ::EntityId();

        if (focusRoot.IsValid())
        {
            return (entityId == focusRoot);
        }
        else
        {
            AZ::EntityId parentId;
            EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);
            return !parentId.IsValid();
        }
    }

    void FocusModeSystemComponent::OnWorldDestroyed(const AzFramework::EntityContextId& worldId)
    {
        m_worldFocus.erase(worldId);
    }

    void FocusModeSystemComponent::OnEntityInfoUpdatedAddChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
    {
        auto focusIt = m_worldFocus.find(GetEntityWorldId(childId));
        if (focusIt == m_worldFocus.end())
        {
            return;
        }
        EntityIdList& focusedEntityIdList = focusIt->second.m_focusedEntityIdList;

        // If the parent's entityId is in the list and the child isn't, add the child to the list.
        bool isParentInList = false;
        bool isChildInList = false;

        for (auto iter = focusedEntityIdList.begin(); iter != focusedEntityIdList.end(); ++iter)
        {
            if (*iter == parentId)
            {
                isParentInList = true;
            }

            if (*iter == childId)
            {
                isChildInList = true;
            }

            // Early out
            if (isChildInList)
            {
                break;
            }
        }

        if (isParentInList && !isChildInList)
        {
            focusedEntityIdList.push_back(childId);
        }
    }

    void FocusModeSystemComponent::OnEntityInfoUpdatedRemoveChildEnd([[maybe_unused]] AZ::EntityId parentId, AZ::EntityId childId)
    {
        // The removed entity's world may already be gone, so sweep every world's list.
        for (auto& [worldId, focus] : m_worldFocus)
        {
            if (auto iter = AZStd::find(focus.m_focusedEntityIdList.begin(), focus.m_focusedEntityIdList.end(), childId);
                iter != focus.m_focusedEntityIdList.end())
            {
                // Swap and pop since we don't care about the ordering.
                *iter = focus.m_focusedEntityIdList.back();
                focus.m_focusedEntityIdList.pop_back();
            }
        }
    }

    void FocusModeSystemComponent::OnPrefabInstancePropagationEnd()
    {
        // Can't rely on any of the entities in the lists to still exist, refresh everything.
        for (auto& [worldId, focus] : m_worldFocus)
        {
            RefreshFocusedEntityIdList(focus);
        }
    }

    void FocusModeSystemComponent::RefreshFocusedEntityIdList(WorldFocus& focus)
    {
        focus.m_focusedEntityIdList.clear();

        AZStd::queue<AZ::EntityId> entityIdQueue;
        entityIdQueue.push(focus.m_focusRoot);

        while (!entityIdQueue.empty())
        {
            AZ::EntityId entityId = entityIdQueue.front();
            entityIdQueue.pop();

            if (entityId.IsValid())
            {
                focus.m_focusedEntityIdList.push_back(entityId);
            }

            EntityIdList children;
            EditorEntityInfoRequestBus::EventResult(children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);

            for (AZ::EntityId childEntityId : children)
            {
                entityIdQueue.push(childEntityId);
            }
        }
    }

} // namespace AzToolsFramework
