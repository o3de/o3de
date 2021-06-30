/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Viewport/ViewportId.h>
#include <AzFramework/Windowing/WindowBus.h>

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

    //! Controller Priority determines when controllers receive input and update events.
    //! Controller Priority is provided by the list containing a viewport controller.
    //! Input channel events are received from highest to lowest priority order,
    //! allowing high priority controllers to consume input events and stop their propagation.
    //! Viewport update events are received from lowest to highest priority order,
    //! allowing high priority controllers to be the last to update the viewport state.
    //! 
    //! Controller lists may receive DispatchToAllPriorities events,  which will in turn
    //! dispatch events to all of their children in priority order.
    //! 
    //! @see AzFramework::ViewportControllerList
    //! @note Because of this behavior, a ViewportControllerList that belongs to another
    //! ViewportControllerList shall not receive a DispatchToAllPriorities event, and instead
    //! shall receive multiple events from its parent at all priority levels.
    enum class ViewportControllerPriority : uint8_t {
        Highest = 0,
        High,
        Normal,
        Low,
        Lowest,
        DispatchToAllPriorities
    };

    //! An event dispatched to ViewportControllers when input occurs.
    struct ViewportControllerInputEvent
    {
        //! The viewport ID this event was dispatched to.
        ViewportId m_viewportId;
        //! The native window handle for the application.
        NativeWindowHandle m_windowHandle;
        //! The input channel data for this event.
        const AzFramework::InputChannel& m_inputChannel;
        //! The priority this event was dispatched at.
        ViewportControllerPriority m_priority;

        ViewportControllerInputEvent(
            ViewportId viewportId, NativeWindowHandle windowHandle, const AzFramework::InputChannel& inputChannel,
            ViewportControllerPriority priority = ViewportControllerPriority::DispatchToAllPriorities)
            : m_viewportId(viewportId)
            , m_windowHandle(windowHandle)
            , m_inputChannel(inputChannel)
            , m_priority(priority)
        {
        }
    };

    //! An event dispatched to ViewportControllers every tick.
    struct ViewportControllerUpdateEvent
    {
        //! The viewport ID this event was dispatched to.
        ViewportId m_viewportId;
        //! The time since the last update event, in seconds.
        FloatSeconds m_deltaTime;
        //! The absolute time point of this event.
        AZ::ScriptTimePoint m_time;
        //! The priority this event was dispatched at.
        ViewportControllerPriority m_priority;

        ViewportControllerUpdateEvent(
            ViewportId viewportId, FloatSeconds deltaTime,
            AZ::ScriptTimePoint time,
            ViewportControllerPriority priority = ViewportControllerPriority::DispatchToAllPriorities
        )
            : m_viewportId(viewportId)
            , m_deltaTime(deltaTime)
            , m_time(time)
            , m_priority(priority)
        {
        }
    };

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
        virtual bool HandleInputChannelEvent([[maybe_unused]]const ViewportControllerInputEvent& event) { return false; }
        //! Called to notify this controller that its input state should be reset.
        //! This is called when input events, such as key up events, may have been missed and it should be assumed that
        //! all input channels are in their default (i.e. no buttons pressed or other input provided) state.
        virtual void ResetInputChannels(){}
        //! Updates the current state of the viewport. This should be used to update e.g. the camera transform and will be called every frame
        //! for each registered viewport.
        virtual void UpdateViewport([[maybe_unused]]const ViewportControllerUpdateEvent& event) {}
        //! Registers a ViewportContext to be handled by this controller.
        //! The controller will receive OnInputChannelEvent and OnUpdateViewport notifications for the viewports.
        virtual void RegisterViewportContext(ViewportId viewport) = 0;
        //! Unregisters a viewport from being handled by this controller.
        //! No further events will be received from this viewport after this is called.
        virtual void UnregisterViewportContext(ViewportId viewport) = 0;
        //! Gets the priority at which this controller will receive input events.
        //! If set to DispatchToAllPriorities, the controller will receive events multiple times for each
        //! available priority level. This typically is only needed in the case of a list of other viewport
        //! controllers, each with their own priority (handled by ViewportControllerList for most cases).
        virtual ViewportControllerPriority GetPriority() const { return ViewportControllerPriority::Normal; }
    };

} //namespace AzFramework
