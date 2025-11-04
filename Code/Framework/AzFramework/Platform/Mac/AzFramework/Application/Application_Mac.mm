/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

#include <AppKit/NSApplication.h>
#include <AppKit/NSEvent.h>

#include <objc/runtime.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ApplicationMac
        : public Application::Implementation
        , public DarwinLifecycleEvents::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CLASS_ALLOCATOR(ApplicationMac, AZ::SystemAllocator);
        ApplicationMac();
        ~ApplicationMac() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // DarwinLifecycleEvents
        void OnDidResignActive() override;  // Constrain
        void OnDidBecomeActive() override;  // Unconstrain
        void OnDidHide() override;          // Suspend
        void OnDidUnhide() override;        // Resume

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Application::Implementation
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;

    protected:
        bool ProcessNextSystemEvent(); // Returns true if an event was processed, false otherwise

    private:
        ApplicationLifecycleEvents::Event m_lastEvent;
        id m_notificationObserver;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Ideally this class would be defined using the standard @interface / @imlementation keywords,
    // but an Objective-C class defined in a static lib linked by multiple dynamic libs results in
    // runtime warnings, because each dynamic lib resgisters a new version of the exact same class:
    //
    // "Class X is implemented in both A and B. One of the two will be used. Which one is undefined."
    //
    // To get around this absurdity we're just defining the Objective-C class dynamically at runtime.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ApplicationNotificationObserver
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Load the Objective-C class. Will be created and registered with the Objective-C runtime,
        //! unless this has already been done, in which case it will simply be returned immediately.
        //! \return The Objective-C class that defines our ApplicationNotificationObserver class
        static Class LoadClassType();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Resgister an instance of this class for application notifications
        //! \param[in] self The instance of this class to register for application notifications
        static void RegisterForNotifications(id self);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Deresgister an instance of this class for application notifications
        //! \param[in] self The instance of this class to deregister for application notifications
        static void DeregisterForNotifications(id self);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The Objective-C method implementation
        ///@{
        static void OnWillResignActive(id self, SEL methodSelector, NSNotification* notification);
        static void OnDidResignActive(id self, SEL methodSelector, NSNotification* notification);
        static void OnWillBecomeActive(id self, SEL methodSelector, NSNotification* notification);
        static void OnDidBecomeActive(id self, SEL methodSelector, NSNotification* notification);
        static void OnWillHide(id self, SEL methodSelector, NSNotification* notification);
        static void OnDidHide(id self, SEL methodSelector, NSNotification* notification);
        static void OnWillUnhide(id self, SEL methodSelector, NSNotification* notification);
        static void OnDidUnhide(id self, SEL methodSelector, NSNotification* notification);
        static void OnWillTerminate(id self, SEL methodSelector, NSNotification* notification);
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The Objective-C method selector
        //! \return The Objective-C method implementation
        ///@{
        static SEL s_applicationWillResignActiveSelector;
        static SEL s_applicationDidResignActiveSelector;
        static SEL s_applicationWillBecomeActiveSelector;
        static SEL s_applicationDidBecomeActiveSelector;
        static SEL s_applicationWillHideSelector;
        static SEL s_applicationDidHideSelector;
        static SEL s_applicationWillUnhideSelector;
        static SEL s_applicationDidUnhideSelector;
        static SEL s_applicationWillTerminateSelector;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The Objective-C class name
        static const char* s_className;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationMac::ApplicationMac()
        : m_lastEvent(ApplicationLifecycleEvents::Event::None)
    {
        DarwinLifecycleEvents::Bus::Handler::BusConnect();
        m_notificationObserver = [[ApplicationNotificationObserver::LoadClassType() alloc] init];
        ApplicationNotificationObserver::RegisterForNotifications(m_notificationObserver);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationMac::~ApplicationMac()
    {
        ApplicationNotificationObserver::DeregisterForNotifications(m_notificationObserver);
        [m_notificationObserver release];
        m_notificationObserver = nullptr;
        DarwinLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationMac::OnDidResignActive()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationConstrained, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Constrain;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationMac::OnDidBecomeActive()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationUnconstrained, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Unconstrain;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationMac::OnDidHide()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationSuspended, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Suspend;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationMac::OnDidUnhide()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationResumed, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Resume;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationMac::PumpSystemEventLoopOnce()
    {
        ProcessNextSystemEvent();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationMac::PumpSystemEventLoopUntilEmpty()
    {
        bool eventProcessed = false;
        do
        {
            eventProcessed = ProcessNextSystemEvent();
        }
        while (eventProcessed);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool ApplicationMac::ProcessNextSystemEvent()
    {
        @autoreleasepool
        {
            NSEvent* event = [NSApp nextEventMatchingMask: NSEventMaskAny
                                    untilDate: [NSDate distantPast]
                                    inMode: NSDefaultRunLoopMode
                                    dequeue: YES];
            if (event != nil)
            {
                RawInputNotificationBusMac::Broadcast(&RawInputNotificationsMac::OnRawInputEvent, event);
                [NSApp sendEvent: event];
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const char* ApplicationNotificationObserver::s_className = "AzFrameworkApplicationNotificationObserver";
    SEL ApplicationNotificationObserver::s_applicationWillResignActiveSelector = @selector(applicationWillResignActive:);
    SEL ApplicationNotificationObserver::s_applicationDidResignActiveSelector = @selector(applicationDidResignActive:);
    SEL ApplicationNotificationObserver::s_applicationWillBecomeActiveSelector = @selector(applicationWillBecomeActive:);
    SEL ApplicationNotificationObserver::s_applicationDidBecomeActiveSelector = @selector(applicationDidBecomeActive:);
    SEL ApplicationNotificationObserver::s_applicationWillHideSelector = @selector(applicationWillHide:);
    SEL ApplicationNotificationObserver::s_applicationDidHideSelector = @selector(applicationDidHide:);
    SEL ApplicationNotificationObserver::s_applicationWillUnhideSelector = @selector(applicationWillUnhide:);
    SEL ApplicationNotificationObserver::s_applicationDidUnhideSelector = @selector(applicationDidUnhide:);
    SEL ApplicationNotificationObserver::s_applicationWillTerminateSelector = @selector(applicationWillTerminate:);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    Class ApplicationNotificationObserver::LoadClassType()
    {
        // Check if the class type already exists
        Class classType = NSClassFromString([NSString stringWithUTF8String: s_className]);
        if (classType != nil)
        {
            // We've already called this function and created/registered the class type below
            return classType;
        }

        // Get the argument types string for a notification observer method
        Method notificationMethod = class_getInstanceMethod([NSNotificationCenter class],
                                                            @selector(postNotification:));
        const char* notificationMethodArgumentTypes = method_getTypeEncoding(notificationMethod);

        // Create the class type
        classType = objc_allocateClassPair([NSObject class], s_className, 0);

        // Add all the class instance methods
        class_addMethod(classType,
                        s_applicationWillResignActiveSelector,
                        (IMP)OnWillResignActive,
                        notificationMethodArgumentTypes);
        class_addMethod(classType,
                        s_applicationDidResignActiveSelector,
                        (IMP)OnDidResignActive,
                        notificationMethodArgumentTypes);
        class_addMethod(classType,
                        s_applicationWillBecomeActiveSelector,
                        (IMP)OnWillBecomeActive,
                        notificationMethodArgumentTypes);
        class_addMethod(classType,
                        s_applicationDidBecomeActiveSelector,
                        (IMP)OnDidBecomeActive,
                        notificationMethodArgumentTypes);
        class_addMethod(classType,
                        s_applicationWillHideSelector,
                        (IMP)OnWillHide,
                        notificationMethodArgumentTypes);
        class_addMethod(classType,
                        s_applicationDidHideSelector,
                        (IMP)OnDidHide,
                        notificationMethodArgumentTypes);
        class_addMethod(classType,
                        s_applicationWillUnhideSelector,
                        (IMP)OnWillUnhide,
                        notificationMethodArgumentTypes);
        class_addMethod(classType,
                        s_applicationDidUnhideSelector,
                        (IMP)OnDidUnhide,
                        notificationMethodArgumentTypes);
        class_addMethod(classType,
                        s_applicationWillTerminateSelector,
                        (IMP)OnWillTerminate,
                        notificationMethodArgumentTypes);

        // Register the class type and return it
        objc_registerClassPair(classType);
        return classType;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationNotificationObserver::RegisterForNotifications(id self)
    {
        NSNotificationCenter* defaultNotificationCenter = [NSNotificationCenter defaultCenter];
        if (!defaultNotificationCenter)
        {
            return;
        }

        [defaultNotificationCenter addObserver: self
                                      selector: s_applicationWillResignActiveSelector
                                          name: NSApplicationWillResignActiveNotification
                                        object: nil];
        [defaultNotificationCenter addObserver: self
                                      selector: s_applicationDidResignActiveSelector
                                          name: NSApplicationDidResignActiveNotification
                                        object: nil];
        [defaultNotificationCenter addObserver: self
                                      selector: s_applicationWillBecomeActiveSelector
                                          name: NSApplicationWillBecomeActiveNotification
                                        object: nil];
        [defaultNotificationCenter addObserver: self
                                      selector: s_applicationDidBecomeActiveSelector
                                          name: NSApplicationDidBecomeActiveNotification
                                        object: nil];
        [defaultNotificationCenter addObserver: self
                                      selector: s_applicationWillHideSelector
                                          name: NSApplicationWillHideNotification
                                        object: nil];
        [defaultNotificationCenter addObserver: self
                                      selector: s_applicationDidHideSelector
                                          name: NSApplicationDidHideNotification
                                        object: nil];
        [defaultNotificationCenter addObserver: self
                                      selector: s_applicationWillUnhideSelector
                                          name: NSApplicationWillUnhideNotification
                                        object: nil];
        [defaultNotificationCenter addObserver: self
                                      selector: s_applicationDidUnhideSelector
                                          name: NSApplicationDidUnhideNotification
                                        object: nil];
        [defaultNotificationCenter addObserver: self
                                      selector: s_applicationWillTerminateSelector
                                          name: NSApplicationWillTerminateNotification
                                        object: nil];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationNotificationObserver::DeregisterForNotifications(id self)
    {
        [[NSNotificationCenter defaultCenter] removeObserver: self];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationNotificationObserver::OnWillResignActive(id, SEL, NSNotification*)
    {
        DarwinLifecycleEvents::Bus::Broadcast(&DarwinLifecycleEvents::OnWillResignActive);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationNotificationObserver::OnDidResignActive(id, SEL, NSNotification*)
    {
        DarwinLifecycleEvents::Bus::Broadcast(&DarwinLifecycleEvents::OnDidResignActive);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationNotificationObserver::OnWillBecomeActive(id, SEL, NSNotification*)
    {
        DarwinLifecycleEvents::Bus::Broadcast(&DarwinLifecycleEvents::OnWillBecomeActive);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationNotificationObserver::OnDidBecomeActive(id, SEL, NSNotification*)
    {
        DarwinLifecycleEvents::Bus::Broadcast(&DarwinLifecycleEvents::OnDidBecomeActive);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationNotificationObserver::OnWillHide(id, SEL, NSNotification*)
    {
        DarwinLifecycleEvents::Bus::Broadcast(&DarwinLifecycleEvents::OnWillHide);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationNotificationObserver::OnDidHide(id, SEL, NSNotification*)
    {
        DarwinLifecycleEvents::Bus::Broadcast(&DarwinLifecycleEvents::OnDidHide);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationNotificationObserver::OnWillUnhide(id, SEL, NSNotification*)
    {
        DarwinLifecycleEvents::Bus::Broadcast(&DarwinLifecycleEvents::OnWillUnhide);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationNotificationObserver::OnDidUnhide(id, SEL, NSNotification*)
    {
        DarwinLifecycleEvents::Bus::Broadcast(&DarwinLifecycleEvents::OnDidUnhide);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationNotificationObserver::OnWillTerminate(id, SEL, NSNotification*)
    {
        DarwinLifecycleEvents::Bus::Broadcast(&DarwinLifecycleEvents::OnWillTerminate);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class MacApplicationImplFactory 
        : public Application::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<Application::Implementation> Create() override
        {
            return AZStd::make_unique<ApplicationMac>();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeApplicationImplementationFactory()
    {
        m_applicationImplFactory = AZStd::make_unique<MacApplicationImplFactory>();
        AZ::Interface<Application::ImplementationFactory>::Register(m_applicationImplFactory.get());
    }
} // namespace AzFramework
