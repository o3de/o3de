/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <atomic>
#include <chrono>
#include <thread>

#include <AzTest/AzTest.h>
#include <AzTest/Platform.h>
#include <AzCore/IO/SystemFile.h>   // for AZ_MAX_PATH_LEN
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/functional.h>

#include <AzCore/Android/AndroidEnv.h>
#include <AzCore/Android/Utils.h>
#include <AzCore/Android/JNI/JNI.h>
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Android/JNI/scoped_ref.h>
#include <AzCore/Component/TickBus.h>

#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>


#include "aztestrunner.h"

#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>

#include <android_native_app_glue.h>

#include <sys/resource.h>
#include <sys/types.h>

constexpr const char* s_logTag = "LMBR";

#define MAIN_EXIT_FAILURE(_appState, ...) \
    __android_log_print(ANDROID_LOG_INFO, s_logTag, "****************************************************************"); \
    __android_log_print(ANDROID_LOG_INFO, s_logTag, "STARTUP FAILURE - EXITING"); \
    __android_log_print(ANDROID_LOG_INFO, s_logTag, "REASON:"); \
    __android_log_print(ANDROID_LOG_INFO, s_logTag, __VA_ARGS__); \
    __android_log_print(ANDROID_LOG_INFO, s_logTag, "****************************************************************"); \
    _appState->userData = nullptr; \
    ANativeActivity_finish(_appState->activity); \
    while (_appState->destroyRequested == 0) { \
        g_eventDispatcher.PumpAllEvents(); \
    } \
    return;

namespace AzTestRunner
{
    void set_quiet_mode()
    {
    }

    const char* get_current_working_directory()
    {
        static char cwd_buffer[AZ_MAX_PATH_LEN] = { '\0' };

        [[maybe_unused]] AZ::Utils::ExecutablePathResult result = AZ::Utils::GetExecutableDirectory(cwd_buffer, AZ_ARRAY_SIZE(cwd_buffer));
        AZ_Assert(result == AZ::Utils::ExecutablePathResult::Success, "Error retrieving executable path");

        return static_cast<const char*>(cwd_buffer);
    }

    void pause_on_completion()
    {
    }
}

static AZ::EnvironmentInstance s_envInst = nullptr;
AZTEST_EXPORT AZ::EnvironmentInstance GetTestRunnerEnvironment()
{
    return s_envInst;
}

namespace
{
    class NativeEventDispatcher
        : public AzFramework::AndroidEventDispatcher
    {
    public:
        NativeEventDispatcher()
            : m_appState(nullptr)
        {
        }

        ~NativeEventDispatcher()
        {
        }

        void PumpAllEvents() override
        {
            bool continueRunning = true;
            while (continueRunning)
            {
                continueRunning = PumpEvents(&ALooper_pollAll);
            }
        }

        void PumpEventLoopOnce() override
        {
            PumpEvents(&ALooper_pollOnce);
        }

        void SetAppState(android_app* appState)
        {
            m_appState = appState;
        }

    private:
        // signature of ALooper_pollOnce and ALooper_pollAll -> int timeoutMillis, int* outFd, int* outEvents, void** outData
        typedef int (*EventPumpFunc)(int, int*, int*, void**);

        bool PumpEvents(EventPumpFunc looperFunc)
        {
            if (!m_appState)
            {
                return false;
            }

            int events = 0;
            android_poll_source* source = nullptr;
            const AZ::Android::AndroidEnv* androidEnv = AZ::Android::AndroidEnv::Get();

            // when timeout is negative, the function will block until an event is received
            const int result = looperFunc(androidEnv->IsRunning() ? 0 : -1, nullptr, &events, reinterpret_cast<void**>(&source));

            // the value returned from the looper poll func is either:
            // 1. the identifier associated with the event source (>= 0) and has event data that needs to be processed manually
            // 2. an ALOOPER_POLL_* enum (< 0) indicating there is no data to be processed due to error or callback(s) registered
            //    with the event source were called
            const bool validIdentifier = (result >= 0);
            if (validIdentifier && source)
            {
                source->process(m_appState, source);
            }

            const bool destroyRequested = (m_appState->destroyRequested != 0);

            return (validIdentifier && !destroyRequested);
        }

        android_app* m_appState;
    };


