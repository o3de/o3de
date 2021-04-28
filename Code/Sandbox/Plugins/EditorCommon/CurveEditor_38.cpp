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
#include "EditorCommon_precompiled.h"
#include "CurveEditor.h"
#include "DrawingPrimitives/TimeSlider.h"
#include "DrawingPrimitives/Ruler.h"

#include <QPainter>
#include <QMouseEvent>
#include <QColor>
#include <QToolBar>
#include <QIcon>

#pragma warning (push)
#pragma warning (disable : 4554)

#define INDEX_NOT_OUT_OF_RANGE PREFAST_SUPPRESS_WARNING(6201)
#define NO_BUFFER_OVERRUN PREFAST_SUPPRESS_WARNING(6385 6386)
#include "Cry_LegacyPhysUtils.h"

#pragma warning (pop)

namespace CurveEditorHelpers
{
    const uint numColors = 4;
    ColorB colors[numColors] =
    {
        ColorB(243, 126, 121),
        ColorB(121, 152, 243),
        ColorB(187, 243, 121),
        ColorB(243, 121, 223),
    };

    ColorB GetCurveColor(const uint n)
    {
        return colors[n % numColors];
    }

    QColor Interpolate(const QColor& a, const QColor& b, float k)
    {
        float mk = 1.0f - k;
        return QColor(a.red() * mk  + b.red() * k,
            a.green() * mk + b.green() * k,
            a.blue() * mk + b.blue() * k,
            a.alpha() * mk + b.alpha() * k);
    }
}

namespace
{
    using namespace LegacyCryPhysicsUtils;

    const int kRulerHeight = 16;
    const int kRulerShadowHeight = 6;
    const int kRulerMarkHeight = 8;
    const float kHitDistance = 5.0f;
    const float kMinZoom = 0.00001f;
    const float kMaxZoom = 1000.0f;
    const float kFitMargin = 30.0f;

    const QPointF kPointRectExtent = QPointF(2.5f, 2.5f);

    Vec2 TransformPointToScreen(const Vec2 zoom, const Vec2 translation, QRect curveArea, Vec2 point)
    {
        Vec2 transformedPoint = Vec2(point.x * zoom.x, point.y * -zoom.y) + translation;
        transformedPoint.x *= curveArea.width();
        transformedPoint.y *= curveArea.height();
        return Vec2(transformedPoint.x + curveArea.left(), transformedPoint.y + curveArea.top());
    }

    Vec2 TransformPointFromScreen(const Vec2 zoom, const Vec2 translation, QRect curveArea, Vec2 point)
    {
        Vec2 transformedPoint = Vec2((point.x - curveArea.left()) / curveArea.width(), (point.y - curveArea.top()) / curveArea.height()) - translation;
        transformedPoint.x /= zoom.x;
        transformedPoint.y /= -zoom.y;
        return Vec2(transformedPoint.x, transformedPoint.y);
    }

    QPointF Vec2ToPoint(Vec2 point)
    {
        return QPointF(point.x, point.y);
    }

    Vec2 PointToVec2(QPointF point)
    {
        return Vec2(point.x(), point.y());
    }

    // This function returns a new key with position and weights affected by eTangentType_Smooth, eTangentType_Linear and eTangentType_Step for the incoming tangent
    SCurveEditorKey ApplyInTangentFlags(const SCurveEditorKey& key, const SCurveEditorKey& leftKey, const SCurveEditorKey* pRightKey)
    {
        SCurveEditorKey newKey = key;

        if (leftKey.m_controlPoint.m_outTangentType == SBezierControlPoint::eTangentType_Step)
        {
            newKey.m_controlPoint.m_inTangent = Vec2(0.0f, 0.0f);
            return newKey;
        }
        else if (key.m_controlPoint.m_inTangentType != SBezierControlPoint::eTangentType_Step)
        {
            const SAnimTime leftTime = leftKey.m_time;
            const SAnimTime rightTime = pRightKey ? pRightKey->m_time : key.m_time;

            // Rebase to [0, rightTime - leftTime] to increase float precision
            const float floatTime = (key.m_time - leftTime).ToFloat();
            const float floatLeftTime = 0.0f;
            const float floatRightTime = (rightTime - leftTime).ToFloat();

            newKey.m_controlPoint = Bezier::CalculateInTangent(floatTime, key.m_controlPoint,
                    floatLeftTime, &leftKey.m_controlPoint,
                    floatRightTime, pRightKey ? &pRightKey->m_controlPoint : nullptr);
        }
        else
        {
            newKey.m_controlPoint.m_inTangent = Vec2(0.0f, 0.0f);
            newKey.m_controlPoint.m_value = leftKey.m_controlPoint.m_value;
        }

        return newKey;
    }

    // This function returns a new key with position and weights affected by eTangentType_Smooth, eTangentType_Linear and eTangentType_Step for the outgoing tangent
    SCurveEditorKey ApplyOutTangentFlags(const SCurveEditorKey& key, const SCurveEditorKey* pLeftKey, const SCurveEditorKey& rightKey)
    {
        SCurveEditorKey newKey = key;

        if (rightKey.m_controlPoint.m_inTangentType == SBezierControlPoint::eTangentType_Step
            && key.m_controlPoint.m_outTangentType != SBezierControlPoint::eTangentType_Step)
        {
            newKey.m_controlPoint.m_outTangent = Vec2(0.0f, 0.0f);
        }
        else if (key.m_controlPoint.m_outTangentType != SBezierControlPoint::eTangentType_Step)
        {
            const SAnimTime leftTime = pLeftKey ? pLeftKey->m_time : key.m_time;
            const SAnimTime rightTime = rightKey.m_time;

            // Rebase to [0, rightTime - leftTime] to increase float precision
            const float floatTime = (key.m_time - leftTime).ToFloat();
            const float floatLeftTime = 0.0f;
            const float floatRightTime = (rightTime - leftTime).ToFloat();

            newKey.m_controlPoint = Bezier::CalculateOutTangent(floatTime, key.m_controlPoint,
                    floatLeftTime, pLeftKey ? &pLeftKey->m_controlPoint : nullptr,
                    floatRightTime, &rightKey.m_controlPoint);
        }
        else
        {
            newKey.m_controlPoint.m_outTangent = Vec2(0.0f, 0.0f);
            newKey.m_controlPoint.m_value = rightKey.m_controlPoint.m_value;
        }

        return newKey;
    }

    QPainterPath CreatePathFromCurve(const SCurveEditorCurve& curve, ECurveEditorCurveType curveType, AZStd::function<Vec2(Vec2)> transformFunc)
    {
        QPainterPath path;

        const Vec2 startPoint(curve.m_keys[0].m_time.ToFloat(), curve.m_keys[0].m_controlPoint.m_value);
        const Vec2 startTransformed = transformFunc(startPoint);
        path.moveTo(startTransformed.x, startTransformed.y);

        const auto endIter = curve.m_keys.end() - 1;
        for (auto iter = curve.m_keys.begin(); iter != endIter; ++iter)
        {
            const SCurveEditorKey* pKeyLeftOfSegment = (iter != curve.m_keys.begin()) ? &*(iter - 1) : nullptr;
            const SCurveEditorKey* pKeyRightOfSegment = (iter != (curve.m_keys.end() - 2)) ? &*(iter + 2) : nullptr;

            const SCurveEditorKey segmentStartKey = ApplyOutTangentFlags(*iter, pKeyLeftOfSegment, *(iter + 1));
            const SCurveEditorKey segmentEndKey = ApplyInTangentFlags(*(iter + 1), *iter, pKeyRightOfSegment);

            const Vec2 p0 = Vec2(segmentStartKey.m_time.ToFloat(), segmentStartKey.m_controlPoint.m_value);
            const Vec2 p3 = Vec2(segmentEndKey.m_time.ToFloat(), segmentEndKey.m_controlPoint.m_value);

            Vec2 p1, p2;
            if (curveType == eCECT_Bezier)
            {
                // Need to compute tangents for x so that the cubic 2D Bezier does a linear interpolation in
                // that dimension, because we actually want to draw a cubic 1D Bezier curve
                const float outTangentX = (2.0f * p0.x + p3.x) / 3.0f;  // p1 = (2 * p0 + p3) / 3
                const float inTangentX = (p0.x + 2.0f * p3.x) / 3.0f;       // p2 = (p0 + 2 * p3) / 3

                p1 = Vec2(outTangentX, p0.y + segmentStartKey.m_controlPoint.m_outTangent.y);
                p2 = Vec2(inTangentX, p3.y + segmentEndKey.m_controlPoint.m_inTangent.y);
            }
            else if (curveType == eCECT_2DBezier)
            {
                p1 = p0 + segmentStartKey.m_controlPoint.m_outTangent;
                p2 = p3 + segmentEndKey.m_controlPoint.m_inTangent;
            }

            const QPointF p0Transformed = Vec2ToPoint(transformFunc(p0));
            const QPointF p1Transformed = Vec2ToPoint(transformFunc(p1));
            const QPointF p2Transformed = Vec2ToPoint(transformFunc(p2));
            const QPointF p3Transformed = Vec2ToPoint(transformFunc(p3));
            path.moveTo(p0Transformed);
            path.cubicTo(p1Transformed, p2Transformed, p3Transformed);
        }

        return path;
    }

