/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/ViewportControllerList.h>

#include <AzCore/std/sort.h>

namespace AzFramework
{
    void ViewportControllerList::Add(ViewportControllerPtr controller)
    {
        for (auto &controllerData : m_controllers)
        {
            auto& controllerList = controllerData.second;
            auto controllerIt = AZStd::find(
                controllerList.begin(), controllerList.end(),
                controller
            );
            if (controllerIt != controllerList.end())
            {
                AZ_Assert(false, "Attempted to add a duplicate controller to a ViewportControllerList");
                return;
            }
        }
        m_controllers[controller->GetPriority()].push_back(controller);
        for (auto viewportId : m_viewports)
        {
            controller->RegisterViewportContext(viewportId);
        }
    }

    void ViewportControllerList::Remove(ViewportControllerPtr controller)
    {
        for (auto &controllerData : m_controllers)
        {
            auto& controllerList = controllerData.second;
            controllerList.erase(AZStd::remove(controllerList.begin(), controllerList.end(), controller));
        }
    }

    bool ViewportControllerList::HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event)
    {
        if (!IsEnabled())
        {
            return false;
        }

        // If our event priority is "custom", we should dispatch at all priority levels in order
        using AzFramework::ViewportControllerPriority;
        if (event.m_priority == AzFramework::ViewportControllerPriority::DispatchToAllPriorities)
        {
            AzFramework::ViewportControllerInputEvent syntheticEvent = event;
            for (const auto priority : {
                ViewportControllerPriority::Highest,
                ViewportControllerPriority::High,
                ViewportControllerPriority::Normal,
                ViewportControllerPriority::Low,
                ViewportControllerPriority::Lowest })
            {
                syntheticEvent.m_priority = priority;
                if (DispatchInputChannelEvent(syntheticEvent))
                {
                    return true;
                }
            }
            return false;
        }
        else
        {
            // Otherwise, dispatch to controllers at our priority
            return DispatchInputChannelEvent(event);
        }
    }

    void ViewportControllerList::ResetInputChannels()
    {
        // We don't need to send this while we're disabled, we're guaranteed to call ResetInputChannels after being re-enabled.
        if (!IsEnabled())
        {
            return;
        }

        for (const auto& controllerList : m_controllers)
        {
            for (const auto& controller : controllerList.second)
            {
                controller->ResetInputChannels();
            }
        }
    }

    bool ViewportControllerList::DispatchInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event)
    {
        if (auto priorityListIt = m_controllers.find(event.m_priority); priorityListIt != m_controllers.end())
        {
            for (const auto& controller : priorityListIt->second)
            {
                if (controller->HandleInputChannelEvent(event))
                {
                    return true;
                }
            }
        }

        // Also dispatch to any nested controllers with "Custom" priority
        if (auto priorityListIt = m_controllers.find(ViewportControllerPriority::DispatchToAllPriorities);
            priorityListIt != m_controllers.end())
        {
            for (const auto& controller : priorityListIt->second)
            {
                if (controller->HandleInputChannelEvent(event))
                {
                    return true;
                }
            }
        }
        return false;
    }

    void ViewportControllerList::UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event)
    {
        if (!IsEnabled())
        {
            return;
        }

        // If our event priority is "custom", we should dispatch at all priority levels in reverse order
        // Reverse order lets high priority controllers get the last say in viewport update operations
        using AzFramework::ViewportControllerPriority;
        if (event.m_priority == AzFramework::ViewportControllerPriority::DispatchToAllPriorities)
        {
            AzFramework::ViewportControllerUpdateEvent syntheticEvent = event;
            for (const auto priority : {
                ViewportControllerPriority::Lowest,
                ViewportControllerPriority::Low,
                ViewportControllerPriority::Normal,
                ViewportControllerPriority::High,
                ViewportControllerPriority::Highest })
            {
                syntheticEvent.m_priority = priority;
                DispatchUpdateViewport(syntheticEvent);
            }
        }
        else
        {
            // Otherwise, dispatch to controllers at our priority
            DispatchUpdateViewport(event);
        }
    }

    void ViewportControllerList::DispatchUpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event)
    {
        if (auto priorityListIt = m_controllers.find(event.m_priority); priorityListIt != m_controllers.end())
        {
            for (const auto& controller : priorityListIt->second)
            {
                controller->UpdateViewport(event);
            }
        }

        // Also dispatch to any nested controllers with "Custom" priority
        if (auto priorityListIt = m_controllers.find(ViewportControllerPriority::DispatchToAllPriorities);
            priorityListIt != m_controllers.end())
        {
            for (const auto& controller : priorityListIt->second)
            {
                controller->UpdateViewport(event);
            }
        }
    }

    void ViewportControllerList::RegisterViewportContext(ViewportId viewport)
    {
        m_viewports.insert(viewport);
        for (auto& controllerData : m_controllers)
        {
            for (auto& controller : controllerData.second)
            {
                controller->RegisterViewportContext(viewport);
            }
        }
    }

    void ViewportControllerList::UnregisterViewportContext(ViewportId viewport)
    {
        m_viewports.erase(viewport);
        for (auto& controllerData : m_controllers)
        {
            for (auto& controller : controllerData.second)
            {
                controller->UnregisterViewportContext(viewport);
            }
        }
    }

    bool ViewportControllerList::IsEnabled() const
    {
        return m_enabled;
    }

    void ViewportControllerList::SetEnabled(bool enabled)
    {
        if (m_enabled != enabled)
        {
            m_enabled = enabled;
            // If we've been re-enabled, reset our input channels as they may have missed state changes.
            if (m_enabled)
            {
                ResetInputChannels();
            }
        }
    }
} //namespace AzFramework
