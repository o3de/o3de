
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Windowing/NativeWindow.h>

#include <AppKit/AppKit.h>

@class NSWindow;

namespace AzFramework
{
    class NativeWindowImpl_Darwin final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Darwin, AZ::SystemAllocator);
        NativeWindowImpl_Darwin() = default;
        ~NativeWindowImpl_Darwin() override;
        
        // NativeWindow::Implementation overrides...
        void InitWindowInternal(const AZStd::string& title,
                        const WindowGeometry& geometry,
                        const WindowStyleMasks& styleMasks) override;
        NativeWindowHandle GetWindowHandle() const override;
        void SetWindowTitle(const AZStd::string& title) override;

        void ResizeClientArea(WindowSize clientAreaSize, const WindowPosOptions& options) override;
        bool SupportsClientAreaResize() const override { return true; }
        bool GetFullScreenState() const override;
        void SetFullScreenState(bool fullScreenState) override;
        bool CanToggleFullScreenState() const override { return true; }
        uint32_t GetDisplayRefreshRate() const override;

    private:
        static NSWindowStyleMask ConvertToNSWindowStyleMask(const WindowStyleMasks& styleMasks);

        NSWindow* m_nativeWindow;
        NSString* m_windowTitle;
        uint32_t m_mainDisplayRefreshRate = 0;
    };

    NativeWindowImpl_Darwin::~NativeWindowImpl_Darwin()
    {
        [m_nativeWindow release];
        m_nativeWindow = nil;
    }

    void NativeWindowImpl_Darwin::InitWindowInternal(const AZStd::string& title,
                                             const WindowGeometry& geometry,
                                             const WindowStyleMasks& styleMasks)
    {
        m_width = geometry.m_width;
        m_height = geometry.m_height;

        SetWindowTitle(title);

        CGRect screenBounds = CGRectMake(geometry.m_posX, geometry.m_posY, geometry.m_width, geometry.m_height);

        // Create the window
        NSUInteger styleMask = ConvertToNSWindowStyleMask(styleMasks);
        m_nativeWindow = [[NSWindow alloc] initWithContentRect: screenBounds styleMask: styleMask backing: NSBackingStoreBuffered defer:false];

        // Add a fullscreen button in the upper right of the title bar.
        [m_nativeWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];

        m_nativeWindow.tabbingMode = NSWindowTabbingModeDisallowed;
        
        // Make the window active
        [m_nativeWindow makeKeyAndOrderFront:nil];
        m_nativeWindow.title = m_windowTitle;

        CGDirectDisplayID display = CGMainDisplayID();
        CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(display);
        m_mainDisplayRefreshRate = CGDisplayModeGetRefreshRate(currentMode);

        // Assume 60hz if 0 is returned.
        // This can happen on OSX. In future we can hopefully use maximumFramesPerSecond which wont have this issue
        if (m_mainDisplayRefreshRate == 0)
        {
            m_mainDisplayRefreshRate = 60;
        }
    }

    NativeWindowHandle NativeWindowImpl_Darwin::GetWindowHandle() const
    {
        return m_nativeWindow;
    }

    void NativeWindowImpl_Darwin::SetWindowTitle(const AZStd::string& title)
    {
        m_windowTitle = [NSString stringWithCString:title.c_str() encoding:NSUTF8StringEncoding];
        m_nativeWindow.title = m_windowTitle;
    }

    void NativeWindowImpl_Darwin::ResizeClientArea(WindowSize clientAreaSize, [[maybe_unused]] const WindowPosOptions& options)
    {
        NSRect contentRect = [m_nativeWindow contentLayoutRect];
        if (clientAreaSize.m_width != NSWidth(contentRect) || clientAreaSize.m_height != NSHeight(contentRect))
        {
            NSRect newContentRect = NSMakeRect(NSMinX(contentRect), NSMinY(contentRect), clientAreaSize.m_width, clientAreaSize.m_height);
            NSRect newFrameRect = [m_nativeWindow frameRectForContentRect:newContentRect];
            //This will also activate windowDidResize callback which in turn will call OnWindowResized event so no need to call it directly here
            [m_nativeWindow setFrame:newFrameRect display:YES animate:NO];
            m_width = clientAreaSize.m_width;
            m_height = clientAreaSize.m_height;
        }
    }

    bool NativeWindowImpl_Darwin::GetFullScreenState() const
    {
        return ([m_nativeWindow styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen;
    }

    void NativeWindowImpl_Darwin::SetFullScreenState(bool fullScreenState)
    {
        if (GetFullScreenState() != fullScreenState)
        {
            [m_nativeWindow toggleFullScreen:nil];
        }
    }

    NSWindowStyleMask NativeWindowImpl_Darwin::ConvertToNSWindowStyleMask(const WindowStyleMasks& styleMasks)
    {
        NSWindowStyleMask nativeMask = styleMasks.m_platformSpecificStyleMask;
        const uint32_t mask = styleMasks.m_platformAgnosticStyleMask;
        if (mask & WindowStyleMasks::WINDOW_STYLE_RESIZEABLE) { nativeMask |= NSWindowStyleMaskResizable; }
        if (mask & WindowStyleMasks::WINDOW_STYLE_TITLED) { nativeMask |= NSWindowStyleMaskTitled; }
        if (mask & WindowStyleMasks::WINDOW_STYLE_CLOSABLE) { nativeMask |= NSWindowStyleMaskClosable; }
        if (mask & WindowStyleMasks::WINDOW_STYLE_MINIMIZE) { nativeMask |= NSWindowStyleMaskMiniaturizable; }

        const NSWindowStyleMask defaultMask = NSWindowStyleMaskResizable | NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        return nativeMask ? nativeMask : defaultMask;
    }

    uint32_t NativeWindowImpl_Darwin::GetDisplayRefreshRate() const
    {
        return m_mainDisplayRefreshRate;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    class MacNativeWindowFactory 
        : public NativeWindow::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<NativeWindow::Implementation> Create() override
        {
            return AZStd::make_unique<NativeWindowImpl_Darwin>();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeNativeWindowImplementationFactory()
    {
        m_nativeWindowImplFactory = AZStd::make_unique<MacNativeWindowFactory>();
        AZ::Interface<NativeWindow::ImplementationFactory>::Register(m_nativeWindowImplFactory.get());
    }
} // namespace AzFramework
