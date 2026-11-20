/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QtEditorApplication_linux.h"

#include <qguiapplication_platform.h>

#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#include <AzFramework/XcbEventHandler.h>
#include <AzFramework/XcbConnectionManager.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/XcbEventHandler.h>
#endif

#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
#include <AzFramework/WaylandInterface.h>
#include <qpa/qplatformnativeinterface.h>
#endif

namespace Editor
{
    EditorQtApplication* EditorQtApplication::newInstance(int& argc, char** argv)
    {
        return new EditorQtApplicationLinux(argc, argv);
    }

    EditorQtApplicationLinux::EditorQtApplicationLinux(int& argc, char** argv)
        : EditorQtApplication(argc, argv)
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
        if (platformName() == "wayland")
        {
            m_wayland = true;
            AzFramework::WaylandDisplayProviderInterface::Register(this);
        }
#endif
    }

    int EditorQtApplicationLinux::GetTickOrder()
    {
        return AZ::TICK_INPUT;
    }

    void EditorQtApplicationLinux::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        #if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
        if (m_wayland)
        {
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);
        }
        #endif
    }

#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
    wl_display* EditorQtApplicationLinux::GetWaylandDisplayFromQt() const
    {
        auto *waylandApp = nativeInterface<QNativeInterface::QWaylandApplication>();
        AZ_Warning("EditorQtApplicationWayland", waylandApp, "Unable to retrieve the native platform interface");
        if (!waylandApp)
        {
            return nullptr;
        }

        return waylandApp->display();
    }

    wl_display* EditorQtApplicationLinux::GetWaylandDisplay() const
    {
        return GetWaylandDisplayFromQt();
    }

    int EditorQtApplicationLinux::GetDisplayFD() const
    {
        return wl_display_get_fd(GetWaylandDisplayFromQt());
    }
#endif

#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    xcb_connection_t* EditorQtApplicationLinux::GetXcbConnectionFromQt()
    {
        QNativeInterface::QX11Application* x11App = QGuiApplication::nativeInterface<QNativeInterface::QX11Application>();
        AZ_Warning("EditorQtApplicationXcb", x11App, "Unable to retrieve the native platform interface");
        if (!x11App)
        {
            return nullptr;
        }
        return x11App->connection();
    }
#endif

    void EditorQtApplicationLinux::OnStartPlayInEditor()
    {
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
        if (m_wayland)
        {
            AZ::TickBus::Handler::BusConnect();
            return;
        }
#endif
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        auto* interface = AzFramework::XcbConnectionManagerInterface::Get();
        interface->SetEnableXInput(GetXcbConnectionFromQt(), true);
#endif
    }

    void EditorQtApplicationLinux::OnStopPlayInEditor()
    {
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
        if (m_wayland)
        {
            AZ::TickBus::Handler::BusDisconnect();
            return;
        }
#endif
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        auto* interface = AzFramework::XcbConnectionManagerInterface::Get();
        interface->SetEnableXInput(GetXcbConnectionFromQt(), false);

        AzFramework::XcbEventHandlerBus::Broadcast(&AzFramework::XcbEventHandler::ResetStoredInputStates);
#endif
    }

    bool EditorQtApplicationLinux::nativeEventFilter([[maybe_unused]] const QByteArray& eventType, void* message, qintptr*)
    {
        if (m_wayland)
        {
            return false;
        }

        if (GetIEditor()->IsInGameMode())
        {
#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
            AzFramework::XcbEventHandlerBus::Broadcast(
                &AzFramework::XcbEventHandler::HandleXcbEvent, static_cast<xcb_generic_event_t*>(message));

            const auto event = static_cast<xcb_generic_event_t*>(message);
            if ((event->response_type & AzFramework::s_XcbResponseTypeMask) == XCB_CLIENT_MESSAGE)
            {
                // Do not filter out XCB_CLIENT_MESSAGE events. These include
                // _NET_WM_PING events, which window managers use to detect if
                // an application is still responding. When Qt creates the
                // window, it sets the _NET_WM_PING atom of the WM_PROTOCOLS
                // property, so window managers will expect the application to
                // support this protocol. By skipping the filtering of this
                // event, Qt processes the ping event normally, so that window
                // managers do not think that the Editor has stopped
                // responding.
                return false;
            }

            auto systemCursorState = AzFramework::SystemCursorState::Unknown;
            AzFramework::InputSystemCursorRequestBus::EventResult(systemCursorState, AzFramework::InputDeviceMouse::Id, &AzFramework::InputSystemCursorRequestBus::Events::GetSystemCursorState);
            if(systemCursorState == AzFramework::SystemCursorState::UnconstrainedAndVisible)
            {
                // If the system cursor is visible and unconstrained, the user
                // can interact with the editor so allow all events.
                return false;
            }
#endif
            // Consume all input so the user cannot use editor menu actions
            // while in game.
            return true;
        }
        return false;
    }
} // namespace Editor
