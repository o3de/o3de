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
#include <AzFramework/Thermal/ThermalInfo_Android.h>

#include <AzCore/Android/AndroidEnv.h>
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Android/JNI/scoped_ref.h>
#include <AzCore/Android/Utils.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/condition_variable.h>

#include <android/input.h>
#include <android/keycodes.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    namespace
    {
        class PermissionRequestResultNotification
            : public AZ::EBusTraits
        {
        public:
            using Bus = AZ::EBus<PermissionRequestResultNotification>;

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            virtual ~PermissionRequestResultNotification() = default;

            virtual void OnRequestPermissionsResult(bool granted) { AZ_UNUSED(granted); };
        };


        void JNI_OnRequestPermissionsResult(JNIEnv* env, jobject obj, bool granted)
        {
            AzFramework::PermissionRequestResultNotification::Bus::Broadcast(&AzFramework::PermissionRequestResultNotification::OnRequestPermissionsResult, granted);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ApplicationAndroid
        : public Application::Implementation
        , public AndroidLifecycleEvents::Bus::Handler
        , public AndroidAppRequests::Bus::Handler
        , public PermissionRequestResultNotification::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CLASS_ALLOCATOR(ApplicationAndroid, AZ::SystemAllocator);
        ApplicationAndroid();
        ~ApplicationAndroid() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AndroidAppRequests
        void SetEventDispatcher(AndroidEventDispatcher* eventDispatcher) override;
        bool RequestPermission(const AZStd::string& permission, const AZStd::string& rationale) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AndroidLifecycleEvents
        void OnLostFocus() override;
        void OnGainedFocus() override;
        void OnPause() override;
        void OnResume() override;
        void OnDestroy() override;
        void OnLowMemory() override;
        void OnWindowInit() override;
        void OnWindowDestroy() override;
        void OnWindowRedrawNeeded() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // PermissionRequestResultNotification
        void OnRequestPermissionsResult(bool granted) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Application::Implementation
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;

    private:
        AndroidEventDispatcher* m_eventDispatcher;
        ApplicationLifecycleEvents::Event m_lastEvent;
#if !defined(AZ_RELEASE_BUILD)
        AZStd::unique_ptr<ThermalInfoHandler> m_thermalInfoHandler;
#endif

        AZStd::atomic<bool> m_requestResponseReceived;
        AZStd::unique_ptr<AZ::Android::JNI::Object> m_lumberyardActivity;
        AZStd::condition_variable m_conditionVar;
        AZStd::mutex m_mutex;
        bool m_permissionGranted;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationAndroid::ApplicationAndroid()
        : m_eventDispatcher(nullptr)
        , m_lastEvent(ApplicationLifecycleEvents::Event::None)
    {
        m_lumberyardActivity.reset(aznew AZ::Android::JNI::Object(AZ::Android::Utils::GetActivityClassRef(), AZ::Android::Utils::GetActivityRef()));

        m_lumberyardActivity->RegisterNativeMethods(
        { { "nativeOnRequestPermissionsResult", "(Z)V", (void*)JNI_OnRequestPermissionsResult } }
        );

        m_lumberyardActivity->RegisterMethod("RequestPermission", "(Ljava/lang/String;Ljava/lang/String;)V");

        AndroidLifecycleEvents::Bus::Handler::BusConnect();
        AndroidAppRequests::Bus::Handler::BusConnect();
        PermissionRequestResultNotification::Bus::Handler::BusConnect();

#if !defined(AZ_RELEASE_BUILD)
        m_thermalInfoHandler = AZStd::make_unique<ThermalInfoAndroidHandler>();
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationAndroid::~ApplicationAndroid()
    {
        m_lumberyardActivity.reset();
        PermissionRequestResultNotification::Bus::Handler::BusDisconnect();
        AndroidAppRequests::Bus::Handler::BusDisconnect();
        AndroidLifecycleEvents::Bus::Handler::BusDisconnect();
        m_conditionVar.notify_all();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::SetEventDispatcher(AndroidEventDispatcher* eventDispatcher)
    {
        AZ_Assert(!m_eventDispatcher, "Duplicate call to setting the Android event dispatcher!");
        m_eventDispatcher = eventDispatcher;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool ApplicationAndroid::RequestPermission(const AZStd::string& permission, const AZStd::string& rationale)
    {
        AZ::Android::JNI::scoped_ref<jstring> permissionString(AZ::Android::JNI::ConvertStringToJstring(permission));
        AZ::Android::JNI::scoped_ref<jstring> permissionRationaleString(AZ::Android::JNI::ConvertStringToJstring(rationale));

        m_requestResponseReceived = false;
        m_permissionGranted = false;

        m_lumberyardActivity->InvokeVoidMethod("RequestPermission", permissionString.get(), permissionRationaleString.get());

        bool looperExistsForThread = (ALooper_forThread() != nullptr);

        // Make sure a looper exists for thread before pumping events.
        // For threads that are not the main thread, we just block and
        // the events will be pumped by the main thread and unblock this
        // thread when the user responds.
        if (looperExistsForThread)
        {
            while (!m_requestResponseReceived.load())
            {
                PumpSystemEventLoopOnce();
            }
        }
        else
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
            m_conditionVar.wait(lock, [&] { return m_requestResponseReceived.load(); });
        }

        return m_permissionGranted;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnLostFocus()
    {
        ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnApplicationConstrained, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Constrain;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnGainedFocus()
    {
        ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnApplicationUnconstrained, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Unconstrain;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnPause()
    {

        ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnApplicationSuspended, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Suspend;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnResume()
    {
        ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnApplicationResumed, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Resume;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnDestroy()
    {
        ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnMobileApplicationWillTerminate);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnLowMemory()
    {
        ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnMobileApplicationLowMemoryWarning);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnWindowInit()
    {
        ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnApplicationWindowCreated);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnWindowDestroy()
    {
        ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnApplicationWindowDestroy);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnWindowRedrawNeeded()
    {
        ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::Bus::Events::OnApplicationWindowRedrawNeeded);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnRequestPermissionsResult(bool granted)
    {
        m_permissionGranted = granted;
        m_requestResponseReceived = true;
        m_conditionVar.notify_all();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::PumpSystemEventLoopOnce()
    {
        AZ_ErrorOnce("ApplicationAndroid", m_eventDispatcher, "The Android event dispatcher is not valid, unable to pump the event loop properly.");
        if (m_eventDispatcher)
        {
            m_eventDispatcher->PumpEventLoopOnce();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::PumpSystemEventLoopUntilEmpty()
    {
        AZ_ErrorOnce("ApplicationAndroid", m_eventDispatcher, "The Android event dispatcher is not valid, unable to pump the event loop properly.");
        if (m_eventDispatcher)
        {
            m_eventDispatcher->PumpAllEvents();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class AndroidApplicationImplFactory 
        : public Application::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<Application::Implementation> Create() override
        {
            return AZStd::make_unique<ApplicationAndroid>();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeApplicationImplementationFactory()
    {
        m_applicationImplFactory = AZStd::make_unique<AndroidApplicationImplFactory>();
        AZ::Interface<Application::ImplementationFactory>::Register(m_applicationImplFactory.get());
    }
} // namespace AzFramework
