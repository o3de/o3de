/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Windowing/NativeWindow.h>

using namespace AZ;
using namespace AzFramework;

namespace UnitTest
{

    class NativeWindowTest : public LeakDetectionFixture
    {
    public:
        NativeWindowTest()
            : LeakDetectionFixture()
        {
            m_nativeUiComponent = AZStd::make_unique<AzFramework::NativeUISystemComponent>();
        }
    private:
        AZStd::unique_ptr<AzFramework::NativeUISystemComponent> m_nativeUiComponent;
    };

    class NativeWindowListener
        : public WindowNotificationBus::Handler
    {
    public:
        NativeWindowListener(NativeWindowHandle windowHandle)
            : m_windowHandle(windowHandle)
        {
            AzFramework::WindowNotificationBus::Handler::BusConnect(m_windowHandle);
        }

        ~NativeWindowListener() override
        {
            AzFramework::WindowNotificationBus::Handler::BusDisconnect(m_windowHandle);
        }

        // WindowNotificationBus::Handler overrides...
        void OnWindowResized(uint32_t width, uint32_t height) override
        {
            AZ_UNUSED(width);
            AZ_UNUSED(height);
            m_wasOnWindowResizedReceived = true;
        }
        void OnWindowClosed() override
        {
            m_wasOnWindowClosedReceived = true;
        }

        NativeWindowHandle m_windowHandle = nullptr;
        bool m_wasOnWindowResizedReceived = false;
        bool m_wasOnWindowClosedReceived = false;
    };

    // Test that a window can be created and will start in the non-active state
#if AZ_TRAIT_DISABLE_FAILED_NATIVE_WINDOWS_TESTS
    TEST_F(NativeWindowTest, DISABLED_CreateWindow)
#else
    TEST_F(NativeWindowTest, CreateWindow)
#endif // AZ_TRAIT_DISABLE_FAILED_NATIVE_WINDOWS_TESTS
    {
        const uint32_t PosX = 0;
        const uint32_t PosY = 0;
        const uint32_t Width = 1280;
        const uint32_t Height = 720;

        WindowGeometry geometry(PosX, PosY, Width, Height);

        AZStd::unique_ptr<NativeWindow> nativeWindow = AZStd::make_unique<AzFramework::NativeWindow>(
            "Test Window", geometry);

        EXPECT_FALSE(nativeWindow == nullptr) << "NativeWindow was not allocated correctly.";   

        EXPECT_FALSE(nativeWindow->IsActive()) << "NativeWindow was in active state after construction.";   
    }

    // Test that a window can be created and activated
#if AZ_TRAIT_DISABLE_FAILED_NATIVE_WINDOWS_TESTS
    TEST_F(NativeWindowTest, DISABLED_ActivateWindow)
#else
    TEST_F(NativeWindowTest, ActivateWindow)
#endif // AZ_TRAIT_DISABLE_FAILED_NATIVE_WINDOWS_TESTS
    {
        const uint32_t PosX = 0;
        const uint32_t PosY = 0;
        const uint32_t Width = 1280;
        const uint32_t Height = 720;

        WindowGeometry geometry(PosX, PosY, Width, Height);

        AZStd::unique_ptr<NativeWindow> nativeWindow = AZStd::make_unique<AzFramework::NativeWindow>(
            "Test Window", geometry);

        EXPECT_FALSE(nativeWindow == nullptr) << "NativeWindow was not allocated correctly.";   

        nativeWindow->Activate();

        EXPECT_TRUE(nativeWindow->IsActive()) << "NativeWindow was in inactive state after Activate called.";

        // The window will get deactivated automatically when the NativeWindow is destructed
    }

    // Test that a window can be created, activated and deactivated
#if AZ_TRAIT_DISABLE_FAILED_NATIVE_WINDOWS_TESTS
    TEST_F(NativeWindowTest, DISABLED_DectivateWindow)
#else
    TEST_F(NativeWindowTest, DectivateWindow)
#endif // AZ_TRAIT_DISABLE_FAILED_NATIVE_WINDOWS_TESTS
    {
        const uint32_t PosX = 0;
        const uint32_t PosY = 0;
        const uint32_t Width = 1280;
        const uint32_t Height = 720;

        WindowGeometry geometry(PosX, PosY, Width, Height);

        AZStd::unique_ptr<NativeWindow> nativeWindow = AZStd::make_unique<AzFramework::NativeWindow>(
            "Test Window", geometry);

        EXPECT_FALSE(nativeWindow == nullptr) << "NativeWindow was not allocated correctly.";   

        nativeWindow->Activate();

        EXPECT_TRUE(nativeWindow->IsActive()) << "NativeWindow was in inactive state after Activate called.";

        nativeWindow->Deactivate();

        EXPECT_FALSE(nativeWindow->IsActive()) << "NativeWindow was in active state after Deactivate called.";
    }

