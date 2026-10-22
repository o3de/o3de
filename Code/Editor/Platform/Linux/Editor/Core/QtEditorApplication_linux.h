/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <Editor/Core/QtEditorApplication.h>

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
using xcb_connection_t = struct xcb_connection_t;
#endif

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
#include "AzFramework/WaylandConnectionManager.h"
#endif


namespace Editor
{
    class EditorQtApplicationLinux
        : public EditorQtApplication
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , public AZ::TickBus::Handler
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
        , public AzFramework::WaylandDisplayProvider
#endif

    {
    public:
        EditorQtApplicationLinux(int& argc, char** argv);

        int GetTickOrder() override;
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
        wl_display* GetWaylandDisplayFromQt() const;
        wl_display* GetWaylandDisplay() const override;
        int GetDisplayFD() const override;
#endif

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        xcb_connection_t* GetXcbConnectionFromQt();
#endif

        ///////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEntityContextNotificationBus overrides
        void OnStartPlayInEditor() override;
        void OnStopPlayInEditor() override;

        // QAbstractNativeEventFilter:
        bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

        bool m_wayland = false;
    };
} // namespace Editor
