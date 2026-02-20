/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzFramework/Application/Application.h>

#include <AzFramework/WaylandInterface.h>
#include <AzFramework/Protocols/OutputManager.h>
#include <AzFramework/WaylandConnectionManager.h>
#include <AzFramework/WaylandNativeWindow.h>
#include <AzFramework/Windowing/NativeWindow.h>

#include <wayland-client.h>

#include "WaylandProtocolNames.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"

namespace AzFramework
{
    [[maybe_unused]] const char WaylandErrorWindow[] = "WaylandNativeWindow";
    AZ_CVAR(bool, wl_resize, 0, nullptr, AZ::ConsoleFunctorFlags::Null, "WAYLAND ONLY: enforce that surfaces should be resizable.");

    void cvar_wl_fullscreen_Changed(const uint8_t& newFullscreen)
    {
        if (newFullscreen == 0 || newFullscreen == 1)
        {
            NativeWindowHandle defaultWindowHandle = nullptr;
            WindowSystemRequestBus::BroadcastResult(defaultWindowHandle, &WindowSystemRequestBus::Events::GetDefaultWindowHandle);

            if (!defaultWindowHandle)
            {
                AZ_Error(WaylandErrorWindow, false, "Failed to get default window handle.");
                return;
            }

            bool canToggleFullScreenStateOfDefaultWindow = false;
            WindowRequestBus::EventResult(
                canToggleFullScreenStateOfDefaultWindow, defaultWindowHandle, &WindowRequestBus::Events::CanToggleFullScreenState);

            if (!canToggleFullScreenStateOfDefaultWindow)
            {
                AZ_Error(WaylandErrorWindow, false, "XDG TopLevel missing or fullscreen unsupported on compositor.");
                return;
            }

            bool isFullscreen = false;
            AzFramework::WindowRequestBus::EventResult(
                isFullscreen, defaultWindowHandle, &AzFramework::WindowRequestBus::Events::GetFullScreenState);

            if (isFullscreen != newFullscreen)
            {
                // changing res
                WindowRequestBus::Event(defaultWindowHandle, &WindowRequestBus::Events::SetFullScreenState, (bool)newFullscreen);
                isFullscreen = newFullscreen;
            }

            if (AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get(); console)
            {
                AZ::CVarFixedString commandString = AZ::CVarFixedString::format("r_fullscreen %u", newFullscreen ? 1 : 0);
                console->PerformCommand(commandString.c_str());
            }
        }
    }

    AZ_CVAR(
        uint8_t,
        wl_fullscreen,
        0,
        cvar_wl_fullscreen_Changed,
        AZ::ConsoleFunctorFlags::DontReplicate,
        "WAYLAND ONLY: Make main surface fullscreen.");

    wl_surface_listener WaylandNativeWindow::s_surfaceListener = {
        .enter = SurfaceEnter,
        .leave = SurfaceLeave,
        .preferred_buffer_scale = SurfacePreferredScale,
        .preferred_buffer_transform = SurfacePreferredTransform,
    };

    xdg_surface_listener WaylandNativeWindow::s_xdgSurfaceListener = {
        .configure = XdgSurfaceConfigure,
    };

    xdg_toplevel_listener WaylandNativeWindow::s_xdgTopLevelListener = {
        .configure = XdgTopLevelConfigure,
        .close = XdgTopLevelClose,
        .configure_bounds = XdgTopLevelConfigureBounds,
        .wm_capabilities = XdgTopLevelWmCaps
    };

    void WaylandNativeWindow::SurfaceEnter(void* data, wl_surface*, wl_output* output)
    {
        auto self = static_cast<WaylandNativeWindow*>(data);

        if (auto outputManager = AzFramework::OutputManagerInterface::Get())
        {
            uint32_t refreshRateMhz = outputManager->GetRefreshRateMhz(output);

            if (refreshRateMhz == 0)
            {
                return;
            }

            self->m_currentEnteredOutput = output;
            self->InternalUpdateRefreshRate(refreshRateMhz);
            AZStd::string name = outputManager->GetOutputName(output); // DP-1 or HDMI
            AZStd::string desc = outputManager->GetOutputDesc(output); // Name of monitor.
            AZ_Info(WaylandErrorWindow, "Entered screen: %s - %s", desc.c_str(), name.c_str());
        }
    }

