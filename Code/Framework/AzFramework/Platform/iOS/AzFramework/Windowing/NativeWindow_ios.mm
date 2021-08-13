
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Windowing/NativeWindow.h>

#include <UIKit/UIKit.h>
@class NSWindow;

namespace AzFramework
{
    class NativeWindowImpl_Ios final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Ios, AZ::SystemAllocator, 0);
        NativeWindowImpl_Ios() = default;
        ~NativeWindowImpl_Ios() override;
        
        // NativeWindow::Implementation overrides...
        void InitWindow(const AZStd::string& title,
                        const WindowGeometry& geometry,
                        const WindowStyleMasks& styleMasks) override;
        NativeWindowHandle GetWindowHandle() const override;
        uint32_t GetDisplayRefreshRate() const override;
        
    private:
        UIWindow* m_nativeWindow;
        uint32_t m_mainDisplayRefreshRate = 0;
    };
    
    NativeWindow::Implementation* NativeWindow::Implementation::Create()
    {
        return aznew NativeWindowImpl_Ios();
    }
    
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

        m_width = geometry.m_width;
        m_height = geometry.m_height;
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
} // namespace AzFramework

