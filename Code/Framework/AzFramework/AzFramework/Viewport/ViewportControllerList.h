/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Viewport/ViewportControllerInterface.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

namespace AzFramework
{
    //! A list of ViewportControllers that allows priority-ordered dispatch to controllers registered to it.
    //! ViewportControllerList itself is-a controller, meaning it controllers can be contained in nested lists.
    class ViewportControllerList final
        : public ViewportControllerInterface
    {
    public:
        //! Adds a controller to this list at the specified priority.
        //! This controller will be notified of all InputChannelEvents not consumed by a higher priority controller
        //! via OnInputChannelEvent.
        void Add(ViewportControllerPtr controller);
        //! Removes a controller from this list.
        void Remove(ViewportControllerPtr controller);

        // ViewportControllerInterface overrides
        //! Dispatches an InputChannelEvent to all controllers registered to this list until
        //! either a controller returns true to consume the event in OnInputChannelEvent or the controller list is exhausted.
        //! InputChannelEvents are sent to controllers in priority order (from the lowest priority value to the highest).
        bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;
        //! Dispatches a ResetInputChannels call to all controllers registered to this list.
        //! Calls to controllers are made in an undefined order.
        void ResetInputChannels() override;
        //! Dispatches an update tick to all controllers registered to this list.
        //! This occurs in *reverse* priority order (i.e. from the highest priority value to the lowest) so that
        //! controllers with the highest registration priority may override the transforms of the controllers with the
        //! lowest registration priority.
        void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;
        //! Registers a Viewport to this list.
        //! All current and added controllers will be registered with this viewport.
        void RegisterViewportContext(ViewportId viewport) override;
        //! Unregisters a Viewport from this list and all associated controllers.
        void UnregisterViewportContext(ViewportId viewport) override;
        //! All ViewportControllerLists have a priority of Custom to ensure
        //! that they receive events at all priorities from any parent controllers.
        AzFramework::ViewportControllerPriority GetPriority() const override { return ViewportControllerPriority::DispatchToAllPriorities; }
        //! Returns true if this controller list is enabled, i.e.
        //! it is accepting and forwarding input and update events to its children.
        bool IsEnabled() const;
        //! Set this controller list's enabled state.
        //! If a controller list is disabled, it will ignore all input and update events rather than dispatching them to its children.
        void SetEnabled(bool enabled);

    private:
        void SortControllers();
        bool DispatchInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event);
        void DispatchUpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event);

        AZStd::unordered_map<AzFramework::ViewportControllerPriority, AZStd::vector<ViewportControllerPtr>> m_controllers;
        AZStd::unordered_set<ViewportId> m_viewports;
        bool m_enabled = true;
    };
} //namespace AzFramework
