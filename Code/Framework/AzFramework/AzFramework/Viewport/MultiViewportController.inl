/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/typetraits/is_constructible.h>

namespace AzFramework
{
    template <class TViewportControllerInstance, ViewportControllerPriority Priority>
    MultiViewportController<TViewportControllerInstance, Priority>::~MultiViewportController()
    {
        static_assert(
            AZStd::is_same<TViewportControllerInstance, decltype(TViewportControllerInstance(0, nullptr))>::value,
            "TViewportControllerInstance must implement a TViewportControllerInstance(ViewportId, ViewportController) constructor"
        );
    }

    template <class TViewportControllerInstance, ViewportControllerPriority Priority>
    bool MultiViewportController<TViewportControllerInstance, Priority>::HandleInputChannelEvent(const ViewportControllerInputEvent& event)
    {
        auto instanceIt = m_instances.find(event.m_viewportId);
        AZ_Assert(instanceIt != m_instances.end(), "Attempted to call HandleInputChannelEvent on an unregistered viewport");
        return instanceIt->second->HandleInputChannelEvent(event);
    }

    template <class TViewportControllerInstance, ViewportControllerPriority Priority>
    void MultiViewportController<TViewportControllerInstance, Priority>::ResetInputChannels()
    {
        for (auto instanceIt = m_instances.begin(); instanceIt != m_instances.end(); ++instanceIt)
        {
            instanceIt->second->ResetInputChannels();
        }
    }

    template <class TViewportControllerInstance, ViewportControllerPriority Priority>
    void MultiViewportController<TViewportControllerInstance, Priority>::UpdateViewport(const ViewportControllerUpdateEvent& event)
    {
        auto instanceIt = m_instances.find(event.m_viewportId);
        AZ_Assert(instanceIt != m_instances.end(), "Attempted to call UpdateViewport on an unregistered viewport");
        instanceIt->second->UpdateViewport(event);
    }

    template <class TViewportControllerInstance, ViewportControllerPriority Priority>
    void MultiViewportController<TViewportControllerInstance, Priority>::RegisterViewportContext(ViewportId viewport)
    {
        m_instances[viewport] = AZStd::make_unique<TViewportControllerInstance>(viewport, static_cast<typename TViewportControllerInstance::ControllerType*>(this));
    }

    template <class TViewportControllerInstance, ViewportControllerPriority Priority>
    void MultiViewportController<TViewportControllerInstance, Priority>::UnregisterViewportContext(ViewportId viewport)
    {
        m_instances.erase(viewport);
    }

    template <class TViewportControllerInstance, ViewportControllerPriority Priority>
    ViewportControllerPriority MultiViewportController<TViewportControllerInstance, Priority>::GetPriority() const
    {
        return Priority;
    }
} //namespace AzFramework