    QPainterPath CreateExtrapolatedPathFromCurve(const SCurveEditorCurve& curve, AZStd::function<Vec2(Vec2)> transformFunc, float windowWidth)
    {
        QPainterPath path;

        if (curve.m_keys.size() > 0)
        {
            const Vec2 startPoint = Vec2(curve.m_keys[0].m_time.ToFloat(), curve.m_keys[0].m_controlPoint.m_value);
            const Vec2 startTransformed = transformFunc(startPoint);
            if (startTransformed.x > 0.0f)
            {
                path.moveTo(std::min(startTransformed.x, windowWidth), startTransformed.y);
                path.lineTo(0.0f, startTransformed.y);
            }

            const Vec2 endPoint(curve.m_keys.back().m_time.ToFloat(), curve.m_keys.back().m_controlPoint.m_value);
            const Vec2 endTransformed = transformFunc(endPoint);
            if (endTransformed.x < windowWidth)
            {
                path.moveTo(std::max(endTransformed.x, 0.0f), endTransformed.y);
                path.lineTo(windowWidth, endTransformed.y);
            }
        }
        else
        {
            const Vec2 pointOnCurve = Vec2(0.0f, curve.m_defaultValue);
            const Vec2 pointOnTransformed = transformFunc(pointOnCurve);
            path.moveTo(0.0, pointOnTransformed.y);
            path.lineTo(windowWidth, pointOnTransformed.y);
        }

        QVector<qreal> dashPattern;
        dashPattern << 16 << 8;

        QPainterPathStroker stroker;
        stroker.setCapStyle(Qt::RoundCap);
        stroker.setDashPattern(dashPattern);
        stroker.setWidth(0.5);

        return stroker.createStroke(path);
    }

    QPainterPath CreateDiscontinuityPathFromCurve(const SCurveEditorCurve& curve, ECurveEditorCurveType curveType, AZStd::function<Vec2(Vec2)> transformFunc)
    {
        QPainterPath path;

        if (curve.m_keys.size() > 0)
        {
            const auto endIter = curve.m_keys.end() - 1;

            for (auto iter = curve.m_keys.begin(); iter != endIter; ++iter)
            {
                const SCurveEditorKey* pKeyLeftOfSegment = (iter != curve.m_keys.begin()) ? &*(iter - 1) : nullptr;
                const SCurveEditorKey* pKeyRightOfSegment = (iter != (curve.m_keys.end() - 2)) ? &*(iter + 2) : nullptr;

                const SCurveEditorKey segmentStartKey = ApplyOutTangentFlags(*iter, pKeyLeftOfSegment, *(iter + 1));
                const SCurveEditorKey segmentEndKey = ApplyInTangentFlags(*(iter + 1), *iter, pKeyRightOfSegment);

                if (segmentStartKey.m_controlPoint.m_value != iter->m_controlPoint.m_value)
                {
                    const Vec2 start = Vec2(segmentStartKey.m_time.ToFloat(), segmentStartKey.m_controlPoint.m_value);
                    const Vec2 end = Vec2(iter->m_time.ToFloat(), iter->m_controlPoint.m_value);

                    const QPointF startTransformed = Vec2ToPoint(transformFunc(start));
                    const QPointF endTransformed = Vec2ToPoint(transformFunc(end));

                    path.moveTo(startTransformed);
                    path.lineTo(endTransformed);
                }

                if (segmentEndKey.m_controlPoint.m_value != (iter + 1)->m_controlPoint.m_value)
                {
                    const Vec2 start = Vec2(segmentEndKey.m_time.ToFloat(), segmentEndKey.m_controlPoint.m_value);
                    const Vec2 end = Vec2((iter + 1)->m_time.ToFloat(), (iter + 1)->m_controlPoint.m_value);

                    const QPointF startTransformed = Vec2ToPoint(transformFunc(start));
                    const QPointF endTransformed = Vec2ToPoint(transformFunc(end));

                    path.moveTo(startTransformed);
                    path.lineTo(endTransformed);
                }
            }
        }

        QVector<qreal> dashPattern;
        dashPattern << 2 << 10;

        QPainterPathStroker stroker;
        stroker.setCapStyle(Qt::RoundCap);
        stroker.setDashPattern(dashPattern);
        stroker.setWidth(0.5);

        return stroker.createStroke(path);
    }

    void DrawPointRect(QPainter& painter, QPointF point, const QColor& color)
    {
        painter.setBrush(QBrush(color));
        painter.setPen(QColor(0, 0, 0));
        painter.drawRect(QRectF(point - kPointRectExtent, point + kPointRectExtent));
    }

    void DrawKeys(QPainter& painter, const QPalette& palette, const SCurveEditorCurve& curve, ECurveEditorCurveType curveType, AZStd::function<Vec2(Vec2)> transformFunc, const bool bDrawHandles)
    {
        const QColor tangentColor = CurveEditorHelpers::Interpolate(QColor(), QColor(curve.m_color.r, curve.m_color.g, curve.m_color.b, curve.m_color.a), 0.3f);
        const QPen tangentPen = QPen(tangentColor, 2.5);

        for (auto iter = curve.m_keys.begin(); iter != curve.m_keys.end(); ++iter)
        {
            SCurveEditorKey key = *iter;

            const Vec2 keyPoint = Vec2(key.m_time.ToFloat(), key.m_controlPoint.m_value);
            const QPointF transformedKeyPoint = Vec2ToPoint(transformFunc(keyPoint));

            const bool bIsFirstKey = (iter == curve.m_keys.begin());
            const bool bIsLastKey = (iter == (curve.m_keys.end() - 1));
            const SCurveEditorKey* pLeftKey = (!bIsFirstKey) ? &*(iter - 1) : nullptr;
            const SCurveEditorKey* pRightKey = (!bIsLastKey) ? &*(iter + 1) : nullptr;
            key = pRightKey ? ApplyOutTangentFlags(key, pLeftKey, *pRightKey) : key;
            key = pLeftKey ? ApplyInTangentFlags(key, *pLeftKey, pRightKey) : key;

            // For 1D Bezier, we need to ignore the X component
            const Vec2 inTangent = (curveType == eCECT_Bezier) ? Vec2(0.0f, key.m_controlPoint.m_inTangent.y) : key.m_controlPoint.m_inTangent;
            const Vec2 outTangent = (curveType == eCECT_Bezier) ? Vec2(0.0f, key.m_controlPoint.m_outTangent.y) : key.m_controlPoint.m_outTangent;

            if (key.m_bSelected && (key.m_controlPoint.m_inTangentType != SBezierControlPoint::eTangentType_Step) && !bIsFirstKey && bDrawHandles)
            {
                // Draw incoming tangent
                const Vec2 tangentHandlePoint = keyPoint + inTangent;
                const QPointF transformedTangentHandlePoint = Vec2ToPoint(transformFunc(tangentHandlePoint));
                painter.setPen(tangentPen);
                painter.drawLine(transformedKeyPoint, transformedTangentHandlePoint);
                DrawPointRect(painter, transformedTangentHandlePoint, palette.color(QPalette::Dark));
            }

            if (key.m_bSelected && (key.m_controlPoint.m_outTangentType != SBezierControlPoint::eTangentType_Step) && !bIsLastKey && bDrawHandles)
            {
                // Draw outgoing tangent
                const Vec2 tangentHandlePoint = keyPoint + outTangent;
                const QPointF transformedTangentHandlePoint = Vec2ToPoint(transformFunc(tangentHandlePoint));
                painter.setPen(tangentPen);
                painter.drawLine(transformedKeyPoint, transformedTangentHandlePoint);
                DrawPointRect(painter, transformedTangentHandlePoint, palette.color(QPalette::Dark));
            }

            const QColor pointColor = key.m_bSelected ? palette.color(QPalette::Highlight) : palette.color(QPalette::Dark);
            DrawPointRect(painter, transformedKeyPoint, pointColor);
        }
    }

    void ForEachKey(SCurveEditorContent& content, AZStd::function<void (SCurveEditorCurve& curve, SCurveEditorKey& key)> fun)
    {
        for (auto iter = content.m_curves.begin(); iter != content.m_curves.end(); ++iter)
        {
            SCurveEditorCurve& curve = *iter;
            for (size_t i = 0; i < curve.m_keys.size(); ++i)
            {
                fun(curve, curve.m_keys[i]);
            }
        }
    }

#pragma warning (push)
#pragma warning (disable : 4554)

    Vec2 ClosestPointOnBezierSegment(const Vec2 point, const float t0, const float t1, const float p0, const float p1, const float p2, const float p3)
    {
        // If values are too close the distance function is too flat to be useful. We just assume the curve is flat then
        if ((p0 * p0 + p1 * p1 + p2 * p2 + p3 * p3) < 1e-10f)
        {
            return Vec2(point.x, p0);
        }

        const float deltaTime = (t1 - t0);
        const float deltaTimeSq = deltaTime * deltaTime;

        // Those are just the normal cubic Bezier formulas B(t) and B'(t) in collected polynomial form
        const P3f cubicBezierPoly = P3f(-p0 + 3.0f * p1 - 3.0f * p2 + p3) + P2f(3.0f * p0 - 6.0f * p1 + 3.0f * p2) + P1f(3.0f * p1 - 3.0f * p0) + p0;
        const P2f cubicBezierDerivativePoly = P2f(-3.0f * p0 + 9.0f * p1 - 6.0f * p2 + 3.0f * (p3 - p2)) + P1f(6.0f * p0 - 12.0f * p1 + 6.0f * p2) - 3.0f * p0 + 3.0f * p1;

        // lerp(t, t0, t1) in polynomial form
        const P1f timePoly = P1f(deltaTime) + t0;

        // Derivative of the distance function (cubicBezierPoly - point.y) ^ 2 + (timePoly - point.x) ^ 2
        const auto distanceDerivativePoly = (cubicBezierDerivativePoly * (cubicBezierPoly - point.y) + (timePoly - point.x) * deltaTime) * 2.0f;

        // The point of minimum distance must be at one of the roots of the distance derivative or at the start/end of the segment
        float checkPoints[7];
        const uint numRoots = distanceDerivativePoly.findroots(0.0f, 1.0f, checkPoints + 2);

        // Start and end of segment
        checkPoints[0] = 0.0f;
        checkPoints[1] = 1.0f;

        // Find the closest point among all the candidates
        Vec2 closestPoint;
        float minDistanceSq = std::numeric_limits<float>::max();
        for (uint i = 0; i < numRoots + 2; ++i)
        {
            const Vec2 rootPoint(Lerp(t0, t1, checkPoints[i]), Bezier::Evaluate(checkPoints[i], p0, p1, p2, p3));
            const float deltaX = rootPoint.x - point.x;
            const float deltaY = rootPoint.y - point.y;
            const float distSq = deltaX * deltaX + deltaY * deltaY;
            if (distSq < minDistanceSq)
            {
                closestPoint = rootPoint;
                minDistanceSq = distSq;
            }
        }

        return closestPoint;
    }

