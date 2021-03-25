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
        void UpdateViewport(const ViewportControllerUpdateEvent& event) override;
        void RegisterViewportContext(ViewportId viewport) override;
        void UnregisterViewportContext(ViewportId viewport) override;
        ViewportControllerPriority GetPriority() const override;

    private:
        AZStd::unordered_map<ViewportId, AZStd::unique_ptr<TViewportControllerInstance>> m_instances;
    };

    //! The interface used by MultiViewportController to manage individual instances.
    class MultiViewportControllerInstanceInterface
    {
    public:
        explicit MultiViewportControllerInstanceInterface(ViewportId viewport)
            : m_viewportId(viewport)
        {
        }

        ViewportId GetViewportId() const { return m_viewportId; }

        virtual bool HandleInputChannelEvent([[maybe_unused]]const ViewportControllerInputEvent& event) { return false; }
        virtual void UpdateViewport([[maybe_unused]]const ViewportControllerUpdateEvent& event) {}

    private:
        ViewportId m_viewportId;
    };
} //namespace AzFramework

#include "AzFramework/Viewport/MultiViewportController.inl"
