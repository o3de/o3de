/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/GlobalEventFilter.h>

#include <QApplication>
#include <QScopedValueRollback>
#include <QWheelEvent>
#include <QWidget>

namespace AzQtComponents
{
    GlobalEventFilter::GlobalEventFilter(QObject* parent)
        : QObject(parent)
    {

    }

    bool GlobalEventFilter::eventFilter(QObject* obj, QEvent* e)
    {
        static bool isRecursing = false;

        if (isRecursing)
        {
            return false;
        }

        QScopedValueRollback<bool> guard(isRecursing, true);

        switch (e->type())
        {
            case QEvent::Wheel:
            {
                auto wheelEvent = static_cast<QWheelEvent*>(e);

                // If the scroll event is set to something other than begin/no phase, let the phased scroll logic in
                // QApplication::notify handle the event
                if (wheelEvent->phase() != Qt::NoScrollPhase && wheelEvent->phase() != Qt::ScrollBegin)
                {
                    return false;
                }

                // Make the wheel event fall through to windows underneath the mouse, even if they don't have focus. If
                // we don't do this, the wheel event gets turned into a focus event, followed by a wheel event. This
                // would cause a user scrolling on an unfocused QSpinBox to accidentally change the value, rather than
                // scrolling the view.
                QWidget* widget = QApplication::widgetAt(wheelEvent->globalPosition().toPoint());
                if (widget && obj != widget)
                {
                    // Run the wheel event up the hierarchy of the target widget until the event is accepted, as the event
                    // is no longer being propagated automatically in Qt5.15
                    while (widget)
                    {
                        QPoint mappedPos = widget->mapFromGlobal(wheelEvent->globalPosition().toPoint());

                        QWheelEvent wheelEventCopy = QWheelEvent(
                            mappedPos,
                            wheelEvent->globalPosition().toPoint(),
                            wheelEvent->pixelDelta(),
                            wheelEvent->angleDelta(),
                            wheelEvent->buttons(),
                            wheelEvent->modifiers(),
                            wheelEvent->phase(),
                            wheelEvent->inverted(),
                            wheelEvent->source()
                        );
                        QApplication::instance()->sendEvent(widget, &wheelEventCopy);
                        if (wheelEventCopy.isAccepted())
                        {
                            return true;
                        }
                        widget = widget->parentWidget();
                    }
                }
            }
            break;
        }

        return false;
    }
} // namespace AzQtComponents
