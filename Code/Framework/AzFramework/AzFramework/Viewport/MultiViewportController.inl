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
#include <AzCore/std/typetraits/is_constructible.h>

namespace AzFramework
{
    template <class TViewportControllerInstance>
    MultiViewportController<TViewportControllerInstance>::~MultiViewportController()
    {
        static_assert(
            AZStd::is_constructible<TViewportControllerInstance, ViewportId>::value,
            "TViewportControllerInstance must implement a TViewportControllerInstance(ViewportId) constructor"
        );
    }

    template <class TViewportControllerInstance>
    bool MultiViewportController<TViewportControllerInstance>::HandleInputChannelEvent(ViewportId viewport, const AzFramework::InputChannel& inputChannel)
    {
        auto instanceIt = m_instances.find(viewport);
        AZ_Assert(instanceIt != m_instances.end(), "Attempted to call HandleInputChannelEvent on an unregistered viewport");
        return instanceIt->second->HandleInputChannelEvent(inputChannel);
    }

    template <class TViewportControllerInstance>
    void MultiViewportController<TViewportControllerInstance>::UpdateViewport(ViewportId viewport, FloatSeconds deltaTime, AZ::ScriptTimePoint time)
    {
        auto instanceIt = m_instances.find(viewport);
        AZ_Assert(instanceIt != m_instances.end(), "Attempted to call UpdateViewport on an unregistered viewport");
        instanceIt->second->UpdateViewport(deltaTime, time);
    }

    template <class TViewportControllerInstance>
    void MultiViewportController<TViewportControllerInstance>::RegisterViewportContext(ViewportId viewport)
    {
        m_instances[viewport] = AZStd::make_unique<TViewportControllerInstance>(viewport);
    }

    template <class TViewportControllerInstance>
    void MultiViewportController<TViewportControllerInstance>::UnregisterViewportContext(ViewportId viewport)
    {
        m_instances.erase(viewport);
    }
} //namespace AzFramework
