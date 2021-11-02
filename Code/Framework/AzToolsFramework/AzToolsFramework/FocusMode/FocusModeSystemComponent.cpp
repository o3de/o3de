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
    }

    void FocusModeSystemComponent::Deactivate()
    {
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

    bool FocusModeSystemComponent::IsInFocusSubTree(AZ::EntityId entityId) const
    {
        if (m_focusRoot == AZ::EntityId())
        {
            return true;
        }

        return AzToolsFramework::IsInFocusSubTree(entityId, m_focusRoot);
    }

} // namespace AzToolsFramework
