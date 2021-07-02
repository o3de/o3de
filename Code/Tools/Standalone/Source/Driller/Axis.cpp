/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <AzCore/Debug/Profiler.h>
#include <QPainter>

#include "Axis.hxx"

namespace Charts
{
    //////////////////////////////////////////////////////////////////////////
    Axis::Axis(QObject* pParent)
        : QObject(pParent)
        , m_lockRange(true)
        , m_lockZoom(true)
        , m_lockRight(false)
        , m_autoWindow(true)
        , m_rangeMin(0)
        , m_rangeMax(0)
        , m_windowMax(0)
        , m_windowMin(0)
        , m_rangeMaxInitialized(false)
        , m_rangeMinInitialized(false)
    {}
    Axis::~Axis()
    {
    }

    bool  Axis::GetValid() const
    {
        return ((m_rangeMinInitialized) && (m_rangeMaxInitialized) && (m_rangeMin < m_rangeMax) && (m_windowMin < m_windowMax));
    }

    void Axis::Clear()
    {
        bool wasValid = GetValid();

        m_rangeMinInitialized = false;
        m_rangeMaxInitialized = false;
        m_rangeMax = m_rangeMin  = m_windowMax = m_windowMin = 0.0f;

        if (wasValid)
        {
            emit Invalidated();
        }
    }

    void Axis::SetAxisRange(float minimum, float maximum)
    {
        float oldRangeMin = m_rangeMin;
        float oldRangeMax = m_rangeMax;
        float oldWindowMin = m_windowMin;
        float oldWindowMax = m_windowMax;

        bool wasValid = GetValid();

        if (m_lockRight)
        {
            m_windowMin += maximum - m_rangeMax;
            m_windowMax = m_rangeMax;
        }
        m_rangeMin = minimum;
        m_rangeMax = maximum;

        if (m_autoWindow && !m_lockRight)
        {
            m_windowMin = m_rangeMin;
            m_windowMax = m_rangeMax;
        }

        m_rangeMinInitialized = true;
        m_rangeMaxInitialized = true;

        if ((oldRangeMax != m_rangeMax) ||
            (oldRangeMin != m_rangeMin) ||
            (oldWindowMin != m_windowMin) ||
            (oldWindowMax != m_windowMax) ||
            (wasValid != GetValid()))
        {
            emit Invalidated();
        }
    }

    void Axis::AddAxisRange(float value)
    {
        bool updateValues = false;
        float minRange = m_rangeMin;
        float maxRange = m_rangeMax;

        if ((value < m_rangeMin) || !m_rangeMinInitialized)
        {
            updateValues = true;
            minRange = value;
        }

        if ((m_rangeMax < value) || !m_rangeMaxInitialized)
        {
            updateValues = true;
            maxRange = value;
        }

        if (updateValues)
        {
            SetAxisRange(minRange, maxRange);
        }
    }

    void Axis::SetRangeMax(float rangemax)
    {
        SetAxisRange(m_rangeMin, rangemax);
    }

    void Axis::SetRangeMin(float rangemin)
    {
        SetAxisRange(rangemin, m_rangeMax);
    }

    void Axis::UpdateWindowRange(float delta)
    {
        if (delta != 0.0f)
        {
            m_windowMax += delta;
            m_windowMin += delta;
            if (m_windowMax > m_rangeMax)
            {
                delta = m_windowMax - m_rangeMax;
                m_windowMax -= delta;
                m_windowMin -= delta;
            }

            if (m_windowMin < m_rangeMin)
            {
                delta = m_rangeMin - m_windowMin;
                m_windowMax += delta;
                m_windowMin += delta;
            }

            emit Invalidated();
        }
    }

    void Axis::SetViewFull()
    {
        float oldWindowMin = m_windowMin;
        float oldWindowMax = m_windowMax;

        m_autoWindow = true;
        m_windowMin = m_rangeMin;
        m_windowMax = m_rangeMax;

        if (
            (oldWindowMin != m_windowMin) ||
            (oldWindowMax != m_windowMax)
            )
        {
            emit Invalidated();
        }
    }

    void Axis::SetLabel(QString newLabel)
    {
        if (m_label != newLabel)
        {
            m_label = newLabel;
            emit Invalidated();
        }
    }

    bool Axis::GetLockedRight() const
    {
        return m_lockRight;
    }

    bool Axis::GetLockedRange() const
    {
        return m_lockRange;
    }

    bool Axis::GetLockedZoom() const
    {
        return m_lockZoom;
    }

