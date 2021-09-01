/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <xcb/xcb.h>

namespace AzFramework
{
    class NativeWindowImpl_Linux final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Linux, AZ::SystemAllocator, 0);
        NativeWindowImpl_Linux();
        ~NativeWindowImpl_Linux() override;

        // NativeWindow::Implementation overrides...
        void InitWindow(const AZStd::string& title,
                        const WindowGeometry& geometry,
                        const WindowStyleMasks& styleMasks) override;
        void Activate() override;
        void Deactivate() override;
        NativeWindowHandle GetWindowHandle() const override;
        uint32_t GetDisplayRefreshRate() const override;
    private:
        xcb_connection_t*     m_xcbConnection = NULL;
        xcb_window_t          m_xcbWindow = 0;
    };

    NativeWindowImpl_Linux::NativeWindowImpl_Linux() 
        : NativeWindow::Implementation()
    {
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
            if (auto xcbConnectionManager = AzFramework::LinuxXcbConnectionManagerInterface::Get();
                xcbConnectionManager != nullptr)
            {
                m_xcbConnection = xcbConnectionManager->GetXcbConnection();
            }
            AZ_Error("AtomVulkan_RHI", m_xcbConnection!=nullptr, "Unable to get XCB Connection");

#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
            #error "Linux Window Manager Wayland not supported."
            return RHI::ResultCode::Unimplemented;
#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_XLIB
            #error "Linux Window Manager XLIB not supported."
            return RHI::ResultCode::Unimplemented;
#else
            #error "Linux Window Manager not recognized."
            return RHI::ResultCode::Unimplemented;
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    }

    NativeWindowImpl_Linux::~NativeWindowImpl_Linux()
    {
    }


    NativeWindow::Implementation* NativeWindow::Implementation::Create()
    {
        return aznew NativeWindowImpl_Linux();
    }

    void NativeWindowImpl_Linux::InitWindow([[maybe_unused]]const AZStd::string& title,
                                            const WindowGeometry& geometry,
                                            [[maybe_unused]]const WindowStyleMasks& styleMasks)
    {
        if (m_xcbConnection == NULL)
        {
            return;
        }
        // Get the parent window 
        const xcb_setup_t* xcbSetup = xcb_get_setup(m_xcbConnection);
        xcb_screen_t * xcbRootScreen = xcb_setup_roots_iterator(xcbSetup).data;
        xcb_window_t xcbParentWindow = xcbRootScreen->root;

        m_xcbWindow = xcb_generate_id(m_xcbConnection);

        uint16_t borderWidth = 0;
        const uint32_t mask = styleMasks.m_platformAgnosticStyleMask;
        if ((mask & WindowStyleMasks::WINDOW_STYLE_BORDERED) ||
            (mask & WindowStyleMasks::WINDOW_STYLE_RESIZEABLE))
        {
            borderWidth = 4;
        }

        uint32_t eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        uint32_t valueList[] = { xcbRootScreen->black_pixel, 0 };

        xcb_create_window(m_xcbConnection,
                          XCB_COPY_FROM_PARENT, //depth
                          m_xcbWindow, // Window ID
                          xcbParentWindow,  // parent.
                          aznumeric_cast<int16_t>(geometry.m_posX),     // X
                          aznumeric_cast<int16_t>(geometry.m_posY),     // Y
                          aznumeric_cast<int16_t>(geometry.m_width),    // Width
                          aznumeric_cast<int16_t>(geometry.m_height),   // Height
                          borderWidth,                                  // Border Width
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,        // Class
                          xcbRootScreen->root_visual,
                          eventMask,
                          valueList);

        xcb_change_property(m_xcbConnection,
                            XCB_PROP_MODE_REPLACE,
                            m_xcbWindow,
                            XCB_ATOM_WM_NAME,
                            XCB_ATOM_STRING,
                            8,
                            static_cast<uint32_t>(title.size()),
                            title.c_str());

        m_width = geometry.m_width;
        m_height = geometry.m_height;
    }

    void NativeWindowImpl_Linux::Activate()
    {
        if (m_xcbConnection==NULL)
        {
            return;
        }
        if (!m_activated) // nothing to do if window was already activated
        {
            m_activated = true;

            xcb_map_window(m_xcbConnection, m_xcbWindow);
            xcb_flush(m_xcbConnection);
        }
    }

    void NativeWindowImpl_Linux::Deactivate()
    {
        if (m_xcbConnection==NULL)
        {
            return;
        }
        if (m_activated) // nothing to do if window was already deactivated
        {
            m_activated = false;

            xcb_unmap_window(m_xcbConnection, m_xcbWindow);
            xcb_flush(m_xcbConnection);
        }
    }    

    NativeWindowHandle NativeWindowImpl_Linux::GetWindowHandle() const
    {
        return reinterpret_cast<NativeWindowHandle>(m_xcbWindow);
    }

    uint32_t NativeWindowImpl_Linux::GetDisplayRefreshRate() const
    {
        //Using 60 for now until proper support is added
        return 60;
    }
} // namespace AzFramework
