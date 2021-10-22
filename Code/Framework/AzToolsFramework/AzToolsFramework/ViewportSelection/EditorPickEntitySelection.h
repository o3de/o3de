/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorHelpers.h>

namespace AzToolsFramework
{
    class ViewportEditorModeTrackerInterface;

    //! Viewport interaction that will handle assigning an entity in the viewport to
    //! an entity field in the entity inspector.
    class EditorPickEntitySelection : public ViewportInteraction::InternalViewportSelectionRequests
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EditorPickEntitySelection(
            const EditorVisibleEntityDataCache* entityDataCache, ViewportEditorModeTrackerInterface* viewportEditorModeTracker);
        ~EditorPickEntitySelection();

    private:
        // ViewportInteraction::InternalViewportSelectionRequests ...
        bool InternalHandleMouseViewportInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        void DisplayViewportSelection(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        AZStd::unique_ptr<EditorHelpers> m_editorHelpers; //!< Editor visualization of entities (icons, shapes, debug visuals etc).
        AZ::EntityId m_hoveredEntityId; //!< What EntityId is the mouse currently hovering over (if any).
        AZ::EntityId m_cachedEntityIdUnderCursor; //!< Store the EntityId on each mouse move for use in Display.
        ViewportEditorModeTrackerInterface* m_viewportEditorModeTracker = nullptr; //!< Tracker for activating/deactivating viewport editor modes.
    };
} // namespace AzToolsFramework