    void WaylandNativeWindow::SurfaceLeave(void* /*data*/, wl_surface*, wl_output*)
    {
    }

    void WaylandNativeWindow::SurfacePreferredScale(void* data, wl_surface*, int32_t factor)
    {
        auto self = static_cast<WaylandNativeWindow*>(data);
        self->InternalUpdateScaleFactor((float)factor);
    }

    void WaylandNativeWindow::SurfacePreferredTransform(void* /*data*/, wl_surface*, uint32_t /*transform*/)
    {
    }

    void WaylandNativeWindow::XdgSurfaceConfigure(void* data, xdg_surface* xdg_surface, uint32_t serial)
    {
        auto self = static_cast<WaylandNativeWindow*>(data);

        xdg_surface_ack_configure(xdg_surface, serial);

        if (self->m_pending.m_fullscreen)
        {
            self->m_flags |= WaylandWindowFlags_InFullscreen;
            self->m_pending.m_fullscreen = false;
        }
        else
        {
            self->m_flags &= ~WaylandWindowFlags_InFullscreen;
        }

        if (self->m_pending.m_size != AzFramework::WindowSize())
        {
            // If there is no state like resize or fullscreen then its prob just
            // notifying us of what size we should be, like we just got out of fullscreen, and
            // now we know our previous size.
            self->ResizeClientArea(self->m_pending.m_size, {});
            self->m_pending.m_size = {};
        }
    }

    void WaylandNativeWindow::XdgTopLevelConfigure(
        void* data, xdg_toplevel*, int32_t width, int32_t height, wl_array* states)
    {
        auto self = static_cast<AzFramework::WaylandNativeWindow*>(data);

        self->m_pending.m_fullscreen = false;
        self->m_pending.m_resize = false;

        xdg_toplevel_state* state;
        wl_array_for_each_cpp(state, states, xdg_toplevel_state)
        {
            if (*state == XDG_TOPLEVEL_STATE_FULLSCREEN)
            {
                self->m_pending.m_fullscreen = true;
            }
            else if (*state == XDG_TOPLEVEL_STATE_RESIZING)
            {
                self->m_pending.m_resize = true;
            }
        }

        if (width && height)
        {
            self->m_pending.m_size = { static_cast<uint32_t>(width * self->m_dpiScaleFactor),
                                       static_cast<uint32_t>(height * self->m_dpiScaleFactor) };
        }
        else
        {
            self->m_pending.m_size = {};
        }
    }

    void WaylandNativeWindow::XdgTopLevelClose(void* data, xdg_toplevel*)
    {
        auto self = static_cast<WaylandNativeWindow*>(data);
        self->Deactivate();
    }

    void WaylandNativeWindow::XdgTopLevelConfigureBounds(void* data, xdg_toplevel*, int32_t width, int32_t height)
    {
        auto self = static_cast<WaylandNativeWindow*>(data);
        self->m_recommendedGeoBounds = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    }

    void WaylandNativeWindow::XdgTopLevelWmCaps(void* data, xdg_toplevel*, wl_array* caps)
    {
        auto self = static_cast<WaylandNativeWindow*>(data);
        xdg_toplevel_wm_capabilities* cap;
        wl_array_for_each_cpp(cap, caps, xdg_toplevel_wm_capabilities)
        {
            if (*cap == XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN)
            {
                self->m_flags |= WaylandWindowFlags_CanFullscreen;
            }
        }
    }

