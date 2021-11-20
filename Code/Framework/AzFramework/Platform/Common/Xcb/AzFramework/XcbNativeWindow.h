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
#include <AzFramework/XcbEventHandler.h>

#include <xcb/xcb.h>

namespace AzFramework
{
    class XcbNativeWindow final
        : public NativeWindow::Implementation
        , public XcbEventHandlerBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(XcbNativeWindow, AZ::SystemAllocator, 0);
        XcbNativeWindow();
        ~XcbNativeWindow() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // NativeWindow::Implementation
        void InitWindow(const AZStd::string& title, const WindowGeometry& geometry, const WindowStyleMasks& styleMasks) override;
        void Activate() override;
        void Deactivate() override;
        NativeWindowHandle GetWindowHandle() const override;
        void SetWindowTitle(const AZStd::string& title) override;
        void ResizeClientArea(WindowSize clientAreaSize) override;
        uint32_t GetDisplayRefreshRate() const override;

        bool GetFullScreenState() const override;
        void SetFullScreenState(bool fullScreenState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // XcbEventHandlerBus::Handler
        void HandleXcbEvent(xcb_generic_event_t* event) override;

    private:
        bool ValidateXcbResult(xcb_void_cookie_t cookie);
        void WindowSizeChanged(const uint32_t width, const uint32_t height);
        int SetAtom(xcb_window_t window, xcb_atom_t atom, xcb_atom_t type, size_t len, void* data);

        // Initialize one atom.
        xcb_atom_t GetAtom(const char* atomName);

        // Initialize all used atoms.
        void InitializeAtoms();
        void GetWMStates();

        xcb_connection_t* m_xcbConnection = nullptr;
        xcb_screen_t* m_xcbRootScreen = nullptr;
        xcb_window_t m_xcbWindow = 0;
        int32_t m_posX;
        int32_t m_posY;
        bool m_fullscreenState = false;
        bool m_horizontalyMaximized = false;
        bool m_verticallyMaximized = false;

        // Use exact atom names for easy readability and usage.
        xcb_atom_t WM_PROTOCOLS;
        xcb_atom_t WM_DELETE_WINDOW;
        // This atom is used to activate a window.
        xcb_atom_t _NET_ACTIVE_WINDOW;
        // This atom is use to bypass a compositor. Used during fullscreen mode.
        xcb_atom_t _NET_WM_BYPASS_COMPOSITOR;
        // This atom is used to change the state of a window using the WM.
        xcb_atom_t _NET_WM_STATE;
        // This atom is used to enable/disable fullscreen mode of a window.
        xcb_atom_t _NET_WM_STATE_FULLSCREEN;
        // This atom is used to extend the window to max vertically.
        xcb_atom_t _NET_WM_STATE_MAXIMIZED_VERT;
        // This atom is used to extend the window to max horizontally.
        xcb_atom_t _NET_WM_STATE_MAXIMIZED_HORZ;
        // This atom is used to position and resize a window.
        xcb_atom_t _NET_MOVERESIZE_WINDOW;
        // This atom is used to request the extent of the window.
        xcb_atom_t _NET_REQUEST_FRAME_EXTENTS;
        // This atom is used to identify the reply event for _NET_REQUEST_FRAME_EXTENTS
        xcb_atom_t _NET_FRAME_EXTENTS;
        // This atom is used to allow WM to kill app if not responsive anymore
        xcb_atom_t _NET_WM_PID;
    };
} // namespace AzFramework
