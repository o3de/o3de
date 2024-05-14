
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Windowing/NativeWindow.h>
#include <AzFramework/Components/NativeUISystemComponent.h>

#if defined(CARBONATED)
#include <AzCore/Console/IConsole.h>
#endif

#include <UIKit/UIKit.h>
@class NSWindow;

namespace AzFramework
{
    class NativeWindowImpl_Ios final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Ios, AZ::SystemAllocator);
        NativeWindowImpl_Ios() = default;
        ~NativeWindowImpl_Ios() override;
        
        // NativeWindow::Implementation overrides...
        void InitWindow(const AZStd::string& title,
                        const WindowGeometry& geometry,
                        const WindowStyleMasks& styleMasks) override;
        NativeWindowHandle GetWindowHandle() const override;
        uint32_t GetDisplayRefreshRate() const override;
        
    private:
#if defined(CARBONATED)
        static bool IsLandscape();
#endif
        UIWindow* m_nativeWindow;
        uint32_t m_mainDisplayRefreshRate = 0;
    };
    
    NativeWindowImpl_Ios::~NativeWindowImpl_Ios()
    {
        if (m_nativeWindow)
        {
            [m_nativeWindow release];
            m_nativeWindow = nil;
        }
    }
    
    void NativeWindowImpl_Ios::InitWindow([[maybe_unused]]const AZStd::string& title,
                                          const WindowGeometry& geometry,
                                          [[maybe_unused]]const WindowStyleMasks& styleMasks)
    {
        CGRect screenBounds = [[UIScreen mainScreen] bounds];
        m_nativeWindow = [[UIWindow alloc] initWithFrame: screenBounds];
        [m_nativeWindow makeKeyAndVisible];

#if defined(CARBONATED)
        m_width = [[UIScreen mainScreen] nativeBounds].size.width;
        m_height = [[UIScreen mainScreen] nativeBounds].size.height;
        // The rectangle returned by nativeBounds is always in a portrait orientation.
        // If the app is landscape, width and height must be swapped.
        if (IsLandscape())
        {
            AZStd::swap(m_width, m_height);
        }
        
        uint32_t resolutionMode = 0;
        AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();
        if (console)
        {
            console->GetCvarValue("r_resolutionMode", resolutionMode);
        }
        if (resolutionMode == 1)
        {
            // Make sure that the window width does not exceed the given geometry width.
            if (geometry.m_width < m_width)
            {
                m_height = static_cast<uint32_t>((static_cast<float>(m_height) / m_width) * geometry.m_width);
                m_width = geometry.m_width;
            }
        }
        if (console)
        {
            // screen to world uses default viewport's size, which is based on these cvars, so we should keep them in sync
            console->PerformCommand(AZStd::string::format("r_width %d", m_width).c_str());
            console->PerformCommand(AZStd::string::format("r_height %d", m_height).c_str());
        }
#else
        m_width = geometry.m_width;
        m_height = geometry.m_height;
#endif
        m_mainDisplayRefreshRate = [[UIScreen mainScreen] maximumFramesPerSecond];
    }
    
    NativeWindowHandle NativeWindowImpl_Ios::GetWindowHandle() const
    {
        return m_nativeWindow;
    }
   
    uint32_t NativeWindowImpl_Ios::GetDisplayRefreshRate() const
    {
        return m_mainDisplayRefreshRate;
    }

#if defined(CARBONATED)
    bool NativeWindowImpl_Ios::IsLandscape()
    {
        UIInterfaceOrientation uiOrientation = UIInterfaceOrientationUnknown;

        UIWindow* foundWindow = nil;
        NSArray* windows = [[UIApplication sharedApplication] windows];
        for (UIWindow* window in windows)
        {
            if (window.isKeyWindow)
            {
                foundWindow = window;
                break;
            }
        }
        UIWindowScene* windowScene = foundWindow ? foundWindow.windowScene : nullptr;
        AZ_Assert(windowScene, "WindowScene is invalid");
        if (windowScene)
        {
            uiOrientation = windowScene.interfaceOrientation;
        }
        return uiOrientation == UIInterfaceOrientationLandscapeLeft || uiOrientation == UIInterfaceOrientationLandscapeRight;
    }
#endif

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class IosNativeWindowFactory 
        : public NativeWindow::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<NativeWindow::Implementation> Create() override
        {
            return AZStd::make_unique<NativeWindowImpl_Ios>();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeNativeWindowImplementationFactory()
    {
        m_nativeWindowImplFactory = AZStd::make_unique<IosNativeWindowFactory>();
        AZ::Interface<NativeWindow::ImplementationFactory>::Register(m_nativeWindowImplFactory.get());
    }
} // namespace AzFramework

