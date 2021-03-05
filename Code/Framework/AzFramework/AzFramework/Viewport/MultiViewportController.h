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

#include <AzFramework/Viewport/Viewport.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzFramework
{
    //! MultiViewportController defines an interface for a many-to-many viewport controller interface
    //! that automatically assigns a specific TViewportControllerInstance to each registered viewport.
    //! Subclasses of MultiViewportController will be provided with one instance of TViewportControllerInstance
    //! per registered viewport, where TViewportControllerInstance must implement the interface of
    //! MultiViewportControllerInstance and provide a TViewportControllerInstance(ViewportId) constructor.
    template <class TViewportControllerInstance>
    class MultiViewportController
        : public ViewportControllerInterface
    {
    public:
        ~MultiViewportController() override;

        // ViewportControllerInterface ...
        bool HandleInputChannelEvent(ViewportId viewport, const AzFramework::InputChannel& inputChannel) override;
        void UpdateViewport(ViewportId viewport, FloatSeconds deltaTime, AZ::ScriptTimePoint time) override;
        void RegisterViewportContext(ViewportId viewport) override;
        void UnregisterViewportContext(ViewportId viewport) override;

    private:
        AZStd::unordered_map<ViewportId, AZStd::unique_ptr<TViewportControllerInstance>> m_instances;
    };

    //! The interface used by MultiViewportController to manage individual instances.
    class MultiViewportControllerInstanceInterface
    {
    public:
        explicit MultiViewportControllerInstanceInterface(ViewportId viewport)
            : m_viewport(viewport)
        {
        }

        ViewportId GetViewportId() const { return m_viewportId; }

        virtual bool HandleInputChannelEvent([[maybe_unused]]const AzFramework::InputChannel& inputChannel) { return false; }
        virtual void UpdateViewport([[maybe_unused]]FloatSeconds deltaTime, [[maybe_unused]]AZ::ScriptTimePoint time) {}

    private:
        ViewportId m_viewportId;
    };
} //namespace AzFramework

#include "AzFramework/Viewport/MultiViewportController.inl"
