/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include "EditorCommon.h"
#endif

//! The RulerWidget is drawn above or to the left of the ViewportWidget
class RulerWidget
    : public QWidget
{
    Q_OBJECT

public: // Types

    enum class Orientation
    {
        Horizontal,
        Vertical
    };

public:

    RulerWidget(Orientation orientation, QWidget* parent, EditorWindow* editorWindow);

    //! Get the orientation of this ruler
    Orientation GetOrientation() { return m_orientation; }

    //! Get the origin position in canvas space of the ruler scale
    float GetOrigin() { return m_origin; }

    //! Tell the ruler where the cursor is in global Qt space
    void SetCursorPos(const QPoint& pos);

    //! Draw anything needed in the viewport (during adding of a guide for example)
    void DrawForViewport(Draw2dHelper& draw2d);

public:

    //! Get the the desired breadth of the ruler widgets when shown
    static int GetRulerBreadth();

protected:

    //! We have a custom paintEvent to draw the tick marks and labels
    void paintEvent(QPaintEvent* event) override;

    //! We handle mouse press events for adding guides
    void mousePressEvent(QMouseEvent* ev) override;

    //! We get this after pressing the mouse in the ruler, even if mouse is over viewport
    void mouseMoveEvent(QMouseEvent* ev) override;

    //! We get this after pressing the mouse in the ruler, even if mouse if over viewport
    void mouseReleaseEvent(QMouseEvent* ev) override;

    //! We need this to cancel the interaction if RMB or a Alt+char combo is pressed
    void focusOutEvent(QFocusEvent* ev) override;

    //! Draw the tick marks and text on the ruler
    void DrawTickMarksWithLabels(QPainter* painter, QRectF rulerRect, float translation, float scale);

    //! Draw one section of the ruller scale - a ssection has one major tick mark with a label plus some smaller tick marks
    void DrawRulerSection(QPainter* painter, QRectF rulerRect, float startInCanvasPixels, float sectionLengthInScreenPixels, int numSubdivisions, float translation, float scale);

    //! Draw the line on the rule that shows where the mouse is
    void DrawCursorPos(QPainter* painter, QRectF rulerRect);

private:

    Orientation m_orientation; //!< The orientation of this ruler (never changes once created)

    float m_origin = 0.0f;     //!< Where the origin of the ruler scale is in canvas space (might be modifiable in future)

    float m_cursorPos;         //!< The current cursor position - a value along ruler in local space

    EditorWindow* m_editorWindow;

    ViewportDragInteraction* m_dragInteraction = nullptr;  //! Used for adding guides
};
