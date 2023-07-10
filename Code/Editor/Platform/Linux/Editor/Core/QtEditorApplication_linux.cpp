/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QtEditorApplication_linux.h"

#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#include <AzFramework/XcbEventHandler.h>
#include <AzFramework/XcbConnectionManager.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <qpa/qplatformnativeinterface.h>
#include <AzFramework/XcbEventHandler.h>
#endif

namespace Editor
{
    EditorQtApplication* EditorQtApplication::newInstance(int& argc, char** argv)
    {
#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        return new EditorQtApplicationXcb(argc, argv);
#endif

        return nullptr;
    }

    xcb_connection_t* EditorQtApplicationXcb::GetXcbConnectionFromQt()
    {
        QPlatformNativeInterface* native = platformNativeInterface();
        AZ_Warning("EditorQtApplicationXcb", native, "Unable to retrieve the native platform interface");
        if (!native)
        {
            return nullptr;
        }
        return reinterpret_cast<xcb_connection_t*>(native->nativeResourceForIntegration(QByteArray("connection")));
    }

    void EditorQtApplicationXcb::OnStartPlayInEditor()
    {
        auto* interface = AzFramework::XcbConnectionManagerInterface::Get();
        interface->SetEnableXInput(GetXcbConnectionFromQt(), true);
    }

    void EditorQtApplicationXcb::OnStopPlayInEditor()
    {
        auto* interface = AzFramework::XcbConnectionManagerInterface::Get();
        interface->SetEnableXInput(GetXcbConnectionFromQt(), false);
    }

    bool EditorQtApplicationXcb::nativeEventFilter([[maybe_unused]] const QByteArray& eventType, void* message, long*)
    {
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