    void Axis::SetLockedRange(bool newValue)
    {
        m_lockRange = newValue;
    }

    void Axis::SetLockedZoom(bool newValue)
    {
        m_lockZoom = newValue;
    }

    void Axis::SetLockedRight(bool newValue)
    {
        m_lockRight = newValue;
    }

    void Axis::SetAutoWindow(bool autoWindow)
    {
        m_autoWindow = autoWindow;
    }

    bool Axis::GetAutoWindow() const
    {
        return m_autoWindow;
    }

    QString Axis::GetLabel() const
    {
        return m_label;
    }

    float Axis::GetWindowMin() const
    {
        return m_windowMin;
    }

    float Axis::GetWindowMax() const
    {
        return m_windowMax;
    }

    float Axis::GetRangeMin()  const
    {
        return m_rangeMin;
    }

    float Axis::GetRangeMax() const
    {
        return m_rangeMax;
    }

    void Axis::SetWindowMin(float newValue)
    {
        if (m_windowMin != newValue)
        {
            m_windowMin = newValue;
            emit Invalidated();
        }
    }

    void Axis::SetWindowMax(float newValue)
    {
        if (m_windowMax != newValue)
        {
            m_windowMax = newValue;
            emit Invalidated();
        }
    }

    float Axis::GetWindowRange() const
    {
        if (!GetValid())
        {
            return 1.0f;
        }
        return m_windowMax - m_windowMin;
    }

    float Axis::GetRange() const
    {
        if (!GetValid())
        {
            return 1.0f;
        }
        return m_rangeMax - m_rangeMin;
    }

    // focuspoint is a number from 0 to 1 that indicates how far along that axis the focal point is
    // for example on a horizontal axis, 0 is the start, or GetwindowMin(), and 1 is the end, GetWindowMaX()

    void Axis::Drag(float delta)
    {
        if (!GetValid())
        {
            return;
        }

        if (GetLockedRange())
        {
            return;
        }

        UpdateWindowRange(delta);
    }

    void Axis::ZoomToRange(float windowMin, float windowMax, bool clamp)
    {
        if (!GetValid())
        {
            return;
        }

        float oldmin = m_windowMin;
        float oldmax = m_windowMax;
        m_windowMin = windowMin;
        m_windowMax = windowMax;

        if (clamp)
        {
            if (m_windowMin < m_rangeMin)
            {
                m_windowMin = m_rangeMin;
            }

            if (m_windowMax > m_rangeMax)
            {
                m_windowMax = m_rangeMax;
            }
        }

        if ((m_windowMin != oldmin) || (m_windowMax != oldmax))
        {
            emit Invalidated();
        }
    }

    void Axis::Zoom(float ratio, float steps, float zoomLimit)
    {
        if (!GetValid())
        {
            return;
        }

        if (GetLockedRight())
        {
            ratio = 1.0f;
        }
        if (!GetLockedZoom())
        {
            SetAutoWindow(false);

            float testMin = GetWindowMin();
            float testMax = GetWindowMax();

            testMin -= float(GetWindowRange()) * 0.05f * ratio * -steps;
            testMax += float(GetWindowRange()) * 0.05f * (1.0f - ratio) * -steps;

            if ((testMax - testMin) > 0.0f)
            {
                if (testMax > GetRangeMax())
                {
                    float offset = GetRangeMax() - testMax;
                    testMax += offset;
                    testMin += offset;
                }
                if (testMin < GetRangeMin())
                {
                    float offset = testMin - GetRangeMin();
                    testMax -= offset;
                    testMin -= offset;
                }
                if (testMax - testMin >= zoomLimit)
                {
                    SetWindowMin(testMin);
                    SetWindowMax(testMax);
                }
                if ((testMax - testMin) > (GetRangeMax() - GetRangeMin()))
                {
                    SetViewFull();
                }
            }
        }
    }

