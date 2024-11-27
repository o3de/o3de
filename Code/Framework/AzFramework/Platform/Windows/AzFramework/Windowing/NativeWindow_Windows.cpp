/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Windows.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <AzFramework/Windowing/NativeWindow_Windows.h>
#include <AzFramework/Windowing/Resources.h>

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/conversions.h>

#include <dbt.h>

namespace AzFramework
{
    const wchar_t* NativeWindowImpl_Win32::s_defaultClassName = L"O3DEWin32Class";

    NativeWindow::Implementation* NativeWindow::Implementation::Create()
    {
        return aznew NativeWindowImpl_Win32();
    }

    NativeWindowImpl_Win32::NativeWindowImpl_Win32()
    {
        // Attempt to load GetDpiForWindow from user32 at runtime, available on Windows 10+ versions >= 1607
        if (auto user32module = AZ::DynamicModuleHandle::Create("user32"); user32module->Load())
        {
            m_getDpiFunction = user32module->GetFunction<GetDpiForWindowType>("GetDpiForWindow");
        }
    }

    NativeWindowImpl_Win32::~NativeWindowImpl_Win32()
    {
        DestroyWindow(m_win32Handle);

        m_win32Handle = nullptr;
    }

    void DrawSplash(HWND hWnd)
    {
        const HINSTANCE hInstance = GetModuleHandle(0);
        auto hImage = (HBITMAP)LoadImageA(hInstance, MAKEINTRESOURCEA(IDB_SPLASH1), IMAGE_BITMAP, 0, 0, 0);
        if (hImage)
        {
            HDC hDC = GetDC(hWnd);
            HDC hDCBitmap = CreateCompatibleDC(hDC);
            BITMAP bm;

            GetObjectA(hImage, sizeof(bm), &bm);
            SelectObject(hDCBitmap, hImage);

            RECT Rect;
            GetWindowRect(hWnd, &Rect);

            DWORD x = ((Rect.right - Rect.left) - bm.bmWidth) >> 1;
            DWORD y = ((Rect.bottom - Rect.top) - bm.bmHeight) >> 1;
            BitBlt(hDC, x, y, bm.bmWidth, bm.bmHeight, hDCBitmap, 0, 0, SRCCOPY);

            DeleteObject(hImage);
            DeleteDC(hDCBitmap);
        }
    }

    void NativeWindowImpl_Win32::InitWindowInternal(
        const AZStd::string& title,
        const WindowGeometry& geometry,
        const WindowStyleMasks& styleMasks)
    {
        const HINSTANCE hInstance = GetModuleHandle(0);

        // register window class if it does not exist
        WNDCLASSEX windowClass;
        if (GetClassInfoExW(hInstance, s_defaultClassName, &windowClass) == false)
        {
            windowClass.cbSize = sizeof(WNDCLASSEX);
            windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            windowClass.lpfnWndProc = &NativeWindowImpl_Win32::WindowCallback;
            windowClass.cbClsExtra = 0;
            windowClass.cbWndExtra = 0;
            windowClass.hInstance = hInstance;
            windowClass.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
            windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
            windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            windowClass.lpszMenuName = NULL;
            windowClass.lpszClassName = s_defaultClassName;
            windowClass.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);

            if (!RegisterClassEx(&windowClass))
            {
                AZ_Error("Windowing", false, "Failed to register Win32 window class with error: %d", GetLastError());
            }
        }

        const DWORD windowStyle = ConvertToWin32WindowStyleMask(styleMasks);
        const BOOL windowHasMenu = FALSE;

        // Adjust the window size so that the geometry we were given is the size of the client area.
        RECT windowRect =
        {
            static_cast<LONG>(geometry.m_posX),
            static_cast<LONG>(geometry.m_posY),
            static_cast<LONG>(geometry.m_posX + geometry.m_width),
            static_cast<LONG>(geometry.m_posY + geometry.m_height)
        };
        AdjustWindowRect(&windowRect, windowStyle, windowHasMenu);

        // These are to store the client sizes, which will be smaller or equal to the window size
        m_width = geometry.m_width;
        m_height = geometry.m_height;