    NativeEventDispatcher g_eventDispatcher;
    bool g_windowInitialized = false;

    int32_t HandleInputEvents(android_app* app, AInputEvent* event)
    {
        AzFramework::RawInputNotificationBusAndroid::Broadcast(&AzFramework::RawInputNotificationsAndroid::OnRawInputEvent, event);
        return 0;
    }

    void HandleApplicationLifecycleEvents(android_app* appState, int32_t command)
    {
        AZ::Android::AndroidEnv* androidEnv = static_cast<AZ::Android::AndroidEnv*>(appState->userData);
        if (!androidEnv)
        {
            return;
        }

        switch (command)
        {
            case APP_CMD_GAINED_FOCUS:
            {
                AzFramework::AndroidLifecycleEvents::Bus::Broadcast(
                    &AzFramework::AndroidLifecycleEvents::Bus::Events::OnGainedFocus);
            }
            break;

            case APP_CMD_LOST_FOCUS:
            {
                AzFramework::AndroidLifecycleEvents::Bus::Broadcast(
                    &AzFramework::AndroidLifecycleEvents::Bus::Events::OnLostFocus);
            }
            break;

            case APP_CMD_PAUSE:
            {
                AzFramework::AndroidLifecycleEvents::Bus::Broadcast(
                    &AzFramework::AndroidLifecycleEvents::Bus::Events::OnPause);
                androidEnv->SetIsRunning(false);
            }
            break;

            case APP_CMD_RESUME:
            {
                androidEnv->SetIsRunning(true);
                AzFramework::AndroidLifecycleEvents::Bus::Broadcast(
                    &AzFramework::AndroidLifecycleEvents::Bus::Events::OnResume);
            }
            break;

            case APP_CMD_DESTROY:
            {
                AzFramework::AndroidLifecycleEvents::Bus::Broadcast(
                    &AzFramework::AndroidLifecycleEvents::Bus::Events::OnDestroy);
            }
            break;

            case APP_CMD_INIT_WINDOW:
            {
                g_windowInitialized = true;
                androidEnv->SetWindow(appState->window);

                AzFramework::AndroidLifecycleEvents::Bus::Broadcast(
                    &AzFramework::AndroidLifecycleEvents::Bus::Events::OnWindowInit);
            }
            break;

            case APP_CMD_TERM_WINDOW:
            {
                AzFramework::AndroidLifecycleEvents::Bus::Broadcast(
                    &AzFramework::AndroidLifecycleEvents::Bus::Events::OnWindowDestroy);

                androidEnv->SetWindow(nullptr);
            }
            break;

            case APP_CMD_LOW_MEMORY:
            {
                AzFramework::AndroidLifecycleEvents::Bus::Broadcast(
                    &AzFramework::AndroidLifecycleEvents::Bus::Events::OnLowMemory);
            }
            break;

            case APP_CMD_CONFIG_CHANGED:
            {
                androidEnv->UpdateConfiguration();
            }
            break;

            case APP_CMD_WINDOW_REDRAW_NEEDED:
            {
                AzFramework::AndroidLifecycleEvents::Bus::Broadcast(
                    &AzFramework::AndroidLifecycleEvents::Bus::Events::OnWindowRedrawNeeded);
            }
            break;
        }
    }

    void OnWindowRedrawNeeded(ANativeActivity* activity, ANativeWindow* rect)
    {
        android_app* app = static_cast<android_app*>(activity->instance);
        int8_t cmd = APP_CMD_WINDOW_REDRAW_NEEDED;
        if (write(app->msgwrite, &cmd, sizeof(cmd)) != sizeof(cmd))
        {
            __android_log_print(ANDROID_LOG_ERROR, s_logTag, "Failure writing android_app cmd: %s\n", strerror(errno));
        }
    }
}

