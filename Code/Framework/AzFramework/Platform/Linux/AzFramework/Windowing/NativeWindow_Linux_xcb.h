/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <xcb/xcb.h>

namespace AzFramework
{
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    class NativeWindowImpl_Linux_xcb final
        : public NativeWindow::Implementation
        , public LinuxXcbEventHandlerBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Linux_xcb, AZ::SystemAllocator, 0);
        NativeWindowImpl_Linux_xcb();
        ~NativeWindowImpl_Linux_xcb() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // NativeWindow::Implementation
        void InitWindow(const AZStd::string& title,
                        const WindowGeometry& geometry,
                        const WindowStyleMasks& styleMasks) override;
        void Activate() override;
        void Deactivate() override;
        NativeWindowHandle GetWindowHandle() const override;
        void SetWindowTitle(const AZStd::string& title) override;
        void ResizeClientArea(WindowSize clientAreaSize) override;
        uint32_t GetDisplayRefreshRate() const override;        

        ////////////////////////////////////////////////////////////////////////////////////////////
        // LinuxXcbEventHandlerBus::Handler
        void HandleXcbEvent(xcb_generic_event_t* event) override;

    private:
        bool ValidateXcbResult(xcb_void_cookie_t cookie);
        void WindowSizeChanged(const uint32_t width, const uint32_t height);

        xcb_connection_t*   m_xcbConnection = nullptr;
        xcb_window_t        m_xcbWindow = 0;
        xcb_atom_t          m_xcbAtomProtocols;
        xcb_atom_t          m_xcbAtomDeleteWindow;
    };
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB

} // namespace AzFramework