    // given a width of a view in pixels, subdivide it and return a vector of domain units that
    // satisfy a human outlook on domain values:
    float Axis::ComputeAxisDivisions(float pixelWidth, AZStd::vector<float>& domainPointsOut, float minPixels, float maxPixels, bool allowFractions)
    {
        if (!GetValid())
        {
            return 1.0f;
        }

        float divisor = 1.0f;
        float windowRange = GetWindowRange();

        // compute the divisor
        float currentDivisionWidthInPixels = pixelWidth / (windowRange / divisor);
        while (currentDivisionWidthInPixels > maxPixels)
        {
            // drop it down by 0.5 then by 0.1
            divisor /= 2.0f;
            currentDivisionWidthInPixels = pixelWidth / (windowRange / divisor);
            if (currentDivisionWidthInPixels <= maxPixels)
            {
                break;
            }

            divisor *= 0.2f; // 0.5f * 0.2f = 0.1
            currentDivisionWidthInPixels = pixelWidth / (windowRange / divisor);
        }
        // the min pixels is an absolute requirement to prevent text from overlapping
        // which is why we apply that constraint LAST.
        // so for example, if window shows 100 units, and divisor is 2.0, then that would yield 50 pieces.
        // 50 pieces over the span of 1000 pixels is 1000 / 50, 20 pixels each;
        // 100 pieces would be (num pieces / pixel space to draw in) pixels
        while (currentDivisionWidthInPixels < minPixels)
        {
            // the division width is too small, there are too many pieces.  Make fewer pieces by choosing a larger
            // divisor
            // drop it down by 0.5 then by 0.1
            divisor *= 5.0f;
            currentDivisionWidthInPixels = pixelWidth / (windowRange / divisor);
            if (currentDivisionWidthInPixels >= minPixels)
            {
                break;
            }

            divisor *= 2.0f;
            currentDivisionWidthInPixels = pixelWidth / (windowRange / divisor);
        }

        if ((divisor < 1.0f) && (!allowFractions))
        {
            divisor = 1.0f;
        }


        // now lay out the domain starting with the first unit of that number:
        float startingUnit = floorf(m_windowMin / divisor) * divisor;

        // to retain precision we are going to try to do the math around the origin.
        startingUnit -= m_windowMin;
        AZStd::size_t maximumAllowedUnitsBeforeSomethingTerribleHasOccurred = (AZStd::size_t)(pixelWidth / 4.0f);

        // so for example if we've decided that the divisor is 10 units, and the window min is 12.5, then it will go to 1.25, chop off the .25, and go back to 10
        while (startingUnit <= windowRange)
        {
            if (startingUnit >= 0.0f)
            {
                domainPointsOut.push_back(startingUnit + m_windowMin);
            }
            startingUnit += divisor;

            if (domainPointsOut.size() > maximumAllowedUnitsBeforeSomethingTerribleHasOccurred)
            {
                break; // you can put a break point here, but it usually means that we've lost enough precision.  Change your axis range or numbering scheme!
            }
        }

        return divisor;
    }
    void Axis::PaintAxis(AxisType axisType, QPainter* painter, const QRect& widgetBounds, const QRect& graphBounds, QAbstractAxisFormatter* formatter)
    {
        switch (axisType)
        {
        case AxisType::Horizontal:
            PaintAsHorizontalAxis(painter, widgetBounds, graphBounds, formatter);
            break;
        case AxisType::Vertical:
            PaintAsVerticalAxis(painter, widgetBounds, graphBounds, formatter);
            break;
        default:
            break;
        }
    }

    void Axis::PaintAsVerticalAxis(QPainter* painter, const QRect& widgetBounds, const QRect& graphBounds, QAbstractAxisFormatter* formatter)
    {
        (void)widgetBounds;

        const QColor axisBrush = QColor(255, 255, 0, 255);
        const QColor axisPen = QColor(0, 255, 255, 255);
        const QColor dottedColor(64, 64, 64, 255);
        const QColor solidColor(0, 255, 255, 255);

        QBrush brush;
        QPen pen;

        brush.setColor(axisBrush);
        pen.setColor(axisPen);
        painter->setPen(pen);

        QFont currentFont = painter->font();
        currentFont.setPointSize(currentFont.pointSize() - 1);
        painter->setFont(currentFont);

        int w = painter->fontMetrics().horizontalAdvance(GetLabel());
        int h = painter->fontMetrics().height();

        int centerHeight = graphBounds.top() + (graphBounds.height() / 2);

        DrawRotatedText(GetLabel(), painter, 270, h, centerHeight + w / 2, 1.25f);

        currentFont.setPointSize(currentFont.pointSize() + 1);
        painter->setFont(currentFont);

        QPoint startPoint = graphBounds.topLeft();
        QPoint endPoint = graphBounds.bottomLeft();        

        int height = endPoint.y() - startPoint.y();
        int fontH = painter->fontMetrics().height();

        if (height == 0)
        {
            height = 1;
        }

        AZStd::vector<float> divisions;
        divisions.reserve(10);

        float divisionSize = ComputeAxisDivisions(static_cast<float>(height), divisions, fontH * 2.0f, fontH * 2.0f);        

        QPen dottedPen;
        dottedPen.setStyle(Qt::DotLine);
        dottedPen.setColor(dottedColor);

        QBrush solidBrush;
        QPen solidPen;
        solidPen.setStyle(Qt::SolidLine);
        solidPen.setColor(solidColor);
        solidPen.setWidth(1);

        float fullRange = fabs(GetWindowRange());

        for (float division : divisions)
        {
            float ratio = static_cast<float>(division - GetWindowMin()) / fullRange;

            QPoint lineStart;
            lineStart.setX(endPoint.x());
            lineStart.setY(endPoint.y() - static_cast<int>(height*ratio));

            QPoint lineEnd(static_cast<int>(lineStart.x() + graphBounds.width()), lineStart.y());

            painter->setPen(dottedPen);
            painter->drawLine(lineStart, lineEnd);

            QString text;            

            if (formatter)
            {
                text = formatter->convertAxisValueToText(Charts::AxisType::Vertical, division, divisions.front(), divisions.back(), divisionSize);
            }
            else
            {
                text = QString("%1").arg((AZ::s64)division);
            }

            int textW = painter->fontMetrics().horizontalAdvance(text);
            painter->setPen(solidPen);
            painter->drawText(lineStart.x() - textW - 2, (int)lineStart.y() + fontH / 2, text);
        }
    }
    