    Vec2 ClosestPointOn2DBezierSegment(const Vec2 point, const Vec2 p0, const Vec2 p1, const Vec2 p2, const Vec2 p3)
    {
        // If values are too close the distance function is too flat to be useful. We just assume the curve is flat then
        if ((p0.y * p0.y + p1.y * p1.y + p2.y * p2.y + p3.y * p3.y) < 1e-10f)
        {
            return Vec2(point.x, p0.y);
        }

        // Those are just the normal cubic Bezier formulas B(t) and B'(t) in collected polynomial form
        const P3f xCubicBezierPoly = P3f(-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) + P2f(3.0f * p0.x - 6.0f * p1.x + 3.0f * p2.x) + P1f(3.0f * p1.x - 3.0f * p0.x) + p0.x;
        const P2f xCubicBezierDerivativePoly = P2f(-3.0f * p0.x + 9.0f * p1.x - 6.0f * p2.x + 3.0f * (p3.x - p2.x)) + P1f(6.0f * p0.x - 12.0f * p1.x + 6.0f * p2.x) - 3.0f * p0.x + 3.0f * p1.x;
        const P3f yCubicBezierPoly = P3f(-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) + P2f(3.0f * p0.y - 6.0f * p1.y + 3.0f * p2.y) + P1f(3.0f * p1.y - 3.0f * p0.y) + p0.y;
        const P2f yCubicBezierDerivativePoly = P2f(-3.0f * p0.y + 9.0f * p1.y - 6.0f * p2.y + 3.0f * (p3.y - p2.y)) + P1f(6.0f * p0.y - 12.0f * p1.y + 6.0f * p2.y) - 3.0f * p0.y + 3.0f * p1.y;

        // Derivative of the distance function (yCubicBezierPoly - point.y) ^ 2 + (xCubicBezierPoly - point.x) ^ 2
        const auto distanceDerivativePoly = yCubicBezierDerivativePoly * (yCubicBezierPoly - point.y) + xCubicBezierDerivativePoly * (xCubicBezierPoly - point.x);

        // The point of minimum distance must be at one of the roots of the distance derivative or at the start/end of the segment
        float checkPoints[7];
        const uint numRoots = distanceDerivativePoly.findroots(0.0f, 1.0f, checkPoints + 2);

        // Start and end of segment
        checkPoints[0] = 0.0f;
        checkPoints[1] = 1.0f;

        // Find the closest point among all the candidates
        Vec2 closestPoint;
        float minDistanceSq = std::numeric_limits<float>::max();
        for (uint i = 0; i < numRoots + 2; ++i)
        {
            const Vec2 rootPoint(Bezier::Evaluate(checkPoints[i], p0.x, p1.x, p2.x, p3.x), Bezier::Evaluate(checkPoints[i], p0.y, p1.y, p2.y, p3.y));
            const float deltaX = rootPoint.x - point.x;
            const float deltaY = rootPoint.y - point.y;
            const float distSq = deltaX * deltaX + deltaY * deltaY;
            if (distSq < minDistanceSq)
            {
                closestPoint = rootPoint;
                minDistanceSq = distSq;
            }
        }

        return closestPoint;
    }

    // This works for 1D and 2D bezier because the y range of values is not affected by the x bezier in the 2D case.
    Range GetBezierSegmentValueRange(const SCurveEditorKey& startKey, const SCurveEditorKey& endKey)
    {
        const float p0 = startKey.m_controlPoint.m_value;
        const float p1 = p0 + startKey.m_controlPoint.m_outTangent.y;
        const float p3 = endKey.m_controlPoint.m_value;
        const float p2 = p3 + endKey.m_controlPoint.m_inTangent.y;

        Range valueRange(std::min(p0, p3), std::max(p0, p3));

        const P2f cubicBezierDerivativePoly = P2f(-3.0f * p0 + 9.0f * p1 - 6.0f * p2 + 3.0f * (p3 - p2)) + P1f(6.0f * p0 - 12.0f * p1 + 6.0f * p2) - 3.0f * p0 + 3.0f * p1;

        float roots[2];
        const uint numRoots = cubicBezierDerivativePoly.findroots(0.0f, 1.0f, roots);
        for (uint i = 0; i < numRoots; ++i)
        {
            const float rootValue = Bezier::Evaluate(roots[i], p0, p1, p2, p3);
            valueRange.start = std::min(valueRange.start, rootValue);
            valueRange.end = std::max(valueRange.end, rootValue);
        }

        return valueRange;
    }
}

#pragma warning (pop)

struct CCurveEditor::SMouseHandler
{
    virtual ~SMouseHandler() = default;
    virtual void mousePressEvent(QMouseEvent* pEvent) {}
    virtual void mouseDoubleClickEvent(QMouseEvent* pEvent) {}
    virtual void mouseMoveEvent(QMouseEvent* pEvent) {}
    virtual void mouseReleaseEvent(QMouseEvent* pEvent) {}
    virtual void focusOutEvent(QFocusEvent* pEvent) {}
    virtual void paintOver(QPainter& painter) {}
};

struct CCurveEditor::SSelectionHandler
    : public CCurveEditor::SMouseHandler
{
    CCurveEditor* m_pCurveEditor;
    QPoint m_startPoint;
    QRect m_rect;
    bool m_bAdd;

    SSelectionHandler(CCurveEditor* pCurveEditor, bool bAdd)
        : m_pCurveEditor(pCurveEditor)
        , m_bAdd(bAdd) {}

    void mousePressEvent(QMouseEvent* pEvent) override
    {
        m_startPoint = pEvent->pos();
        m_rect = QRect(m_startPoint, m_startPoint + QPoint(1, 1));
    }

    void mouseMoveEvent(QMouseEvent* pEvent) override
    {
        m_rect = QRect(m_startPoint, pEvent->pos() + QPoint(1, 1));
    }

    void mouseReleaseEvent(QMouseEvent* pEvent) override
    {
        m_pCurveEditor->SelectInRect(m_rect);
    }

    void paintOver(QPainter& painter) override
    {
        painter.save();
        QColor highlightColor = m_pCurveEditor->palette().color(QPalette::Highlight);
        QColor highlightColorA = QColor(highlightColor.red(), highlightColor.green(), highlightColor.blue(), 128);
        painter.setPen(QPen(highlightColor));
        painter.setBrush(QBrush(highlightColorA));
        painter.drawRect(QRectF(m_rect));
        painter.restore();
    }
};

struct CCurveEditor::SPanHandler
    : public CCurveEditor::SMouseHandler
{
    CCurveEditor* m_pCurveEditor;
    QPoint m_startPoint;
    Vec2 m_startTranslation;

    SPanHandler(CCurveEditor* pCurveEditor)
        : m_pCurveEditor(pCurveEditor)
    {
    }

    void mousePressEvent(QMouseEvent* pEvent) override
    {
        m_startPoint = QPoint(int(pEvent->x()), int(pEvent->y()));
        m_startTranslation = m_pCurveEditor->m_translation;
    }

    void mouseMoveEvent(QMouseEvent* pEvent) override
    {
        const Vec2 windowSize((float)m_pCurveEditor->size().width(), (float)m_pCurveEditor->size().height());

        const int pixelDeltaX = pEvent->x() - m_startPoint.x();
        const int pixelDeltaY = pEvent->y() - m_startPoint.y();

        const float deltaX = float(pixelDeltaX) / (windowSize.x);
        const float deltaY = float(pixelDeltaY) / (windowSize.y);

        const Vec2 delta(deltaX, deltaY);
        m_pCurveEditor->m_translation = m_startTranslation + delta;
        m_pCurveEditor->update();
    }
};