    WaylandNativeWindow::WaylandNativeWindow()
        : NativeWindow::Implementation()
        , m_display(nullptr)
        , m_compositor(nullptr)
        , m_surface(nullptr)
        , m_xdgSurface(nullptr)
    {
        if (auto connectionManager = AzFramework::WaylandConnectionManagerInterface::Get())
        {
            m_display = connectionManager->GetWaylandDisplay();
            m_compositor = connectionManager->GetWaylandCompositor();
        }

        wl_proxy* proxy = nullptr;
        WaylandProxyBus::EventResult(proxy, XdgWmBaseName, &WaylandProxyBus::Events::GetProxy, XdgWmBaseName);
        m_xdgWmBase = reinterpret_cast<xdg_wm_base*>(proxy);

        proxy = nullptr;
        WaylandProxyBus::EventResult(proxy, XdgDecorationManagerName, &WaylandProxyBus::Events::GetProxy, XdgDecorationManagerName);
        m_xdgDecor = reinterpret_cast<zxdg_decoration_manager_v1*>(proxy);

        AZ_Error(WaylandErrorWindow, m_display != nullptr, "Unable to get Wayland display.");
        AZ_Error(WaylandErrorWindow, m_compositor != nullptr, "Unable to get Wayland compositor.");
        // TODO: Allow building with libdecor.
        // TODO: If building with libdecor is disabled and XDG Decor is missing maybe enforce fullscreen?
        AZ_Warning(WaylandErrorWindow, m_xdgDecor != nullptr, "Unable to get XDG Decor, decorations will be missing");
    }

    WaylandNativeWindow::~WaylandNativeWindow()
    {
    }

    void WaylandNativeWindow::Activate()
    {
        if (!m_activated && m_surface != nullptr)
        {
            m_activated = true;
        }
    }

    void WaylandNativeWindow::Deactivate()
    {
        if (!m_activated)
        {
            return;
        }

        WindowNotificationBus::Event(m_surface, &WindowNotificationBus::Events::OnWindowClosed);

        if (m_xdgTopLevelDecor != nullptr)
        {
            zxdg_toplevel_decoration_v1_destroy(m_xdgTopLevelDecor);
            m_xdgTopLevelDecor = nullptr;
        }
        if (m_xdgToplevel != nullptr)
        {
            xdg_toplevel_destroy(m_xdgToplevel);
            m_xdgToplevel = nullptr;
        }
        if (m_xdgSurface != nullptr)
        {
            xdg_surface_destroy(m_xdgSurface);
            m_xdgSurface = nullptr;
        }
        if (m_surface != nullptr)
        {
            wl_surface_destroy(m_surface);
            m_surface = nullptr;
        }

        m_activated = false;
    }

