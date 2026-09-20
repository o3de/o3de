/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxComponent.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
// WorldFromLocalWithUniformScale lives in ManipulatorView.h, despite the name.
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace WhiteBox
{
    EditorWhiteBoxComponent* FindWhiteBoxComponent(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            entity, &AZ::ComponentApplicationRequests::FindEntity, entityComponentIdPair.GetEntityId());
        return entity != nullptr
            ? azrtti_cast<EditorWhiteBoxComponent*>(entity->FindComponent(entityComponentIdPair.GetComponentId()))
            : nullptr;
    }

    AZ::Transform EditorSpaceFromLocal(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        const AZ::Transform worldFromEntity =
            AzToolsFramework::WorldFromLocalWithUniformScale(entityComponentIdPair.GetEntityId());
        const EditorWhiteBoxComponent* component = FindWhiteBoxComponent(entityComponentIdPair);
        return component != nullptr ? worldFromEntity * component->GetActiveLayerTransform() : worldFromEntity;
    }

    AZ::Transform EditorSpaceFromLocal(const AZ::EntityId entityId)
    {
        const AZ::Transform worldFromEntity = AzToolsFramework::WorldFromLocalWithUniformScale(entityId);
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        const auto* component = entity != nullptr ? entity->FindComponent<EditorWhiteBoxComponent>() : nullptr;
        return component != nullptr ? worldFromEntity * component->GetActiveLayerTransform() : worldFromEntity;
    }

    void RequestEditSourceControl(const char* absoluteFilePath)
    {
        bool active = false;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(
            active, &AzToolsFramework::SourceControlConnectionRequestBus::Events::IsActive);

        if (active)
        {
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit, absoluteFilePath, true,
                []([[maybe_unused]] bool success, [[maybe_unused]] AzToolsFramework::SourceControlFileInfo info)
                {
                });
        }
    }

    IEditor* GetIEditor()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        return editor;
    }
} // namespace WhiteBox
