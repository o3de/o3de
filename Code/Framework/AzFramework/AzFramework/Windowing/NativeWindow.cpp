/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Windowing/NativeWindow.h>

#include <AzCore/Console/IConsole.h>

namespace AzFramework
{
    //////////////////////////////////////////////////////////////////////////
    // Console functions for testing transitions to/from full screen mode
    //////////////////////////////////////////////////////////////////////////

    void FullScreenEnter([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        NativeWindow::SetFullScreenStateOfDefaultWindow(true);
    }
    AZ_CONSOLEFREEFUNC(FullScreenEnter, AZ::ConsoleFunctorFlags::DontReplicate, "Request the default window to enter full screen mode.");

    void FullScreenExit([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        NativeWindow::SetFullScreenStateOfDefaultWindow(false);
    }
    AZ_CONSOLEFREEFUNC(FullScreenExit, AZ::ConsoleFunctorFlags::DontReplicate, "Request the default window to exit full screen mode.");

    void FullScreenToggle([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        NativeWindow::ToggleFullScreenStateOfDefaultWindow();
    }
    AZ_CONSOLEFREEFUNC(FullScreenToggle, AZ::ConsoleFunctorFlags::DontReplicate, "Request the default window to toggle full screen mode.");


    //////////////////////////////////////////////////////////////////////////
    // NativeWindow class implementation
    //////////////////////////////////////////////////////////////////////////

    NativeWindow::NativeWindow(const AZStd::string& title,
                               const WindowGeometry& geometry,
                               const WindowStyleMasks styleMasks)
        : m_pimpl()
    {
        m_pimpl.reset(Implementation::Create());
        m_pimpl->InitWindow(title, geometry, styleMasks);
    }
    
    NativeWindow::~NativeWindow()
    {
        Deactivate();
        m_pimpl.reset();
    }

    void NativeWindow::Activate()
    {
        // If the underlying window implementation failed to create a window,
        // don't attach to the window bus
        if (m_pimpl != nullptr)
        {
            auto windowHandle = m_pimpl->GetWindowHandle();

            WindowRequestBus::Handler::BusConnect(windowHandle);
            m_pimpl->Activate();
        }
    }

    void NativeWindow::Deactivate()
    {
        if (m_pimpl != nullptr)
        {
            auto windowHandle = m_pimpl->GetWindowHandle();
            m_pimpl->Deactivate();
            WindowRequestBus::Handler::BusDisconnect(windowHandle);
        }
    }

    bool NativeWindow::IsActive() const
    {
        return m_pimpl != nullptr && m_pimpl->IsActive();
    }

    void NativeWindow::SetWindowTitle(const AZStd::string& title)
    {
        m_pimpl->SetWindowTitle(title);
    }

    WindowSize NativeWindow::GetClientAreaSize() const
    {
        return m_pimpl->GetClientAreaSize();
    }

    void NativeWindow::ResizeClientArea(WindowSize clientAreaSize)
    {
        m_pimpl->ResizeClientArea(clientAreaSize);
    }

    bool NativeWindow::GetFullScreenState() const
    {
        return m_pimpl->GetFullScreenState();
    }

    void NativeWindow::SetFullScreenState(bool fullScreenState)
    {
        m_pimpl->SetFullScreenState(fullScreenState);
    }

    bool NativeWindow::CanToggleFullScreenState() const
    {
        return m_pimpl->CanToggleFullScreenState();
    }

    void NativeWindow::ToggleFullScreenState()
    {
        SetFullScreenState(!GetFullScreenState());
    }

    /*static*/ bool NativeWindow::GetFullScreenStateOfDefaultWindow()
    {
        NativeWindowHandle defaultWindowHandle = nullptr;
        WindowSystemRequestBus::BroadcastResult(defaultWindowHandle,
                                                &WindowSystemRequestBus::Events::GetDefaultWindowHandle);

        bool defaultWindowFullScreenState = false;
        if (defaultWindowHandle)
        {
            WindowRequestBus::EventResult(defaultWindowFullScreenState,
                                          defaultWindowHandle,
                                          &WindowRequestBus::Events::GetFullScreenState);
        }
        return defaultWindowFullScreenState;
    }

    /*static*/ void NativeWindow::SetFullScreenStateOfDefaultWindow(bool fullScreenState)
    {
        NativeWindowHandle defaultWindowHandle = nullptr;
        WindowSystemRequestBus::BroadcastResult(defaultWindowHandle,
                                                &WindowSystemRequestBus::Events::GetDefaultWindowHandle);
        if (defaultWindowHandle)
        {
            WindowRequestBus::Event(defaultWindowHandle,
                                    &WindowRequestBus::Events::SetFullScreenState,
                                    fullScreenState);
        }
    }

    /*static*/ bool NativeWindow::CanToggleFullScreenStateOfDefaultWindow()
    {
        NativeWindowHandle defaultWindowHandle = nullptr;
        WindowSystemRequestBus::BroadcastResult(defaultWindowHandle,
                                                &WindowSystemRequestBus::Events::GetDefaultWindowHandle);

        bool canToggleFullScreenStateOfDefaultWindow = false;
        if (defaultWindowHandle)
        {
            WindowRequestBus::EventResult(canToggleFullScreenStateOfDefaultWindow,
                                          defaultWindowHandle,
                                          &WindowRequestBus::Events::CanToggleFullScreenState);
        }
        return canToggleFullScreenStateOfDefaultWindow;
    }

    /*static*/ void NativeWindow::ToggleFullScreenStateOfDefaultWindow()
    {
        NativeWindowHandle defaultWindowHandle = nullptr;
        WindowSystemRequestBus::BroadcastResult(defaultWindowHandle,
                                                &WindowSystemRequestBus::Events::GetDefaultWindowHandle);
        if (defaultWindowHandle)
        {
            WindowRequestBus::Event(defaultWindowHandle,
                                    &WindowRequestBus::Events::ToggleFullScreenState);
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // NativeWindow::Implementation class implementation
    //////////////////////////////////////////////////////////////////////////

    void NativeWindow::Implementation::Activate()
    {
        if (!m_activated)
        {
            m_activated = true;
            WindowNotificationBus::Event(GetWindowHandle(), &WindowNotificationBus::Events::OnWindowResized, m_width, m_height);
        }
    }

    void NativeWindow::Implementation::Deactivate()
    {
        if (m_activated) // nothing to do if window was already deactivated
        {
            m_activated = false;
            WindowNotificationBus::Event(GetWindowHandle(), &WindowNotificationBus::Events::OnWindowClosed);
        }
    }

    void NativeWindow::Implementation::SetWindowTitle(const AZStd::string& title)
    {
        // For most implementations this does nothing
        AZ_UNUSED(title);
    }

    WindowSize NativeWindow::Implementation::GetClientAreaSize() const
    {
        return WindowSize(m_width, m_height);
    }

    void NativeWindow::Implementation::ResizeClientArea([[maybe_unused]] WindowSize clientAreaSize)
    {
    }

    bool NativeWindow::Implementation::GetFullScreenState() const
    {
        // For most implementations full screen is the only option
        return true;
    }

    void NativeWindow::Implementation::SetFullScreenState([[maybe_unused]] bool fullScreenState)
    {
        // For most implementations full screen is the only option
    }

    bool NativeWindow::Implementation::CanToggleFullScreenState() const
    {
        // For most implementations full screen is the only option
        return false;
    }

} // namespace AzFramework
