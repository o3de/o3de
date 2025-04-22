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
    class EditorVisibleEntityDataCacheInterface;
    class ViewportEditorModeTrackerInterface;

    //! Viewport interaction helper that handles highlighting entities and picking them within the viewport.
    //! This helper can be used from within other Viewport Interaction modes, such as the EditorDefaultSelection mode.
    class EditorPickEntitySelectionHelper
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EditorPickEntitySelectionHelper(
            const EditorVisibleEntityDataCacheInterface* entityDataCache, ViewportEditorModeTrackerInterface* viewportEditorModeTracker);
        ~EditorPickEntitySelectionHelper();

        //! Detects the entity under the cursor and the viewport and selects it when the button is pressed.
        //! @param mouseInteraction The mouse interaction event to use for entity detection and selection.
        //! @return True if the event was handled, false if it should continue being processed. (Currently always returns false)
        bool HandleMouseViewportInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        //! Highlight the currently-selected entity in the viewport.
        void HighlightSelectedEntity();

        //! Draw the EditorHelpers.
        //! This should only need to get called when used from EditorPickEntitySelection, since ViewportInteraction modes need
        //! to own the full display of the EditorHelpers.
        //! If this is being used as a helper with a different ViewportInteraction mode such as EditorDefaultSelection, then
        //! calling this would cause a crash due to a recursive loop from multiple EditorHelper display calls.
        void DisplayEditorHelpers(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay);

    protected:
        AZStd::unique_ptr<EditorHelpers> m_editorHelpers; //!< Editor visualization of entities (icons, shapes, debug visuals etc).
        AZ::EntityId m_hoveredEntityId; //!< What EntityId is the mouse currently hovering over (if any).
        AZ::EntityId m_cachedEntityIdUnderCursor; //!< Store the EntityId on each mouse move for use in Display.
        //! Tracker for activating/deactivating viewport editor modes.
        ViewportEditorModeTrackerInterface* m_viewportEditorModeTracker = nullptr;
    };


    //! Viewport interaction that will handle assigning an entity in the viewport to
    //! an entity field in the entity inspector.
    class EditorPickEntitySelection : public ViewportInteraction::InternalViewportSelectionRequests
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EditorPickEntitySelection(
            const EditorVisibleEntityDataCacheInterface* entityDataCache, ViewportEditorModeTrackerInterface* viewportEditorModeTracker);
        ~EditorPickEntitySelection() = default;

        // ViewportInteraction::InternalViewportSelectionRequests ...
        bool InternalHandleMouseViewportInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        void DisplayViewportSelection(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        EditorPickEntitySelectionHelper m_pickEntitySelectionHelper;
    };
} // namespace AzToolsFramework
