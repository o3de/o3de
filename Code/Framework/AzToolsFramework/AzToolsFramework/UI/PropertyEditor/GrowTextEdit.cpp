/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // 4251: 'QRawFont::d': class 'QExplicitlySharedDataPointer<QRawFontPrivate>' needs to have dll-interface to be used by clients of class 'QRawFont'
                                                               // 4800: 'QTextEngine *const ': forcing value to bool 'true' or 'false' (performance warning)
#include <QTextBlock>
AZ_POP_DISABLE_WARNING
#include "GrowTextEdit.h"
#include "PropertyQTConstants.h"
#include <QKeyEvent>
#include <QTimer>

namespace AzToolsFramework
{
    const int GrowTextEdit::s_padding = 10;
    const int GrowTextEdit::s_minHeight = PropertyQTConstant_DefaultHeight * 3;
    const int GrowTextEdit::s_maxHeight = PropertyQTConstant_DefaultHeight * 10;

    AZ_CLASS_ALLOCATOR_IMPL(GrowTextEdit, AZ::SystemAllocator)

    GrowTextEdit::GrowTextEdit(QWidget* parent)
        : QTextEdit(parent)
        , m_textChanged(false)
    {
        setMinimumHeight(s_minHeight);
        setMaximumHeight(s_maxHeight);

        connect(this, &GrowTextEdit::textChanged, this, [this]()
        {
            if (isVisible())
            {
                updateGeometry();
            }

            m_textChanged = true;
        });
    }

    void GrowTextEdit::SetText(const AZStd::string& text)
    {
        int cursorPos = textCursor().position();
        setPlainText(text.c_str());
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::MoveAnchor, cursorPos);
        setTextCursor(cursor);
        updateGeometry();
    }

    AZStd::string GrowTextEdit::GetText() const
    {
        return AZStd::string(toPlainText().toUtf8());
    }

    void GrowTextEdit::setVisible(bool visible)
    {
        QTextEdit::setVisible(visible);
        if (visible)
        {
            updateGeometry();
        }
    }

    QSize GrowTextEdit::sizeHint() const
    {
        QSize sizeHint = QTextEdit::sizeHint();
        QSize documentSize = document()->size().toSize();
        sizeHint.setHeight(AZStd::min(s_maxHeight, documentSize.height() + s_padding));
        return sizeHint;
    }

    // =========================================================================
    // Focus-In: Capture text for Esc / Ctrl+Z revert
    // =========================================================================
    void GrowTextEdit::focusInEvent(QFocusEvent* event)
    {
        if (event->reason() != Qt::PopupFocusReason)
        {
            m_textOnFocusIn = toPlainText();
        }
        QTextEdit::focusInEvent(event);
    }

    void GrowTextEdit::focusOutEvent(QFocusEvent* event)
    {
        if (m_textChanged)
        {
            emit EditCompleted();
        }

        m_textChanged = false;
        QTextEdit::focusOutEvent(event);
    }

    // =========================================================================
    // ShortcutOverride: Claim Esc always, Ctrl+Z when text has changed
    // =========================================================================
    bool GrowTextEdit::event(QEvent* event)
    {
        if (event->type() == QEvent::ShortcutOverride)
        {
            auto* ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Escape)
            {
                event->accept();
                return true;
            }
            if (ke->matches(QKeySequence::Undo))
            {
                if (toPlainText() != m_textOnFocusIn)
                {
                    event->accept();
                }
                else
                {
                    event->ignore();
                }
                return true;
            }
        }
        return QTextEdit::event(event);
    }

    // =========================================================================
    // KeyPress: Esc and Ctrl+Z revert to focus-in text and exit field
    // =========================================================================
    void GrowTextEdit::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Escape)
        {
            setPlainText(m_textOnFocusIn);
            m_textChanged = false;
            QTimer::singleShot(0, this, [this]() { clearFocus(); });
            return;
        }
        if (event->matches(QKeySequence::Undo))
        {
            if (toPlainText() != m_textOnFocusIn)
            {
                setPlainText(m_textOnFocusIn);
                m_textChanged = false;
                QTimer::singleShot(0, this, [this]() { clearFocus(); });
                return;
            }
        }
        QTextEdit::keyPressEvent(event);
    }
}

