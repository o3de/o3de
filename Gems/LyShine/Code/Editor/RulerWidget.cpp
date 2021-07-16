/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "RulerWidget.h"
#include "CanvasHelpers.h"
#include "GuideHelpers.h"
#include "QtHelpers.h"
#include "ViewportAddGuideInteraction.h"
#include <LyShine/Bus/UiEditorCanvasBus.h>

#include <QPainter>
#include <QMouseEvent>

RulerWidget::RulerWidget(Orientation orientation, QWidget* parent, EditorWindow* editorWindow)
    : QWidget(parent)
    , m_orientation(orientation)
    , m_cursorPos(0.0f)
    , m_editorWindow(editorWindow)
{
    // needed so we can cancel interaction on loss of focus
    setFocusPolicy(Qt::StrongFocus);
}

void RulerWidget::SetCursorPos(const QPoint& pos)
{
    // store the cursor value to use when painting the ruler
    QPoint localCursorPos = mapFromGlobal(pos);
    m_cursorPos = aznumeric_cast<float>((m_orientation == Orientation::Horizontal) ? localCursorPos.x() : localCursorPos.y());

    update();
}

void RulerWidget::DrawForViewport(Draw2dHelper& draw2d)
{
    // if there is an interaction in progress then draw the visual aids for the interaction
    if (m_dragInteraction)
    {
        m_dragInteraction->Render(draw2d);
    }
}

int RulerWidget::GetRulerBreadth()
{
    // This is how wide the rulers are. It is possible that at some point it could be configurable or
    // computed from some other UI or stylesheet setting.
    const int rulerBreadth = 16;

    return rulerBreadth;
}

void RulerWidget::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    // Note: If the ruler is hidden it will have a zero width or height. In this case Qt never even calls paintEvent

    // get the scale and translation of the viewport
    const ViewportInteraction::TranslationAndScale& translationAndScale = m_editorWindow->GetViewport()->GetViewportInteraction()->GetCanvasViewportMatrixProps();
    float scale = translationAndScale.scale;
    float translation = (m_orientation == Orientation::Horizontal) ? translationAndScale.translation.GetX() : translationAndScale.translation.GetY();

    // If the viewport is really small then scale can be zero (or very close) which would cause a divide by zero in later math so we just don't paint anything
    const float epsilon = 0.00001f;
    if (scale < epsilon)
    {
        return;
    }

    // Create a painter for doing the drawing
    QPainter painter(this);
    painter.setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing);

    QRectF rulerRect = rect();
 
    // We could fill the rect here if we wanted the ruler background to be a diferent color to the default
    // e.g.: painter.fillRect(rulerRect, QColor(30,35,40));

    // Draw the tick marks and number labels
    DrawTickMarksWithLabels(&painter, rulerRect, translation, scale);

    // Indicate the position of the mouse on the rulers
    DrawCursorPos(&painter, rulerRect);
}

void RulerWidget::mousePressEvent(QMouseEvent* ev)
{
    // start a drag interaction to create a guide
    AZ::Vector2 viewportMousePos = QtHelpers::MapGlobalPosToLocalVector2(m_editorWindow->GetViewport(), ev->globalPos());
    bool isVertical = m_orientation == Orientation::Vertical;
    m_dragInteraction = new ViewportAddGuideInteraction(m_editorWindow, m_editorWindow->GetCanvas(), isVertical, viewportMousePos);
}

void RulerWidget::mouseMoveEvent(QMouseEvent* ev)
{
    // If the mouse press event was on the ruler then we will get move events here even if the mouse is over the viewport.
    // We only get the events if the mouse is pressed down. So we only get here when adding a ruler.
    if (m_dragInteraction)
    {
        AZ::Vector2 viewportMousePos = QtHelpers::MapGlobalPosToLocalVector2(m_editorWindow->GetViewport(), ev->globalPos());
        m_dragInteraction->Update(viewportMousePos);
    }

    // SetCursorPos does not get called from the viewport while we are dragging from the ruler so update both rulers from here
    m_editorWindow->GetViewport()->SetRulerCursorPositions(ev->globalPos());
}

