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

namespace AzFramework
{
    class DebugDisplayRequests;
    struct ViewportInfo;
    struct CameraState;
} // namespace AzFramework

namespace AzToolsFramework
{
    class EditorVisibleEntityDataCache;
    class FocusModeInterface;

    namespace ViewportInteraction
    {
        struct MouseInteractionEvent;
    }

    //!< Represents the result of a query to find the id of the entity under the cursor (if any).
    class EntityIdUnderCursor
    {
    public:
        EntityIdUnderCursor(AZ::EntityId entityId);
        EntityIdUnderCursor(AZ::EntityId entityId, AZ::EntityId rootEntityId);

        //! Returns the entity id under the cursor (if any).
        //! @note In the case of no entity id under the cursor, an invalid entity id is returned.
        AZ::EntityId GetEntityId() const;

        //! Returns the root entity id if the entity id under the cursor is part of a prefab, otherwise returns the entity id.
        //! @note In the case of no entity id under the cursor, an invalid entity id is returned.
        AZ::EntityId GetRootEntityId() const;

        //! Returns true if the entity id under the cursor is part of a prefab, otherwise false.
        //! @note In the case of no entity id under the cursor, true is returned (although consider this behavior undefined).
        bool IsPrefabEntity() const;

    private:
        AZ::EntityId m_entityId; //<! The entity id under the cursor.
        AZ::EntityId m_rootEntityId; //<! For prefabs, the root entity id, otherwise the entity id under the cursor.
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
        explicit EditorHelpers(const EditorVisibleEntityDataCache* entityDataCache);
        EditorHelpers(const EditorHelpers&) = delete;
        EditorHelpers& operator=(const EditorHelpers&) = delete;
        ~EditorHelpers() = default;

        //! Determines the id of the entity under the cursor (if any). For prefabs, also determines the root entity id.
        //! Used to check if a particular entity was selected.
        EntityIdUnderCursor GetEntityIdUnderCursor(
            const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        //! Do the drawing responsible for the EditorHelpers.
        //! @param showIconCheck Provide a custom callback to filter certain entities from
        //! displaying an icon under certain conditions.
        void DisplayHelpers(
            const AzFramework::ViewportInfo& viewportInfo,
            const AzFramework::CameraState& cameraState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZStd::function<bool(AZ::EntityId)>& showIconCheck);

        //! Returns whether the entityId can be selected in the viewport according
        //! to the current Editor Focus Mode and Container Entity setup.
        bool IsSelectableInViewport(AZ::EntityId entityId);

    private:
        //! Returns whether the entityId can be selected in the viewport according
        //! to the current Editor Focus Mode setup.
        bool IsSelectableAccordingToFocusMode(AZ::EntityId entityId);

        //! Returns whether the entityId can be selected in the viewport according
        //! to the current Container Entityu setup.
        bool IsSelectableAccordingToContainerEntities(AZ::EntityId entityId);

        const EditorVisibleEntityDataCache* m_entityDataCache = nullptr; //!< Entity Data queried by the EditorHelpers.
        const FocusModeInterface* m_focusModeInterface = nullptr;
    };
} // namespace AzToolsFramework
