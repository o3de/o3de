/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "ClickableLabel.h"


ClickableLabel::ClickableLabel(const QString& text, QWidget* parent)
    : QLabel(parent)
    , m_text(text)
    , m_showDecoration(false)
{
    setTextFormat(Qt::RichText);
    setTextInteractionFlags(Qt::TextBrowserInteraction);
}

ClickableLabel::ClickableLabel(QWidget* parent)
    : QLabel(parent)
    , m_showDecoration(false)
{
    setTextFormat(Qt::RichText);
    setTextInteractionFlags(Qt::TextBrowserInteraction);
}

void ClickableLabel::showEvent([[maybe_unused]] QShowEvent* event)
{
    updateFormatting(false);
}

void ClickableLabel::enterEvent(QEvent* ev)
{
    if (!isEnabled())
    {
        return;
    }

    updateFormatting(true);
    QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
    QLabel::enterEvent(ev);
}

void ClickableLabel::leaveEvent(QEvent* ev)
{
    if (!isEnabled())
    {
        return;
    }

    updateFormatting(false);
    QApplication::restoreOverrideCursor();
    QLabel::leaveEvent(ev);
}

void ClickableLabel::setText(const QString& text)
{
    m_text = text;
    QLabel::setText(text);
    updateFormatting(false);
}

void ClickableLabel::setShowDecoration(bool b)
{
    m_showDecoration = b;
    updateFormatting(false);
}

void ClickableLabel::updateFormatting(bool mouseOver)
{
    //FIXME: this should be done differently. Using a style sheet would be easiest.

    QColor c = palette().color(QPalette::WindowText);
    if (mouseOver || m_showDecoration)
    {
        QLabel::setText(QString(R"(<a href="dummy" style="color: %1";>%2</a>)").arg(c.name(), m_text));
    }
    else
    {
        QLabel::setText(m_text);
    }
}

bool ClickableLabel::event(QEvent* e)
{
    if (isEnabled())
    {
        if (e->type() == QEvent::MouseButtonDblClick)
        {
            emit linkActivated(QString());
            return true; //ignore
        }
    }

    return QLabel::event(e);
}

#include <QtUI/moc_ClickableLabel.cpp>
