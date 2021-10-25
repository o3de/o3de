
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#import <AppKit/NSEvent.h>

#include "EditorDefs.h"
#include "QtEditorApplication.h"

// AzFramework
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

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
            return true;
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
