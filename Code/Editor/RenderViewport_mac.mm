/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QAbstractNativeEventFilter>
#include <QApplication>

#include <Carbon/Carbon.h>
#include <Cocoa/Cocoa.h>

class HiddenMouseNativeEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override
    {
        assert(eventType == "mac_generic_NSEvent");
        NSEvent *event = (NSEvent *)message;
        switch ([event type])
        {
        case NSEventTypeRightMouseDragged:
            sendFakeMouseMoveEvent([event deltaX], [event deltaY], Qt::RightButton);
            return true;
        case NSEventTypeOtherMouseDragged:
            sendFakeMouseMoveEvent([event deltaX], [event deltaY], Qt::MiddleButton);
            return true;
        default:
            return false;
        }
    }

    void sendFakeMouseMoveEvent(int deltaX, int deltaY, Qt::MouseButtons buttons)
    {
        assert(m_target);
        QMetaObject::invokeMethod(m_target, "InjectFakeMouseMove", Q_ARG(int, deltaX), Q_ARG(int, deltaY), Q_ARG(Qt::MouseButtons, buttons));
    }

    QObject *m_target = nullptr;
};

Q_GLOBAL_STATIC(HiddenMouseNativeEventFilter, eventFilter)

void StopFixedCursorMode()
{
    qApp->removeNativeEventFilter(eventFilter());
    eventFilter()->m_target = nullptr;
    CGAssociateMouseAndMouseCursorPosition(true);
}

void StartFixedCursorMode(QObject* viewport)
{
    CGAssociateMouseAndMouseCursorPosition(false);
    eventFilter()->m_target = viewport;
    qApp->installNativeEventFilter(eventFilter());
}