void RulerWidget::mouseReleaseEvent(QMouseEvent* ev)
{
    // This is a drag that started on the ruler, this is used to add guides.
    // If the mouse is released inside the viewport window then the guide is added, otherwise the add is canceled.
    if (m_dragInteraction)
    {
        // test to see if the mouse position is inside the viewport on each axis
        const QPoint& pos = ev->pos();
        const QSize& size = m_editorWindow->GetViewport()->size();

        ViewportDragInteraction::EndState inViewport;
        if (pos.x() >= 0 && pos.x() < size.width())
            inViewport = pos.y() >= 0 && pos.y() < size.height() ? ViewportDragInteraction::EndState::Inside : ViewportDragInteraction::EndState::OutsideY;
        else
            inViewport = pos.y() >= 0 && pos.y() < size.height() ? ViewportDragInteraction::EndState::OutsideX : ViewportDragInteraction::EndState::OutsideXY;

        m_dragInteraction->EndInteraction(inViewport);

        SAFE_DELETE(m_dragInteraction);
    }
}

void RulerWidget::focusOutEvent([[maybe_unused]] QFocusEvent* ev)
{
    // If we are in the middle of an interaction and this widget loses focus this is typically because right-mouse button
    // or ALT+char etc was pressed while the left mouse button was still down. In this case cancel the interaction so
    // that we don't keep displaying the guide position.
    SAFE_DELETE(m_dragInteraction);
}

void RulerWidget::DrawTickMarksWithLabels(QPainter* painter, QRectF rulerRect, float translation, float scale)
{
    // Internal structure used to define the different scales used depending on the zoom level
    struct RulerScale
    {
        float canvasPixelsPerSection;
        int preferredNumSubdivisions;
        int reducedNumSubdivisions;
    };

    // define the different ruler section sizes and the prefered and reduced number of subdivisions in the section
    const RulerScale validRulerScales[] = {
        { 1.0f,    1,  0 },
        { 2.0f,    2,  0 },
        { 5.0f,    5,  0 },
        { 10.0f,   10, 4 },
        { 20.0f,   10, 4 },
        { 50.0f,   10, 5 },
        { 100.0f,  10, 4 },
        { 200.0f,  10, 4 },
        { 500.0f,  10, 5 },
        { 1000.0f, 10, 5 }
    };

    // A "ruler section" is one part of the ruler, it contains one "major tick" with a text label
    // followed by a number of minor ticks that divide the section into subdivisions.
    // For now we assume the the units are always pixels.
    // The length of one RulerSection in canvas pixels depends on the scale.
    // When the scale is 1 there is one pixel on the screen for every pixel on the canvas.

    const float minRulerSectionLengthOnScreen = 40.0f;
    const float minDistanceBetweenTicks = 5.0f;

    float unroundedCanvasPixelsInSection = minRulerSectionLengthOnScreen / scale;
    
    const int numRulerScales = sizeof(validRulerScales) / sizeof(validRulerScales[0]);
    const RulerScale* selectedRulerScale = &validRulerScales[numRulerScales - 1];
    for (const RulerScale& rulerScale : validRulerScales)
    {
        if (unroundedCanvasPixelsInSection <= rulerScale.canvasPixelsPerSection)
        {
            selectedRulerScale = &rulerScale;
            break;
        }
    }

    // Having determined which ruler scale to use we know the number of canvas pixels in a ruler section
    float canvasPixelsPerSection = selectedRulerScale->canvasPixelsPerSection;

    // Compute the visible range of the ruler 
    float rulerRectMin = aznumeric_cast<float>((m_orientation == Orientation::Horizontal) ? rulerRect.left() : rulerRect.top());
    float rulerRectMax = aznumeric_cast<float>((m_orientation == Orientation::Horizontal) ? rulerRect.right() : rulerRect.bottom());

    float rulerStartInCanvasPixels = ((rulerRectMin - translation) / scale) + m_origin;
    float rulerEndInCanvasPixels = ((rulerRectMax - translation) / scale) + m_origin;

    // We will draw whole ruler sections, relying on the Qt clipping to clip off the non-visible parts.
    // So compute the ruler sections we should start and end with
    float rulerStartInSections = floorf(rulerStartInCanvasPixels / canvasPixelsPerSection);
    float rulerEndInSections = floorf(rulerEndInCanvasPixels / canvasPixelsPerSection);

    float firstRulerSectionStart = rulerStartInSections * canvasPixelsPerSection;
    float lastRulerSectionStart = rulerEndInSections * canvasPixelsPerSection;

    // draw the subdivision hatch marks
    float sectionLengthInScreenPixels = selectedRulerScale->canvasPixelsPerSection * scale;
    int numSubdivisions = selectedRulerScale->reducedNumSubdivisions;
    if (sectionLengthInScreenPixels / selectedRulerScale->preferredNumSubdivisions > minDistanceBetweenTicks)
    {
        numSubdivisions = selectedRulerScale->preferredNumSubdivisions;
    }

    // Set the pen to use for drawing all the tick marks
    QPen pen(QColor(204, 204, 204), 1);
    painter->setPen(pen);

    // set the font to use for drawing the ruler labels
    QFont font(QWidget::font());
    font.setPixelSize(10);
    painter->setFont(font);

    // for each visible section draw that ruler section
    for (float startInCanvasPixels = firstRulerSectionStart; startInCanvasPixels <= lastRulerSectionStart; startInCanvasPixels += canvasPixelsPerSection)
    {
        DrawRulerSection(painter, rulerRect, startInCanvasPixels, sectionLengthInScreenPixels, numSubdivisions, translation, scale);
    }
}

