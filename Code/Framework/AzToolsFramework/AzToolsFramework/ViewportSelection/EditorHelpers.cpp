/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorHelpers.h"

#include <AzCore/Console/Console.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/EditorViewportIconDisplayInterface.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponentBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>

AZ_CVAR(
    bool,
    ed_visibility_showAggregateEntitySelectionBounds,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Display the aggregate selection bounds for a given entity (the union of all component Aabbs)");
AZ_CVAR(
    bool,
    ed_visibility_showAggregateEntityTransformedLocalBounds,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Display the aggregate transformed local bounds for a given entity (the union of all local component Aabbs)");
AZ_CVAR(
    bool,
    ed_visibility_showAggregateEntityWorldBounds,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Display the aggregate world bounds for a given entity (the union of all world component Aabbs)");
AZ_CVAR(
    bool,
    ed_useCursorLockIconInFocusMode,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Use a lock icon when the cursor is over entities that cannot be interacted with");

AZ_CVAR(float, ed_iconMinScale, 0.1f, nullptr, AZ::ConsoleFunctorFlags::Null, "Minimum scale for icons in the distance");
AZ_CVAR(float, ed_iconMaxScale, 1.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum scale for icons near the camera");
AZ_CVAR(float, ed_iconCloseDist, 3.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "Distance at which icons are at maximum scale");
AZ_CVAR(float, ed_iconFarDist, 40.f, nullptr, AZ::ConsoleFunctorFlags::Null, "Distance at which icons are at minimum scale");

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorHelpers, AZ::SystemAllocator)

    static const int IconSize = 36; // icon display size (in pixels)

    // helper function to wrap EBus call to check if helpers are being displayed
    static bool HelpersVisible(const AzFramework::ViewportId viewportId)
    {
        bool helpersVisible = false;
        ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            helpersVisible, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::HelpersVisible);
        return helpersVisible;
    }

    // helper function to wrap EBus call to check if icons are being displayed
    static bool IconsVisible(const AzFramework::ViewportId viewportId)
    {
        bool iconsVisible = false;
        ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            iconsVisible, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::IconsVisible);
        return iconsVisible;
    }

    // helper function to wrap EBus call to check if helpers should only be drawn for selected entities
    static bool OnlyShowHelpersForSelectedEntities(const AzFramework::ViewportId viewportId)
    {
        bool onlyShowHelpersForSelectedEntities = false;
        ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            onlyShowHelpersForSelectedEntities, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::OnlyShowHelpersForSelectedEntities);
        return onlyShowHelpersForSelectedEntities;
    }

    float GetIconScale(const float distance)
    {
        return ed_iconMinScale +
            (ed_iconMaxScale - ed_iconMinScale) *
            (1.0f - AZ::GetClamp(AZ::GetMax(0.0f, distance - ed_iconCloseDist) / (ed_iconFarDist - ed_iconCloseDist), 0.0f, 1.0f));
    }

    float GetIconSize(const float distance)
    {
        return GetIconScale(distance) * IconSize;
    }

    static void DisplayComponents(
        const AZ::EntityId entityId, const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AzFramework::EntityDebugDisplayEventBus::Event(
            entityId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport, viewportInfo, debugDisplay);

        if (ed_visibility_showAggregateEntitySelectionBounds)
        {
            if (const AZ::Aabb aabb = AzToolsFramework::CalculateEditorEntitySelectionBounds(entityId, viewportInfo); aabb.IsValid())
            {
                debugDisplay.SetColor(AZ::Colors::Orange);
                debugDisplay.DrawWireBox(aabb.GetMin(), aabb.GetMax());
            }
        }

        if (ed_visibility_showAggregateEntityTransformedLocalBounds)
        {
            const AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityId);
            if (const AZ::Aabb localAabb = AzFramework::CalculateEntityLocalBoundsUnion(entity); localAabb.IsValid())
            {
                const AZ::Transform worldFromLocal = entity->GetTransform()->GetWorldTM();
                const AZ::Aabb worldAabb = localAabb.GetTransformedAabb(worldFromLocal);
                debugDisplay.SetColor(AZ::Colors::Turquoise);
                debugDisplay.DrawWireBox(worldAabb.GetMin(), worldAabb.GetMax());
            }
        }

        if (ed_visibility_showAggregateEntityWorldBounds)
        {
            const AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityId);
            if (const AZ::Aabb worldAabb = AzFramework::CalculateEntityWorldBoundsUnion(entity); worldAabb.IsValid())
            {
                debugDisplay.SetColor(AZ::Colors::Magenta);
                debugDisplay.DrawWireBox(worldAabb.GetMin(), worldAabb.GetMax());
            }
        }
    }

    CursorEntityIdQuery::CursorEntityIdQuery(AZ::EntityId entityId, AZ::EntityId rootEntityId)
        : m_entityId(entityId)
        , m_containerAncestorEntityId(rootEntityId)
    {
    }

    AZ::EntityId CursorEntityIdQuery::EntityIdUnderCursor() const
    {
        return m_entityId;
    }

    AZ::EntityId CursorEntityIdQuery::ContainerAncestorEntityId() const
    {
        return m_containerAncestorEntityId;
    }

    bool CursorEntityIdQuery::HasContainerAncestorEntityId() const
    {
        if (m_entityId.IsValid())
        {
            return m_entityId != m_containerAncestorEntityId;
        }

        return false;
    }

    EditorHelpers::EditorHelpers(const EditorVisibleEntityDataCacheInterface* entityDataCache)
        : m_entityDataCache(entityDataCache)
    {
        m_focusModeInterface = AZ::Interface<FocusModeInterface>::Get();
        AZ_Assert(
            m_focusModeInterface,
            "EditorHelpers - "
            "Focus Mode Interface could not be found. "
            "Check that it is being correctly initialized.");

        AZStd::vector<AZStd::unique_ptr<InvalidClick>> invalidClicks;
        invalidClicks.push_back(AZStd::make_unique<FadingText>("Not in focus"));
        invalidClicks.push_back(AZStd::make_unique<ExpandingFadingCircles>());
        m_invalidClicks = AZStd::make_unique<InvalidClicks>(AZStd::move(invalidClicks));
    }

    CursorEntityIdQuery EditorHelpers::FindEntityIdUnderCursor(
        const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const int viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;

        const bool iconsVisible = IconsVisible(viewportId);

        const AZ::Matrix3x4 cameraView = AzFramework::CameraView(cameraState);
        const AZ::Matrix4x4 cameraProjection = AzFramework::CameraProjection(cameraState);

        // selecting new entities
        AZ::EntityId entityIdUnderCursor;
        float closestDistance = AZStd::numeric_limits<float>::max();
        for (size_t entityCacheIndex = 0, visibleEntityCount = m_entityDataCache->VisibleEntityDataCount();
             entityCacheIndex < visibleEntityCount;
             ++entityCacheIndex)
        {
            const AZ::EntityId entityId = m_entityDataCache->GetVisibleEntityId(entityCacheIndex);

            if (m_entityDataCache->IsVisibleEntityLocked(entityCacheIndex) || !m_entityDataCache->IsVisibleEntityVisible(entityCacheIndex))
            {
                continue;
            }

            if (iconsVisible)
            {
                // some components choose to hide their icons (e.g. meshes)
                // we also do not want to test against icons that may not be showing as they're inside a 'closed' entity container
                // (these icons only become visible when it is opened for editing)
                if (!m_entityDataCache->IsVisibleEntityIconHidden(entityCacheIndex) &&
                    m_entityDataCache->IsVisibleEntityIndividuallySelectableInViewport(entityCacheIndex))
                {
                    const AZ::Vector3& entityPosition = m_entityDataCache->GetVisibleEntityPosition(entityCacheIndex);

                    // selecting based on 2d icon - should only do it when visible and not selected
                    const AZ::Vector3 ndcPoint = AzFramework::WorldToScreenNdc(entityPosition, cameraView, cameraProjection);
                    const AzFramework::ScreenPoint screenPosition =
                        AzFramework::ScreenPointFromNdc(AZ::Vector2(ndcPoint), cameraState.m_viewportSize);

                    const float distanceFromCamera = cameraState.m_position.GetDistance(entityPosition);
                    const auto iconRange = GetIconSize(distanceFromCamera) * 0.5f;
                    const auto screenCoords = mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates;

                    // 2d screen space selection - did we click an icon
                    if (screenCoords.m_x >= screenPosition.m_x - iconRange && screenCoords.m_x <= screenPosition.m_x + iconRange &&
                        screenCoords.m_y >= screenPosition.m_y - iconRange && screenCoords.m_y <= screenPosition.m_y + iconRange &&
                        ndcPoint.GetZ() < closestDistance)
                    {
                        // use ndc z value for distance here which is in 0-1 range so will most likely 'win' when it comes to the
                        // distance check (this is what we want as the cursor should always favor icons if they are hovered)
                        closestDistance = ndcPoint.GetZ();
                        entityIdUnderCursor = entityId;
                    }
                }
            }

            float closestBoundDifference;
            if (PickEntity(entityId, mouseInteraction.m_mouseInteraction, closestBoundDifference, viewportId))
            {
                if (closestBoundDifference < closestDistance)
                {
                    closestDistance = closestBoundDifference;
                    entityIdUnderCursor = entityId;
                }
            }
        }

        // verify if the entity Id corresponds to an entity that is focused; if not, halt selection.
        if (entityIdUnderCursor.IsValid() && !IsSelectableAccordingToFocusMode(entityIdUnderCursor))
        {
            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                    mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down ||
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::DoubleClick)
            {
                m_invalidClicks->AddInvalidClick(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);
            }

            return CursorEntityIdQuery(AZ::EntityId(), AZ::EntityId());
        }

        ViewportInteraction::ViewportMouseCursorRequestBus::Event(
            viewportId, &ViewportInteraction::ViewportMouseCursorRequestBus::Events::ClearOverrideCursor);

        // container entity support - if the entity that is being selected is part of a closed container,
        // change the selection to the container instead.
        if (ContainerEntityInterface* containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get())
        {
            const auto highestSelectableEntity = containerEntityInterface->FindHighestSelectableEntity(entityIdUnderCursor);
            return CursorEntityIdQuery(entityIdUnderCursor, highestSelectableEntity);
        }

        return CursorEntityIdQuery(entityIdUnderCursor, AZ::EntityId());
    }

    void EditorHelpers::Display2d(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        m_invalidClicks->Display2d(viewportInfo, debugDisplay);
    }

    void EditorHelpers::DisplayHelpers(
        const AzFramework::ViewportInfo& viewportInfo,
        const AzFramework::CameraState& cameraState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZStd::function<bool(AZ::EntityId)>& showIconCheck)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const bool iconsVisible = IconsVisible(viewportInfo.m_viewportId);
        const bool helpersVisible = HelpersVisible(viewportInfo.m_viewportId);
        const bool onlyDrawSelectedEntities = OnlyShowHelpersForSelectedEntities(viewportInfo.m_viewportId);

        if (helpersVisible || onlyDrawSelectedEntities)
        {
            for (size_t entityCacheIndex = 0, visibleEntityCount = m_entityDataCache->VisibleEntityDataCount();
                 entityCacheIndex < visibleEntityCount;
                 ++entityCacheIndex)
            {
                if (const AZ::EntityId entityId = m_entityDataCache->GetVisibleEntityId(entityCacheIndex);
                    m_entityDataCache->IsVisibleEntityVisible(entityCacheIndex))
                {
                    if (onlyDrawSelectedEntities && m_entityDataCache->IsVisibleEntitySelected(entityCacheIndex))
                    {
                        // notify components to display
                        DisplayComponents(entityId, viewportInfo, debugDisplay);
                    }
                    else if (!onlyDrawSelectedEntities)
                    {
                        DisplayComponents(entityId, viewportInfo, debugDisplay);
                    }
                }
            }
        }

        if (iconsVisible)
        {
            auto editorViewportIconDisplay = EditorViewportIconDisplay::Get();
            if (!editorViewportIconDisplay)
            {
                return;
            }

            for (size_t entityCacheIndex = 0, visibleEntityCount = m_entityDataCache->VisibleEntityDataCount();
                 entityCacheIndex < visibleEntityCount;
                 ++entityCacheIndex)
            {
                if (const AZ::EntityId entityId = m_entityDataCache->GetVisibleEntityId(entityCacheIndex);
                    m_entityDataCache->IsVisibleEntityVisible(entityCacheIndex) && IsSelectableInViewport(entityCacheIndex))
                {
                    if (m_entityDataCache->IsVisibleEntityIconHidden(entityCacheIndex) ||
                        (m_entityDataCache->IsVisibleEntitySelected(entityCacheIndex) && !showIconCheck(entityId)))
                    {
                        continue;
                    }

                    const AZ::Vector3& entityPosition = m_entityDataCache->GetVisibleEntityPosition(entityCacheIndex);
                    const AZ::Vector3 entityCameraVector = entityPosition - cameraState.m_position;

                    if (const float directionFromCamera = entityCameraVector.Dot(cameraState.m_forward); directionFromCamera < 0.0f)
                    {
                        continue;
                    }

                    const float distanceFromCamera = entityCameraVector.GetLength();
                    if (distanceFromCamera < cameraState.m_nearClip)
                    {
                        continue;
                    }

                    using ComponentEntityAccentType = Components::EditorSelectionAccentSystemComponent::ComponentEntityAccentType;
                    const AZ::Color iconHighlight = [this, entityCacheIndex]()
                    {
                        if (m_entityDataCache->IsVisibleEntityLocked(entityCacheIndex))
                        {
                            return AZ::Color(AZ::u8(100), AZ::u8(100), AZ::u8(100), AZ::u8(255));
                        }

                        if (m_entityDataCache->GetVisibleEntityAccent(entityCacheIndex) == ComponentEntityAccentType::Hover)
                        {
                            return AZ::Color(AZ::u8(255), AZ::u8(120), AZ::u8(0), AZ::u8(204));
                        }

                        return AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
                    }();

                    int iconTextureId = 0;
                    EditorEntityIconComponentRequestBus::EventResult(
                        iconTextureId, entityId, &EditorEntityIconComponentRequestBus::Events::GetEntityIconTextureId);

                    editorViewportIconDisplay->AddIcon(EditorViewportIconDisplayInterface::DrawParameters{
                        viewportInfo.m_viewportId, iconTextureId, iconHighlight, entityPosition,
                        EditorViewportIconDisplayInterface::CoordinateSpace::WorldSpace, AZ::Vector2(GetIconSize(distanceFromCamera)) });
                }
            }

            editorViewportIconDisplay->DrawIcons();
        }
    }

    bool EditorHelpers::IsSelectableInViewport(const AZ::EntityId entityId) const
    {
        if (auto entityCacheIndex = m_entityDataCache->GetVisibleEntityIndexFromId(entityId); entityCacheIndex.has_value())
        {
            return m_entityDataCache->IsVisibleEntityIndividuallySelectableInViewport(entityCacheIndex.value());
        }

        return false;
    }

    bool EditorHelpers::IsSelectableInViewport(size_t entityCacheIndex) const
    {
        return m_entityDataCache->IsVisibleEntityIndividuallySelectableInViewport(entityCacheIndex);
    }

    bool EditorHelpers::IsSelectableAccordingToFocusMode(const AZ::EntityId entityId) const
    {
        if (auto entityCacheIndex = m_entityDataCache->GetVisibleEntityIndexFromId(entityId); entityCacheIndex.has_value())
        {
            return m_entityDataCache->IsVisibleEntityInFocusSubTree(entityCacheIndex.value());
        }

        return m_focusModeInterface->IsInFocusSubTree(entityId);
    }

    bool EditorHelpers::IsSelectableAccordingToContainerEntities(const AZ::EntityId entityId) const
    {
        if (const auto* containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get())
        {
            return !containerEntityInterface->IsUnderClosedContainerEntity(entityId);
        }

        return true;
    }
} // namespace AzToolsFramework
