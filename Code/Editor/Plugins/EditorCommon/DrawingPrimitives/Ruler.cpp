/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "Ruler.h"

#include <QPainter>
#include <QPalette>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/algorithm.h>

namespace DrawingPrimitives
{
    enum
    {
        RULER_MIN_PIXELS_PER_TICK = 3,
    };

    std::vector<STick> CalculateTicks(uint size, Range visibleRange, Range rulerRange, int* pRulerPrecision, Range* pScreenRulerRange)
    {
        std::vector<STick> ticks;

        if (size == 0)
        {
            if (pRulerPrecision)
            {
                *pRulerPrecision = 0;
            }

            return ticks;
        }

        const float pixelsPerUnit = visibleRange.Length() > 0.0f ? (float)size / visibleRange.Length() : 1.0f;

        const float startTime = rulerRange.start;
        const float endTime = rulerRange.end;
        const float totalDuration = endTime - startTime;

        const float ticksMinPower = log10f(RULER_MIN_PIXELS_PER_TICK);
        const float ticksPowerDelta = ticksMinPower - log10f(pixelsPerUnit);

        const int digitsAfterPoint = AZStd::max(-int(ceil(ticksPowerDelta)) - 1, 0);
        if (pRulerPrecision)
        {
            *pRulerPrecision = digitsAfterPoint;
        }

        const float scaleStep = powf(10.0f, ceil(ticksPowerDelta));
        const float scaleStepPixels = scaleStep * pixelsPerUnit;
        const int numMarkers = int(totalDuration / scaleStep) + 1;

        const float startTimeRound = int(startTime / scaleStep) * scaleStep;
        const int startOffsetMod = int(startTime / scaleStep) % 10;
        const int scaleOffsetPixels = aznumeric_cast<int>((startTime - startTimeRound) * pixelsPerUnit);

        const int startX = aznumeric_cast<int>((rulerRange.start - visibleRange.start) * pixelsPerUnit);
        const int endX = aznumeric_cast<int>(startX + (numMarkers - 1) * scaleStepPixels - scaleOffsetPixels);

        if (pScreenRulerRange)
        {
            *pScreenRulerRange = Range(aznumeric_cast<float>(startX), aznumeric_cast<float>(endX));
        }

        const int startLoop = std::max((int)((scaleOffsetPixels - startX) / scaleStepPixels) - 1, 0);
        const int endLoop = std::min((int)((size + scaleOffsetPixels - startX) / scaleStepPixels) + 1, numMarkers);

        for (int i = startLoop; i < endLoop; ++i)
        {
            STick tick;

            const int x = aznumeric_cast<int>(startX + i * scaleStepPixels - scaleOffsetPixels);
            const float value = startTimeRound + i * scaleStep;

            tick.m_bTenth = (startOffsetMod + i) % 10 != 0;
            tick.m_position = x;
            tick.m_value = value;

            ticks.push_back(tick);
        }

        return ticks;
    }

    QColor Interpolate(const QColor& a, const QColor& b, float k)
    {
        float mk = 1.0f - k;
        return QColor(aznumeric_cast<int>(a.red() * mk  + b.red() * k),
            aznumeric_cast<int>(a.green() * mk + b.green() * k),
            aznumeric_cast<int>(a.blue() * mk + b.blue() * k),
            aznumeric_cast<int>(a.alpha() * mk + b.alpha() * k));
    }

    void DrawTicks(const std::vector<STick>& ticks, QPainter& painter, const QPalette& palette, const STickOptions& options)
    {
        QColor midDark = DrawingPrimitives::Interpolate(palette.color(QPalette::Dark), palette.color(QPalette::Button), 0.5f);
        painter.setPen(QPen(midDark));

        const int height = options.m_rect.height();
        const int top = options.m_rect.top();

        for (const STick& tick : ticks)
        {
            const int x = tick.m_position + options.m_rect.left();

            if (tick.m_bTenth)
            {
                painter.drawLine(QPoint(x, top + height - options.m_markHeight / 2), QPoint(x, top + height));
            }
            else
            {
                painter.drawLine(QPoint(x, top + height - options.m_markHeight), QPoint(x, top + height));
            }
        }
    }

    void DrawTicks(QPainter& painter, const QPalette& palette, const SRulerOptions& options)
    {
        const std::vector<STick> ticks = CalculateTicks(options.m_rect.width(), options.m_visibleRange, options.m_rulerRange, nullptr, nullptr);
        DrawTicks(ticks, painter, palette, options);
    }

    void DrawRuler(QPainter& painter, const QPalette& palette, const SRulerOptions& options, int* pRulerPrecision)
    {
        int rulerPrecision;
        Range screenRulerRange;
        const std::vector<STick> ticks = CalculateTicks(options.m_rect.width(), options.m_visibleRange, options.m_rulerRange, &rulerPrecision, &screenRulerRange);

        if (pRulerPrecision)
        {
            *pRulerPrecision = rulerPrecision;
        }

        if (options.m_shadowSize > 0)
        {
            QRect shadowRect = QRect(options.m_rect.left(), options.m_rect.height(), options.m_rect.width(), options.m_shadowSize);
            QLinearGradient upperGradient(shadowRect.left(), shadowRect.top(), shadowRect.left(), shadowRect.bottom());
            upperGradient.setColorAt(0.0f, QColor(0, 0, 0, 128));
            upperGradient.setColorAt(1.0f, QColor(0, 0, 0, 0));
            QBrush upperBrush(upperGradient);
            painter.fillRect(shadowRect, upperBrush);
        }

        painter.fillRect(options.m_rect, DrawingPrimitives::Interpolate(palette.color(QPalette::Button), palette.color(QPalette::Midlight), 0.25f));
        if (options.m_drawBackgroundCallback)
        {
            options.m_drawBackgroundCallback();
        }

        QColor midDark = DrawingPrimitives::Interpolate(palette.color(QPalette::Dark), palette.color(QPalette::Button), 0.5f);
        painter.setPen(QPen(midDark));

        QFont font;
        font.setPixelSize(10);
        painter.setFont(font);


        char format[16] = "";
        azsprintf(format, "%%.%df", rulerPrecision);

        const int height = options.m_rect.height();
        const int top = options.m_rect.top();

        QString str;
        for (const STick& tick : ticks)
        {
            const int x = tick.m_position + options.m_rect.left();
            const float value = tick.m_value;

            if (tick.m_bTenth)
            {
                painter.drawLine(QPoint(x, top + height - options.m_markHeight / 2), QPoint(x, top + height));
            }
            else
            {
                painter.drawLine(QPoint(x, top + height - options.m_markHeight), QPoint(x, top + height));
                painter.setPen(palette.color(QPalette::Disabled, QPalette::Text));
                str.asprintf(format, value);
                painter.drawText(QPoint(x + 2, top + height - options.m_markHeight + 1), str);
                painter.setPen(midDark);
            }
        }

        painter.setPen(QPen(palette.color(QPalette::Dark)));
        painter.drawLine(QPoint(aznumeric_cast<int>(options.m_rect.left() + screenRulerRange.start), 0), QPoint(aznumeric_cast<int>(options.m_rect.left() + screenRulerRange.start), options.m_rect.top() + height));
        painter.drawLine(QPoint(aznumeric_cast<int>(options.m_rect.left() + screenRulerRange.end), 0), QPoint(aznumeric_cast<int>(options.m_rect.left() + screenRulerRange.end), options.m_rect.top() + height));
    }
}
