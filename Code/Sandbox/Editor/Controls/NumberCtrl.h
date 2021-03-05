/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
