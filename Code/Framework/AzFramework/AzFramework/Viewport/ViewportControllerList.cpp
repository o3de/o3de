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

#include <AzFramework/Viewport/ViewportControllerList.h>

#include <AzCore/std/sort.h>

namespace AzFramework
{
    void ViewportControllerList::Add(ViewportControllerPtr controller, ViewportControllerList::Priority priority)
    {
        auto controllerIt = AZStd::find_if(
            m_controllers.begin(), m_controllers.end(),
            [controller](const ViewportControllerData& data)
        {
            return data.controller == controller;
        });
        if (controllerIt != m_controllers.end())
        {
            AZ_Assert(false, "Attempted to add a duplicate controller to a ViewportControllerList");
            return;
        }
        m_controllers.push_back({controller, priority});
        for (auto viewportId : m_viewports)
        {
            controller->RegisterViewportContext(viewportId);
        }
        SortControllers();
    }

    void ViewportControllerList::Remove(ViewportControllerPtr controller)
    {
        m_controllers.erase(AZStd::remove_if(
            m_controllers.begin(), m_controllers.end(),
            [controller](const ViewportControllerData& data)
        {
            return data.controller == controller;
        }));
    }

    void ViewportControllerList::SetPriority(ViewportControllerPtr controller, ViewportControllerList::Priority priority)
    {
        auto controllerIt = AZStd::find_if(
            m_controllers.begin(), m_controllers.end(),
            [controller](const ViewportControllerData& data)
        {
            return data.controller == controller;
        });
        if (controllerIt != m_controllers.end())
        {
            controllerIt->priority = priority;
        }
        SortControllers();
    }

    bool ViewportControllerList::HandleInputChannelEvent(ViewportId viewport, const AzFramework::InputChannel& inputChannel)
    {
        // Iterate in forward order, so that the lowest priority values get the first opportunity to consume the event
        for (const auto& controllerInfo : m_controllers)
        {
            if (controllerInfo.controller->HandleInputChannelEvent(viewport, inputChannel))
            {
                return true;
            }
        }
        return false;
    }

    void ViewportControllerList::UpdateViewport(ViewportId viewport, FloatSeconds deltaTime, AZ::ScriptTimePoint time)
    {
        // Iterate in reverse order, so that the lowest priority values go last
        // This lets authoritative state changes in controllers with priority "win"
        for (auto controllerIt = m_controllers.rbegin(), end = m_controllers.rend(); controllerIt != end; ++controllerIt)
        {
            controllerIt->controller->UpdateViewport(viewport, deltaTime, time);
        }
    }

    void ViewportControllerList::RegisterViewportContext(ViewportId viewport)
    {
        m_viewports.insert(viewport);
        for (const auto& controllerInfo : m_controllers)
        {
            controllerInfo.controller->RegisterViewportContext(viewport);
        }
    }

    void ViewportControllerList::UnregisterViewportContext(ViewportId viewport)
    {
        m_viewports.erase(viewport);
        for (const auto& controllerInfo : m_controllers)
        {
            controllerInfo.controller->UnregisterViewportContext(viewport);
        }
    }

    void ViewportControllerList::SortControllers()
    {
        AZStd::sort(
            m_controllers.begin(), m_controllers.end(),
            [](const ViewportControllerData& d1, const ViewportControllerData& d2)
        {
            return d1.priority < d2.priority;
        });
    }
} //namespace AzFramework