    // Test that a window responds to the GetClientAreaSize bus request
#if AZ_TRAIT_DISABLE_FAILED_NATIVE_WINDOWS_TESTS
    TEST_F(NativeWindowTest, DISABLED_GetClientAreaSize)
#else
    TEST_F(NativeWindowTest, GetClientAreaSize)
#endif // AZ_TRAIT_DISABLE_FAILED_NATIVE_WINDOWS_TESTS
    {
        const uint32_t PosX = 0;
        const uint32_t PosY = 0;
        const uint32_t Width = 1280;
        const uint32_t Height = 720;

        WindowGeometry geometry(PosX, PosY, Width, Height);

        AZStd::unique_ptr<NativeWindow> nativeWindow = AZStd::make_unique<AzFramework::NativeWindow>(
            "Test Window", geometry);

        EXPECT_FALSE(nativeWindow == nullptr) << "NativeWindow was not allocated correctly.";   

        nativeWindow->Activate();

        EXPECT_TRUE(nativeWindow->IsActive()) << "NativeWindow was in inactive state after activation.";

        NativeWindowHandle windowHandle = nativeWindow->GetWindowHandle();

        WindowSize windowSize;
        WindowRequestBus::EventResult(windowSize, windowHandle, &WindowRequestBus::Events::GetClientAreaSize);

        EXPECT_TRUE(windowSize.m_width > 0) << "NativeWindow was created with wrong geometry.";   
        EXPECT_TRUE(windowSize.m_height > 0) << "NativeWindow was created with wrong geometry.";   
        EXPECT_TRUE(windowSize.m_width <= geometry.m_width) << "NativeWindow was created with wrong geometry.";   
        EXPECT_TRUE(windowSize.m_height <= geometry.m_height) << "NativeWindow was created with wrong geometry.";   
    }

    // Test that a window sends the correct notifications
#if AZ_TRAIT_DISABLE_FAILED_NATIVE_WINDOWS_TESTS
    TEST_F(NativeWindowTest, DISABLED_Notifications)
#else
    TEST_F(NativeWindowTest, Notifications)
#endif // AZ_TRAIT_DISABLE_FAILED_NATIVE_WINDOWS_TESTS
    {
        const uint32_t PosX = 0;
        const uint32_t PosY = 0;
        const uint32_t Width = 1280;
        const uint32_t Height = 720;

        WindowGeometry geometry(PosX, PosY, Width, Height);

        AZStd::unique_ptr<NativeWindow> nativeWindow = AZStd::make_unique<AzFramework::NativeWindow>(
            "Test Window", geometry);

        EXPECT_FALSE(nativeWindow == nullptr) << "NativeWindow was not allocated correctly.";   

        NativeWindowHandle windowHandle = nativeWindow->GetWindowHandle();

        EXPECT_FALSE(windowHandle == nullptr) << "NativeWindow has invalid handle.";   

        NativeWindowListener listener(windowHandle);

        EXPECT_FALSE(listener.m_wasOnWindowResizedReceived) << "No notifications should be received yet.";   
        EXPECT_FALSE(listener.m_wasOnWindowClosedReceived) << "No notifications should be received yet.";   

        nativeWindow->Activate();

        WindowSize windowSize;
        WindowRequestBus::EventResult(windowSize, windowHandle, &WindowRequestBus::Events::GetClientAreaSize);
        bool windowSizeChanged = windowSize.m_width != geometry.m_width || windowSize.m_height != geometry.m_height;

        EXPECT_TRUE(nativeWindow->IsActive()) << "NativeWindow was in inactive state after activation.";

        EXPECT_TRUE(listener.m_wasOnWindowResizedReceived || !windowSizeChanged) << "Expected the OnWindowResized notification to have occurred.";
        EXPECT_FALSE(listener.m_wasOnWindowClosedReceived) << "Did not expect the OnWindowClosed notification to have occurred.";
        listener.m_wasOnWindowResizedReceived = false;

        nativeWindow->Deactivate();

        EXPECT_FALSE(nativeWindow->IsActive()) << "NativeWindow was in active state after deactivation.";

        EXPECT_FALSE(listener.m_wasOnWindowResizedReceived) << "Did not expect the OnWindowResized notification to have occurred.";   
        EXPECT_TRUE(listener.m_wasOnWindowClosedReceived) << "Expected the OnWindowClosed notification to have occurred.";
    }

} // namespace UnitTest
