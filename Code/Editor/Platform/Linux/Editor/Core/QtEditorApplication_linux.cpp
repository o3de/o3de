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
#include <qpa/qplatformnativeinterface.h>
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
#endif
            return true;
        }
        return false;
    }
} // namespace Editor
