/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/LineEditRevertHandler.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QLineEdit>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QTimer>
AZ_POP_DISABLE_WARNING

namespace AzQtComponents
{
    LineEditRevertHandler::LineEditRevertHandler(QLineEdit* lineEdit, QWidget* clearFocusTarget)
        : QObject(lineEdit)
        , m_lineEdit(lineEdit)
        , m_clearFocusTarget(clearFocusTarget ? clearFocusTarget : lineEdit)
    {
        m_lineEdit->installEventFilter(this);
    }

    bool LineEditRevertHandler::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched != m_lineEdit)
        {
            return QObject::eventFilter(watched, event);
        }

        switch (event->type())
        {
            // =================================================================
            // Focus-In: Capture text for Esc / Ctrl+Z revert
            // =================================================================
            case QEvent::FocusIn:
            {
                auto* fe = static_cast<QFocusEvent*>(event);
                if (fe->reason() != Qt::PopupFocusReason)
                {
                    m_textOnFocusIn = m_lineEdit->text();
                }
                break;
            }

            // =================================================================
            // ShortcutOverride: Claim Esc always, Ctrl+Z when text has changed
            // =================================================================
            case QEvent::ShortcutOverride:
            {
                if (m_lineEdit->isReadOnly())
                {
                    break;
                }
                auto* ke = static_cast<QKeyEvent*>(event);
                if (ke->key() == Qt::Key_Escape)
                {
                    event->accept();
                    return true;
                }
                if (ke->matches(QKeySequence::Undo))
                {
                    if (m_lineEdit->text() != m_textOnFocusIn)
                    {
                        event->accept();
                    }
                    else
                    {
                        event->ignore();
                    }
                    return true;
                }
                break;
            }

            // =================================================================
            // KeyPress: Esc and Ctrl+Z revert to focus-in text and exit field
            // =================================================================
            case QEvent::KeyPress:
            {
                if (m_lineEdit->isReadOnly())
                {
                    break;
                }
                auto* ke = static_cast<QKeyEvent*>(event);
                if (ke->key() == Qt::Key_Escape)
                {
                    m_lineEdit->setText(m_textOnFocusIn);
                    QWidget* target = m_clearFocusTarget;
                    QTimer::singleShot(0, target, [target]() { target->clearFocus(); });
                    return true;
                }
                if (ke->matches(QKeySequence::Undo))
                {
                    if (m_lineEdit->text() != m_textOnFocusIn)
                    {
                        m_lineEdit->setText(m_textOnFocusIn);
                        QWidget* target = m_clearFocusTarget;
                        QTimer::singleShot(0, target, [target]() { target->clearFocus(); });
                        return true;
                    }
                }
                break;
            }

            default:
                break;
        }

        return QObject::eventFilter(watched, event);
    }

} // namespace AzQtComponents
