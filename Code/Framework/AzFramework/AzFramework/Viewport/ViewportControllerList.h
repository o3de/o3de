/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        //! Dispatches an update tick to all controllers registered to this list.
        //! This occurs in *reverse* priority order (i.e. from the highest priority value to the lowest) so that
        //! controllers with the highest registration priority may override the transforms of the controllers with the
        //! lowest registration priority.
        void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;
        //! Registers a Viewport to this list.
        //! All current and added controllers will be registered with this viewport.
        void RegisterViewportContext(ViewportId viewport) override;
        //! Unregisters a Viewport from this list and all associated controllers.
        void UnregisterViewportContext(ViewportId viewport);
        //! All ViewportControllerLists have a priority of Custom to ensure
        //! that they receive events at all priorities from any parent controllers.
        AzFramework::ViewportControllerPriority GetPriority() const { return ViewportControllerPriority::DispatchToAllPriorities; }

    private:
        void SortControllers();
        bool DispatchInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event);
        void DispatchUpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event);

        AZStd::unordered_map<AzFramework::ViewportControllerPriority, AZStd::vector<ViewportControllerPtr>> m_controllers;
        AZStd::unordered_set<ViewportId> m_viewports;
    };
} //namespace AzFramework
