/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzToolsFramework/ViewportSelection/InvalidClicks.h>

namespace AzFramework
{
    class DebugDisplayRequests;
    struct ViewportInfo;
    struct CameraState;
} // namespace AzFramework

namespace AzToolsFramework
{
    class EditorVisibleEntityDataCacheInterface;
    class FocusModeInterface;

    namespace ViewportInteraction
    {
        struct MouseInteractionEvent;
    }

    //!< Represents the result of a query to find the id of the entity under the cursor (if any).
    class CursorEntityIdQuery
    {
    public:
        CursorEntityIdQuery(AZ::EntityId entityId, AZ::EntityId rootEntityId);

        //! Returns the entity id under the cursor (if any).
        //! @note In the case of no entity id under the cursor, an invalid entity id is returned.
        AZ::EntityId EntityIdUnderCursor() const;

        //! Returns the topmost container entity id in the hierarchy if the entity id under the cursor is inside a container entity, otherwise returns the entity id.
        //! @note In the case of no entity id under the cursor, an invalid entity id is returned.
        AZ::EntityId ContainerAncestorEntityId() const;

        //! Returns true if the query has a container ancestor entity id, otherwise false.
        bool HasContainerAncestorEntityId() const;

    private:
        AZ::EntityId m_entityId; //<! The entity id under the cursor.
        AZ::EntityId m_containerAncestorEntityId; //<! For entities in container entities, the topmost container entity id in the hierarchy, otherwise the entity id under the cursor.
    };

    //! EditorHelpers are the visualizations that appear for entities
    //! when 'Display Helpers' is toggled on inside the editor.
    //! These include but are not limited to entity icons and shape visualizations.
    class EditorHelpers
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        //! An EditorVisibleEntityDataCache must be passed to EditorHelpers to allow it to
        //! efficiently read entity data without resorting to EBus calls.
        explicit EditorHelpers(const EditorVisibleEntityDataCacheInterface* entityDataCache);
        EditorHelpers(const EditorHelpers&) = delete;
        EditorHelpers& operator=(const EditorHelpers&) = delete;
        ~EditorHelpers() = default;

        //! Finds the id of the entity under the cursor (if any). For entities in container entities, also finds the topmost container entity id in the hierarchy.
        //! Used to check if a particular entity was selected.
        CursorEntityIdQuery FindEntityIdUnderCursor(
            const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        //! Do the drawing responsible for the EditorHelpers.
        //! @param showIconCheck Provide a custom callback to filter certain entities from
        //! displaying an icon under certain conditions.
        void DisplayHelpers(
            const AzFramework::ViewportInfo& viewportInfo,
            const AzFramework::CameraState& cameraState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZStd::function<bool(AZ::EntityId)>& showIconCheck);

        //! Handle 2d drawing for EditorHelper functionality.
        void Display2d(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay);

        //! Returns whether the entityId can be selected in the viewport according
        //! to the current Editor Focus Mode and Container Entity setup.
        bool IsSelectableInViewport(AZ::EntityId entityId) const;

    private:
        //! Returns whether the entityId can be selected in the viewport according
        //! to the current Editor Focus Mode setup.
        bool IsSelectableAccordingToFocusMode(AZ::EntityId entityId) const;

        //! Returns whether the entityId can be selected in the viewport according
        //! to the current Container Entity setup.
        bool IsSelectableAccordingToContainerEntities(AZ::EntityId entityId) const;

        //! Returns whether the entityCacheIndex can be selected in the viewport according
        //! to the current Editor Focus Mode and Container Entity setup.
        bool IsSelectableInViewport(size_t entityCacheIndex) const;

        AZStd::unique_ptr<InvalidClicks> m_invalidClicks; //!< Display for invalid click behavior.

        const EditorVisibleEntityDataCacheInterface* m_entityDataCache = nullptr; //!< Entity Data queried by the EditorHelpers.
        const FocusModeInterface* m_focusModeInterface = nullptr; //!< API to interact with focus mode functionality.
    };

    //! Calculate the icon scale based on how far away it is from a given point.
    //! @note This is mostly likely distance from the camera.
    float GetIconScale(float distance);

    //! Calculate the icon size based on how far away it is from a given point.
    //! @note This is the base icon size multiplied by the icon scale to give a final viewport size.
    float GetIconSize(float distance);
} // namespace AzToolsFramework
