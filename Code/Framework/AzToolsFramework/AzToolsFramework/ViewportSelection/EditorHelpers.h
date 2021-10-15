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
    class EditorVisibleEntityDataCache;
    class FocusModeInterface;

    namespace ViewportInteraction
    {
        struct MouseInteractionEvent;
    }

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

        //! Handle any mouse interaction with the EditorHelpers.
        //! Used to check if a particular entity was selected.
        AZ::EntityId HandleMouseInteraction(
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

        AZStd::unique_ptr<InvalidClicks> m_invalidClicks; //!< Display for invalid click behavior.

        const EditorVisibleEntityDataCache* m_entityDataCache = nullptr; //!< Entity Data queried by the EditorHelpers.
        const FocusModeInterface* m_focusModeInterface = nullptr; //!< API to interact with focus mode functionality.
    };
} // namespace AzToolsFramework