        // create main window
        AZStd::wstring titleW;
        AZStd::to_wstring(titleW, title);
        m_win32Handle = CreateWindowW(
            s_defaultClassName, titleW.c_str(),
            windowStyle,
            geometry.m_posX, geometry.m_posY, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
            NULL, NULL, hInstance, NULL);

        if (m_win32Handle == nullptr)
        {
            AZ_Error("Windowing", false, "Failed to create Win32 window with error: %d", GetLastError());
        }
        else
        {
            SetWindowLongPtr(m_win32Handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        }

        DEVMODE DisplayConfig;
        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &DisplayConfig);
        m_mainDisplayRefreshRate = DisplayConfig.dmDisplayFrequency;
    }

    void NativeWindowImpl_Win32::Activate()
    {
        if (!m_activated)
        {
            m_activated = true;

            // This will result in a WM_SIZE message which will send the OnWindowResized notification
            ShowWindow(m_win32Handle, SW_SHOW);
            UpdateWindow(m_win32Handle);
            DrawSplash(m_win32Handle);
        }
    }

    void NativeWindowImpl_Win32::Deactivate()
    {
        if (m_activated) // nothing to do if window was already deactivated
        {
            m_activated = false;

            WindowNotificationBus::Event(m_win32Handle, &WindowNotificationBus::Events::OnWindowClosed);

            ShowWindow(m_win32Handle, SW_HIDE);
            UpdateWindow(m_win32Handle);
        }
    }

    NativeWindowHandle NativeWindowImpl_Win32::GetWindowHandle() const
    {
        return m_win32Handle;
    }
    HWND NativeWindowImpl_Win32::GetWindowedPriority()
    {
        // If a debugger is attached and we're running in Windowed mode instead of Fullscreen mode,
        // don't make the window TOPMOST. Otherwise, the window will stay on top of the debugger window
        // at every breakpoint, crash, etc, making it extremely difficult to debug when working on a single monitor system.
        bool allowTopMost = !AZ::Debug::Trace::Instance().IsDebuggerPresent();

        // If we are launching with pix enabled we are probably trying to do a gpu capture and hence need to be able to
        // access RenderDoc or Pix behind the O3de app.
#if defined(USE_PIX)
        allowTopMost = false;
#endif
        return allowTopMost ? HWND_TOPMOST : HWND_NOTOPMOST;
    }

    void NativeWindowImpl_Win32::SetWindowTitle(const AZStd::string& title)
    {
        AZStd::wstring titleW;
        AZStd::to_wstring(titleW, title);
        SetWindowTextW(m_win32Handle, titleW.c_str());
    }

    RECT NativeWindowImpl_Win32::GetMonitorRect() const
    {
        // Get the monitor on which the window is currently displayed.
        HMONITOR monitor = MonitorFromWindow(m_win32Handle, MONITOR_DEFAULTTONEAREST);
        if (!monitor)
        {
            AZ_Warning("NativeWindowImpl_Win32::GetMonitorRect", false, "Could not find any monitor.");
            return { 0, 0, 0, 0 };
        }

        // Get the dimensions of the display device on which the window is currently displayed.
        MONITORINFO monitorInfo;
        memset(&monitorInfo, 0, sizeof(MONITORINFO)); // C4701 potentially uninitialized local variable 'monitorInfo' used
        monitorInfo.cbSize = sizeof(MONITORINFO);
        if (!GetMonitorInfo(monitor, &monitorInfo))
        {
            AZ_Warning("NativeWindowImpl_Win32::GetMonitorRect", false, "Could not get monitor info.");
            return { 0, 0, 0, 0 };
        }

        // This is the full dimensions of the monitor, including the task bar area.
        // Since we keep our window as "TOPMOST" when it's active, it can draw over the task bar.
        return monitorInfo.rcMonitor;
    }

    WindowSize NativeWindowImpl_Win32::GetMaximumClientAreaSize() const
    {
        // Calculate the size of the entire monitor that the window is currently on.
        RECT monitorRect = GetMonitorRect();
        WindowSize monitorSize = { aznumeric_cast<uint32_t>(monitorRect.right - monitorRect.left),
                                   aznumeric_cast<uint32_t>(monitorRect.bottom - monitorRect.top) };

        // Using that size as a client area, calculate how much bigger the full window would be with the current style
        RECT fullWindowRect = monitorRect;
        const UINT currentWindowStyle = GetWindowLong(m_win32Handle, GWL_STYLE);
        AdjustWindowRect(&fullWindowRect, currentWindowStyle, false);
        WindowSize fullWindowSize = 
        {
            aznumeric_cast<uint32_t>(fullWindowRect.right - fullWindowRect.left),
            aznumeric_cast<uint32_t>(fullWindowRect.bottom - fullWindowRect.top)
        };

        // Subtract the additional overhead of the borders and title bar from the monitor size to get the maximum client area.
        return
        {
            monitorSize.m_width - (fullWindowSize.m_width - monitorSize.m_width),
            monitorSize.m_height - (fullWindowSize.m_height - monitorSize.m_height)
        };
    }


    DWORD NativeWindowImpl_Win32::ConvertToWin32WindowStyleMask(const WindowStyleMasks& styleMasks)
    {
        DWORD nativeMask = styleMasks.m_platformSpecificStyleMask;
        const uint32_t mask = styleMasks.m_platformAgnosticStyleMask;
        if (mask & WindowStyleMasks::WINDOW_STYLE_BORDERED) { nativeMask |= WS_BORDER; }
        if (mask & WindowStyleMasks::WINDOW_STYLE_RESIZEABLE) { nativeMask |= WS_BORDER | WS_THICKFRAME; }
        if (mask & WindowStyleMasks::WINDOW_STYLE_TITLED) { nativeMask |= WS_CAPTION; }
        if (mask & WindowStyleMasks::WINDOW_STYLE_TITLED_MENU) { nativeMask |= WS_CAPTION | WS_SYSMENU; }
        if (mask & WindowStyleMasks::WINDOW_STYLE_MAXIMIZE) { nativeMask |= WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX; }
        if (mask & WindowStyleMasks::WINDOW_STYLE_MINIMIZE) { nativeMask |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX; }

        const DWORD defaultMask = WS_OVERLAPPEDWINDOW;
        return nativeMask ? nativeMask : defaultMask;
    }

    // Handles Win32 Window Event callbacks
    LRESULT CALLBACK NativeWindowImpl_Win32::WindowCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        NativeWindowImpl_Win32* nativeWindowImpl = reinterpret_cast<NativeWindowImpl_Win32*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        // If set to true, call DefWindowProc to ensure the default Windows behavior occurs
        bool shouldBubbleEventUp = false;

        switch (message)
        {
        case WM_CLOSE:
        {
            nativeWindowImpl->Deactivate();
            break;
        }
        case WM_SIZE:
        {
            const uint16_t newWidth = LOWORD(lParam);
            const uint16_t newHeight = HIWORD(lParam);
            
            nativeWindowImpl->WindowSizeChanged(static_cast<uint32_t>(newWidth), static_cast<uint32_t>(newHeight));
            break;
        }
        case WM_INPUT:
        {
            UINT rawInputSize;
            const UINT rawInputHeaderSize = sizeof(RAWINPUTHEADER);
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &rawInputSize, rawInputHeaderSize);

            AZStd::array<BYTE, sizeof(RAWINPUT)> rawInputBytesArray;
            LPBYTE rawInputBytes = rawInputBytesArray.data();
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInputBytes, &rawInputSize, rawInputHeaderSize);

            RAWINPUT* rawInput = (RAWINPUT*)rawInputBytes;
            AzFramework::RawInputNotificationBusWindows::Broadcast(
                &AzFramework::RawInputNotificationBusWindows::Events::OnRawInputEvent, *rawInput);

            break;
        }
        case WM_CHAR:
        {
            const unsigned short codeUnitUTF16 = static_cast<unsigned short>(wParam);
            AzFramework::RawInputNotificationBusWindows::Broadcast(&AzFramework::RawInputNotificationsWindows::OnRawInputCodeUnitUTF16Event, codeUnitUTF16);
            break;
        }
        case WM_ACTIVATE:
        {
            // Alt-tabbing out of the app while it is in a full screen state does not
            // work unless we explicitly exit the full screen state upon deactivation,
            // in which case we want to enter full screen state again upon activation.
            const bool windowIsNowInactive = (LOWORD(wParam) == WA_INACTIVE);
            const bool windowFullScreenState = nativeWindowImpl->GetFullScreenState();

            if (windowIsNowInactive)
            {
                if (windowFullScreenState)
                {
                    nativeWindowImpl->m_shouldEnterFullScreenStateOnActivate = true;
                    nativeWindowImpl->SetFullScreenState(false);
                }

                // When becoming inactive, transition from TOPMOST to NOTOPMOST.
                SetWindowPos(nativeWindowImpl->m_win32Handle,
                    HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
            }
            else
            {
                // When running in Windowed mode, we might want either NOTOPMOST or TOPMOST, depending on whether or not a debugger is attached.
                HWND windowPriority = GetWindowedPriority();

                if (!windowFullScreenState && nativeWindowImpl->m_shouldEnterFullScreenStateOnActivate)
                {
                    nativeWindowImpl->m_shouldEnterFullScreenStateOnActivate = false;
                    nativeWindowImpl->SetFullScreenState(true);

                    // If we're going to fullscreen, then we presumably want to be TOPMOST whether or not a debugger is attached.
                    windowPriority = HWND_TOPMOST;
                }

                // When becoming active again, transition from NOTOPMOST to TOPMOST. (Or stay NOTOPMOST if a debugger is attached)
                SetWindowPos(
                    nativeWindowImpl->m_win32Handle,
                    windowPriority, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
            }
            break;
        }
        case WM_SYSKEYDOWN:
        {
            // Handle ALT+ENTER to toggle full screen unless exclsuive full screen
            // mode is preferred, in which case this will be handled by the system.
            if ((wParam == VK_RETURN) && (lParam & (1 << 29)))
            {
                bool isExclusiveFullScreenPreferred = false;
                ExclusiveFullScreenRequestBus::EventResult(isExclusiveFullScreenPreferred,
                                                           nativeWindowImpl->GetWindowHandle(),
                                                           &ExclusiveFullScreenRequests::IsExclusiveFullScreenPreferred);
                if (!isExclusiveFullScreenPreferred)
                {
                    WindowRequestBus::Event(nativeWindowImpl->GetWindowHandle(), &WindowRequests::ToggleFullScreenState);
                    return 0;
                }
            }
            // Send all other WM_SYSKEYDOWN messages to the default WndProc.
            break;
        }
        case WM_DPICHANGED:
        {
            const float newScaleFactor = nativeWindowImpl->GetDpiScaleFactor();
            WindowNotificationBus::Event(nativeWindowImpl->GetWindowHandle(), &WindowNotificationBus::Events::OnDpiScaleFactorChanged, newScaleFactor);
            break;
        }
        case WM_WINDOWPOSCHANGED:
        {
            // Any time the window position has changed, there's a possibility that it has moved to a different monitor
            // with a different refresh rate. Get the refresh rate and send out a notification that it changed.
            // It appears this notification also triggers even if you change the refresh rate on the same monitor and don't
            // actually change the window position at all.
            DEVMODE displayConfig;
            EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &displayConfig);
            if (displayConfig.dmDisplayFrequency != nativeWindowImpl->m_mainDisplayRefreshRate)
            {
                nativeWindowImpl->m_mainDisplayRefreshRate = displayConfig.dmDisplayFrequency;
                WindowNotificationBus::Event(
                    nativeWindowImpl->GetWindowHandle(),
                    &WindowNotificationBus::Events::OnRefreshRateChanged,
                    displayConfig.dmDisplayFrequency);
            }

            // Don't mark this event as handled since we aren't actually processing the window positional change itself.
            shouldBubbleEventUp = true;
            break;
        }
        case WM_DEVICECHANGE:
            if (wParam == DBT_DEVNODES_CHANGED)
            {
                // If any device changes were detected, broadcast to the input device notification
                AzFramework::RawInputNotificationBusWindows::Broadcast(&AzFramework::RawInputNotificationsWindows::OnRawInputDeviceChangeEvent);
            }
        default:
            shouldBubbleEventUp = true;
            break;
        }

        if (!shouldBubbleEventUp)
        {
            return 0;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    void NativeWindowImpl_Win32::WindowSizeChanged(const uint32_t width, const uint32_t height)
    {
        if (m_width != width || m_height != height)
        {
            m_width = width;
            m_height = height;

            DrawSplash(m_win32Handle);

            if (m_activated)
            {
                WindowNotificationBus::Event(m_win32Handle, &WindowNotificationBus::Events::OnWindowResized, width, height);
            }
        }
    }

    void NativeWindowImpl_Win32::ResizeClientArea(WindowSize clientAreaSize, const WindowPosOptions& options)
    {
        // Get the coordinates of the window's current client area.
        // We'll use this to keep the left and top coordinates pinned to the same spot
        // and just resize by changing the bottom right corner.
        RECT rect = {};
        GetClientRect(m_win32Handle, &rect);

        // Change the bottom right corner to reflect the new desired client area size.
        rect.right = rect.left + clientAreaSize.m_width;
        rect.bottom = rect.top + clientAreaSize.m_height;

        // Given a desired client area rectangle, calculate the rectangle needed to support that client area,
        // taking into account the title bar and window border.
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

        // By default, SetWindowPos() will clamp the size of the window to the maximum size of the screen.
        // This doesn't take window positioning into account, so it's not clamping the window to be fully visible,
        // it's just clamping it to a maximum *possible* visible size.
        // If we want to circumvent this behavior, the solution is to add the flag SWP_NOSENDCHANGING
        // when setting the new window pos. Without this flag, Windows will send out the WM_WINDOWPOSCHANGING message
        // and then the WM_GETMINMAXINFO message, so that the window size will be clipped to the screen max size.
        // https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-windowposchanging
        // https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-getminmaxinfo
        const UINT flag = SWP_NOMOVE | (options.m_ignoreScreenSizeLimit ? SWP_NOSENDCHANGING : 0);

        // When running in Windowed mode, we might want either NOTOPMOST or TOPMOST, depending on whether or not a debugger is attached.
        const HWND windowPriority = GetWindowedPriority();

        SetWindowPos(m_win32Handle, windowPriority, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, flag);
    }

    bool NativeWindowImpl_Win32::GetFullScreenState() const
    {
        if (m_isInBorderlessWindowFullScreenState)
        {
            return true;
        }

        bool isExclusiveFullScreenPreferred = false;
        ExclusiveFullScreenRequestBus::EventResult(isExclusiveFullScreenPreferred,
                                                   m_win32Handle,
                                                   &ExclusiveFullScreenRequests::IsExclusiveFullScreenPreferred);
        if (isExclusiveFullScreenPreferred)
        {
            // The renderer has assumed responsibility for maintaining the full screen state of this NativeWindow.
            // This should only happen if tearing is not supported when using DX12 (see SwapChain::InitInternal).
            bool exclusiveFullScreenState = false;
            ExclusiveFullScreenRequestBus::EventResult(exclusiveFullScreenState,
                                                       m_win32Handle,
                                                       &ExclusiveFullScreenRequests::GetExclusiveFullScreenState);
            return exclusiveFullScreenState;
        }

        return m_isInBorderlessWindowFullScreenState;
    }

    void NativeWindowImpl_Win32::SetFullScreenState(bool fullScreenState)
    {
        bool isExclusiveFullScreenPreferred = false;
        ExclusiveFullScreenRequestBus::EventResult(isExclusiveFullScreenPreferred,
                                                   m_win32Handle,
                                                   &ExclusiveFullScreenRequests::IsExclusiveFullScreenPreferred);
        if (isExclusiveFullScreenPreferred)
        {
            // The renderer has assumed responsibility for transitioning the full screen state of this NativeWindow.
            // This should only happen if tearing is not supported when using DX12 (see SwapChain::InitInternal).
            if (m_isInBorderlessWindowFullScreenState)
            {
                ExitBorderlessWindowFullScreen();
            }

            bool wasExclusiveFullScreenStateSet = false;
            ExclusiveFullScreenRequestBus::EventResult(wasExclusiveFullScreenStateSet,
                                                       m_win32Handle,
                                                       &ExclusiveFullScreenRequests::SetExclusiveFullScreenState,
                                                       fullScreenState);
            AZ_Warning("NativeWindowImpl_Win32::SetFullScreenState", wasExclusiveFullScreenStateSet,
                       "Could not set full screen state using ExclusiveFullScreenRequests::SetExclusiveFullScreenState.");
            return;
        }

        if (fullScreenState)
        {
            EnterBorderlessWindowFullScreen();
        }
        else
        {
            ExitBorderlessWindowFullScreen();
        }
    }

    float NativeWindowImpl_Win32::GetDpiScaleFactor() const
    {
        constexpr UINT defaultDotsPerInch = 96;
        UINT dotsPerInch = defaultDotsPerInch;
        if (m_getDpiFunction)
        {
            dotsPerInch = m_getDpiFunction(m_win32Handle);
        }
        return aznumeric_cast<float>(dotsPerInch) / aznumeric_cast<float>(defaultDotsPerInch);
    }

    uint32_t NativeWindowImpl_Win32::GetDisplayRefreshRate() const
    {
        return m_mainDisplayRefreshRate;
    }

    void NativeWindowImpl_Win32::EnterBorderlessWindowFullScreen()
    {
        if (m_isInBorderlessWindowFullScreenState)
        {
            return;
        }

        // Set this first so that any calls to GetFullscreenState() that occur during any window notifications
        // caused by the window state changes below will return the correct state.
        m_isInBorderlessWindowFullScreenState = true;

        // Get the dimensions of the display device on which the window is currently displayed.
        const RECT fullScreenWindowRect = GetMonitorRect();

        // Store the current window rect and style so we can restore them when exiting full screen.
        GetWindowRect(m_win32Handle, &m_windowRectToRestoreOnFullScreenExit);
        const UINT currentWindowStyle = GetWindowLong(m_win32Handle, GWL_STYLE);
        m_windowStyleToRestoreOnFullScreenExit = currentWindowStyle;
        m_windowExtendedStyleToRestoreOnFullScreenExit = GetWindowLong(m_win32Handle, GWL_EXSTYLE);

        // Style, resize, and position the window such that it fills the entire screen.
        const UINT fullScreenWindowStyle = currentWindowStyle & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME);
        SetWindowLong(m_win32Handle, GWL_STYLE, fullScreenWindowStyle);
        SetWindowPos(m_win32Handle,
                        HWND_TOPMOST,
                        fullScreenWindowRect.left,
                        fullScreenWindowRect.top,
                        fullScreenWindowRect.right - fullScreenWindowRect.left,
                        fullScreenWindowRect.bottom - fullScreenWindowRect.top,
                        SWP_FRAMECHANGED | SWP_NOACTIVATE);
        ShowWindow(m_win32Handle, SW_MAXIMIZE);

        WindowNotificationBus::Event(GetWindowHandle(), &WindowNotificationBus::Events::OnFullScreenModeChanged, true);
    }

    void NativeWindowImpl_Win32::ExitBorderlessWindowFullScreen()
    {
        if (!m_isInBorderlessWindowFullScreenState)
        {
            return;
        }

        // Set this first so that any calls to GetFullscreenState() that occur during any window notifications
        // caused by the window state changes below will return the correct state.
        m_isInBorderlessWindowFullScreenState = false;

        // Restore the style, size, and position of the window.
        SetWindowLong(m_win32Handle, GWL_STYLE, m_windowStyleToRestoreOnFullScreenExit);
        SetWindowLong(m_win32Handle, GWL_EXSTYLE, m_windowExtendedStyleToRestoreOnFullScreenExit);
        SetWindowPos(m_win32Handle,
                        GetWindowedPriority(),
                        m_windowRectToRestoreOnFullScreenExit.left,
                        m_windowRectToRestoreOnFullScreenExit.top,
                        m_windowRectToRestoreOnFullScreenExit.right - m_windowRectToRestoreOnFullScreenExit.left,
                        m_windowRectToRestoreOnFullScreenExit.bottom - m_windowRectToRestoreOnFullScreenExit.top,
                        SWP_FRAMECHANGED | SWP_NOACTIVATE);
        ShowWindow(m_win32Handle, SW_NORMAL);

        // Sometimes, the above code doesn't set the window above the taskbar, even though other times it does.
        // This might be a bug in the Windows SDK?
        // By setting topmost a second time with ASYNCWINDOWPOS, the topmost setting seems to apply correctly 100% of the time.
        SetWindowPos(m_win32Handle, GetWindowedPriority(), 0, 0, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE);

        WindowNotificationBus::Event(GetWindowHandle(), &WindowNotificationBus::Events::OnFullScreenModeChanged, false);
    }
} // namespace AzFramework
