/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxLayerUtil.h"

#include "Components/EditorWhiteBoxColliderComponent.h"
#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxComponentModeBus.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>

namespace WhiteBox
{
    //! The top-most consecutive White Box ancestor; all layers parent to it to keep the hierarchy flat.
    static AZ::EntityId FindRootWhiteBoxEntity(const AZ::EntityId entityId)
    {
        AZ::EntityId root = entityId;
        while (true)
        {
            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(parentId, root, &AZ::TransformBus::Events::GetParentId);
            if (!parentId.IsValid())
            {
                break;
            }
            AZ::Entity* parentEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                parentEntity, &AZ::ComponentApplicationRequests::FindEntity, parentId);
            if (parentEntity == nullptr || parentEntity->FindComponent<EditorWhiteBoxComponent>() == nullptr)
            {
                break; // the parent is not (part of) the White Box, so this is the root
            }
            root = parentId;
        }
        return root;
    }

    //! The current sub-mode of a White Box entity's component mode (Default if it is not being edited).
    static SubMode CurrentSubModeOf(const AZ::EntityId entityId)
    {
        SubMode subMode = SubMode::Default;
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        if (entity != nullptr)
        {
            if (auto* whiteBox = entity->FindComponent<EditorWhiteBoxComponent>())
            {
                EditorWhiteBoxComponentModeRequestBus::EventResult(
                    subMode, AZ::EntityComponentIdPair(entityId, whiteBox->GetId()),
                    &EditorWhiteBoxComponentModeRequests::GetCurrentSubMode);
            }
        }
        return subMode;
    }

    AZ::EntityId CreateChildWhiteBoxLayer(const AZ::EntityId currentEntityId)
    {
        // Capture the sub-mode of the entity currently being edited so the new layer opens in the
        // same mode (e.g. if you were in Draw Shape mode the new layer starts in Draw Shape too).
        const SubMode sourceSubMode = CurrentSubModeOf(currentEntityId);

        // Keep the hierarchy flat: every layer is a direct child of the ROOT White Box, no matter
        // which layer was active when "New Layer" was pressed.
        const AZ::EntityId rootId = FindRootWhiteBoxEntity(currentEntityId);

        // Leave edit mode before creating, so the viewport switcher and inspector see the new entity outside component mode.
        namespace Cmf = AzToolsFramework::ComponentModeFramework;
        Cmf::ComponentModeSystemRequestBus::Broadcast(&Cmf::ComponentModeSystemRequests::EndComponentMode);

        // Group the whole operation (create entity + add component) into one undo step.
        AzToolsFramework::ScopedUndoBatch undoBatch("Create White Box Layer");

        // 1. Create a child entity parented to the root. The child's identity local transform means
        //    it shares the parent's world origin so any geometry drawn/stamped into it lines up.
        AZ::EntityId childId;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            childId, &AzToolsFramework::ToolsApplicationRequests::CreateNewEntity, rootId);
        if (!childId.IsValid())
        {
            AZ_Warning("WhiteBox", false, "Create White Box Layer: failed to create the child entity.");
            return AZ::EntityId();
        }

        // 2. Give the layer a friendly, descriptive name (set on the entity directly).
        {
            AZ::Entity* childEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                childEntity, &AZ::ComponentApplicationRequests::FindEntity, childId);
            if (childEntity != nullptr)
            {
                childEntity->SetName("White Box Layer");
            }
        }

        // 3. Add an Editor White Box component to the child.
        AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome addOutcome =
            AZ::Failure(AZStd::string("uninitialized"));
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            addOutcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities,
            AzToolsFramework::EntityIdList{ childId },
            AZ::ComponentTypeList{ azrtti_typeid<EditorWhiteBoxComponent>() });
        if (!addOutcome.IsSuccess())
        {
            AZ_Warning(
                "WhiteBox", false, "Create White Box Layer: failed to add the White Box component (%s).",
                addOutcome.GetError().c_str());
        }

        // 3b. If the root White Box has a collider, give the new layer one too so it collides the
        //     same way (child colliders are independent, driven by each layer's own mesh).
        {
            AZ::Entity* rootEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                rootEntity, &AZ::ComponentApplicationRequests::FindEntity, rootId);
            if (rootEntity != nullptr && rootEntity->FindComponent<EditorWhiteBoxColliderComponent>() != nullptr)
            {
                AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome colliderOutcome =
                    AZ::Failure(AZStd::string("uninitialized"));
                AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
                    colliderOutcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities,
                    AzToolsFramework::EntityIdList{ childId },
                    AZ::ComponentTypeList{ azrtti_typeid<EditorWhiteBoxColliderComponent>() });
            }
        }

        undoBatch.MarkEntityDirty(childId);

        // 4. Select the child and enter its edit mode next tick, once it has fully activated.
        AZ::TickBus::QueueFunction(
            [childId, sourceSubMode]()
            {
                // Select the child so its component-mode delegate is active.
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
                    AzToolsFramework::EntityIdList{ childId });

                // Enter the child's White Box edit mode via its component's delegate (which
                // issues ComponentModeSystemRequests::BeginComponentMode with the right builders).
                AZ::Entity* childEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(
                    childEntity, &AZ::ComponentApplicationRequests::FindEntity, childId);
                if (childEntity != nullptr)
                {
                    if (auto* whiteBox = childEntity->FindComponent<EditorWhiteBoxComponent>())
                    {
                        whiteBox->EnterComponentMode();

                        // Mirror the source entity's sub-mode onto the new layer.
                        EditorWhiteBoxComponentModeRequestBus::Event(
                            AZ::EntityComponentIdPair(childId, whiteBox->GetId()),
                            &EditorWhiteBoxComponentModeRequests::SetSubMode, sourceSubMode);
                    }
                }
            });

        return childId;
    }
} // namespace WhiteBox
