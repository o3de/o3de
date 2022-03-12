/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QtEditorApplication_windows.h"

// Qt
#include <QAbstractEventDispatcher>
#include <QScopedValueRollback>
#include <QToolBar>
#include <QLoggingCategory>
#include <QTimer>

#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qpa/qplatformnativeinterface.h>

// AzQtComponents
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>

// AzFramework
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

namespace Editor
{
    EditorQtApplication* EditorQtApplication::newInstance(int& argc, char** argv)
    {
        return new EditorQtApplicationWindows(argc, argv);
    }

    bool EditorQtApplicationWindows::nativeEventFilter([[maybe_unused]] const QByteArray& eventType, void* message, long* result)
    {
        MSG* msg = (MSG*)message;

        if (msg->message == WM_MOVING || msg->message == WM_SIZING)
        {
            m_isMovingOrResizing = true;
        }
        else if (msg->message == WM_EXITSIZEMOVE)
        {
            m_isMovingOrResizing = false;
        }

        // Prevent the user from being able to move the window in game mode.
        // This is done during the hit test phase to bypass the native window move messages. If the window
        // decoration wrapper title bar contains the cursor, set the result to HTCLIENT instead of
        // HTCAPTION.
        if (msg->message == WM_NCHITTEST && GetIEditor()->IsInGameMode())
        {
            const LRESULT defWinProcResult = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
            if (defWinProcResult == 1)
            {
                if (QWidget* widget = QWidget::find((WId)msg->hwnd))
                {
                    if (auto wrapper = qobject_cast<const AzQtComponents::WindowDecorationWrapper*>(widget))
                    {
                        AzQtComponents::TitleBar* titleBar = wrapper->titleBar();
                        const short global_x = static_cast<short>(LOWORD(msg->lParam));
                        const short global_y = static_cast<short>(HIWORD(msg->lParam));

                        const QPoint globalPos = QHighDpi::fromNativePixels(QPoint(global_x, global_y), widget->window()->windowHandle());
                        const QPoint local = titleBar->mapFromGlobal(globalPos);
                        if (titleBar->draggableRect().contains(local) && !titleBar->isTopResizeArea(globalPos))
                        {
                            *result = HTCLIENT;
                            return true;
                        }
                    }
                }
            }
        }

        // Ensure that the Windows WM_INPUT messages get passed through to the AzFramework input system.
        // These events are only broadcast in game mode. In Editor mode, RenderViewportWidget creates synthetic
        // keyboard and mouse events via Qt.
        if (GetIEditor()->IsInGameMode())
        {
            if (msg->message == WM_INPUT)
            {
                UINT rawInputSize;
                const UINT rawInputHeaderSize = sizeof(RAWINPUTHEADER);
                GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, nullptr, &rawInputSize, rawInputHeaderSize);

                AZStd::array<BYTE, sizeof(RAWINPUT)> rawInputBytesArray;
                LPBYTE rawInputBytes = rawInputBytesArray.data();

                [[maybe_unused]] const UINT bytesCopied =
                    GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, rawInputBytes, &rawInputSize, rawInputHeaderSize);
                CRY_ASSERT(bytesCopied == rawInputSize);

                RAWINPUT* rawInput = (RAWINPUT*)rawInputBytes;
                CRY_ASSERT(rawInput);

                AzFramework::RawInputNotificationBusWindows::Broadcast(
                    &AzFramework::RawInputNotificationsWindows::OnRawInputEvent, *rawInput);

                return false;
            }
            else if (msg->message == WM_DEVICECHANGE)
            {
                if (msg->wParam == 0x0007) // DBT_DEVNODES_CHANGED
                {
                    AzFramework::RawInputNotificationBusWindows::Broadcast(
                        &AzFramework::RawInputNotificationsWindows::OnRawInputDeviceChangeEvent);
                }
                return true;
            }
        }

        return false;
    }

    bool EditorQtApplicationWindows::eventFilter(QObject* object, QEvent* event)
    {
        switch (event->type())
        {
        case QEvent::Leave:
            {
                // if we receive a leave event for a toolbar on Windows
                // check first whether we really left it. If we didn't: start checking
                // for the tool bar under the mouse by timer to check when we really left.
                // Synthesize a new leave event then. Workaround for LY-69788
                auto toolBarAt = [](const QPoint& pos) -> QToolBar*
                {
                    QWidget* widget = qApp->widgetAt(pos);
                    while (widget != nullptr)
                    {
                        if (QToolBar* tb = qobject_cast<QToolBar*>(widget))
                        {
                            return tb;
                        }
                        widget = widget->parentWidget();
                    }
                    return false;
                };
                if (object == toolBarAt(QCursor::pos()))
                {
                    QTimer* t = new QTimer(object);
                    t->start(100);
                    connect(
                        t, &QTimer::timeout, object,
                        [t, object, toolBarAt]()
                        {
                            if (object != toolBarAt(QCursor::pos()))
                            {
                                QEvent event(QEvent::Leave);
                                qApp->sendEvent(object, &event);
                                t->deleteLater();
                            }
                        });
                    return true;
                }
                break;
            }
        default:
            break;
        }

        return EditorQtApplication::eventFilter(object, event);
    }
} // namespace Editor
