/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>

#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/FocusMode/FocusModeSystemComponent.h>

namespace AzToolsFramework
{
    FocusModeSystemComponent::~FocusModeSystemComponent()
    {
        AZ::Interface<FocusModeInterface>::Unregister(this);
    }

    void FocusModeSystemComponent::Init()
    {
        AZ::Interface<FocusModeInterface>::Register(this);
    }

    void FocusModeSystemComponent::Activate()
    {
    }

    void FocusModeSystemComponent::Deactivate()
    {
    }

    void FocusModeSystemComponent::Reflect(AZ::ReflectContext* /*context*/)
    {
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
        m_focusRoot = entityId;

        // TODO - If m_focusRoot != AZ::EntityId(), activate focus mode via ViewportEditorModeTrackerInterface; else, deactivate focus mode
    }

    AZ::EntityId FocusModeSystemComponent::GetFocusRoot()
    {
        return m_focusRoot;
    }

    bool FocusModeSystemComponent::IsInFocusSubTree(AZ::EntityId entityId)
    {
        if (m_focusRoot == AZ::EntityId())
        {
            return true;
        }

        return IsInFocusSubTree_helper(entityId, m_focusRoot);
    }

    bool FocusModeSystemComponent::IsInFocusSubTree_helper(AZ::EntityId entityId, AZ::EntityId focusRootId)
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

        return IsInFocusSubTree_helper(parentId, focusRootId);
    }

} // namespace AzToolsFramework