void RulerWidget::DrawRulerSection(QPainter* painter, QRectF rulerRect, float startInCanvasPixels, float sectionLengthInScreenPixels, int numSubdivisions, float translation, float scale)
{
    // compute the position in Qt local pixels for the start of this ruler section
    float posOnRuler = (startInCanvasPixels - m_origin) * scale + translation;

    posOnRuler -= 0.5f; // without this the ticks do not line up with the viewport exactly

    // Set the painter translation and scale so that we can do the drawing regardless of whether this is a horizontal or vertical ruler.
    // This sets to origin to the "bottom left" of the ruler section. I.e. where the major tick ends on the viewport side of the ruler.
    painter->save();
    float rulerBreadth;
    float directionAlongSection = 1.0f;
    if (m_orientation == Orientation::Horizontal)
    {
        rulerBreadth = aznumeric_cast<float>(rulerRect.height());
        painter->translate(posOnRuler, rulerBreadth);
    }
    else
    {
        rulerBreadth = aznumeric_cast<float>(rulerRect.width());
        painter->translate(rulerBreadth, posOnRuler);
        painter->rotate(-90);
        directionAlongSection = -1.0f; // for the vertical section the major tick is visually at the "end" of the section
    }

    // Constants that can be used to tune the tick marks on the ruler
    const float tickLengthRatioSmallTick = 0.33f;
    const float tickLengthRatioMediumTick = 0.66f;
    const float tickLengthRatioLargeTick = 1.0f;

    // Draw major tick
    painter->drawLine(QLineF(0, 0, 0, -rulerBreadth * tickLengthRatioLargeTick));

    // draw the subdivision hatch marks
    if (numSubdivisions > 0)
    {
        float tickSpacing = sectionLengthInScreenPixels / numSubdivisions;

        // The number of minor ticks is the number of subdivisions-1 since a subdivision is the space between ticks
        for (int i = 0 ; i < numSubdivisions-1; ++i)
        {
            // The only time we draw a "medium" tick is when there are 10 subdivisions and the medium tick is the fifth tick
            const float tickLengthRatio = (i == 4) ? tickLengthRatioMediumTick : tickLengthRatioSmallTick;

            float pos = (i+1) * tickSpacing * directionAlongSection;
            painter->drawLine(QLineF(pos, 0, pos, -rulerBreadth * tickLengthRatio));
        }
    }

    // Draw the label text to the right (horizontal) or left (vertical) of the top of the major tick.
    // Using QPainterPath is supposed to give better quality text especially when rotated. It looks worse, but it does
    // look consistent when rotated (consistently bad). So use just use drawText.

    QString label = QString::number(startInCanvasPixels);
    float textPosAlongSection;
    if (m_orientation == Orientation::Horizontal)
    {
        textPosAlongSection = 2;
    }
    else
    {
        QFontMetrics fontMetrics(painter->font());
        textPosAlongSection = aznumeric_cast<float>(-(2 + fontMetrics.horizontalAdvance(label)));
    }

    painter->drawText(aznumeric_cast<int>(textPosAlongSection), aznumeric_cast<int>(-(rulerBreadth-8)) , label);

    // restore the painter translation and rotation
    painter->restore();
}

void RulerWidget::DrawCursorPos(QPainter* painter, QRectF rulerRect)
{
    // Use a dotted magenta line for the cursor indicator
    QPen pen;
    pen.setStyle(Qt::DotLine);
    pen.setWidth(1);
    pen.setBrush(Qt::magenta);
    painter->setPen(pen);

    float x1, x2, y1, y2;

    if (m_orientation == Orientation::Horizontal)
    {
        x1 = x2 = m_cursorPos;
        y1 = aznumeric_cast<float>(rulerRect.top());
        y2 = aznumeric_cast<float>(rulerRect.bottom());
    }
    else
    {
        y1 = y2 = m_cursorPos;
        x1 = aznumeric_cast<float>(rulerRect.left());
        x2 = aznumeric_cast<float>(rulerRect.right());
    }

    painter->drawLine(QLineF(x1,y1,x2,y2));
}

#include <moc_RulerWidget.cpp>
