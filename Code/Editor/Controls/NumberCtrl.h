/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CONTROLS_NUMBERCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_NUMBERCTRL_H
#pragma once

// NumberCtrl.h : header file
//

#if !defined(Q_MOC_RUN)
#include <QDoubleSpinBox>
#endif

class QNumberCtrl
    : public QDoubleSpinBox
{
    Q_OBJECT

public:
    QNumberCtrl(QWidget* parent = nullptr);

    bool IsDragging() const { return m_bDragged; }

    //! If called will enable undo with given text when control is modified.
    void EnableUndo(const QString& undoText);
    void SetRange(double newMin, double maxRange);

Q_SIGNALS:
    void dragStarted();
    void dragFinished();

    void valueUpdated();
    void valueChanged();

    void mouseReleased();
    void mousePressed();

protected:
    void changeEvent(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void onEditingFinished();
    void onValueChanged(double d);

    bool m_bMouseDown;
    bool m_bDragged;
    QPoint m_mousePos;
    bool m_bUndoEnabled;
    double m_prevValue;
    QString m_undoText;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_NUMBERCTRL_H
