/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Viewport/ViewportControllerInterface.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzFramework
{
    //! MultiViewportController defines an interface for a many-to-many viewport controller interface
    //! that automatically assigns a specific TViewportControllerInstance to each registered viewport.
    //! Subclasses of MultiViewportController will be provided with one instance of TViewportControllerInstance
    //! per registered viewport, where TViewportControllerInstance must implement the interface of
    //! MultiViewportControllerInstance and provide a TViewportControllerInstance(ViewportId) constructor.
    //! @param TViewportControllerInstance is the instance type of the controller,
    //! one shall be instantiated per registered viewport. This child should conform to the MultiViewportControllerInstanceInterface
    //! @param Priority is the priority at which this controller should be dispatched events.
    //! Input events may not be received if a higher prioririty controller consumes the event.
    //! To receive events at all priorities, DispatchToAllPriorities may be specified.
    template <class TViewportControllerInstance, ViewportControllerPriority Priority = ViewportControllerPriority::Normal>
    class MultiViewportController
        : public ViewportControllerInterface
    {
    public:
        ~MultiViewportController() override;

        // ViewportControllerInterface ...
        bool HandleInputChannelEvent(const ViewportControllerInputEvent& event) override;
        void ResetInputChannels() override;
        void UpdateViewport(const ViewportControllerUpdateEvent& event) override;
        void RegisterViewportContext(ViewportId viewport) override;
        void UnregisterViewportContext(ViewportId viewport) override;
        ViewportControllerPriority GetPriority() const override;

    private:
        AZStd::unordered_map<ViewportId, AZStd::unique_ptr<TViewportControllerInstance>> m_instances;
    };

    //! The interface used by MultiViewportController to manage individual instances.
    template <class TController>
    class MultiViewportControllerInstanceInterface
    {
    public:
        using ControllerType = TController;

        MultiViewportControllerInstanceInterface(ViewportId viewport, ControllerType* controller)
            : m_viewportId(viewport)
            , m_controller(controller)
        {
        }

        ViewportId GetViewportId() const { return m_viewportId; }
        ControllerType* GetController() { return m_controller; }
        const ControllerType* GetController() const { return m_controller; }

        virtual bool HandleInputChannelEvent([[maybe_unused]]const ViewportControllerInputEvent& event) { return false; }
        virtual void ResetInputChannels() {}
        virtual void UpdateViewport([[maybe_unused]]const ViewportControllerUpdateEvent& event) {}

    private:
        ViewportId m_viewportId;
        ControllerType* m_controller;
    };
} //namespace AzFramework

#include "AzFramework/Viewport/MultiViewportController.inl"
