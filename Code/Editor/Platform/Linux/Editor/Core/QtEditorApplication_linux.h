/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Editor/Core/QtEditorApplication.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#endif

using xcb_connection_t = struct xcb_connection_t;

namespace Editor
{
    class EditorQtApplicationXcb
        : public EditorQtApplication
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
    {
        Q_OBJECT
    public:
        EditorQtApplicationXcb(int& argc, char** argv)
            : EditorQtApplication(argc, argv)
        {
            // Connect bus to listen for OnStart/StopPlayInEditor events
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        }

        xcb_connection_t* GetXcbConnectionFromQt();

        ///////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEntityContextNotificationBus overrides
        void OnStartPlayInEditor() override;
        void OnStopPlayInEditor() override;

        // QAbstractNativeEventFilter:
        bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;
    };
} // namespace Editor