struct CCurveEditor::SZoomHandler
    : public CCurveEditor::SMouseHandler
{
    CCurveEditor* m_pCurveEditor;
    Vec2 m_pivot;
    QPoint m_lastPoint;

    SZoomHandler(CCurveEditor* pCurveEditor)
        : m_pCurveEditor(pCurveEditor)
    {
    }

    void mousePressEvent(QMouseEvent* pEvent) override
    {
        m_lastPoint = QPoint(int(pEvent->x()), int(pEvent->y()));

        const QRect curveArea = m_pCurveEditor->GetCurveArea();
        const float pivotXNormalized = (float)(m_lastPoint.x() - curveArea.left()) / (float)curveArea.width();
        const float pivotYNormalized = (float)(m_lastPoint.y() - curveArea.top()) / (float)curveArea.height();
        m_pivot = Vec2(pivotXNormalized, pivotYNormalized);
    }

    void mouseMoveEvent(QMouseEvent* pEvent) override
    {
        const Vec2 windowSize((float)m_pCurveEditor->size().width(), (float)m_pCurveEditor->size().height());

        const int pixelDeltaX = pEvent->x() - m_lastPoint.x();
        const int pixelDeltaY = -(pEvent->y() - m_lastPoint.y());
        m_lastPoint = QPoint(int(pEvent->x()), int(pEvent->y()));

        Vec2& translation = m_pCurveEditor->m_translation;
        Vec2& zoom = m_pCurveEditor->m_zoom;

        const float pivotX = (m_pivot.x - translation.x) / zoom.x;
        const float pivotY = (m_pivot.y - translation.y) / zoom.y;

        zoom.x *= pow(1.2f, (float)pixelDeltaX * 0.03f);
        zoom.y *= pow(1.2f, (float)pixelDeltaY * 0.03f);

        zoom.x = clamp_tpl(zoom.x, kMinZoom, kMaxZoom);
        zoom.y = clamp_tpl(zoom.y, kMinZoom, kMaxZoom);

        // Adjust translation so pivot point stays at same x and y position on screen
        translation.x += ((m_pivot.x - translation.x) / zoom.x - pivotX) * zoom.x;
        translation.y += ((m_pivot.y - translation.y) / zoom.y - pivotY) * zoom.y;

        m_pCurveEditor->update();
    }
};

struct CCurveEditor::SScrubHandler
    : SMouseHandler
{
    CCurveEditor* m_pCurveEditor;
    SAnimTime m_startThumbPosition;
    QPoint m_startPoint;

    SScrubHandler(CCurveEditor* pCurveEditor)
        : m_pCurveEditor(pCurveEditor)
    {
    }

    void mousePressEvent(QMouseEvent* ev) override
    {
        QPoint point = QPoint(ev->pos().x(), ev->pos().y());

        const Vec2 pointInCurveSpace = TransformPointFromScreen(m_pCurveEditor->m_zoom, m_pCurveEditor->m_translation, m_pCurveEditor->GetCurveArea(), PointToVec2(point));

        m_pCurveEditor->m_time = clamp_tpl(SAnimTime(pointInCurveSpace.x), m_pCurveEditor->m_timeRange.start, m_pCurveEditor->m_timeRange.end);
        m_startThumbPosition = m_pCurveEditor->m_time;
        m_startPoint = point;

        m_pCurveEditor->SignalScrub();
    }

    void Apply(QMouseEvent* ev, bool continuous)
    {
        QPoint point = QPoint(ev->pos().x(), ev->pos().y());

        bool shift = ev->modifiers().testFlag(Qt::ShiftModifier);
        bool control = ev->modifiers().testFlag(Qt::ControlModifier);

        const float deltaX = (float)(point.x() - m_startPoint.x());
        const float width = (float)m_pCurveEditor->size().width();
        float delta = float(deltaX) / (width * m_pCurveEditor->m_zoom.x);

        if (shift)
        {
            delta *= 0.01f;
        }

        if (control)
        {
            delta *= 0.1f;
        }

        m_pCurveEditor->m_time = clamp_tpl(m_startThumbPosition + SAnimTime(delta), m_pCurveEditor->m_timeRange.start, m_pCurveEditor->m_timeRange.end);
        m_pCurveEditor->SignalScrub();
    }

    void mouseMoveEvent(QMouseEvent* ev) override
    {
        Apply(ev, true);
    }

    void mouseReleaseEvent(QMouseEvent* ev) override
    {
        Apply(ev, false);
    }
};

struct CCurveEditor::SMoveHandler
    : public CCurveEditor::SMouseHandler
{
    CCurveEditor* m_pCurveEditor;
    bool m_bCycleSelection;
    Vec2 m_startPoint;
    SAnimTime m_minSelectedTime;
    std::vector<SAnimTime> m_keyTimes;
    std::vector<float> m_keyValues;

    SMoveHandler(CCurveEditor* pCurveEditor, bool bCycleSelection)
        : m_pCurveEditor(pCurveEditor)
        , m_bCycleSelection(bCycleSelection)
        , m_startPoint(0.0f, 0.0f)
    {}

    void mousePressEvent(QMouseEvent* pEvent) override
    {
        const QPoint currentPos = pEvent->pos();
        m_startPoint = TransformPointFromScreen(m_pCurveEditor->m_zoom, m_pCurveEditor->m_translation, m_pCurveEditor->GetCurveArea(), PointToVec2(currentPos));
        StoreKeyPositions();
    }

    void mouseMoveEvent(QMouseEvent* pEvent) override
    {
        RestoreKeyPositions();

        const QPoint currentPos = pEvent->pos();
        const Vec2 transformedPos = TransformPointFromScreen(m_pCurveEditor->m_zoom, m_pCurveEditor->m_translation, m_pCurveEditor->GetCurveArea(), PointToVec2(currentPos));
        const Vec2 offset = transformedPos - m_startPoint;

        SAnimTime deltaTime = SAnimTime(offset.x);
        if (m_pCurveEditor->m_bSnapKeys)
        {
            SAnimTime newMinKeyTime = m_minSelectedTime + deltaTime;
            newMinKeyTime = newMinKeyTime.SnapToNearest(m_pCurveEditor->m_frameRate);
            deltaTime = newMinKeyTime - m_minSelectedTime;
        }

        SCurveEditorContent* pContent = m_pCurveEditor->m_pContent;
        for (auto curveIter = pContent->m_curves.begin(); curveIter != pContent->m_curves.end(); ++curveIter)
        {
            SCurveEditorCurve& curve = *curveIter;

            for (auto iter = curve.m_keys.begin(); iter != curve.m_keys.end(); ++iter)
            {
                if (iter->m_bSelected)
                {
                    iter->m_time += deltaTime;
                    iter->m_controlPoint.m_value += offset.y;
                    iter->m_bModified = true;
                }
            }

            m_pCurveEditor->SortKeys(curve);
        }
    }

    void focusOutEvent(QFocusEvent* pEvent) override
    {
        RestoreKeyPositions();
    }

    void mouseReleaseEvent(QMouseEvent* pEvent) override
    {
        m_pCurveEditor->ContentChanged();
    }

    void StoreKeyPositions()
    {
        m_minSelectedTime = SAnimTime::Max();

        SCurveEditorContent* pContent = m_pCurveEditor->m_pContent;
        for (auto curveIter = pContent->m_curves.begin(); curveIter != pContent->m_curves.end(); ++curveIter)
        {
            SCurveEditorCurve& curve = *curveIter;
            for (auto iter = curve.m_keys.begin(); iter != curve.m_keys.end(); ++iter)
            {
                if (iter->m_bSelected)
                {
                    m_keyTimes.push_back(iter->m_time);
                    m_keyValues.push_back(iter->m_controlPoint.m_value);
                    m_minSelectedTime = min(m_minSelectedTime, iter->m_time);
                }
            }
        }
    }

    void RestoreKeyPositions()
    {
        SCurveEditorContent* pContent = m_pCurveEditor->m_pContent;

        auto timeIter = m_keyTimes.begin();
        auto valueIter = m_keyValues.begin();

        for (auto curveIter = pContent->m_curves.begin(); curveIter != pContent->m_curves.end(); ++curveIter)
        {
            SCurveEditorCurve& curve = *curveIter;
            for (auto iter = curve.m_keys.begin(); iter != curve.m_keys.end(); ++iter)
            {
                if (iter->m_bSelected)
                {
                    iter->m_time = *(timeIter++);
                    iter->m_controlPoint.m_value = *(valueIter++);
                    iter->m_bModified = false;
                }
            }
        }
    }
};

