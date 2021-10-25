/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Application/Application.h>

#include <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ApplicationIos
        : public Application::Implementation
        , public IosLifecycleEvents::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CLASS_ALLOCATOR(ApplicationIos, AZ::SystemAllocator, 0);
        ApplicationIos();
        ~ApplicationIos() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // IosLifecycleEvents
        void OnWillResignActive() override;
        void OnDidBecomeActive() override;
        void OnDidEnterBackground() override;
        void OnWillEnterForeground() override;
        void OnWillTerminate() override;
        void OnDidReceiveMemoryWarning() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Application::Implementation
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;

    private:
        ApplicationLifecycleEvents::Event m_lastEvent;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    Application::Implementation* Application::Implementation::Create()
    {
        return aznew ApplicationIos();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationIos::ApplicationIos()
        : m_lastEvent(ApplicationLifecycleEvents::Event::None)
    {
        IosLifecycleEvents::Bus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationIos::~ApplicationIos()
    {
        IosLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationIos::OnWillResignActive()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationConstrained, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Constrain;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationIos::OnDidBecomeActive()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationUnconstrained, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Unconstrain;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationIos::OnDidEnterBackground()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationSuspended, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Suspend;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationIos::OnWillEnterForeground()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationResumed, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Resume;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationIos::OnWillTerminate()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnMobileApplicationWillTerminate);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationIos::OnDidReceiveMemoryWarning()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnMobileApplicationLowMemoryWarning);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationIos::PumpSystemEventLoopOnce()
    {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, TRUE);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationIos::PumpSystemEventLoopUntilEmpty()
    {
        SInt32 result;
        const CFTimeInterval MaxSecondsInRunLoop = 0.001; // One millisecond
        do
        {
            result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, MaxSecondsInRunLoop, TRUE);
        }
        while (result == kCFRunLoopRunHandledSource);
    }
} // namespace AzFramework
