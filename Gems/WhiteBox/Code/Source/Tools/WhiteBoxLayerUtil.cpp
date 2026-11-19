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
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>

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

    static AZ::Entity* FindEditorEntity(const AZ::EntityId entityId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        return entity;
    }

    //! Undoably add a heap-allocated component to @p entityId; frees it on failure.
    static bool AddExistingComponent(const AZ::EntityId entityId, AZ::Component* component, AZStd::string& error)
    {
        AZ::Component* const components[] = { component };
        AzToolsFramework::EntityCompositionRequests::AddExistingComponentsOutcome outcome =
            AZ::Failure(AZStd::string("no entity composition handler"));
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            outcome, &AzToolsFramework::EntityCompositionRequests::AddExistingComponentsToEntityById, entityId,
            AZStd::span<AZ::Component* const>(components));
        if (!outcome.IsSuccess())
        {
            error = outcome.GetError();
            if (component->GetEntity() == nullptr)
            {
                delete component;
            }
            return false;
        }
        return true;
    }

    AZ::EntityId ExportWhiteBoxLayerToEntity(
        const AZ::EntityId sourceEntityId, const int layerIndex, const bool keepInSource, AZStd::string& error)
    {
        AZ::Entity* sourceEntity = FindEditorEntity(sourceEntityId);
        EditorWhiteBoxComponent* source =
            sourceEntity != nullptr ? sourceEntity->FindComponent<EditorWhiteBoxComponent>() : nullptr;
        if (source == nullptr)
        {
            error = "The entity has no White Box component.";
            return AZ::EntityId();
        }
        if (source->AssetInUse())
        {
            error = "This mesh lives in a shared White Box asset, which has no layers.";
            return AZ::EntityId();
        }
        if (layerIndex < 0 || layerIndex >= source->GetLayerCount())
        {
            error = "Select a layer to export.";
            return AZ::EntityId();
        }

        // Entity reactivation and layer removal must not happen under component mode.
        namespace Cmf = AzToolsFramework::ComponentModeFramework;
        Cmf::ComponentModeSystemRequestBus::Broadcast(&Cmf::ComponentModeSystemRequests::EndComponentMode);

        AzToolsFramework::ScopedUndoBatch undoBatch("Export White Box Layer To Entity");

        // Removal below goes through the active layer.
        if (source->GetActiveLayerIndex() != layerIndex)
        {
            source->SetActiveLayer(layerIndex);
        }
        const AZStd::string layerName = source->GetLayerMeta(layerIndex).m_name;

        EditorWhiteBoxComponent* whiteBoxCopy = source->CloneWithSingleLayer(layerIndex);
        if (whiteBoxCopy == nullptr)
        {
            error = "Could not copy the layer.";
            return AZ::EntityId();
        }

        // Same parent and local transform, so the geometry stays in place.
        AZ::EntityId parentId;
        AZ::TransformBus::EventResult(parentId, sourceEntityId, &AZ::TransformBus::Events::GetParentId);
        AZ::Transform localTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(localTM, sourceEntityId, &AZ::TransformBus::Events::GetLocalTM);

        AZ::EntityId newEntityId;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            newEntityId, &AzToolsFramework::ToolsApplicationRequests::CreateNewEntity, parentId);
        AZ::Entity* newEntity = FindEditorEntity(newEntityId);
        if (newEntity == nullptr)
        {
            delete whiteBoxCopy;
            error = "Could not create the entity.";
            return AZ::EntityId();
        }
        newEntity->SetName(layerName.empty() ? AZStd::string("White Box Layer") : layerName);
        AZ::TransformBus::Event(newEntityId, &AZ::TransformBus::Events::SetLocalTM, localTM);

        // Non-uniform scale is a separate component; copy it or the shape changes.
        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, sourceEntityId, &AZ::NonUniformScaleRequests::GetScale);
        if (!nonUniformScale.IsClose(AZ::Vector3::CreateOne()))
        {
            AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome scaleOutcome =
                AZ::Failure(AZStd::string("uninitialized"));
            AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
                scaleOutcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities,
                AzToolsFramework::EntityIdList{ newEntityId },
                AZ::ComponentTypeList{ azrtti_typeid<AzToolsFramework::Components::EditorNonUniformScaleComponent>() });
            AZ::NonUniformScaleRequestBus::Event(newEntityId, &AZ::NonUniformScaleRequests::SetScale, nonUniformScale);
        }

        if (!AddExistingComponent(newEntityId, whiteBoxCopy, error))
        {
            error = "Could not add the White Box component: " + error;
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::DeleteEntityById, newEntityId);
            return AZ::EntityId();
        }
        whiteBoxCopy->RebuildWhiteBox();

        // Clone the collider so its settings (physics material, layer) come along.
        if (const auto* sourceCollider = sourceEntity->FindComponent<EditorWhiteBoxColliderComponent>())
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            if (serializeContext != nullptr)
            {
                if (auto* colliderCopy = serializeContext->CloneObject(sourceCollider))
                {
                    colliderCopy->SetId(AZ::InvalidComponentId);
                    AZStd::string colliderError;
                    if (!AddExistingComponent(newEntityId, colliderCopy, colliderError))
                    {
                        AZ_Warning("WhiteBox", false, "Export Layer: could not copy the White Box Collider (%s).",
                            colliderError.c_str());
                    }
                }
            }
        }

        if (!keepInSource)
        {
            source->DeleteActiveLayer();
        }
        undoBatch.MarkEntityDirty(sourceEntityId);
        undoBatch.MarkEntityDirty(newEntityId);

        // Select once the entity has settled; the pane follows the selection.
        AZ::TickBus::QueueFunction(
            [newEntityId]()
            {
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
                    AzToolsFramework::EntityIdList{ newEntityId });
            });
        return newEntityId;
    }
} // namespace WhiteBox