// In order to read the logcat from adb, stdout and stderr needs to be redirected custom handles and we will need
// to spawn a thread to read from the custom handles and pass the output of those streams through __android_log_print
// to a specia 'LMBR' tag
static int s_pfd[2];
static std::atomic_bool s_testRunComplete { false };
static void *thread_logger_func(void*)
{
    ssize_t readSize;
    char logBuffer[256] = {'\0'};
    while (!s_testRunComplete)
    {
        readSize = read(s_pfd[0], logBuffer, sizeof(logBuffer)-1);
        if (readSize>0) {
            // Trim the tailing return character
            if (logBuffer[readSize - 1] == '\n') {
                --readSize;
            }
            logBuffer[readSize] = '\0';
            ((void) __android_log_print(ANDROID_LOG_INFO, s_logTag, "%s", logBuffer));
        }
    }
    return 0;
}

constexpr const char* s_defaultAppName = "AzTestRunner";
constexpr size_t s_maxArgCount = 8;
constexpr size_t s_maxArgLength = 64;

void android_main(android_app* appState)
{
    // Adding a start up banner so you can see when the test runner is starting up in amongst the logcat spam
    __android_log_print(ANDROID_LOG_INFO, s_logTag, "****************************************************************");
    __android_log_print(ANDROID_LOG_INFO, s_logTag, " Starting %s", s_defaultAppName);
    __android_log_print(ANDROID_LOG_INFO, s_logTag, "****************************************************************");

    // setup the android environment
    AZ::AllocatorInstance<AZ::OSAllocator>::Create();
    s_envInst = AZ::Environment::GetInstance();

    // setup the system command handler which are guaranteed to be called on the same
    // thread the events are pumped
    appState->onAppCmd = HandleApplicationLifecycleEvents;
    appState->onInputEvent = HandleInputEvents;
    g_eventDispatcher.SetAppState(appState);

    // This callback will notify us when the orientation of the device changes.
    // While Android does have an onNativeWindowResized callback, it is never called in android_native_app_glue when the window size changes.
    // The onNativeConfigChanged callback is called too early(before the window size has changed), so we won't have the correct window size at that point.
    appState->activity->callbacks->onNativeWindowRedrawNeeded = OnWindowRedrawNeeded;

    {
        AZ::Android::AndroidEnv::Descriptor descriptor;

        descriptor.m_jvm = appState->activity->vm;
        descriptor.m_activityRef = appState->activity->clazz;
        descriptor.m_assetManager = appState->activity->assetManager;

        descriptor.m_configuration = appState->config;

        descriptor.m_appPrivateStoragePath = appState->activity->internalDataPath;
        descriptor.m_appPublicStoragePath = appState->activity->externalDataPath;
        descriptor.m_obbStoragePath = appState->activity->obbPath;

        if (!AZ::Android::AndroidEnv::Create(descriptor))
        {
            AZ::Android::AndroidEnv::Destroy();
            AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
            MAIN_EXIT_FAILURE(appState, "Failed to create the AndroidEnv");
        }

        AZ::Android::AndroidEnv* androidEnv = AZ::Android::AndroidEnv::Get();
        appState->userData = androidEnv;
        androidEnv->SetIsRunning(true);
    }

    g_eventDispatcher.PumpAllEvents();

    // Prepare the command line args to pass to main
    char commandLineArgs[s_maxArgCount][s_maxArgLength] = { {'\0'} };
    size_t currentCommandLineIndex = 0;

    // Always add the app as the first arg to mimic the way other platforms start with the executable name.
    const char* packageName = AZ::Android::Utils::GetPackageName();
    const char* appName = (packageName) ? packageName : s_defaultAppName;
    size_t appNameLen = strlen(appName);

    azstrncpy(commandLineArgs[currentCommandLineIndex++], s_maxArgLength, appName, appNameLen + 1);

    // "activityObject" and "intent" need to be destroyed before we call Destroy() on the allocator to ensure graceful shutdown.
    {
        // Get the string extras and pass them along as cmd line params
        AZ::Android::JNI::Internal::Object<AZ::OSAllocator> activityObject(AZ::Android::JNI::GetEnv()->GetObjectClass(appState->activity->clazz), appState->activity->clazz);

        activityObject.RegisterMethod("getIntent", "()Landroid/content/Intent;");
        jobject intent = activityObject.InvokeObjectMethod<jobject>("getIntent");

        AZ::Android::JNI::Internal::Object<AZ::OSAllocator> intentObject(AZ::Android::JNI::GetEnv()->GetObjectClass(intent), intent);
        intentObject.RegisterMethod("getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");
        intentObject.RegisterMethod("getExtras", "()Landroid/os/Bundle;");
        jobject extras = intentObject.InvokeObjectMethod<jobject>("getExtras");

        int start_delay = 0;

        if (extras)
        {
            // Get the set of keys
            AZ::Android::JNI::Internal::Object<AZ::OSAllocator> extrasObject(AZ::Android::JNI::GetEnv()->GetObjectClass(extras), extras);
            extrasObject.RegisterMethod("keySet", "()Ljava/util/Set;");
            jobject extrasKeySet = extrasObject.InvokeObjectMethod<jobject>("keySet");

            // get the array of string objects
            AZ::Android::JNI::Internal::Object<AZ::OSAllocator> extrasKeySetObject(AZ::Android::JNI::GetEnv()->GetObjectClass(extrasKeySet), extrasKeySet);
            extrasKeySetObject.RegisterMethod("toArray", "()[Ljava/lang/Object;");
            jobjectArray extrasKeySetArray = extrasKeySetObject.InvokeObjectMethod<jobjectArray>("toArray");

            int extrasKeySetArraySize = AZ::Android::JNI::GetEnv()->GetArrayLength(extrasKeySetArray);
            for (int x = 0; x < extrasKeySetArraySize; x++)
            {
                jstring keyObject = static_cast<jstring>(AZ::Android::JNI::GetEnv()->GetObjectArrayElement(extrasKeySetArray, x));
                AZ::OSString value = intentObject.InvokeStringMethod("getStringExtra", keyObject);

                const char* keyChars = AZ::Android::JNI::GetEnv()->GetStringUTFChars(keyObject, 0);

                if (azstricmp("startdelay", keyChars) == 0)
                {
                    start_delay = strtol(value.c_str(), nullptr, 10);
                }
                else if (azstricmp("gtest_filter", keyChars) == 0)
                {
                    azsnprintf(commandLineArgs[currentCommandLineIndex++], s_maxArgLength, "--gtest_filter=%.*s", aznumeric_cast<int>(value.size()), value.c_str());
                }
                else
                {
                    azstrncpy(commandLineArgs[currentCommandLineIndex++], s_maxArgLength, keyChars, strlen(keyChars) + 1);
                    azstrncpy(commandLineArgs[currentCommandLineIndex++], s_maxArgLength, value.c_str(), value.length() + 1);
                }
                AZ::Android::JNI::GetEnv()->ReleaseStringUTFChars(keyObject, keyChars);
            }
        }

        if (start_delay > 0)
        {
            std::this_thread::sleep_for(std::chrono::seconds(start_delay));
        }
    }

    char* argv[s_maxArgCount];
    for (size_t index = 0; index < s_maxArgCount; index++)
    {
        argv[index] = commandLineArgs[index];
    }

    // Redirect stdout and stderr to custom handles and prepare the thread to read from the custom handles
    pthread_t log_thread;

    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    pipe(s_pfd);
    dup2(s_pfd[1], 1);
    dup2(s_pfd[1], 2);

    if (pthread_create(&log_thread, 0, thread_logger_func, 0) == -1)
    {
        ((void)__android_log_print(ANDROID_LOG_INFO, s_logTag, "[FAILURE] Unable to spawn logging thread"));
        return;
    }

    // Execute the unit test main
    int result = AzTestRunner::wrapped_main(aznumeric_cast<int>(currentCommandLineIndex), argv);

    g_eventDispatcher.PumpAllEvents();

    s_testRunComplete = true;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (result == 0)
    {
        ((void)__android_log_print(ANDROID_LOG_INFO, s_logTag, "[SUCCESS]"));
    }
    else
    {
        ((void)__android_log_print(ANDROID_LOG_INFO, s_logTag, "[FAILURE]"));
    }


    AZ::Android::AndroidEnv::Destroy();

    AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
}
