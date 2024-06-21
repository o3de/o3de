/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Windows.h>
#include <AzFramework/Windowing/NativeWindow.h>

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/conversions.h>

namespace AzFramework
{
    class NativeWindowImpl_Win32 final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Win32, AZ::SystemAllocator);
        NativeWindowImpl_Win32();
        ~NativeWindowImpl_Win32() override;

        // NativeWindow::Implementation overrides...
        void InitWindowInternal(const AZStd::string& title,
                        const WindowGeometry& geometry,
                        const WindowStyleMasks& styleMasks) override;
        void Activate() override;
        void Deactivate() override;
        NativeWindowHandle GetWindowHandle() const override;
        void SetWindowTitle(const AZStd::string& title) override;

        WindowSize GetMaximumClientAreaSize() const override;
        void ResizeClientArea(WindowSize clientAreaSize, const WindowPosOptions& options) override;
        bool SupportsClientAreaResize() const override { return true; }
        bool GetFullScreenState() const override;
        void SetFullScreenState(bool fullScreenState) override;
        bool CanToggleFullScreenState() const override { return true; }
        float GetDpiScaleFactor() const override;
        uint32_t GetDisplayRefreshRate() const override;

    private:
        RECT GetMonitorRect() const;
        static HWND GetWindowedPriority();

        static DWORD ConvertToWin32WindowStyleMask(const WindowStyleMasks& styleMasks);
        static LRESULT CALLBACK WindowCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        static const wchar_t* s_defaultClassName;

        void WindowSizeChanged(const uint32_t width, const uint32_t height);

        void EnterBorderlessWindowFullScreen();
        void ExitBorderlessWindowFullScreen();

        HWND m_win32Handle = nullptr;
        RECT m_windowRectToRestoreOnFullScreenExit{};          //!< The position and size of the window to restore when exiting full screen.
        UINT m_windowStyleToRestoreOnFullScreenExit{};         //!< The style(s) of the window to restore when exiting full screen.
        UINT m_windowExtendedStyleToRestoreOnFullScreenExit{}; //!< The style(s) of the window to restore when exiting full screen.

        bool m_isInBorderlessWindowFullScreenState = false;    //!< Was a borderless window used to enter full screen state?
        bool m_shouldEnterFullScreenStateOnActivate = false;   //!< Should we enter full screen state when the window is activated?

        using GetDpiForWindowType = UINT(*)(HWND hwnd);
        GetDpiForWindowType m_getDpiFunction = nullptr;
        uint32_t m_mainDisplayRefreshRate = 0;
    };
} // namespace AzFramework
