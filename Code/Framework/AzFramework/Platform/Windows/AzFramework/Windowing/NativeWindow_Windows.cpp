/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Windows.h>
#include <AzFramework/Windowing/NativeWindow.h>

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/conversions.h>

namespace AzFramework
{
    class NativeWindowImpl_Win32 final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Win32, AZ::SystemAllocator, 0);
        NativeWindowImpl_Win32();
        ~NativeWindowImpl_Win32() override;

        // NativeWindow::Implementation overrides...
        void InitWindow(const AZStd::string& title,
                        const WindowGeometry& geometry,
                        const WindowStyleMasks& styleMasks) override;
        void Activate() override;
        void Deactivate() override;
        NativeWindowHandle GetWindowHandle() const override;
        void SetWindowTitle(const AZStd::string& title) override;

        void ResizeClientArea( WindowSize clientAreaSize ) override;
        bool GetFullScreenState() const override;
        void SetFullScreenState(bool fullScreenState) override;
        bool CanToggleFullScreenState() const override { return true; }
        float GetDpiScaleFactor() const override;
        uint32_t GetDisplayRefreshRate() const override;

    private:
        static DWORD ConvertToWin32WindowStyleMask(const WindowStyleMasks& styleMasks);
        static LRESULT CALLBACK WindowCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        static const wchar_t* s_defaultClassName;

        void WindowSizeChanged(const uint32_t width, const uint32_t height);

        void EnterBorderlessWindowFullScreen();
        void ExitBorderlessWindowFullScreen();

        HWND m_win32Handle = nullptr;
        RECT m_windowRectToRestoreOnFullScreenExit; //!< The position and size of the window to restore when exiting full screen.
        UINT m_windowStyleToRestoreOnFullScreenExit; //!< The style(s) of the window to restore when exiting full screen.
        bool m_isInBorderlessWindowFullScreenState = false; //!< Was a borderless window used to enter full screen state?

        using GetDpiForWindowType = UINT(HWND hwnd);
        GetDpiForWindowType* m_getDpiFunction = nullptr;
        uint32_t m_mainDisplayRefreshRate = 0;
    };

    const wchar_t* NativeWindowImpl_Win32::s_defaultClassName = L"O3DEWin32Class";

    NativeWindow::Implementation* NativeWindow::Implementation::Create()
    {
        return aznew NativeWindowImpl_Win32();
    }

    NativeWindowImpl_Win32::NativeWindowImpl_Win32()
    {
        // Attempt to load GetDpiForWindow from user32 at runtime, available on Windows 10+ versions >= 1607
        if (auto user32module = AZ::DynamicModuleHandle::Create("user32"); user32module->Load(false))
        {
            m_getDpiFunction = user32module->GetFunction<GetDpiForWindowType*>("GetDpiForWindow");
        }
    }

    NativeWindowImpl_Win32::~NativeWindowImpl_Win32()
    {
        DestroyWindow(m_win32Handle);

        m_win32Handle = nullptr;
    }

    void NativeWindowImpl_Win32::InitWindow(const AZStd::string& title,
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
            windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
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

    void NativeWindowImpl_Win32::SetWindowTitle(const AZStd::string& title)
    {
        AZStd::wstring titleW;
        AZStd::to_wstring(titleW, title);
        SetWindowTextW(m_win32Handle, titleW.c_str());
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

            LPBYTE rawInputBytes = new BYTE[rawInputSize];
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInputBytes, &rawInputSize, rawInputHeaderSize);

            RAWINPUT* rawInput = (RAWINPUT*)rawInputBytes;
            AzFramework::RawInputNotificationBusWindows::Broadcast(
                &AzFramework::RawInputNotificationBusWindows::Events::OnRawInputEvent, *rawInput);

            delete [] rawInputBytes;
            break;
        }
        case WM_CHAR:
        {
            const unsigned short codeUnitUTF16 = static_cast<unsigned short>(wParam);
            AzFramework::RawInputNotificationBusWindows::Broadcast(&AzFramework::RawInputNotificationsWindows::OnRawInputCodeUnitUTF16Event, codeUnitUTF16);
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
            DEVMODE DisplayConfig;
            EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &DisplayConfig);
            uint32_t refreshRate = DisplayConfig.dmDisplayFrequency;
            WindowNotificationBus::Event(
                nativeWindowImpl->GetWindowHandle(), &WindowNotificationBus::Events::OnRefreshRateChanged, refreshRate);
            shouldBubbleEventUp = true;
            break;
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

            if (m_activated)
            {
                WindowNotificationBus::Event(m_win32Handle, &WindowNotificationBus::Events::OnWindowResized, width, height);
            }
        }
    }

    void NativeWindowImpl_Win32::ResizeClientArea(WindowSize clientAreaSize)
    {
        RECT rect = {};
        GetClientRect(m_win32Handle, &rect);
        rect.right = rect.left + clientAreaSize.m_width;
        rect.bottom = rect.top + clientAreaSize.m_height;
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

        SetWindowPos(m_win32Handle, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE);
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

        // Get the monitor on which the window is currently displayed.
        HMONITOR monitor = MonitorFromWindow(m_win32Handle, MONITOR_DEFAULTTONEAREST);
        if (!monitor)
        {
            AZ_Warning("NativeWindowImpl_Win32::EnterBorderlessWindowFullScreen", false,
                       "Could not find any monitor.");            
            return;
        }

        // Get the dimensions of the display device on which the window is currently displayed.
        MONITORINFO monitorInfo;
        memset(&monitorInfo, 0, sizeof(MONITORINFO)); // C4701 potentially uninitialized local variable 'monitorInfo' used
        monitorInfo.cbSize = sizeof(MONITORINFO);
        const BOOL success = monitor ? GetMonitorInfo(monitor, &monitorInfo) : FALSE;
        if (!success)
        {
            AZ_Warning("NativeWindowImpl_Win32::SetFullScreenState", false,
                       "Could not get monitor info.");            
            return;
        }

        // Store the current window rect and style so we can restore them when exiting full screen.
        GetWindowRect(m_win32Handle, &m_windowRectToRestoreOnFullScreenExit);
        const UINT currentWindowStyle = GetWindowLong(m_win32Handle, GWL_STYLE);
        m_windowStyleToRestoreOnFullScreenExit = currentWindowStyle;

        // Style, resize, and position the window such that it fills the entire screen.
        const RECT fullScreenWindowRect = monitorInfo.rcMonitor;
        const UINT fullScreenWindowStyle = currentWindowStyle & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME);
        SetWindowLong(m_win32Handle, GWL_STYLE, fullScreenWindowStyle);
        SetWindowPos(m_win32Handle,
                        HWND_TOPMOST,
                        fullScreenWindowRect.left,
                        fullScreenWindowRect.top,
                        fullScreenWindowRect.right,
                        fullScreenWindowRect.bottom,
                        SWP_FRAMECHANGED | SWP_NOACTIVATE);
        ShowWindow(m_win32Handle, SW_MAXIMIZE);

        m_isInBorderlessWindowFullScreenState = true;
    }

    void NativeWindowImpl_Win32::ExitBorderlessWindowFullScreen()
    {
        if (!m_isInBorderlessWindowFullScreenState)
        {
            return;
        }

        // Restore the style, size, and position of the window.
        SetWindowLong(m_win32Handle, GWL_STYLE, m_windowStyleToRestoreOnFullScreenExit);
        SetWindowPos(m_win32Handle,
                        HWND_NOTOPMOST,
                        m_windowRectToRestoreOnFullScreenExit.left,
                        m_windowRectToRestoreOnFullScreenExit.top,
                        m_windowRectToRestoreOnFullScreenExit.right - m_windowRectToRestoreOnFullScreenExit.left,
                        m_windowRectToRestoreOnFullScreenExit.bottom - m_windowRectToRestoreOnFullScreenExit.top,
                        SWP_FRAMECHANGED | SWP_NOACTIVATE);
        ShowWindow(m_win32Handle, SW_NORMAL);

        m_isInBorderlessWindowFullScreenState = false;
    }
} // namespace AzFramework
