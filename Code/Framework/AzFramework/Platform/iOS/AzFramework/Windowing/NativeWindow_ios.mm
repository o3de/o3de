
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Windowing/NativeWindow.h>
#include <AzFramework/Components/NativeUISystemComponent.h>

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
        void InitWindowInternal(const AZStd::string& title,
                        const WindowGeometry& geometry,
                        const WindowStyleMasks& styleMasks) override;
        NativeWindowHandle GetWindowHandle() const override;
        uint32_t GetDisplayRefreshRate() const override;
        
    private:
        bool IsLandscape() const;

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
    
    void NativeWindowImpl_Ios::InitWindowInternal([[maybe_unused]]const AZStd::string& title,
                                          const WindowGeometry& geometry,
                                          [[maybe_unused]]const WindowStyleMasks& styleMasks)
    {
        CGRect screenBounds = [[UIScreen mainScreen] bounds];
        m_nativeWindow = [[UIWindow alloc] initWithFrame: screenBounds];
        [m_nativeWindow makeKeyAndVisible];
        
        // On iOS we don't set the window size, we use the OS window's' size
        m_width = [[UIScreen mainScreen] nativeBounds].size.width;
        m_height = [[UIScreen mainScreen] nativeBounds].size.height;

        // The rectangle returned by nativeBounds is always in a portrait orientation.
        // If the app is landscape, width and height must be swapped.
        if (IsLandscape())
        {
            AZStd::swap(m_width, m_height);
        }
        
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

    bool NativeWindowImpl_Ios::IsLandscape() const
    {
        UIInterfaceOrientation uiOrientation = m_nativeWindow.windowScene.interfaceOrientation;
        return uiOrientation == UIInterfaceOrientationLandscapeLeft || uiOrientation == UIInterfaceOrientationLandscapeRight;
    }

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

