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

#include <AzFramework/Viewport/ViewportId.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Script/ScriptTimePoint.h>

namespace AzFramework
{
    class InputChannel;

    class ViewportControllerInterface;
    class ViewportControllerList;

    using ViewportControllerPtr = AZStd::shared_ptr<ViewportControllerInterface>;
    using ConstViewportControllerPtr = AZStd::shared_ptr<const ViewportControllerInterface>;
    using ViewportControllerListPtr = AZStd::shared_ptr<ViewportControllerList>;
    using ConstViewportControllerListPtr = AZStd::shared_ptr<const ViewportControllerList>;

    using FloatSeconds = AZStd::chrono::duration<float>;

    //! The interface for a Viewport Controller which handles input events and periodic updates for one or more registered Viewports.
    //! @see SingleViewportController for simple cases involving only one viewport.
    //! @see MultiViewportController for cases involving multiple viewports with no shared state.
    class ViewportControllerInterface
    {
    public:
        virtual ~ViewportControllerInterface() = default;

        //! Handles an input event dispatched to a given viewportContext.
        //! @return A "handled" flag. If OnInputChannelEvent returns true, the event is considered handled and all further input handling
        //! in the containing ViewportControllerList halts.
        virtual bool HandleInputChannelEvent([[maybe_unused]]ViewportId viewport, [[maybe_unused]]const AzFramework::InputChannel& inputChannel) { return false; }
        //! Updates the current state of the viewport. This should be used to update e.g. the camera transform and will be called every frame
        //! for each registered viewport.
        virtual void UpdateViewport([[maybe_unused]]ViewportId viewport, [[maybe_unused]]FloatSeconds deltaTime, [[maybe_unused]]AZ::ScriptTimePoint time) {}
        //! Registers a ViewportContext to be handled by this controller.
        //! The controller will receive OnInputChannelEvent and OnUpdateViewport notifications for the viewports.
        virtual void RegisterViewportContext(ViewportId viewport) = 0;
        //! Unregisters a viewport from being handled by this controller.
        //! No further events will be received from this viewport after this is called.
        virtual void UnregisterViewportContext(ViewportId viewport) = 0;
    };

} //namespace AzFramework
