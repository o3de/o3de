/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Application/Application_Windows.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationWindows::ApplicationWindows()
        : m_lastEvent(ApplicationLifecycleEvents::Event::None)
    {
        WindowsLifecycleEvents::Bus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationWindows::~ApplicationWindows()
    {
        WindowsLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::OnMinimized()
    {
        // guard against duplicate events
        if (m_lastEvent != ApplicationLifecycleEvents::Event::Suspend)
        {
            ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnApplicationSuspended, m_lastEvent);
            m_lastEvent = ApplicationLifecycleEvents::Event::Suspend;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::OnRestored()
    {
        // guard against duplicate events
        if (m_lastEvent != ApplicationLifecycleEvents::Event::Resume)
        {
            ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnApplicationResumed, m_lastEvent);
            m_lastEvent = ApplicationLifecycleEvents::Event::Resume;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::OnKillFocus()
    {
        // guard against duplicate events
        if (m_lastEvent != ApplicationLifecycleEvents::Event::Constrain)
        {
            ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnApplicationConstrained, m_lastEvent);
            m_lastEvent = ApplicationLifecycleEvents::Event::Constrain;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::OnSetFocus()
    {
        // guard against duplicate events
        if (m_lastEvent != ApplicationLifecycleEvents::Event::Unconstrain)
        {
            ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnApplicationUnconstrained, m_lastEvent);
            m_lastEvent = ApplicationLifecycleEvents::Event::Unconstrain;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::PumpSystemEventLoopOnce()
    {
        MSG msg;
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ProcessSystemEvent(msg);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::PumpSystemEventLoopUntilEmpty()
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ProcessSystemEvent(msg);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::ProcessSystemEvent(MSG& msg)
    {
        if (msg.message == WM_QUIT)
        {
            ApplicationRequests::Bus::Broadcast(&ApplicationRequests::ExitMainLoop);
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

} // namespace AzFramework