struct CCurveEditor::SHandleMoveHandler
    : public CCurveEditor::SMouseHandler
{
    CCurveEditor* m_pCurveEditor;
    SCurveEditorKey m_appliedHandlesKey;
    SCurveEditorKey* m_pKey;
    CCurveEditor::ETangent m_tangent;
    Vec2 m_startPoint;
    Vec2 m_inTangentStartPosition;
    Vec2 m_outTangentStartPosition;
    SBezierControlPoint::ETangentType m_inTangentStartType;
    SBezierControlPoint::ETangentType m_outTangentStartType;
    float m_inTangentStartLength;
    float m_outTangentStartLength;

    SHandleMoveHandler(CCurveEditor* pCurveEditor, SCurveEditorKey appliedHandlesKey, SCurveEditorKey* pKey, CCurveEditor::ETangent tangent)
        : m_pCurveEditor(pCurveEditor)
        , m_appliedHandlesKey(appliedHandlesKey)
        , m_pKey(pKey)
        , m_tangent(tangent)
        , m_inTangentStartPosition(ZERO)
        , m_inTangentStartType(SBezierControlPoint::eTangentType_Auto)
        , m_inTangentStartLength(0.0f)
        , m_outTangentStartPosition(ZERO)
        , m_outTangentStartType(SBezierControlPoint::eTangentType_Auto)
        , m_outTangentStartLength(0.0f)
    {
    }

    void mousePressEvent(QMouseEvent* pEvent) override
    {
        const QPoint currentPos = pEvent->pos();
        m_startPoint = TransformPointFromScreen(m_pCurveEditor->m_zoom, m_pCurveEditor->m_translation, m_pCurveEditor->GetCurveArea(), PointToVec2(currentPos));

        m_inTangentStartPosition = m_appliedHandlesKey.m_controlPoint.m_inTangent;
        m_inTangentStartType = m_appliedHandlesKey.m_controlPoint.m_inTangentType;
        m_inTangentStartLength = m_inTangentStartPosition.GetLength();
        m_outTangentStartPosition = m_appliedHandlesKey.m_controlPoint.m_outTangent;
        m_outTangentStartType = m_appliedHandlesKey.m_controlPoint.m_outTangentType;
        m_outTangentStartLength = m_outTangentStartPosition.GetLength();
    }

    void mouseMoveEvent(QMouseEvent* pEvent) override
    {
        const QPoint currentPos = pEvent->pos();
        const Vec2 transformedPos = TransformPointFromScreen(m_pCurveEditor->m_zoom, m_pCurveEditor->m_translation, m_pCurveEditor->GetCurveArea(), PointToVec2(currentPos));

        if (m_tangent == CCurveEditor::ETangent_In)
        {
            const Vec2 newPos = m_inTangentStartPosition + (transformedPos - m_startPoint);

            m_pKey->m_controlPoint.m_inTangent = newPos;
            m_pKey->m_controlPoint.m_inTangentType = SBezierControlPoint::eTangentType_Custom;

            if (!m_pKey->m_controlPoint.m_bBreakTangents)
            {
                m_pKey->m_controlPoint.m_outTangent = -newPos.GetNormalizedSafe() * m_outTangentStartLength;
                m_pKey->m_controlPoint.m_outTangentType = SBezierControlPoint::eTangentType_Custom;
            }
        }
        else
        {
            const Vec2 newPos = m_outTangentStartPosition + (transformedPos - m_startPoint);

            m_pKey->m_controlPoint.m_outTangent = newPos;
            m_pKey->m_controlPoint.m_outTangentType = SBezierControlPoint::eTangentType_Custom;

            if (!m_pKey->m_controlPoint.m_bBreakTangents)
            {
                m_pKey->m_controlPoint.m_inTangent = -newPos.GetNormalizedSafe() * m_inTangentStartLength;
                m_pKey->m_controlPoint.m_inTangentType = SBezierControlPoint::eTangentType_Custom;
            }
        }

        m_pKey->m_bModified = true;
    }

    void focusOutEvent(QFocusEvent* pEvent) override
    {
        m_pKey->m_controlPoint.m_inTangent = m_inTangentStartPosition;
        m_pKey->m_controlPoint.m_inTangentType = m_inTangentStartType;
        m_pKey->m_controlPoint.m_outTangent = m_outTangentStartPosition;
        m_pKey->m_controlPoint.m_outTangentType = m_outTangentStartType;
        m_pKey->m_bModified = false;
    }

    void mouseReleaseEvent(QMouseEvent* pEvent) override
    {
        m_pCurveEditor->ContentChanged();
    }
};

CCurveEditor::CCurveEditor(QWidget* parent)
    : QWidget(parent)
    , m_pContent(nullptr)
    , m_pMouseHandler(nullptr)
    , m_curveType(eCECT_Bezier)
    , m_frameRate(SAnimTime::eFrameRate_30fps)
    , m_bWeighted(false)
    , m_bHandlesVisible(true)
    , m_bRulerVisible(true)
    , m_bTimeSliderVisible(true)
    , m_bGridVisible(false)
    , m_bSnapTime(false)
    , m_bSnapKeys(false)
    , m_time(SAnimTime(0))
    , m_zoom(0.5f, 0.5f)
    , m_translation(0.5f, 0.5f)
    , m_timeRange(SAnimTime::Min(), SAnimTime::Max())
    , m_valueRange(-1e10f, 1e10f)
{
    setMouseTracking(true);
}

CCurveEditor::~CCurveEditor()
{
}

void CCurveEditor::SetContent(SCurveEditorContent* pContent)
{
    m_pContent = pContent;
    update();
}

void CCurveEditor::SetTime(const SAnimTime time)
{
    m_time = clamp_tpl<>(time, m_timeRange.start, m_timeRange.end);
    update();
}

void CCurveEditor::SetTimeRange(const SAnimTime start, const SAnimTime end)
{
    if (start <= end)
    {
        m_timeRange = TRange<SAnimTime>(start, end);
        update();
    }
}

void CCurveEditor::SetValueRange(const float min, const float max)
{
    if (min <= max)
    {
        m_valueRange = Range(min, max);
        update();
    }
}

void CCurveEditor::ZoomToTimeRange(const float start, const float end)
{
    const float delta = (end - start);

    if (delta > 1e-10f)
    {
        m_zoom.x = 1.0f / (end - start);
        m_translation.x = start / (start - end);
    }
    else
    {
        // Just center around value with zoom = 1.0f
        m_zoom.x = 1.0f;
        m_translation.x = 0.5f - start;
    }
}

void CCurveEditor::ZoomToValueRange(const float min, const float max)
{
    const float delta = (max - min);

    if (delta > 1e-10f)
    {
        m_zoom.y = 1.0f / (max - min);
        m_translation.y = max / (max - min);
    }
    else
    {
        // Just center around value with zoom = 1.0f
        m_zoom.y = 1.0f;
        m_translation.y = 0.5f + min;
    }
}

void CCurveEditor::paintEvent(QPaintEvent* pEvent)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(0.5f, 0.5f);

    const QPalette& palette = this->palette();

    auto transformFunc = [&](Vec2 point)
        {
            return TransformPointToScreen(m_zoom, m_translation, GetCurveArea(), point);
        };

    const QColor rangeHighlightColor = CurveEditorHelpers::Interpolate(palette.color(QPalette::Foreground), palette.color(QPalette::Background), 0.95f);
    const QRectF rangesRect(Vec2ToPoint(transformFunc(Vec2(m_timeRange.start.ToFloat(), m_valueRange.start))), Vec2ToPoint(transformFunc(Vec2(m_timeRange.end.ToFloat(), m_valueRange.end))));
    painter.setPen(QPen(Qt::NoPen));
    painter.setBrush(rangeHighlightColor);
    painter.drawRect(rangesRect);

    if (m_bGridVisible)
    {
        DrawGrid(painter, palette);
    }

    if (m_pContent)
    {
        const QPen extrapolatedCurvePen = QPen(palette.color(QPalette::Highlight));

        TCurveEditorCurves& curves = m_pContent->m_curves;
        for (auto curveIter = curves.begin(); curveIter != curves.end(); ++curveIter)
        {
            SCurveEditorCurve& curve = *curveIter;

            painter.setBrush(QBrush(Qt::NoBrush));
            const QPen curvePen = QPen(QColor(curve.m_color.r, curve.m_color.g, curve.m_color.b, curve.m_color.a), 2);
            const QPen narrowCurvePen = QPen(QColor(curve.m_color.r, curve.m_color.g, curve.m_color.b, curve.m_color.a));

            const QPainterPath extrapolatedPath = CreateExtrapolatedPathFromCurve(curve, transformFunc, width());
            painter.setPen(narrowCurvePen);
            painter.drawPath(extrapolatedPath);

            const QPainterPath discontinuityPath = CreateDiscontinuityPathFromCurve(curve, m_curveType, transformFunc);
            painter.setPen(narrowCurvePen);
            painter.drawPath(discontinuityPath);

            if (curve.m_keys.size() > 0)
            {
                const QPainterPath path = CreatePathFromCurve(curve, m_curveType, transformFunc);
                painter.setPen(curvePen);
                painter.drawPath(path);

                DrawKeys(painter, palette, curve, m_curveType, transformFunc, m_bHandlesVisible);
            }
        }
    }

    if (m_pMouseHandler)
    {
        m_pMouseHandler->paintOver(painter);
    }

    DrawingPrimitives::SRulerOptions rulerOptions;
    rulerOptions.m_rect = QRect(0, -1, size().width(), kRulerHeight + 2);
    rulerOptions.m_visibleRange = Range(-m_translation.x / m_zoom.x, (1.0f - m_translation.x) / m_zoom.x);
    rulerOptions.m_rulerRange = rulerOptions.m_visibleRange;
    rulerOptions.m_markHeight = kRulerMarkHeight;
    rulerOptions.m_shadowSize = kRulerShadowHeight;

    int rulerPrecision;
    DrawingPrimitives::DrawRuler(painter, palette, rulerOptions, &rulerPrecision);

    if (m_pContent && isEnabled())
    {
        DrawingPrimitives::STimeSliderOptions timeSliderOptions;
        timeSliderOptions.m_rect = rect();
        timeSliderOptions.m_precision = rulerPrecision;
        timeSliderOptions.m_position = transformFunc(Vec2(m_time.ToFloat(), 0.0f)).x;
        timeSliderOptions.m_time = m_time.ToFloat();
        timeSliderOptions.m_bHasFocus = hasFocus();
        DrawingPrimitives::DrawTimeSlider(painter, palette, timeSliderOptions);
    }
}

void CCurveEditor::mousePressEvent(QMouseEvent* pEvent)
{
    setFocus();

    if (pEvent->button() == Qt::LeftButton)
    {
        LeftButtonMousePressEvent(pEvent);
    }
    else if (pEvent->button() == Qt::MiddleButton)
    {
        MiddleButtonMousePressEvent(pEvent);
    }
    else if (pEvent->button() == Qt::RightButton)
    {
        RightButtonMousePressEvent(pEvent);
    }
}

