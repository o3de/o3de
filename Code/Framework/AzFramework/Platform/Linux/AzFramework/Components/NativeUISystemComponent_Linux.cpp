/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/NativeUISystemComponent.h>

#include <sys/resource.h>

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#include <AzFramework/XcbApplication.h>
#include <AzFramework/XcbInputDeviceKeyboard.h>
#include <AzFramework/XcbInputDeviceMouse.h>
#include <AzFramework/XcbNativeWindow.h>
#endif

constexpr rlim_t g_minimumOpenFileHandles = 65536L;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    struct LinuxApplicationImplFactory 
        : public Application::ImplementationFactory
    {
        AZStd::unique_ptr<Application::Implementation> Create() override
        {
            // The default open file limit for processes may not be enough for O3DE applications. 
            // We will need to increase to the recommended value if the current open file limit
            // is not sufficient.
            rlimit currentLimit;
            int get_limit_result = getrlimit(RLIMIT_NOFILE, &currentLimit);
            AZ_Warning("Application", get_limit_result == 0, "Unable to read current ulimit open file limits");
            // non-privileged systems may not update the maximum hard cap, only the current limit and it cannot exceed the max:
            if ((get_limit_result == 0) && (currentLimit.rlim_cur < g_minimumOpenFileHandles) && ( currentLimit.rlim_max > 0))
            {
                rlimit newLimit;
                newLimit.rlim_cur = AZ::GetMin(g_minimumOpenFileHandles, currentLimit.rlim_max); // Soft Limit
                newLimit.rlim_max = currentLimit.rlim_max; // Hard Limit
                AZ_WarningOnce("Init", newLimit.rlim_cur >= g_minimumOpenFileHandles, "System maximum ulimit open file handles (%i) is less than required amount (%i), an admin will have to update the max # of open files allowed on this system.", (int)currentLimit.rlim_max, (int)g_minimumOpenFileHandles);
            
                [[maybe_unused]] int set_limit_result = setrlimit(RLIMIT_NOFILE, &newLimit);
                AZ_Assert(set_limit_result == 0, "Unable to update open file limits");
            }
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
            return AZStd::make_unique<XcbApplication>();
#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
            #error "Linux Window Manager Wayland not supported."
            return nullptr;
#else
            #error "Linux Window Manager not recognized."
            return nullptr;
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        }
    };

    struct LinuxDeviceKeyboardImplFactory
        : public InputDeviceKeyboard::ImplementationFactory
    {
        AZStd::unique_ptr<InputDeviceKeyboard::Implementation> Create(InputDeviceKeyboard& inputDevice) override
        {
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
            return AZStd::make_unique<XcbInputDeviceKeyboard>(inputDevice);
#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
            #error "Linux Window Manager Wayland not supported."
            return nullptr;
#else
            #error "Linux Window Manager not recognized."
            return nullptr;
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        }
    };

    struct LinuxDeviceMouseImplFactory
        : public InputDeviceMouse::ImplementationFactory
    {
        AZStd::unique_ptr<InputDeviceMouse::Implementation> Create(InputDeviceMouse& inputDevice) override
        {
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
            
            return AZStd::unique_ptr<InputDeviceMouse::Implementation>(XcbInputDeviceMouse::Create(inputDevice));
#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
#error "Linux Window Manager Wayland not supported."
            return nullptr;
#else
#error "Linux Window Manager not recognized."
            return nullptr;
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        }
    };

    struct LinuxNativeWindowFactory 
        : public NativeWindow::ImplementationFactory
    {
        AZStd::unique_ptr<NativeWindow::Implementation> Create() override
        {
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
            return AZStd::make_unique<XcbNativeWindow>();
#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
            #error "Linux Window Manager Wayland not supported."
            return nullptr;
#else
            #error "Linux Window Manager not recognized."
            return nullptr;
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        }
    };
    
    void NativeUISystemComponent::InitializeApplicationImplementationFactory()
    {
        m_applicationImplFactory = AZStd::make_unique<LinuxApplicationImplFactory>();
        AZ::Interface<Application::ImplementationFactory>::Register(m_applicationImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceGamepadImplentationFactory()
    {
        // Gamepad not supported on Linux
    }

    void NativeUISystemComponent::InitializeDeviceKeyboardImplementationFactory()
    {
        m_deviceKeyboardImplFactory = AZStd::make_unique<LinuxDeviceKeyboardImplFactory>();
        AZ::Interface<InputDeviceKeyboard::ImplementationFactory>::Register(m_deviceKeyboardImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceMotionImplentationFactory()
    {
        // Motion Input not supported on Linux
    }

    void NativeUISystemComponent::InitializeDeviceMouseImplentationFactory()
    {
        m_deviceMouseImplFactory = AZStd::make_unique<LinuxDeviceMouseImplFactory>();
        AZ::Interface<InputDeviceMouse::ImplementationFactory>::Register(m_deviceMouseImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceTouchImplentationFactory()
    {
        // Touch Input not supported on Linux
    }

    void NativeUISystemComponent::InitializeDeviceVirtualKeyboardImplentationFactory()
    {
        // Virtual Keyboard not supported on Linux
    }

    void NativeUISystemComponent::InitializeNativeWindowImplementationFactory()
    {
        m_nativeWindowImplFactory = AZStd::make_unique<LinuxNativeWindowFactory>();
        AZ::Interface<NativeWindow::ImplementationFactory>::Register(m_nativeWindowImplFactory.get());
    }
} // namespace AzFramework
