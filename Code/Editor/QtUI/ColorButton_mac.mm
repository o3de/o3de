/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#import <Cocoa/Cocoa.h>

#include <QTimer>
#include <QVariant>
#include <QWidget>

// this makes sure the native widget of the window
// has the same window level as it should have
// the problem is that for some reason the window level is changed to 0
// unfortunately not directly so we have to check repeated times
void macRaiseWindowDelayed(QWidget* window)
{
    NSWindow* nativeWindow = [(NSView*)(window->winId()) window];
    CGWindowLevel level = static_cast<CGWindowLevel>(nativeWindow.level);
    QTimer* t = new QTimer(window);
    QObject::connect(t, &QTimer::timeout, t, [t,nativeWindow,level] {
        if (nativeWindow.level != level)
        {
            // level was changed -> we can break as soon as we fixed it
            t->setProperty("raiseCounter", 99);
        }
        if (nativeWindow.level == level && t->property("raiseCounter").toInt() >= 99)
        {
            // break condition met and correct level -> end this
            t->deleteLater();
        }
        t->setProperty("raiseCounter", t->property("raiseCounter").toInt() + 1);
        // try to set the right level
        nativeWindow.level = level;
    });
    t->start(10);
}