void CCurveEditor::mouseDoubleClickEvent(QMouseEvent* pEvent)
{
    if (pEvent->button() == Qt::LeftButton)
    {
        auto curveHitPair = HitDetectCurve(pEvent->pos());
        if (curveHitPair.first)
        {
            AddPointToCurve(curveHitPair.second, curveHitPair.first);
            setCursor(QCursor(Qt::SizeAllCursor));
        }
    }
}

void CCurveEditor::DrawGrid(QPainter& painter, const QPalette& palette)
{
    using namespace DrawingPrimitives;

    QColor gridColor = CurveEditorHelpers::Interpolate(palette.color(QPalette::Dark), palette.color(QPalette::Button), 0.5f);
    gridColor.setAlpha(128);
    const QColor textColor = palette.color(QPalette::BrightText);

    const Range horizontalVisibleRange = Range(-m_translation.x / m_zoom.x, (1.0f - m_translation.x) / m_zoom.x);
    const Range verticalVisibleRange = Range((m_translation.y - 1.0f) / m_zoom.y, m_translation.y / m_zoom.y);

    const int height = size().height();
    const int width = size().width();

    int verticalRulerPrecision;

    std::vector<STick> horizontalTicks = CalculateTicks(width, horizontalVisibleRange, horizontalVisibleRange, nullptr, nullptr);
    std::vector<STick> verticalTicks = CalculateTicks(height, verticalVisibleRange, verticalVisibleRange, &verticalRulerPrecision, nullptr);

    char format[16] = "";
    sprintf_s(format, "%%.%df", verticalRulerPrecision);

    const QPen gridPen(gridColor, 1.0);
    painter.setPen(gridPen);

    for (const STick& tick : horizontalTicks)
    {
        if (!tick.m_bTenth)
        {
            const int x = tick.m_position;
            painter.drawLine(x, kRulerHeight, x, height);
        }
    }

    for (const STick& tick : verticalTicks)
    {
        if (!tick.m_bTenth)
        {
            const int y = height - tick.m_position;
            painter.drawLine(0, y, width, y);
        }
    }

    const QPen textPen(textColor);
    painter.setPen(textPen);

    QString str;
    for (const STick& tick : verticalTicks)
    {
        if (!tick.m_bTenth)
        {
            const int y = height - tick.m_position;
            str.sprintf(format, tick.m_value);
            painter.drawText(5, y - 4, str);
        }
    }
}

void CCurveEditor::LeftButtonMousePressEvent(QMouseEvent* pEvent)
{
    const bool bCtrlPressed = (pEvent->modifiers() & Qt::CTRL) != 0;
    const bool bAltPressed = (pEvent->modifiers() & Qt::ALT) != 0;

    if (pEvent->y() < kRulerHeight)
    {
        m_pMouseHandler.reset(new SScrubHandler(this));
        m_pMouseHandler->mousePressEvent(pEvent);
    }
    else
    {
        if (bCtrlPressed)
        {
            auto curveHitPair = HitDetectCurve(pEvent->pos());
            if (curveHitPair.first)
            {
                AddPointToCurve(curveHitPair.second, curveHitPair.first);
                setCursor(QCursor(Qt::SizeAllCursor));
            }
        }
        else if (bAltPressed)
        {
            auto curveKeyPair = HitDetectKey(pEvent->pos());
            if (curveKeyPair.first)
            {
                curveKeyPair.second->m_bDeleted = true;
                ContentChanged();
            }
        }
        else
        {
            auto curveKeyPair = HitDetectKey(pEvent->pos());
            auto handleKeyTuple = HitDetectHandle(pEvent->pos());

            if (std::get<0>(handleKeyTuple))
            {
                m_pMouseHandler.reset(new SHandleMoveHandler(this, std::get<1>(handleKeyTuple), std::get<2>(handleKeyTuple), std::get<3>(handleKeyTuple)));
            }
            else if (curveKeyPair.first)
            {
                bool useExistingSelection = curveKeyPair.second->m_bSelected;
                if (!useExistingSelection)
                {
                    ForEachKey(*m_pContent, [](SCurveEditorCurve& curve, SCurveEditorKey& key)
                        {
                            key.m_bSelected = false;
                        });
                    curveKeyPair.second->m_bSelected = true;
                }

                m_pMouseHandler.reset(new SMoveHandler(this, false));
            }
            else
            {
                m_pMouseHandler.reset(new SSelectionHandler(this, false));
            }

            m_pMouseHandler->mousePressEvent(pEvent);
        }
    }

    update();
}

void CCurveEditor::MiddleButtonMousePressEvent(QMouseEvent* pEvent)
{
    const bool bShiftPressed = (pEvent->modifiers() & Qt::SHIFT) != 0;

    if (!bShiftPressed)
    {
        m_pMouseHandler.reset(new SPanHandler(this));
    }
    else
    {
        m_pMouseHandler.reset(new SZoomHandler(this));
    }

    m_pMouseHandler->mousePressEvent(pEvent);
    update();
}

void CCurveEditor::RightButtonMousePressEvent(QMouseEvent* pEvent)
{
}

void CCurveEditor::mouseMoveEvent(QMouseEvent* pEvent)
{
    if (m_pMouseHandler)
    {
        m_pMouseHandler->mouseMoveEvent(pEvent);
    }
    else
    {
        if (HitDetectKey(pEvent->pos()).first || std::get<0>(HitDetectHandle(pEvent->pos())))
        {
            setCursor(QCursor(Qt::SizeAllCursor));
        }
        else
        {
            setCursor(QCursor());
        }
    }

    update();
}

void CCurveEditor::mouseReleaseEvent(QMouseEvent* pEvent)
{
    if (m_pMouseHandler)
    {
        m_pMouseHandler->mouseReleaseEvent(pEvent);
        m_pMouseHandler.reset();
        update();
    }
}

void CCurveEditor::focusOutEvent(QFocusEvent* pEvent)
{
    if (m_pMouseHandler)
    {
        m_pMouseHandler->focusOutEvent(pEvent);
        m_pMouseHandler.reset();
        update();
    }
}

void CCurveEditor::wheelEvent(QWheelEvent* pEvent)
{
    Vec2 windowSize((float)size().width(), (float)size().height());
    windowSize.y = (windowSize.y > 0.0f) ? windowSize.y : 1.0f;

    const QRect curveArea = GetCurveArea();
    const float mouseXNormalized = (float)(pEvent->x() - curveArea.left()) / (float)curveArea.width();
    const float mouseYNormalized = (float)(pEvent->y() - curveArea.top()) / (float)curveArea.height();

    const float pivotX = (mouseXNormalized - m_translation.x) / m_zoom.x;
    const float pivotY = (mouseYNormalized - m_translation.y) / m_zoom.y;

    m_zoom *= pow(1.2f, (float)pEvent->delta() * 0.01f);
    m_zoom.x = clamp_tpl(m_zoom.x, kMinZoom, kMaxZoom);
    m_zoom.y = clamp_tpl(m_zoom.y, kMinZoom, kMaxZoom);

    // Adjust translation so pivot point stays at same x and y position on screen
    m_translation.x += ((mouseXNormalized - m_translation.x) / m_zoom.x - pivotX) * m_zoom.x;
    m_translation.y += ((mouseYNormalized - m_translation.y) / m_zoom.y - pivotY) * m_zoom.y;

    update();
}

void CCurveEditor::keyPressEvent(QKeyEvent* pEvent)
{
    if (!m_pContent)
    {
        return;
    }

    QKeySequence key(pEvent->key());

    if (key == QKeySequence(Qt::Key_Delete))
    {
        OnDeleteSelectedKeys();
    }

    update();
}

void CCurveEditor::SetCurveType(ECurveEditorCurveType curveType)
{
    m_curveType = curveType;
    update();
}

void CCurveEditor::SetHandlesVisible(bool bVisible)
{
    m_bHandlesVisible = bVisible;
    update();
}

void CCurveEditor::SetRulerVisible(bool bVisible)
{
    m_bRulerVisible = bVisible;
    update();
}

void CCurveEditor::SetTimeSliderVisible(bool bVisible)
{
    m_bTimeSliderVisible = bVisible;
    update();
}

void CCurveEditor::SetGridVisible(bool bVisible)
{
    m_bGridVisible = bVisible;
    update();
}

void CCurveEditor::SelectInRect(const QRect& rect)
{
    if (!m_pContent)
    {
        return;
    }

    ForEachKey(*m_pContent, [&](SCurveEditorCurve& curve, SCurveEditorKey& key)
        {
            const Vec2 screenPoint = TransformPointToScreen(m_zoom, m_translation, GetCurveArea(), Vec2(key.m_time.ToFloat(), key.m_controlPoint.m_value));
            key.m_bSelected = rect.contains((int)screenPoint.x, (int)screenPoint.y);
        });

    update();
    SignalContentChanged();
}

std::pair<SCurveEditorCurve*, Vec2> CCurveEditor::HitDetectCurve(const QPoint point)
{
    if (!m_pContent)
    {
        return std::make_pair(nullptr, Vec2(ZERO));
    }

    SCurveEditorCurve* pNearestCurve = nullptr;
    Vec2 closestPoint = Vec2(ZERO);

    float nearestDistance = std::numeric_limits<float>::max();
    for (auto iter = m_pContent->m_curves.rbegin(); iter != m_pContent->m_curves.rend(); ++iter)
    {
        SCurveEditorCurve& curve = *iter;
        const Vec2 closestPointOnCurve = ClosestPointOnCurve(PointToVec2(point), curve, m_curveType);

        const float distance = (PointToVec2(point) - closestPointOnCurve).GetLength();
        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            pNearestCurve = &curve;
            closestPoint = closestPointOnCurve;
        }
    }

    if (nearestDistance <= kHitDistance)
    {
        return std::make_pair(pNearestCurve, TransformPointFromScreen(m_zoom, m_translation, GetCurveArea(), closestPoint));
    }

    return std::make_pair(nullptr, Vec2(ZERO));
}

