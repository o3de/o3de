/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#import <AppKit/NSEvent.h>

#include "EditorDefs.h"
#include "QtEditorApplication_mac.h"

// AzFramework
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

// Qt
#include <QCursor>

namespace Editor
{
    EditorQtApplication* EditorQtApplication::newInstance(int& argc, char** argv)
    {
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);

        return new EditorQtApplicationMac(argc, argv);
    }

    bool EditorQtApplicationMac::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
    {
        NSEvent* event = (NSEvent*)message;
        if (GetIEditor()->IsInGameMode())
        {
            AzFramework::RawInputNotificationBusMac::Broadcast(&AzFramework::RawInputNotificationsMac::OnRawInputEvent, event);

            auto systemCursorState = AzFramework::SystemCursorState::Unknown;
            AzFramework::InputSystemCursorRequestBus::EventResult(systemCursorState, 
                AzFramework::InputDeviceMouse::Id,
                &AzFramework::InputSystemCursorRequestBus::Events::GetSystemCursorState);

            // filter Qt input events while in game mode if system cursor not visible
            // to prevent the user from activating Qt menu actions
            const bool filterInputEvents = systemCursorState != AzFramework::SystemCursorState::UnconstrainedAndVisible;
            return filterInputEvents;
        }

        if ([event type] == NSEventTypeMouseEntered)
        {
            // filter out mouse enter events when the widget the mouse entered into
            // does not exist anymore. This happened occasionally on startup when hovering
            // over the splash screen - LY-84100
            if (widgetAt(QCursor::pos()) == nullptr)
            {
                return true;
            }
        }

        return false;
    }
} // end namespace Editor
