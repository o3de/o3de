/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Application/Application.h>
#include <AzFramework/Windowing/NativeWindow.h>

#include <AzFramework/WaylandInputDeviceKeyboard.h>
#include <AzFramework/WaylandInputDeviceMouse.h>
#include <wayland-client.h>
#include <xdg-shell-client-protocol.h>

struct xdg_wm_base;
struct zxdg_decoration_manager_v1;
struct zxdg_toplevel_decoration_v1;

namespace AzFramework
{
    typedef uint16_t WaylandWindowFlags;

    enum WaylandWindowFlags_ : uint16_t
    {
        WaylandWindowFlags_None = 0,
        WaylandWindowFlags_CanFullscreen = 1 << 0, // Will the compositor even let us fullscreen?
        WaylandWindowFlags_InFullscreen = 1 << 1, // In fullscreen
        WaylandWindowFlags_Resizable = 1 << 2, // O3DE wants this to be resizable
    };

    enum XdgSurfaceEventMask : uint16_t
    {
        XSCM_Enter = 1 << 0,
        XSCM_Leave = 1 << 1,
        XSCM_Bounds = 1 << 2,
        XSCM_WmCaps = 1 << 3,
    };

    class WaylandNativeWindow final : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(WaylandNativeWindow, AZ::SystemAllocator);
        WaylandNativeWindow();
        ~WaylandNativeWindow() override;

        void Activate() override;
        void Deactivate() override;

        void InitWindowInternal(
            const AZStd::string& title,
            const AzFramework::WindowGeometry& geometry,
            const AzFramework::WindowStyleMasks& styleMasks) override;
        NativeWindowHandle GetWindowHandle() const override;

        void SetWindowTitle(const AZStd::string& title) override;

        bool SupportsClientAreaResize() const override;
        void ResizeClientArea(AzFramework::WindowSize clientAreaSize, const AzFramework::WindowPosOptions& options) override;

        float GetDpiScaleFactor() const override;
        uint32_t GetDisplayRefreshRate() const override;

        WindowSize GetMaximumClientAreaSize() const override;

        bool GetFullScreenState() const override;
        void SetFullScreenState(bool fullScreenState) override;
        bool CanToggleFullScreenState() const override;

        void InternalSetResizable(bool isResizable);
        void InternalWindowSizeChanged(uint32_t newWidth, uint32_t newHeight);
        void InternalUpdateRefreshRate(uint32_t newRefreshMhz);
        void InternalUpdateBufferScale();
        void InternalUpdateScaleFactor(float newScale);

    private:
        static void SurfaceEnter(void* data, wl_surface* wl_surface, wl_output* output);
        static void SurfaceLeave(void* data, wl_surface* wl_surface, wl_output* output);
        static void SurfacePreferredScale(void* data, wl_surface* wl_surface, int32_t factor);
        static void SurfacePreferredTransform(void* data, wl_surface* wl_surface, uint32_t transform);

        static wl_surface_listener s_surfaceListener;

        static void XdgSurfaceConfigure(void* data, xdg_surface* xdg_surface, uint32_t serial);

        static xdg_surface_listener s_xdgSurfaceListener;

        static void XdgTopLevelConfigure(void*, xdg_toplevel*, int32_t, int32_t, wl_array*);
        static void XdgTopLevelClose(void* data, xdg_toplevel* xdg_toplevel);
        static void XdgTopLevelConfigureBounds(void*, xdg_toplevel*, int32_t, int32_t);
        static void XdgTopLevelWmCaps(void*, xdg_toplevel*, wl_array*);

        static xdg_toplevel_listener s_xdgTopLevelListener;

    private:
        WaylandWindowFlags m_flags = WaylandWindowFlags_None;

        // Globals cache
        wl_display* m_display = nullptr;
        wl_compositor* m_compositor = nullptr;
        xdg_wm_base* m_xdgWmBase = nullptr;
        zxdg_decoration_manager_v1* m_xdgDecor = nullptr;

        // Per window
        wl_surface* m_surface = nullptr;
        xdg_surface* m_xdgSurface = nullptr;
        xdg_toplevel* m_xdgToplevel = nullptr;
        zxdg_toplevel_decoration_v1* m_xdgTopLevelDecor = nullptr;

        WindowSize m_recommendedGeoBounds = {};
        wl_output* m_currentEnteredOutput = nullptr;

        uint32_t m_currentRefreshMhz = 0;
        float m_dpiScaleFactor = 1.0f;

        struct
        {
            bool m_fullscreen = false;
            bool m_resize = false;
            AzFramework::WindowSize m_size = {};
        } m_pending;
    };
} // namespace AzFramework