std::pair<SCurveEditorCurve*, SCurveEditorKey*> CCurveEditor::HitDetectKey(const QPoint point)
{
    if (!m_pContent)
    {
        return std::make_pair(nullptr, nullptr);
    }

    for (auto curvesIter = m_pContent->m_curves.rbegin(); curvesIter != m_pContent->m_curves.rend(); ++curvesIter)
    {
        SCurveEditorCurve& curve = *curvesIter;
        for (auto iter = curve.m_keys.rbegin(); iter != curve.m_keys.rend(); ++iter)
        {
            SCurveEditorKey& key = *iter;
            const Vec2 keyPoint = Vec2(key.m_time.ToFloat(), key.m_controlPoint.m_value);
            const Vec2 transformedPoint = TransformPointToScreen(m_zoom, m_translation, GetCurveArea(), keyPoint);
            if ((transformedPoint - PointToVec2(point)).GetLength() <= kHitDistance)
            {
                return std::make_pair(&curve, &key);
            }
        }
    }

    return std::make_pair(nullptr, nullptr);
}

std::tuple<SCurveEditorCurve*, SCurveEditorKey, SCurveEditorKey*, CCurveEditor::ETangent> CCurveEditor::HitDetectHandle(const QPoint point)
{
    if (!m_pContent || !m_bHandlesVisible)
    {
        return std::make_tuple(nullptr, SCurveEditorKey(), nullptr, ETangent_In);
    }

    for (auto curvesIter = m_pContent->m_curves.rbegin(); curvesIter != m_pContent->m_curves.rend(); ++curvesIter)
    {
        SCurveEditorCurve& curve = *curvesIter;
        for (auto iter = curve.m_keys.begin(); iter != curve.m_keys.end(); ++iter)
        {
            SCurveEditorKey key = *iter;

            const bool bIsFirstKey = (iter == curve.m_keys.begin());
            const bool bIsLastKey = (iter == (curve.m_keys.end() - 1));
            const SCurveEditorKey* pLeftKey = (!bIsFirstKey) ? &*(iter - 1) : nullptr;
            const SCurveEditorKey* pRightKey = (!bIsLastKey) ? &*(iter + 1) : nullptr;
            key = pRightKey ? ApplyOutTangentFlags(key, pLeftKey, *pRightKey) : key;
            key = pLeftKey ? ApplyInTangentFlags(key, *pLeftKey, pRightKey) : key;

            const Vec2 inTangent = (m_curveType == eCECT_Bezier) ? Vec2(0.0f, key.m_controlPoint.m_inTangent.y) : key.m_controlPoint.m_inTangent;
            const Vec2 outTangent = (m_curveType == eCECT_Bezier) ? Vec2(0.0f, key.m_controlPoint.m_outTangent.y) : key.m_controlPoint.m_outTangent;

            const Vec2 keyPoint = Vec2(key.m_time.ToFloat(), key.m_controlPoint.m_value);

            if (!bIsFirstKey && (key.m_controlPoint.m_inTangentType != SBezierControlPoint::eTangentType_Step))
            {
                const Vec2 tangentHandlePoint = keyPoint + inTangent;
                const Vec2 transformedTangentHandlePoint = TransformPointToScreen(m_zoom, m_translation, GetCurveArea(), tangentHandlePoint);
                if ((transformedTangentHandlePoint - PointToVec2(point)).GetLength() <= kHitDistance)
                {
                    return std::make_tuple(&curve, key, &(*iter), ETangent_In);
                }
            }

            if (!bIsLastKey && (key.m_controlPoint.m_outTangentType != SBezierControlPoint::eTangentType_Step))
            {
                const Vec2 tangentHandlePoint = keyPoint + outTangent;
                const Vec2 transformedTangentHandlePoint = TransformPointToScreen(m_zoom, m_translation, GetCurveArea(), tangentHandlePoint);
                if ((transformedTangentHandlePoint - PointToVec2(point)).GetLength() <= kHitDistance)
                {
                    return std::make_tuple(&curve, key, &(*iter), ETangent_Out);
                }
            }
        }
    }

    return std::make_tuple(nullptr, SCurveEditorKey(), nullptr, ETangent_In);
}

// Input and output are in screen space
Vec2 CCurveEditor::ClosestPointOnCurve(const Vec2 point, const SCurveEditorCurve& curve, const ECurveEditorCurveType curveType)
{
    auto transformFunc = [&](Vec2 point)
        {
            return TransformPointToScreen(m_zoom, m_translation, GetCurveArea(), point);
        };

    if (curve.m_keys.size() == 0)
    {
        const Vec2 pointOnCurve = transformFunc(Vec2(0.0f, curve.m_defaultValue));
        return Vec2(point.x, pointOnCurve.y);
    }

    Vec2 closestPoint;
    float minDistance = std::numeric_limits<float>::max();

    const Vec2 startKeyTransformed = transformFunc(Vec2(curve.m_keys.front().m_time.ToFloat(), curve.m_keys.front().m_controlPoint.m_value));
    if (point.x < startKeyTransformed.x)
    {
        const float distanceToCurve = std::abs(point.y - startKeyTransformed.y);
        if (distanceToCurve < minDistance)
        {
            closestPoint = Vec2(point.x, startKeyTransformed.y);
            minDistance = distanceToCurve;
        }
    }

    const Vec2 endKeyTransformed = transformFunc(Vec2(curve.m_keys.back().m_time.ToFloat(), curve.m_keys.back().m_controlPoint.m_value));
    if (point.x > endKeyTransformed.x)
    {
        const float distanceToCurve = std::abs(point.y - endKeyTransformed.y);
        if (distanceToCurve < minDistance)
        {
            closestPoint = Vec2(point.x, endKeyTransformed.y);
            minDistance = distanceToCurve;
        }
    }

    const auto endIter = curve.m_keys.end() - 1;
    for (auto iter = curve.m_keys.begin(); iter != endIter; ++iter)
    {
        const SCurveEditorKey* pKeyLeftOfSegment = (iter != curve.m_keys.begin()) ? &*(iter - 1) : nullptr;
        const SCurveEditorKey* pKeyRightOfSegment = (iter != (curve.m_keys.end() - 2)) ? &*(iter + 2) : nullptr;

        const SCurveEditorKey segmentStartKey = ApplyOutTangentFlags(*iter, pKeyLeftOfSegment, *(iter + 1));
        const SCurveEditorKey segmentEndKey = ApplyInTangentFlags(*(iter + 1), *iter, pKeyRightOfSegment);

        const Vec2 p0 = transformFunc(Vec2(segmentStartKey.m_time.ToFloat(), segmentStartKey.m_controlPoint.m_value));
        const Vec2 p3 = transformFunc(Vec2(segmentEndKey.m_time.ToFloat(), segmentEndKey.m_controlPoint.m_value));
        const Vec2 p1 = transformFunc(Vec2(segmentStartKey.m_time.ToFloat() + segmentStartKey.m_controlPoint.m_outTangent.x,
                    segmentStartKey.m_controlPoint.m_value + segmentStartKey.m_controlPoint.m_outTangent.y));
        const Vec2 p2 = transformFunc(Vec2(segmentEndKey.m_time.ToFloat() + segmentEndKey.m_controlPoint.m_inTangent.x,
                    segmentEndKey.m_controlPoint.m_value + segmentEndKey.m_controlPoint.m_inTangent.y));

        const Vec2 closestOnSegment = (curveType == eCECT_Bezier) ? ClosestPointOnBezierSegment(point, p0.x, p3.x, p0.y, p1.y, p2.y, p3.y) : ClosestPointOn2DBezierSegment(point, p0, p1, p2, p3);
        const float distanceToSegment = (closestOnSegment - point).GetLength();
        if (distanceToSegment < minDistance)
        {
            closestPoint = closestOnSegment;
            minDistance = distanceToSegment;
        }
    }

    return closestPoint;
}

void CCurveEditor::ContentChanged()
{
    SignalContentChanged();

    DeleteMarkedKeys();

    ForEachKey(*m_pContent, [](SCurveEditorCurve& curve, SCurveEditorKey& key)
        {
            key.m_bModified = false;
        });

    update();
}

void CCurveEditor::DeleteMarkedKeys()
{
    if (m_pContent)
    {
        for (auto iter = m_pContent->m_curves.begin(); iter != m_pContent->m_curves.end(); ++iter)
        {
            SCurveEditorCurve& curve = *iter;
            for (auto keyIter = curve.m_keys.begin(); keyIter != curve.m_keys.end(); )
            {
                if (keyIter->m_bDeleted)
                {
                    keyIter = curve.m_keys.erase(keyIter);
                }
                else
                {
                    ++keyIter;
                }
            }
        }
    }
}

void CCurveEditor::AddPointToCurve(const Vec2 point, SCurveEditorCurve* pCurve)
{
    SCurveEditorKey key;
    key.m_time = SAnimTime(point.x);
    if (m_bSnapKeys)
    {
        key.m_time.SnapToNearest(m_frameRate);
    }
    key.m_controlPoint.m_value = point.y;
    key.m_bAdded = true;
    pCurve->m_keys.push_back(key);

    SortKeys(*pCurve);
    ContentChanged();
}

