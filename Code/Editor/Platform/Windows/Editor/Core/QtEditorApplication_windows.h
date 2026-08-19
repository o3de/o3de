/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Buses/Notifications/InputDeviceNotificationBus.h>

#include <Editor/Core/QtEditorApplication.h>

namespace Editor
{
    class EditorQtApplicationWindows
    : public EditorQtApplication
    , private AzFramework::InputDeviceNotificationBus::Handler
    {
    public:
        EditorQtApplicationWindows(int& argc, char** argv)
            : EditorQtApplication(argc, argv)
        {
            AzFramework::InputDeviceNotificationBus::Handler::BusConnect();
        }

        ~EditorQtApplicationWindows() override
        {
            AzFramework::InputDeviceNotificationBus::Handler::BusDisconnect();
        }

        // QAbstractNativeEventFilter:
        bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

        bool eventFilter(QObject* object, QEvent* event) override;

    private:
        void OnInputDeviceConnectedEvent(const AzFramework::InputDevice& inputDevice) override;
        void OnInputDeviceDisconnectedEvent(const AzFramework::InputDevice& inputDevice) override;
    };
} // namespace Editor