    void Axis::PaintAsHorizontalAxis(QPainter* painter, const QRect& widgetBounds, const QRect& graphBounds, QAbstractAxisFormatter* formatter)
    {
        const QColor axisBrush = QColor(255, 255, 0, 255);
        const QColor axisPen = QColor(0, 255, 255, 255);
        const QColor dottedColor(64, 64, 64,255);
        const QColor solidColor(0, 255, 255, 255);

        QBrush brush;
        QPen pen;

        brush.setColor(axisBrush);
        pen.setColor(axisPen);
        painter->setPen(pen);

        QFont currentFont = painter->font();
        currentFont.setPointSize(currentFont.pointSize() - 1);
        painter->setFont(currentFont);

        painter->drawText(0, 0, widgetBounds.width(), widgetBounds.height(), Qt::AlignHCenter | Qt::AlignBottom, GetLabel());

        currentFont.setPointSize(currentFont.pointSize() + 1);
        painter->setFont(currentFont);

        QPoint startPoint = graphBounds.bottomLeft();
        QPoint endPoint = graphBounds.bottomRight();
        int width = endPoint.x() - startPoint.x();

        if (width == 0)
        {
            width = 1;
        }

        float textSpaceRequired = (float)painter->fontMetrics().horizontalAdvance("9,999,999.99");

        int fontH = painter->fontMetrics().height();

        AZStd::vector<float> divisions;        
        divisions.reserve(10);

        float divisionSize = ComputeAxisDivisions(static_cast<float>(width), divisions, textSpaceRequired, textSpaceRequired);        

        QPen dottedPen;
        dottedPen.setStyle(Qt::DotLine);
        dottedPen.setColor(dottedColor);

        QBrush solidBrush;
        QPen solidPen;
        solidPen.setStyle(Qt::SolidLine);
        solidPen.setColor(solidColor);
        solidPen.setWidth(1);

        float fullRange = fabs(GetWindowRange());

        for (float division : divisions)
        {
            float ratio = float(division - GetWindowMin()) / fullRange;

            QPoint lineStart;
            lineStart.setX(startPoint.x() + static_cast<int>(width*ratio));
            lineStart.setY(startPoint.y());

            QPoint lineEnd(lineStart.x(), static_cast<int>(startPoint.y() - graphBounds.height()));

            painter->setPen(dottedPen);
            painter->drawLine(lineStart, lineEnd);

            QString text;

            if (formatter)
            {
                text = formatter->convertAxisValueToText(Charts::AxisType::Horizontal, division, divisions.front(), divisions.back(), divisionSize);
            }
            else
            {
                text = QString("%1").arg((AZ::s64)division);
            }

            int textW = painter->fontMetrics().horizontalAdvance(text);

            painter->setPen(solidPen);
            painter->drawText(lineStart.x() - textW / 2, startPoint.y() + fontH, text);
        }
    }

    void Axis::DrawRotatedText(QString text, QPainter *painter, float degrees, int x, int y, float scale)
    {
        painter->save();
        painter->translate(x, y);
        painter->scale(scale, scale);
        painter->rotate(degrees);
        painter->drawText(0, 0, text);
        painter->restore();
    }
}

#include <Source/Driller/moc_Axis.cpp>