void CCurveEditor::SortKeys(SCurveEditorCurve& curve)
{
    std::stable_sort(curve.m_keys.begin(), curve.m_keys.end(), [](const SCurveEditorKey& a, const SCurveEditorKey& b)
        {
            return a.m_time < b.m_time;
        });
}

void CCurveEditor::OnDeleteSelectedKeys()
{
    ForEachKey(*m_pContent, [](SCurveEditorCurve& curve, SCurveEditorKey& key)
        {
            key.m_bDeleted = key.m_bDeleted || key.m_bSelected;
        });

    ContentChanged();
}

void CCurveEditor::OnSetSelectedKeysTangentAuto()
{
    SetSelectedKeysTangentType(ETangent_In, SBezierControlPoint::eTangentType_Auto);
    SetSelectedKeysTangentType(ETangent_Out, SBezierControlPoint::eTangentType_Auto);
}

void CCurveEditor::OnSetSelectedKeysInTangentZero()
{
    SetSelectedKeysTangentType(ETangent_In, SBezierControlPoint::eTangentType_Zero);
}

void CCurveEditor::OnSetSelectedKeysInTangentStep()
{
    SetSelectedKeysTangentType(ETangent_In, SBezierControlPoint::eTangentType_Step);
}

void CCurveEditor::OnSetSelectedKeysInTangentLinear()
{
    SetSelectedKeysTangentType(ETangent_In, SBezierControlPoint::eTangentType_Linear);
}

void CCurveEditor::OnSetSelectedKeysOutTangentZero()
{
    SetSelectedKeysTangentType(ETangent_Out, SBezierControlPoint::eTangentType_Zero);
}

void CCurveEditor::OnSetSelectedKeysOutTangentStep()
{
    SetSelectedKeysTangentType(ETangent_Out, SBezierControlPoint::eTangentType_Step);
}

void CCurveEditor::OnSetSelectedKeysOutTangentLinear()
{
    SetSelectedKeysTangentType(ETangent_Out, SBezierControlPoint::eTangentType_Linear);
}

void CCurveEditor::OnFitCurvesHorizontally()
{
    if (m_pContent)
    {
        bool bAnyKeyFound = false;
        SAnimTime timeMin = SAnimTime::Max();
        SAnimTime timeMax = SAnimTime::Min();

        TCurveEditorCurves& curves = m_pContent->m_curves;
        for (auto curveIter = curves.begin(); curveIter != curves.end(); ++curveIter)
        {
            SCurveEditorCurve& curve = *curveIter;
            if (curve.m_keys.size() > 0)
            {
                bAnyKeyFound = true;
                timeMin = std::min(curve.m_keys.front().m_time, timeMin);
                timeMax = std::max(curve.m_keys.back().m_time, timeMax);
            }
        }

        if (!bAnyKeyFound)
        {
            timeMin = m_timeRange.start;
            timeMax = m_timeRange.end;
        }

        ZoomToTimeRange(timeMin.ToFloat(), timeMax.ToFloat());

        // Adjust zoom and translation depending on kFitMargin
        const float pivot = (0.5f - m_translation.x) / m_zoom.x;
        m_zoom.x /= 1.0f + 2.0f * (kFitMargin / GetCurveArea().width());
        m_translation.x += ((0.5f - m_translation.x) / m_zoom.x - pivot) * m_zoom.x;
    }

    update();
}

void CCurveEditor::OnFitCurvesVertically()
{
    if (m_pContent)
    {
        bool bAnyKeyFound = false;
        float valueMin = std::numeric_limits<float>::max();
        float valueMax = -std::numeric_limits<float>::max();

        TCurveEditorCurves& curves = m_pContent->m_curves;
        for (auto curveIter = curves.begin(); curveIter != curves.end(); ++curveIter)
        {
            SCurveEditorCurve& curve = *curveIter;

            if (curve.m_keys.size() > 1)
            {
                const auto endIter = curve.m_keys.end() - 1;

                for (auto iter = curve.m_keys.begin(); iter != endIter; ++iter)
                {
                    bAnyKeyFound = true;

                    const SCurveEditorKey* pKeyLeftOfSegment = (iter != curve.m_keys.begin()) ? &*(iter - 1) : nullptr;
                    const SCurveEditorKey* pKeyRightOfSegment = (iter != (curve.m_keys.end() - 2)) ? &*(iter + 2) : nullptr;

                    const SCurveEditorKey segmentStartKey = ApplyOutTangentFlags(*iter, pKeyLeftOfSegment, *(iter + 1));
                    const SCurveEditorKey segmentEndKey = ApplyInTangentFlags(*(iter + 1), *iter, pKeyRightOfSegment);

                    const Range valueRange = GetBezierSegmentValueRange(segmentStartKey, segmentEndKey);
                    valueMin = std::min(valueMin, valueRange.start);
                    valueMax = std::max(valueMax, valueRange.end);
                }
            }
            else if (curve.m_keys.size() == 1)
            {
                bAnyKeyFound = true;
                valueMin = valueMax = curve.m_keys[0].m_controlPoint.m_value;
            }
        }

        if (!bAnyKeyFound)
        {
            valueMin = -0.5f;
            valueMax = 0.5f;
        }

        ZoomToValueRange(valueMin, valueMax);

        // Adjust zoom and translation depending on kFitMargin
        const float pivot = (0.5f - m_translation.y) / m_zoom.y;
        m_zoom.y /= 1.0f + 2.0f * (kFitMargin / GetCurveArea().height());
        m_translation.y += ((0.5f - m_translation.y) / m_zoom.y - pivot) * m_zoom.y;
    }

    update();
}

void CCurveEditor::OnBreakTangents()
{
    if (m_pContent)
    {
        ForEachKey(*m_pContent, [&](SCurveEditorCurve& curve, SCurveEditorKey& key)
            {
                if (key.m_bSelected)
                {
                    key.m_controlPoint.m_bBreakTangents = true;
                }
            });
    }

    SignalContentChanged();
}

void CCurveEditor::OnUnifyTangents()
{
    if (m_pContent)
    {
        ForEachKey(*m_pContent, [&](SCurveEditorCurve& curve, SCurveEditorKey& key)
            {
                if (key.m_bSelected)
                {
                    key.m_controlPoint.m_bBreakTangents = false;
                }
            });
    }

    SignalContentChanged();
}

void CCurveEditor::SetSelectedKeysTangentType(const ETangent tangent, const SBezierControlPoint::ETangentType type)
{
    if (m_pContent)
    {
        ForEachKey(*m_pContent, [&](SCurveEditorCurve& curve, SCurveEditorKey& key)
            {
                if (key.m_bSelected)
                {
                    if (tangent == ETangent_In)
                    {
                        key.m_controlPoint.m_inTangentType = type;
                    }
                    else
                    {
                        key.m_controlPoint.m_outTangentType = type;
                    }
                }
            });

        update();
    }

    SignalContentChanged();
}

QRect CCurveEditor::GetCurveArea()
{
    const uint rulerAreaHeight = m_bRulerVisible ? kRulerHeight : 0;
    return QRect(0, rulerAreaHeight, width(), height() - rulerAreaHeight);
}

void CCurveEditor::FillWithCurveToolsAndConnect(QToolBar* pToolBar)
{
    pToolBar->addAction(QIcon(":/Icons/CurveEditor/auto.png"), "Set in and out tangents to auto", this, SLOT(OnSetSelectedKeysTangentAuto()));
    pToolBar->addSeparator();
    pToolBar->addAction(QIcon(":/Icons/CurveEditor/zero_in.png"), "Set in tangent to zero", this, SLOT(OnSetSelectedKeysInTangentZero()));
    pToolBar->addAction(QIcon(":/Icons/CurveEditor/step_in.png"), "Set in tangent to step", this, SLOT(OnSetSelectedKeysInTangentStep()));
    pToolBar->addAction(QIcon(":/Icons/CurveEditor/linear_in.png"), "Set in tangent to linear", this, SLOT(OnSetSelectedKeysInTangentLinear()));
    pToolBar->addSeparator();
    pToolBar->addAction(QIcon(":/Icons/CurveEditor/zero_out.png"), "Set out tangent to zero", this, SLOT(OnSetSelectedKeysOutTangentZero()));
    pToolBar->addAction(QIcon(":/Icons/CurveEditor/step_out.png"), "Set out tangent to step", this, SLOT(OnSetSelectedKeysOutTangentStep()));
    pToolBar->addAction(QIcon(":/Icons/CurveEditor/linear_out.png"), "Set out tangent to linear", this, SLOT(OnSetSelectedKeysOutTangentLinear()));
    pToolBar->addSeparator();
    pToolBar->addAction(QIcon(":/Icons/CurveEditor/fit_horizontal.png"), "Fit curves horizontally", this, SLOT(OnFitCurvesHorizontally()));
    pToolBar->addAction(QIcon(":/Icons/CurveEditor/fit_vertical.png"), "Fit curves vertically", this, SLOT(OnFitCurvesVertically()));
    pToolBar->addSeparator();
    pToolBar->addAction(QIcon(":/Icons/CurveEditor/break.png"), "Break tangents", this, SLOT(OnBreakTangents()));
    pToolBar->addAction(QIcon(":/Icons/CurveEditor/unify.png"), "Unify tangents", this, SLOT(OnUnifyTangents()));
}
