/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "NumberCtrl.h"


QNumberCtrl::QNumberCtrl(QWidget* parent)
    : QDoubleSpinBox(parent)
    , m_bMouseDown(false)
    , m_bDragged(false)
    , m_bUndoEnabled(false)
    , m_prevValue(0)
{
    connect(this, &QAbstractSpinBox::editingFinished, this, &QNumberCtrl::onEditingFinished);
}

void QNumberCtrl::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange)
    {
        setButtonSymbols(isEnabled() ? UpDownArrows : NoButtons);
    }

    QDoubleSpinBox::changeEvent(event);
}


void QNumberCtrl::SetRange(double newMin, double newMax)
{
    // Avoid setting this value if its close to the current value, because otherwise qt will pump events into the queue to redraw/etc.
    if ( (!AZ::IsClose(this->minimum(), newMin, DBL_EPSILON)) || (!AZ::IsClose(this->maximum(), newMax, DBL_EPSILON)) )
    {
        setRange(newMin, newMax);
    }
}


void QNumberCtrl::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        emit mousePressed();

        m_bMouseDown = true;
        m_bDragged = false;
        m_mousePos = event->pos();

        if (m_bUndoEnabled && !CUndo::IsRecording())
        {
            GetIEditor()->BeginUndo();
        }

        emit dragStarted();

        grabMouse();
    }

    QDoubleSpinBox::mousePressEvent(event);
}

void QNumberCtrl::mouseReleaseEvent(QMouseEvent* event)
{
    QDoubleSpinBox::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton)
    {
        m_bMouseDown = m_bDragged = false;

        emit valueUpdated();
        emit valueChanged();

        if (m_bUndoEnabled && CUndo::IsRecording())
        {
            GetIEditor()->AcceptUndo(m_undoText);
        }

        emit dragFinished();

        releaseMouse();

        m_prevValue = value();

        emit mouseReleased();
    }
}

void QNumberCtrl::mouseMoveEvent(QMouseEvent* event)
{
    QDoubleSpinBox::mousePressEvent(event);

    if (m_bMouseDown)
    {
        m_bDragged = true;

        int dy = event->pos().y() - m_mousePos.y();
        setValue(value() - singleStep() * dy);

        emit valueUpdated();

        m_mousePos = event->pos();
    }
}

void QNumberCtrl::EnableUndo(const QString& undoText)
{
    m_undoText = undoText;
    m_bUndoEnabled = true;
}

void QNumberCtrl::focusInEvent(QFocusEvent* event)
{
    m_prevValue = value();
    QDoubleSpinBox::focusInEvent(event);
}

void QNumberCtrl::onEditingFinished()
{
    bool undo = m_bUndoEnabled && !CUndo::IsRecording() && m_prevValue != value();
    if (undo)
    {
        GetIEditor()->BeginUndo();
    }

    emit valueUpdated();
    emit valueChanged();

    if (undo)
    {
        GetIEditor()->AcceptUndo(m_undoText);
    }

    m_prevValue = value();
}

#include <Controls/moc_NumberCtrl.cpp>
