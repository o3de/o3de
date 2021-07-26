/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WindowObserver_mac.h"

#include <AppKit/AppKit.h>

#include <QApplication>
#include <QtGui/qpa/qplatformnativeinterface.h>


@interface NSWindowObserver : NSObject
{
    Editor::WindowObserver* m_observer;
    NSWindow* m_window;
}
@end

@implementation NSWindowObserver

- (id)initWithWindow:(NSWindow*)window :(Editor::WindowObserver*)observer
{
    if ((self = [super init])) {
        m_observer = observer;
        m_window = window;
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(onWindowWillMove)
                                                     name:NSWindowWillMoveNotification
                                                   object:m_window];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(onWindowDidMove)
                                                     name:NSWindowDidMoveNotification
                                                   object:m_window];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(onWindowWillStartLiveResize)
                                                     name:NSWindowWillStartLiveResizeNotification
                                                   object:m_window];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(onWindowDidLiveResize)
                                                     name:NSWindowDidEndLiveResizeNotification
                                                   object:m_window];

    }
    return self;
}

-(void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

- (void)onWindowWillMove
{
    m_observer->setWindowIsMoving(true);
    // This notification only indicates that the window is likely to move because the used clicked
    // the title bar. If the user releases the mouse without moving the title bar we won't receive
    // a NSWindowDidMoveNotification, so we add an additional observer to check the state of the
    // mouse. Note that we don't care whether the window moved or not, we just want to be able to
    // disable rendering when it is likely
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onWindowDidUpdate:)
                                                 name:NSWindowDidUpdateNotification
                                               object:m_window];
}

- (void)onWindowDidMove
{
    m_observer->setWindowIsMoving(false);
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:NSWindowDidUpdateNotification
                                                  object:m_window];
}

- (void)onWindowDidUpdate:(NSNotification*)notification
{
    static const unsigned long leftMouseButton = 1;  // a value of 1 << 0 corresponds to the left mouse button

    if (([NSEvent pressedMouseButtons] & leftMouseButton) == 0)
    {
        m_observer->setWindowIsMoving(false);
        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                        name:NSWindowDidUpdateNotification
                                                      object:m_window];
    }
}

- (void)onWindowWillStartLiveResize
{
    m_observer->setWindowIsResizing(true);
}

- (void)onWindowDidLiveResize
{
    m_observer->setWindowIsResizing(false);
}

@end

namespace Editor
{
    WindowObserver::WindowObserver(QWindow* window, QObject* parent)
        : QObject(parent)
        , m_windowIsMoving(false)
        , m_windowIsResizing(false)
        , m_windowObserver(nullptr)
    {
        if (!window)
        {
            return;
        }

        QPlatformNativeInterface* nativeInterface = QApplication::platformNativeInterface();
        NSWindow* nsWindow = static_cast<NSWindow*>(nativeInterface->nativeResourceForWindow("nswindow", window));
        if (nsWindow == nullptr)
        {
            return;
        }

        m_windowObserver = [[NSWindowObserver alloc] initWithWindow:nsWindow:this];
    }

    WindowObserver::~WindowObserver()
    {
        if (m_windowObserver != nil)
        {
            [m_windowObserver release];
            m_windowObserver = nil;
        }
    }

    void WindowObserver::setWindowIsMoving(bool isMoving)
    {
        if (isMoving == m_windowIsMoving)
        {
            return;
        }

        const bool wasMovingOrResizing = m_windowIsMoving || m_windowIsResizing;
        m_windowIsMoving = isMoving;
        const bool isMovingOrResizing = m_windowIsMoving || m_windowIsResizing;

        if (isMovingOrResizing != wasMovingOrResizing)
        {
            emit windowIsMovingOrResizingChanged(isMovingOrResizing);
        }
    }

    void WindowObserver::setWindowIsResizing(bool isResizing)
    {
        if (isResizing == m_windowIsResizing)
        {
            return;
        }

        const bool wasMovingOrResizing = m_windowIsMoving || m_windowIsResizing;
        m_windowIsResizing = isResizing;
        const bool isMovingOrResizing = m_windowIsMoving || m_windowIsResizing;

        if (isMovingOrResizing != wasMovingOrResizing)
        {
            emit windowIsMovingOrResizingChanged(isMovingOrResizing);
        }
    }
}

#include <moc_WindowObserver_mac.cpp>