    void WaylandNativeWindow::InitWindowInternal(
        const AZStd::string& title, const WindowGeometry& geometry, const WindowStyleMasks& styleMasks)
    {
        m_surface = wl_compositor_create_surface(m_compositor);
        AZ_Error(WaylandErrorWindow, m_surface != nullptr, "Failed to create surface.");
        wl_surface_set_user_data(m_surface, this);

        wl_surface_add_listener(m_surface, &s_surfaceListener, this);

        m_width = geometry.m_width;
        m_height = geometry.m_height;

        // While we guarantee we have xdg_wm_base right now, it's possible that we'll support other shell protocols,
        // so im leaving the checks here to make it a smooth transition.
        if (m_xdgWmBase != nullptr)
        {
            m_xdgSurface = xdg_wm_base_get_xdg_surface(m_xdgWmBase, m_surface);
            AZ_Error(WaylandErrorWindow, m_xdgSurface != nullptr, "Failed to create XDG surface.");

            m_xdgToplevel = xdg_surface_get_toplevel(m_xdgSurface);
            AZ_Error(WaylandErrorWindow, m_xdgSurface != nullptr, "Failed to create XDG Toplevel surface.");

            xdg_surface_add_listener(m_xdgSurface, &s_xdgSurfaceListener, this);
            xdg_toplevel_add_listener(m_xdgToplevel, &s_xdgTopLevelListener, this);

            xdg_surface_set_window_geometry(m_xdgSurface, 0, 0, (int32_t)geometry.m_width, (int32_t)geometry.m_height);
            xdg_toplevel_set_title(m_xdgToplevel, title.c_str());

            uint32_t mask = styleMasks.m_platformAgnosticStyleMask;
            if (mask & WindowStyleMasks::WINDOW_STYLE_RESIZEABLE || wl_resize == true)
            {
                InternalSetResizable(true);
                m_flags |= WaylandWindowFlags_Resizable;

                // If wl_resize is true, we should ensure this flag is set.
                mask |= WindowStyleMasks::WINDOW_STYLE_RESIZEABLE;
            }
            else
            {
                InternalSetResizable(false);
            }

            if (m_xdgDecor != nullptr)
            {
                m_xdgTopLevelDecor = zxdg_decoration_manager_v1_get_toplevel_decoration(m_xdgDecor, m_xdgToplevel);
                AZ_Error(WaylandErrorWindow, m_xdgTopLevelDecor != nullptr, "Failed to create XDG Toplevel decor.");

                if (mask & WindowStyleMasks::WINDOW_STYLE_BORDERED || mask & WindowStyleMasks::WINDOW_STYLE_RESIZEABLE)
                {
                    zxdg_toplevel_decoration_v1_set_mode(
                        m_xdgTopLevelDecor, zxdg_toplevel_decoration_v1_mode::ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
                }
                else
                {
                    zxdg_toplevel_decoration_v1_set_mode(
                        m_xdgTopLevelDecor, zxdg_toplevel_decoration_v1_mode::ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
                }
            }
        }

        wl_surface_commit(m_surface);
        if (auto con = WaylandConnectionManagerInterface::Get())
        {
            con->DoRoundtrip();
        }
    }

    NativeWindowHandle WaylandNativeWindow::GetWindowHandle() const
    {
        return m_surface;
    }

    void WaylandNativeWindow::SetWindowTitle(const AZStd::string& title)
    {
        if (m_xdgToplevel == nullptr)
        {
            return;
        }

        xdg_toplevel_set_title(m_xdgToplevel, title.c_str());
    }

    bool WaylandNativeWindow::SupportsClientAreaResize() const
    {
        return m_xdgSurface != nullptr;
    }

    void WaylandNativeWindow::ResizeClientArea(AzFramework::WindowSize clientAreaSize, const WindowPosOptions& options)
    {
        if (m_xdgSurface == nullptr || clientAreaSize == AzFramework::WindowSize())
        {
            return;
        }

        if (clientAreaSize.m_width == m_width && clientAreaSize.m_height == m_height)
        {
            return;
        }

        // TODO: WindowPosOptions::m_ignoreScreenSizeLimit
        xdg_surface_set_window_geometry(m_xdgSurface, 0, 0, (int32_t)clientAreaSize.m_width, (int32_t)clientAreaSize.m_height);
        InternalWindowSizeChanged(clientAreaSize.m_width, clientAreaSize.m_height);
    }

    float WaylandNativeWindow::GetDpiScaleFactor() const
    {
        return m_dpiScaleFactor;
    }

    uint32_t WaylandNativeWindow::GetDisplayRefreshRate() const
    {
        return static_cast<uint32_t>(AZStd::ceil(static_cast<float>(m_currentRefreshMhz) / 1000.0f));
    }

    WindowSize WaylandNativeWindow::GetMaximumClientAreaSize() const
    {
        return m_recommendedGeoBounds;
    }

    bool WaylandNativeWindow::GetFullScreenState() const
    {
        return m_flags & WaylandWindowFlags_InFullscreen;
    }

    void WaylandNativeWindow::SetFullScreenState(bool fullScreenState)
    {
        if (m_xdgToplevel == nullptr)
        {
            // Cant do fullscreen.
            AZ_Warning(WaylandErrorWindow, false, "Compositor needs to support XDG TopLevel for fullscreen.");
            return;
        }

        if (GetFullScreenState() == fullScreenState || !CanToggleFullScreenState())
        {
            return;
        }

        if (!(m_flags & WaylandWindowFlags_Resizable))
        {
            // Window shouldn't be resizable

            // XDG Shell spec states that compositors can ignore fullscreen requests if the
            // window isn't resizable.
            InternalSetResizable(fullScreenState);
        }

        if (fullScreenState)
        {
            xdg_toplevel_set_fullscreen(m_xdgToplevel, nullptr);
        }
        else
        {
            xdg_toplevel_unset_fullscreen(m_xdgToplevel);
        }

        WindowNotificationBus::Event(
            m_surface, &WindowNotificationBus::Events::OnFullScreenModeChanged, fullScreenState);
    }

    bool WaylandNativeWindow::CanToggleFullScreenState() const
    {
        if (m_xdgToplevel == nullptr)
        {
            // No access to xdg top level.
            return false;
        }

        if (xdg_toplevel_get_version(m_xdgToplevel) < XDG_TOPLEVEL_WM_CAPABILITIES_SINCE_VERSION)
        {
            // Implicitly allowed since the compositor can't tell us if its supported or not, this doesn't mean it will do it.
            return true;
        }

        return m_flags & WaylandWindowFlags_CanFullscreen;
    }

    void WaylandNativeWindow::InternalSetResizable(bool isResizable)
    {
        if (m_xdgToplevel == nullptr)
        {
            return;
        }

        if (isResizable)
        {
            xdg_toplevel_set_min_size(m_xdgToplevel, 1, 1);
            xdg_toplevel_set_max_size(m_xdgToplevel, 0, 0);
        }
        else
        {
            xdg_toplevel_set_min_size(m_xdgToplevel, (int32_t)m_width, (int32_t)m_height);
            xdg_toplevel_set_max_size(m_xdgToplevel, (int32_t)m_width, (int32_t)m_height);
        }
    }

    void WaylandNativeWindow::InternalWindowSizeChanged(uint32_t newWidth, uint32_t newHeight)
    {
        if (newWidth != m_width || newHeight != m_height)
        {
            m_width = newWidth;
            m_height = newHeight;

            if (m_activated)
            {
                WindowNotificationBus::Event(
                    m_surface, &WindowNotificationBus::Events::OnWindowResized, m_width, m_height);
            }
        }
    }

    void WaylandNativeWindow::InternalUpdateRefreshRate(uint32_t newRefreshMhz)
    {
        m_currentRefreshMhz = newRefreshMhz;
        WindowNotificationBus::Event(GetWindowHandle(), &WindowNotificationBus::Events::OnRefreshRateChanged, GetDisplayRefreshRate());
    }

    void WaylandNativeWindow::InternalUpdateBufferScale()
    {
        if (m_surface != nullptr)
        {
            wl_surface_set_buffer_scale(m_surface, (int32_t)m_dpiScaleFactor);
            AZ_Info(WaylandErrorWindow, "Setting buffer scale to %i", (int32_t)m_dpiScaleFactor);
        }
    }

    void WaylandNativeWindow::InternalUpdateScaleFactor(float newScale)
    {
        if (m_dpiScaleFactor != newScale)
        {
            m_dpiScaleFactor = newScale;
            WindowNotificationBus::Event(GetWindowHandle(), &WindowNotificationBus::Events::OnDpiScaleFactorChanged, m_dpiScaleFactor);
        }

        InternalUpdateBufferScale();
    }

    void WaylandNativeWindow::SetPointerFocus(WaylandInputDeviceMouse* pointer)
    {
        m_focusedCursor = pointer;
    }

    void WaylandNativeWindow::SetKeyboardFocus(WaylandInputDeviceKeyboard* keyboard)
    {
        m_focusedKeyboard = keyboard;
    }

} // namespace AzFramework
