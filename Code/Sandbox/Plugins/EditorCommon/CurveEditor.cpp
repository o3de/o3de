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

#include "EditorCommon_precompiled.h"
#include "CurveEditor.h"
#include "CurveEditorControl.h"
#include "DrawingPrimitives/TimeSlider.h"
#include "DrawingPrimitives/Ruler.h"

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // class '...' needs to have dll-interface to be used by clients of class '...'
#include <QBitmap>
#include <QColor>
#include <QIcon>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QToolTip>
AZ_POP_DISABLE_WARNING

// C6201: buffer overrun for <variable>, which is possibly stack allocated: index <name> is out of valid index range <min> to <max>
#if defined(__clang__)
#define INDEX_NOT_OUT_OF_RANGE _Pragma("clang diagnostic ignored \"-Warray-bounds\"")
#else
#define INDEX_NOT_OUT_OF_RANGE PREFAST_SUPPRESS_WARNING(6201)
#endif

#define NO_BUFFER_OVERRUN PREFAST_SUPPRESS_WARNING(6385 6386)
#include <ISplines.h>
#include "../Cry3DEngine/Cry_LegacyPhysUtils.h"

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

    QColor LerpColor(const QColor& a, const QColor& b, float k)
    {
        float mk = 1.0f - k;
        return QColor(aznumeric_cast<int>(a.red() * mk + b.red() * k),
            aznumeric_cast<int>(a.green() * mk + b.green() * k),
            aznumeric_cast<int>(a.blue() * mk + b.blue() * k),
            aznumeric_cast<int>(a.alpha() * mk + b.alpha() * k));
    }
}

namespace
{
    const int kRulerHeight = 16;
    const int kRulerShadowHeight = 6;
    const int kRulerMarkHeight = 8;
    const int kTextXOffset = -1;
    const int kTextYOffset = 16;
    const int kTangentLength = 24;

    const float kHitDistance = 15.0f;
    const float kMinZoom = 0.001f;
    const float kMaxZoom = 1000.0f;
    const float kFitMargin = 16.0f;

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

    float EvaluateBezier(float t, float p0, float p1, float p2, float p3)
    {
        const float a = 1 - t;
        const float aSq = a * a;
        const float tSq = t * t;
        return (aSq * a * p0) + (3.0f * aSq * t * p1) + (3.0f * a * tSq * p2) + (tSq * t * p3);
    }

    void SplitBezier(SCurveEditorKey& newKey, SCurveEditorKey& leftKey, SCurveEditorKey& rightKey)
    {
        // use De Casteljau's algorithm to find the
        float normalizedTime = (newKey.m_time - leftKey.m_time) / (rightKey.m_time - leftKey.m_time);

        Vec2 p0 = Vec2(leftKey.m_time, leftKey.m_value);
        Vec2 p3 = Vec2(rightKey.m_time, rightKey.m_value);
        Vec2 p1 = p0 + leftKey.m_outTangent;
        Vec2 p2 = p3 + rightKey.m_inTangent;

        Vec2 q0 = p0 + (p1 - p0) * normalizedTime;
        Vec2 q1 = p1 + (p2 - p1) * normalizedTime;
        Vec2 q2 = p2 + (p3 - p2) * normalizedTime;

        Vec2 r0 = q0 + (q1 - q0) * normalizedTime;
        Vec2 r1 = q1 + (q2 - q1) * normalizedTime;

        Vec2 s0 = r0 + (r1 - r0) * normalizedTime;

        newKey.m_inTangent = r0 - s0;
        newKey.m_outTangent = r1 - s0;

        leftKey.m_outTangent = q0 - p0;
        rightKey.m_inTangent = q2 - p3;
    }

    QPointF Vec2ToPoint(Vec2 point)
    {
        return QPointF(point.x, point.y);
    }

    Vec2 PointToVec2(QPointF point)
    {
        return Vec2(aznumeric_cast<float>(point.x()), aznumeric_cast<float>(point.y()));
    }

    // This function returns a new key with position and weights affected by eTangentType_Smooth, eTangentType_Linear and eTangentType_Step for the outgoing tangent
    SCurveEditorKey ApplyOutTangentFlags(const SCurveEditorKey& key, [[maybe_unused]] const SCurveEditorKey* pLeftKey, const SCurveEditorKey& rightKey)
    {
        SCurveEditorKey newKey = key;

        if (rightKey.m_inTangentType == SCurveEditorKey::eTangentType_Step
            && key.m_outTangentType != SCurveEditorKey::eTangentType_Step)
        {
            newKey.m_outTangent.y = 0.0f;
            return newKey;
        }

        switch (key.m_outTangentType)
        {
        case SCurveEditorKey::eTangentType_Linear:
            newKey.m_outTangent.y = (rightKey.m_value - key.m_value) / 3.0f;
            break;
        case SCurveEditorKey::eTangentType_Step:
            newKey.m_outTangent.x = 0.0f;
            newKey.m_outTangent.y = 0.0f;
            newKey.m_value = rightKey.m_value;
            break;
        default:
            const float oneThirdDeltaTime = (rightKey.m_time - newKey.m_time) / 3.0f;
            float ratio = oneThirdDeltaTime / newKey.m_outTangent.x;
            newKey.m_outTangent *= ratio;
            break;
        }

        return newKey;
    }

    // This function returns a new key with position and weights affected by eTangentType_Smooth, eTangentType_Linear and eTangentType_Step for the incoming tangent
    SCurveEditorKey ApplyInTangentFlags(const SCurveEditorKey& key, const SCurveEditorKey& leftKey, [[maybe_unused]] const SCurveEditorKey* pRightKey)
    {
        SCurveEditorKey newKey = key;

        if (leftKey.m_outTangentType == SCurveEditorKey::eTangentType_Step)
        {
            newKey.m_inTangent.y = 0.0f;
            return newKey;
        }

        switch (key.m_inTangentType)
        {
        case SCurveEditorKey::eTangentType_Linear:
            newKey.m_inTangent.y = (leftKey.m_value - key.m_value) / 3.0f;
            break;
        case SCurveEditorKey::eTangentType_Step:
            newKey.m_inTangent.x = 0.0f;
            newKey.m_inTangent.y = 0.0f;
            newKey.m_value = leftKey.m_value;
            break;
        default:
            const float oneThirdDeltaTime = (newKey.m_time - leftKey.m_time) / 3.0f;
            float ratio = oneThirdDeltaTime / -newKey.m_inTangent.x;
            newKey.m_inTangent *= ratio;
            break;
        }

        return newKey;
    }

    QPainterPath CreatePathFromCurve(const SCurveEditorCurve& curve,
        ECurveEditorCurveType curveType, AZStd::function<Vec2(Vec2)> transformFunc)
    {
        QPainterPath path;

        const Vec2 startPoint(curve.m_keys[0].m_time, curve.m_keys[0].m_value);
        const Vec2 startTransformed = transformFunc(startPoint);
        path.moveTo(startTransformed.x, startTransformed.y);

        const auto endIter = curve.m_keys.end() - 1;

        if (curve.m_customInterpolator && curve.m_keys.size() > 1)
        {
            const float range_start = curve.m_customInterpolator->GetKeyTime(0);
            const float range_end = curve.m_customInterpolator->GetKeyTime(curve.m_customInterpolator->GetKeyCount() - 1);
            const float range_delta = range_end - range_start;

            if (range_delta > 0)
            {
                const int drawResolution = (int)ceil_tpl(transformFunc(Vec2(range_end, 0.0f)).x -
                        transformFunc(Vec2(range_start, 0.0f)).x);

                const float increment = range_delta / drawResolution;
                std::vector<Vec2> drawList;
                drawList.reserve(512);
                float value;
                for (float time = range_start; time < range_end; time += increment)
                {
                    curve.m_customInterpolator->InterpolateFloat(time, value);
                    drawList.push_back(Vec2(time, value));
                }

                path.moveTo(Vec2ToPoint(transformFunc(drawList.front())));
                for (int i = 1; i < drawList.size(); i++)
                {
                    path.lineTo(Vec2ToPoint(transformFunc(drawList[i])));
                }
            }
        }
        else if (curveType == eCECT_Bezier)
        {
            for (auto iter = curve.m_keys.begin(); iter != endIter; ++iter)
            {
                const SCurveEditorKey* pKeyLeftOfSegment = (iter != curve.m_keys.begin()) ? &*(iter - 1) : nullptr;
                const SCurveEditorKey* pKeyRightOfSegment = (iter != (curve.m_keys.end() - 2)) ? &*(iter + 2) : nullptr;

                const SCurveEditorKey segmentStartKey = ApplyOutTangentFlags(*iter, pKeyLeftOfSegment, *(iter + 1));
                const SCurveEditorKey segmentEndKey = ApplyInTangentFlags(*(iter + 1), *iter, pKeyRightOfSegment);

                const Vec2 p0 = Vec2(segmentStartKey.m_time, segmentStartKey.m_value);
                const Vec2 p3 = Vec2(segmentEndKey.m_time, segmentEndKey.m_value);

                // Need to compute tangents for x so that the cubic 2D Bezier does a linear interpolation in
                // that dimension, because we actually want to draw a cubic 1D Bezier curve
                //const float outTangentX = (2.0f * p0.x + p3.x) / 3.0f;    // p1 = (2 * p0 + p3) / 3
                //const float inTangentX = (p0.x + 2.0f * p3.x) / 3.0f;     // p2 = (p0 + 2 * p3) / 3

                const Vec2 p1 = p0 + segmentStartKey.m_outTangent; // Vec2(inTangentX, p0.y + segmentStartKey.m_outTangent.y);
                const Vec2 p2 = p3 + segmentEndKey.m_inTangent; // Vec2(inTangentX, p3.y + segmentEndKey.m_inTangent.y);

                const QPointF p0Transformed = Vec2ToPoint(transformFunc(p0));
                const QPointF p1Transformed = Vec2ToPoint(transformFunc(p1));
                const QPointF p2Transformed = Vec2ToPoint(transformFunc(p2));
                const QPointF p3Transformed = Vec2ToPoint(transformFunc(p3));
                path.moveTo(p0Transformed);
                path.cubicTo(p1Transformed, p2Transformed, p3Transformed);
            }
        }
        else if (curveType == eCECT_2DBezier)
        {
            for (auto iter = curve.m_keys.begin(); iter != endIter; ++iter)
            {
                const SCurveEditorKey& segmentStartKey = *iter;
                const SCurveEditorKey& segmentEndKey = *(iter + 1);

                const Vec2 p0 = Vec2(segmentStartKey.m_time, segmentStartKey.m_value);
                const Vec2 p3 = Vec2(segmentEndKey.m_time, segmentEndKey.m_value);
                const Vec2 p1 = p0 + segmentStartKey.m_outTangent;
                const Vec2 p2 = p3 + segmentEndKey.m_inTangent;

                const QPointF p1Transformed = Vec2ToPoint(transformFunc(p1));
                const QPointF p2Transformed = Vec2ToPoint(transformFunc(p2));
                const QPointF p3Transformed = Vec2ToPoint(transformFunc(p3));
                path.cubicTo(p1Transformed, p2Transformed, p3Transformed);
            }
        }

        return path;
    }

    // Renders path outside of the current range of the curve
    QPainterPath CreateExtrapolatedPathFromCurve(const SCurveEditorCurve& curve, AZStd::function<Vec2(Vec2)> transformFunc, float windowWidth)
    {
        QPainterPath path;

        if (curve.m_keys.size() > 0)
        {
            const Vec2 startPoint = Vec2(curve.m_keys[0].m_time, curve.m_keys[0].m_value);
            const Vec2 startTransformed = transformFunc(startPoint);
            if (startTransformed.x > 0.0f)
            {
                path.moveTo(std::min(startTransformed.x, windowWidth), startTransformed.y);
                path.lineTo(0.0f, startTransformed.y);
            }

            const Vec2 endPoint(curve.m_keys.back().m_time, curve.m_keys.back().m_value);
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

    // Renders line between discontinuous path when step mode is used for a control point
    QPainterPath CreateDiscontinuinityPathFromCurve(const SCurveEditorCurve& curve, ECurveEditorCurveType curveType, AZStd::function<Vec2(Vec2)> transformFunc)
    {
        QPainterPath path;

        if (curve.m_keys.size() > 0)
        {
            const auto endIter = curve.m_keys.end() - 1;

            if (curveType == eCECT_Bezier && !curve.m_customInterpolator)
            {
                for (auto iter = curve.m_keys.begin(); iter != endIter; ++iter)
                {
                    const SCurveEditorKey* pKeyLeftOfSegment = (iter != curve.m_keys.begin()) ? &*(iter - 1) : nullptr;
                    const SCurveEditorKey* pKeyRightOfSegment = (iter != (curve.m_keys.end() - 2)) ? &*(iter + 2) : nullptr;

                    const SCurveEditorKey segmentStartKey = ApplyOutTangentFlags(*iter, pKeyLeftOfSegment, *(iter + 1));
                    const SCurveEditorKey segmentEndKey = ApplyInTangentFlags(*(iter + 1), *iter, pKeyRightOfSegment);

                    if (segmentStartKey.m_value != iter->m_value)
                    {
                        const Vec2 start = Vec2(segmentStartKey.m_time, segmentStartKey.m_value);
                        const Vec2 end = Vec2(iter->m_time, iter->m_value);

                        const QPointF startTransformed = Vec2ToPoint(transformFunc(start));
                        const QPointF endTransformed = Vec2ToPoint(transformFunc(end));

                        path.moveTo(startTransformed);
                        path.lineTo(endTransformed);
                    }

                    if (segmentEndKey.m_value != (iter + 1)->m_value)
                    {
                        const Vec2 start = Vec2(segmentEndKey.m_time, segmentEndKey.m_value);
                        const Vec2 end = Vec2((iter + 1)->m_time, (iter + 1)->m_value);

                        const QPointF startTransformed = Vec2ToPoint(transformFunc(start));
                        const QPointF endTransformed = Vec2ToPoint(transformFunc(end));

                        path.moveTo(startTransformed);
                        path.lineTo(endTransformed);
                    }
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

    Vec2 ClosestPointOnBezierSegment(const Vec2 point, const float t0, const float t1, const float p0, const float p1, const float p2, const float p3)
    {
        using namespace LegacyCryPhysicsUtils;

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

        // Find the closest point under all the candidates
        Vec2 closestPoint;
        float minDistanceSq = std::numeric_limits<float>::max();
        for (uint i = 0; i < numRoots + 2; ++i)
        {
            const Vec2 rootPoint(Lerp(t0, t1, checkPoints[i]), EvaluateBezier(checkPoints[i], p0, p1, p2, p3));
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

    Range GetBezierSegmentValueRange(const SCurveEditorKey& startKey, const SCurveEditorKey& endKey)
    {
        using namespace LegacyCryPhysicsUtils;

        const float p0 = startKey.m_value;
        const float p1 = p0 + startKey.m_outTangent.y;
        const float p3 = endKey.m_value;
        const float p2 = p3 + endKey.m_inTangent.y;

        Range valueRange(std::min(p0, p3), std::max(p0, p3));
        const P2f cubicBezierDerivativePoly = P2f(-3.0f * p0 + 9.0f * p1 - 6.0f * p2 + 3.0f * (p3 - p2)) + P1f(6.0f * p0 - 12.0f * p1 + 6.0f * p2) - 3.0f * p0 + 3.0f * p1;

        float roots[2];
        const uint numRoots = cubicBezierDerivativePoly.findroots(0.0f, 1.0f, roots);
        for (uint i = 0; i < numRoots; ++i)
        {
            const float rootValue = EvaluateBezier(roots[i], p0, p1, p2, p3);
            valueRange.start = std::min(valueRange.start, rootValue);
            valueRange.end = std::max(valueRange.end, rootValue);
        }
        return valueRange;
    }

    float DistanceTo2DBezierSegment([[maybe_unused]] const Vec2 point, [[maybe_unused]] const SCurveEditorKey& startKey, [[maybe_unused]] const SCurveEditorKey& endKey)
    {
        return std::numeric_limits<float>::max();
    }


    void SmoothTangents(const SCurveEditorKey& key, Vec2& inTangent, Vec2& outTangent, SCurveEditorKey* pLeftKey, SCurveEditorKey* pRightKey, bool applyInverseSegmentLengthFactor)
    {
        inTangent.Normalize();
        outTangent.Normalize();

        if (!pLeftKey && !pRightKey)
        {
            return;
        }
        else if (!pLeftKey)
        {
            inTangent = -outTangent;
        }
        else if (!pRightKey)
        {
            outTangent = -inTangent;
        }
        else
        {
            const float deltaTime = pRightKey->m_time - pLeftKey->m_time;
            const float ratio = (key.m_time - pLeftKey->m_time) / deltaTime;

            Vec2 smoothedTangent = Vec2::CreateLerp(-inTangent, outTangent, applyInverseSegmentLengthFactor ? ratio : 0.5f);
            inTangent = -smoothedTangent;
            outTangent = smoothedTangent;
        }

        if (pLeftKey)
        {
            float leftSegmentTime = key.m_time - pLeftKey->m_time;
            float inFactor = (leftSegmentTime / -inTangent.x) / 3.0f;
            inTangent *= inFactor;
        }
        if (pRightKey)
        {
            float rightSegmentTime = pRightKey->m_time - key.m_time;
            float outFactor = (rightSegmentTime / outTangent.x) / 3.0f;
            outTangent *= outFactor;
        }
    }

    Vec2 GetSmoothInTangent(SCurveEditorKey& key, Vec2 inTangent, Vec2 outTangent, SCurveEditorKey* pLeftKey, SCurveEditorKey* pRightKey, bool applyInverseSegmentLengthFactor)
    {
        SmoothTangents(key, inTangent, outTangent, pLeftKey, pRightKey, applyInverseSegmentLengthFactor);
        return inTangent;
    }

    Vec2 GetSmoothOutTangent(SCurveEditorKey& key, Vec2 inTangent, Vec2 outTangent, SCurveEditorKey* pLeftKey, SCurveEditorKey* pRightKey, bool applyInverseSegmentLengthFactor)
    {
        SmoothTangents(key, inTangent, outTangent, pLeftKey, pRightKey, applyInverseSegmentLengthFactor);
        return outTangent;
    }
}

void showTooltip(const SCurveEditorKey& key, const QPoint& pos, QWidget* parent, QString tipOverride = QString())
{
    if (!tipOverride.isEmpty())
    {
        return QToolTip::showText(pos, tipOverride, parent);
    }

    QString tip = QString().asprintf("%s <- [%5.2f, %5.2f] -> %s",
            CCurveEditor::TangentTypeToString(key.m_inTangentType).toUtf8().data(),
            key.m_time, key.m_time,
            CCurveEditor::TangentTypeToString(key.m_outTangentType).toUtf8().data());

    QToolTip::showText(pos, tip, parent);
}

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

    void mouseReleaseEvent([[maybe_unused]] QMouseEvent* pEvent) override
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
        if (m_pCurveEditor->m_optOutFlags & EOptOutZoomingAndPanning)
        {
            return;
        }
        m_startPoint = QPoint(int(pEvent->x()), int(pEvent->y()));
        m_startTranslation = m_pCurveEditor->m_translation;
    }

    void mouseMoveEvent(QMouseEvent* pEvent) override
    {
        if (m_pCurveEditor->m_optOutFlags & EOptOutZoomingAndPanning)
        {
            return;
        }
        const Vec2 windowSize((float)m_pCurveEditor->size().width(), (float)m_pCurveEditor->size().height());

        const int pixelDeltaX = pEvent->x() - m_startPoint.x();
        const int pixelDeltaY = pEvent->y() - m_startPoint.y();

        float deltaX = float(pixelDeltaX) / (windowSize.x);
        float deltaY = float(pixelDeltaY) / (windowSize.y);

        if (m_pCurveEditor->IsTimeRangeEnforced())
        {
            deltaX = 0;
        }

        const Vec2 delta(deltaX, deltaY);
        m_pCurveEditor->m_translation = m_startTranslation + delta;
        m_pCurveEditor->update();
    }
};

struct CCurveEditor::SZoomHandler
    : public CCurveEditor::SMouseHandler
{
    CCurveEditor* m_pCurveEditor;
    QPoint m_lastPoint;

    SZoomHandler(CCurveEditor* pCurveEditor)
        : m_pCurveEditor(pCurveEditor)
    {
    }

    void mousePressEvent(QMouseEvent* pEvent) override
    {
        if (m_pCurveEditor->m_optOutFlags & EOptOutZoomingAndPanning)
        {
            return;
        }
        m_lastPoint = QPoint(int(pEvent->x()), int(pEvent->y()));
    }

    void mouseMoveEvent(QMouseEvent* pEvent) override
    {
        if (m_pCurveEditor->m_optOutFlags & EOptOutZoomingAndPanning)
        {
            return;
        }
        const Vec2 windowSize((float)m_pCurveEditor->size().width(), (float)m_pCurveEditor->size().height());

        const int pixelDeltaX = pEvent->x() - m_lastPoint.x();
        const int pixelDeltaY = pEvent->y() - m_lastPoint.y();

        m_lastPoint = QPoint(int(pEvent->x()), int(pEvent->y()));

        m_pCurveEditor->m_zoom.x *= pow(1.2f, (float)pixelDeltaX * 0.03f);
        m_pCurveEditor->m_zoom.y *= pow(1.2f, (float)pixelDeltaY * 0.03f);

        m_pCurveEditor->m_zoom.x = clamp_tpl(m_pCurveEditor->m_zoom.x, kMinZoom, kMaxZoom);
        m_pCurveEditor->m_zoom.y = clamp_tpl(m_pCurveEditor->m_zoom.y, kMinZoom, kMaxZoom);

        m_pCurveEditor->update();
    }
};

struct CCurveEditor::SScrubHandler
    : SMouseHandler
{
    CCurveEditor* m_pCurveEditor;
    float m_startThumbPosition;
    QPoint m_startPoint;

    SScrubHandler(CCurveEditor* pCurveEditor)
        : m_pCurveEditor(pCurveEditor)
    {
    }

    void mousePressEvent(QMouseEvent* ev) override
    {
        QPoint point = QPoint(ev->pos().x(), ev->pos().y());

        const Vec2 pointInCurveSpace = TransformPointFromScreen(m_pCurveEditor->m_zoom, m_pCurveEditor->m_translation, m_pCurveEditor->GetCurveArea(), PointToVec2(point));

        m_pCurveEditor->m_time = pointInCurveSpace.x;
        m_startThumbPosition = m_pCurveEditor->m_time;
        m_startPoint = point;

        m_pCurveEditor->SignalScrub();
    }

    void Apply(QMouseEvent* ev, [[maybe_unused]] bool continuous)
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

        m_pCurveEditor->m_time = m_startThumbPosition + delta;
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

struct CCurveEditor::SMoveKeyHandler
    : public CCurveEditor::SMouseHandler
{
    CCurveEditor* m_pCurveEditor;
    bool m_bCycleSelection;
    Vec2 m_startPoint;
    std::vector<Vec2> m_keyPositions;
    bool m_clamp;
    QRectF m_range;

    SMoveKeyHandler(CCurveEditor* pCurveEditor, bool bCycleSelection, QRectF* clampRange = nullptr)
        : m_pCurveEditor(pCurveEditor)
        , m_bCycleSelection(bCycleSelection)
        , m_startPoint(0.0f, 0.0f)
        , m_clamp(clampRange ? true : false)
        , m_range(clampRange ? *clampRange : QRectF())
    {}

    void mousePressEvent(QMouseEvent* pEvent) override
    {
        const QPoint currentPos = pEvent->pos();
        m_startPoint = TransformPointFromScreen(m_pCurveEditor->m_zoom,
                m_pCurveEditor->m_translation, m_pCurveEditor->GetCurveArea(), PointToVec2(currentPos));
        StoreKeyPositions();
        m_pCurveEditor->SignalKeyMoveStarted();
    }

    void mouseMoveEvent(QMouseEvent* pEvent) override
    {
        RestoreKeyPositions();
        const QPoint currentPos = pEvent->pos();
        Vec2 transformedPos = TransformPointFromScreen(m_pCurveEditor->m_zoom,
                m_pCurveEditor->m_translation, m_pCurveEditor->GetCurveArea(), PointToVec2(currentPos));

        const Vec2 offset = transformedPos - m_startPoint;

        SCurveEditorContent* pContent = m_pCurveEditor->m_pContent;
        for (auto curveIter = pContent->m_curves.begin(); curveIter != pContent->m_curves.end(); ++curveIter)
        {
            SCurveEditorCurve& curve = *curveIter;

            for (auto iter = curve.m_keys.begin(); iter != curve.m_keys.end(); ++iter)
            {
                if (iter->m_bSelected)
                {
                    iter->m_time += offset.x;
                    iter->m_value += offset.y;
                    if (m_clamp)
                    {
                        iter->m_time = clamp_tpl(iter->m_time, (float)m_range.left(), (float)m_range.right());
                        iter->m_value = clamp_tpl(iter->m_value, (float)m_range.bottom(), (float)m_range.top());
                    }
                    iter->m_bModified = true;
                }
            }

            m_pCurveEditor->SortKeys(curve);
        }

        m_pCurveEditor->SignalKeyMoved();
    }

    void focusOutEvent([[maybe_unused]] QFocusEvent* pEvent) override
    {
        RestoreKeyPositions();
    }

    void mouseReleaseEvent([[maybe_unused]] QMouseEvent* pEvent) override
    {
        m_pCurveEditor->ContentChanged();
    }

    void StoreKeyPositions()
    {
        SCurveEditorContent* pContent = m_pCurveEditor->m_pContent;
        for (auto curveIter = pContent->m_curves.begin(); curveIter != pContent->m_curves.end(); ++curveIter)
        {
            SCurveEditorCurve& curve = *curveIter;
            for (auto iter = curve.m_keys.begin(); iter != curve.m_keys.end(); ++iter)
            {
                if (iter->m_bSelected)
                {
                    m_keyPositions.push_back(Vec2(iter->m_time, iter->m_value));
                }
            }
        }
    }

    void RestoreKeyPositions()
    {
        SCurveEditorContent* pContent = m_pCurveEditor->m_pContent;
        auto posIter = m_keyPositions.begin();
        for (auto curveIter = pContent->m_curves.begin(); curveIter != pContent->m_curves.end(); ++curveIter)
        {
            SCurveEditorCurve& curve = *curveIter;
            for (auto iter = curve.m_keys.begin(); iter != curve.m_keys.end(); ++iter)
            {
                if (iter->m_bSelected)
                {
                    iter->m_time = posIter->x;
                    iter->m_value = (posIter++)->y;
                }
            }
        }
    }
};

struct CCurveEditor::SRotateTangentHandler
    : public CCurveEditor::SMouseHandler
{
    CCurveEditor* m_pCurveEditor;
    CCurveEditorTangentControl* m_pSelectedTangent;
    Vec2 m_StartPoint;
    Vec2 m_InitialInTangent;
    SCurveEditorKey::ETangentType m_InitialInTangentType;
    Vec2 m_InitialOutTangent;
    SCurveEditorKey::ETangentType m_InitialOutTangentType;

    SRotateTangentHandler(CCurveEditor* pCurveEditor, CCurveEditorTangentControl* pTangentControl)
        : m_pCurveEditor(pCurveEditor)
        , m_pSelectedTangent(pTangentControl)
        , m_StartPoint(0.0f, 0.0f)
    {}

    void mousePressEvent(QMouseEvent* pEvent) override
    {
        const QPoint currentPos = pEvent->pos();
        m_StartPoint = m_pCurveEditor->TransformFromScreenCoordinates(PointToVec2(currentPos));

        StoreTangents();
    }

    void mouseMoveEvent(QMouseEvent* pEvent) override
    {
        const QPoint currentPos = pEvent->pos();
        const Vec2 transformedPos = m_pCurveEditor->TransformFromScreenCoordinates(PointToVec2(currentPos));
        const Vec2 offset = transformedPos - m_StartPoint;

        SCurveEditorKey& key = m_pSelectedTangent->GetControl().GetKey();
        Vec2 keyPos(key.m_time, key.m_value);

        bool isInTangent = m_pSelectedTangent->GetTangentDirection() == ETangent_In;
        bool shouldTangentsBePaired = key.m_inTangentType == key.m_outTangentType
            && (key.m_inTangentType == SCurveEditorKey::eTangentType_Standard
                || key.m_inTangentType == SCurveEditorKey::eTangentType_Smooth
                || key.m_inTangentType == SCurveEditorKey::eTangentType_Flat);

        float tangentEpsilon = 1e-6f;

        // strictly left or right of key - have a fairly large epsilon to avoid weird floating point innaccuracies in editor
        bool leftOfKey = (transformedPos.x - keyPos.x) < -tangentEpsilon;
        bool rightOfKey = (transformedPos.x - keyPos.x) > tangentEpsilon;
        bool tangentVertical = !(leftOfKey || rightOfKey);

        if ((isInTangent && leftOfKey) || (shouldTangentsBePaired && !tangentVertical))
        {
            Vec2 newInTangent = transformedPos - keyPos;
            if (rightOfKey)
            {
                // mirror tangent
                newInTangent *= -1;
            }
            float scale = key.m_inTangent.x / newInTangent.x;
            newInTangent *= scale;
            key.m_inTangent = newInTangent;

            key.m_inTangentType = shouldTangentsBePaired ? SCurveEditorKey::eTangentType_Standard : key.m_inTangentType;
        }

        if ((!isInTangent && rightOfKey) || (shouldTangentsBePaired && !tangentVertical))
        {
            Vec2 newOutTangent = transformedPos - keyPos;
            if (leftOfKey)
            {
                // mirror tangent
                newOutTangent *= -1;
            }
            float scale = key.m_outTangent.x / newOutTangent.x;
            newOutTangent *= scale;
            key.m_outTangent = newOutTangent;

            key.m_outTangentType = shouldTangentsBePaired ? SCurveEditorKey::eTangentType_Standard : key.m_outTangentType;
        }
    }

    void focusOutEvent([[maybe_unused]] QFocusEvent* pEvent) override
    {
        RestoreTangents();
    }

    void mouseReleaseEvent([[maybe_unused]] QMouseEvent* pEvent) override
    {
        m_pCurveEditor->ContentChanged();
    }

    void StoreTangents()
    {
        SCurveEditorKey& key = m_pSelectedTangent->GetControl().GetKey();

        m_InitialInTangent = key.m_inTangent;
        m_InitialInTangentType = key.m_inTangentType;

        m_InitialOutTangent = key.m_outTangent;
        m_InitialOutTangentType = key.m_outTangentType;
    }

    void RestoreTangents()
    {
        SCurveEditorKey& key = m_pSelectedTangent->GetControl().GetKey();

        key.m_inTangent = m_InitialInTangent;
        key.m_inTangentType = m_InitialInTangentType;

        key.m_outTangent = m_InitialOutTangent;
        key.m_outTangentType = m_InitialOutTangentType;
    }
};

CCurveEditor::CCurveEditor(QWidget* parent)
    : QWidget(parent)
    , m_pContent(nullptr)
    , m_pMouseHandler(nullptr)
    , m_curveType(eCECT_Bezier)
    , m_bWeighted(false)
    , m_bHandlesVisible(true)
    , m_bRulerVisible(true)
    , m_bTimeSliderVisible(true)
    , m_time(0.0f)
    , m_zoom(0.5f, 0.5f)
    , m_translation(0.5f, 0.5f)
    , m_timeRange(0, 1)
    , m_timeRangeEnforced(false)
    , m_valueRange(0, 1)
    , m_optOutFlags(0)
{
    setMouseTracking(true);
    //EnforceTimeRange(0.0f, 1.0f);
    SetTimeRange(0, 1);
    SetValueRange(0, 1);
    ZoomToTimeRange(-0.1f, 1.1f);
    ZoomToValueRange(-0.1f, 1.1f);
    SetRulerVisible(true);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setSizePolicy(sizePolicy);
}

CCurveEditor::~CCurveEditor()
{
}

void CCurveEditor::SetContent(SCurveEditorContent* pContent)
{
    m_pContent = pContent;

    ContentChanged();

    update();
}

void CCurveEditor::SetTime(const float time)
{
    m_time = time;
    update();
}

void CCurveEditor::SetTimeRange(const float start, const float end)
{
    SetTimeRange(start, end, false);
}

void CCurveEditor::EnforceTimeRange(const float start, const float end)
{
    SetTimeRange(start, end, true);
    ZoomToTimeRange(start, end);
}

bool CCurveEditor::IsTimeRangeEnforced() const
{
    return m_timeRangeEnforced;
}

void CCurveEditor::SetTimeRange(const float start, const float end, bool enforce)
{
    if (start <= end)
    {
        m_timeRangeEnforced = enforce;
        m_timeRange = Range(start, end);
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
    if (start < end)
    {
        m_zoom.x = 1.0f / (end - start);
        m_translation.x = start / (start - end);
    }
}

void CCurveEditor::ZoomToValueRange(const float min, const float max)
{
    if (min < max)
    {
        m_zoom.y = 1.0f / (max - min);
        m_translation.y = max / (max - min);
    }
}

void CCurveEditor::paintEvent([[maybe_unused]] QPaintEvent* pEvent)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(0.5f, 0.5f);

    const QPalette& palette = this->palette();

    auto transformFunc = [&](Vec2 point)
        {
            return TransformPointToScreen(m_zoom, m_translation, GetCurveArea(), point);
        };

    auto invTransformFunc = [&](Vec2 screenPoint)
        {
            return TransformPointFromScreen(m_zoom, m_translation, GetCurveArea(), screenPoint);
        };

    const QColor rangeHighlightColor = CurveEditorHelpers::LerpColor(palette.color(QPalette::WindowText), palette.color(QPalette::Window), 0.95f);
    const QRectF rangesRect(Vec2ToPoint(transformFunc(Vec2(m_timeRange.start, m_valueRange.start))), Vec2ToPoint(transformFunc(Vec2(m_timeRange.end, m_valueRange.end))));
    painter.setPen(QPen(Qt::NoPen));
    if ((m_optOutFlags & EOptOutBackground))
    {
        painter.setBrush(Qt::transparent);
    }
    else
    {
        painter.setBrush(rangeHighlightColor);
    }

    if ((m_optOutFlags & EOptOutRuler))
    {
        painter.drawRect(rangesRect);
    }
    else
    {
        painter.drawRect(rect());
    }

    if (m_pContent)
    {
        const QPen extrapolatedCurvePen = QPen(palette.color(QPalette::Highlight));

        TCurveEditorCurves& curves = m_pContent->m_curves;
        for (auto curveIter = curves.begin(); curveIter != curves.end(); ++curveIter)
        {
            SCurveEditorCurve& curve = *curveIter;
            QColor penColor = QColor(curve.m_color.r, curve.m_color.g, curve.m_color.b, curve.m_color.a);

            if (!(m_optOutFlags & EOptOutCustomPenColor) && m_penColor.isValid())
            {
                penColor = m_penColor;
            }

            painter.setBrush(QBrush(Qt::NoBrush));
            const QPen curvePen = QPen(penColor, 2);
            const QPen narrowCurvePen = QPen(penColor);
            if (!(m_optOutFlags & eOptOutDashedPath))
            {
                const QPainterPath extrapolatedPath = CreateExtrapolatedPathFromCurve(curve, transformFunc, aznumeric_cast<float>(width()));
                painter.setPen(narrowCurvePen);
                painter.drawPath(extrapolatedPath);
            }

            const QPainterPath discontinuinityPath = CreateDiscontinuinityPathFromCurve(curve, m_curveType, transformFunc);
            painter.setPen(narrowCurvePen);
            painter.drawPath(discontinuinityPath);

            if (curve.m_keys.size() > 0)
            {
                UpdateTangents();
                const QPainterPath path = CreatePathFromCurve(curve, m_curveType, transformFunc);
                painter.setPen(curvePen);
                painter.drawPath(path);
            }
        }
    }

    if (!(m_optOutFlags & EOptOutSelectionKey))
    {
        if ((m_optOutFlags & EOptOutKeyIcon))
        {
            for (CCurveEditorControl* pControlKey : m_pControlKeys)
            {
                pControlKey->Paint(painter, palette, !(m_optOutFlags & EOptOutSelectionInOutTangent));
            }
        }
        else
        {
            for (CCurveEditorControl* pControlKey : m_pControlKeys)
            {
                pControlKey->PaintIcon(painter, palette, !(m_optOutFlags & EOptOutSelectionInOutTangent));
            }
        }
    }

    if (m_pMouseHandler)
    {
        m_pMouseHandler->paintOver(painter);
    }
    if (!(m_optOutFlags & EOptOutRuler))
    {
        DrawingPrimitives::SRulerOptions rulerOptions;
        rulerOptions.m_rect = QRect(0, -1, size().width(), kRulerHeight + 2);
        rulerOptions.m_visibleRange = Range(-m_translation.x / m_zoom.x, (1.0f - m_translation.x) / m_zoom.x);
        rulerOptions.m_rulerRange = rulerOptions.m_visibleRange;
        rulerOptions.m_markHeight = kRulerMarkHeight;
        rulerOptions.m_shadowSize = kRulerShadowHeight;
        rulerOptions.m_textXOffset = kTextXOffset;
        rulerOptions.m_textYOffset = kTextYOffset;

        int rulerPrecision;
        DrawingPrimitives::DrawRuler(painter, palette, rulerOptions, &rulerPrecision);

        if (m_pContent && isEnabled() && !(m_optOutFlags & EOptOutTimeSlider))
        {
            DrawingPrimitives::STimeSliderOptions timeSliderOptions;
            timeSliderOptions.m_rect = rect();
            timeSliderOptions.m_precision = rulerPrecision;
            timeSliderOptions.m_position = aznumeric_cast<int>(transformFunc(Vec2(m_time, 0.0f)).x);
            timeSliderOptions.m_time = m_time;
            timeSliderOptions.m_bHasFocus = hasFocus();
            DrawingPrimitives::DrawTimeSlider(painter, palette, timeSliderOptions);
        }
    }
}

void CCurveEditor::mousePressEvent(QMouseEvent* pEvent)
{
    if (m_optOutFlags & EOptOutControls)
    {
        return QWidget::mousePressEvent(pEvent);
    }
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
    if (m_optOutFlags & EOptOutControls)
    {
        return QWidget::mouseDoubleClickEvent(pEvent);
    }
    if (pEvent->button() == Qt::LeftButton)
    {
        auto curveHitPair = HitDetectCurve(pEvent->pos());
        if (curveHitPair.first)
        {
            if (AddPointToCurve(curveHitPair.second, curveHitPair.first))
            {
                setCursor(QCursor(Qt::SizeAllCursor));
            }
        }
    }
}

void CCurveEditor::LeftButtonMousePressEvent(QMouseEvent* pEvent)
{
    const bool bCtrlPressed = (pEvent->modifiers() & Qt::CTRL) != 0;
    const bool bAltPressed = (pEvent->modifiers() & Qt::ALT) != 0;

    if (pEvent->y() < kRulerHeight && !(m_optOutFlags & EOptOutRuler))
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
                if (AddPointToCurve(curveHitPair.second, curveHitPair.first))
                {
                    setCursor(QCursor(Qt::SizeAllCursor));
                }
            }
        }
        else if (bAltPressed)
        {
            auto pCurveKey = HitDetectKey(pEvent->pos());
            if (pCurveKey)
            {
                pCurveKey->MarkKeyForRemoval();
                ContentChanged();
            }
        }
        else
        {
            auto pTangentKey = HitDetectTangent(pEvent->pos());
            if (pTangentKey)
            {
                SelectTangent(pTangentKey);

                m_pMouseHandler.reset(new SRotateTangentHandler(this, pTangentKey));
            }
            else
            {
                auto pCurveKey = HitDetectKey(pEvent->pos());
                if (pCurveKey)
                {
                    SelectKey(pCurveKey, false);

                    QRectF range;
                    range.setLeft(m_timeRange.start);
                    range.setRight(m_timeRange.end);
                    range.setBottom(m_valueRange.start);
                    range.setTop(m_valueRange.end);

                    m_pMouseHandler.reset(new SMoveKeyHandler(this, false,
                            m_timeRangeEnforced ? &range : nullptr));
                }
                else
                {
                    m_pMouseHandler.reset(new SSelectionHandler(this, false));
                }
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
    CCurveEditorControl* pCurveKey = HitDetectKey(pEvent->pos());
    if (pCurveKey)
    {
        SelectKey(pCurveKey, false);
        update(); //Repaint so that we see that the key is selected

        QMenu* pMenu = new QMenu(this);
        PopulateControlContextMenu(pMenu);
        pMenu->popup(pEvent->globalPos());
    }
    else
    {
        return QWidget::mousePressEvent(pEvent);
    }
}

void CCurveEditor::mouseMoveEvent(QMouseEvent* pEvent)
{
    if (m_optOutFlags & EOptOutControls)
    {
        return QWidget::mouseMoveEvent(pEvent);
    }
    const CCurveEditorControl* control = HitDetectKey(pEvent->pos());
    if (control)
    {
        if (!(m_optOutFlags & EOptOutDefaultTooltip))
        {
            showTooltip(control->GetKey(), pEvent->globalPos(), this, control->GetToolTip());
        }
        setCursor(QCursor(Qt::SizeAllCursor));
    }
    else
    {
        if (!(m_optOutFlags & EOptOutDefaultTooltip))
        {
            QToolTip::hideText();
        }
        setCursor(QCursor());
    }

    if (m_pMouseHandler)
    {
        m_pMouseHandler->mouseMoveEvent(pEvent);
    }

    update();
}

void CCurveEditor::mouseReleaseEvent(QMouseEvent* pEvent)
{
    if (m_optOutFlags & EOptOutControls)
    {
        return QWidget::mouseReleaseEvent(pEvent);
    }
    if (m_pMouseHandler)
    {
        m_pMouseHandler->mouseReleaseEvent(pEvent);
        m_pMouseHandler.reset();
        update();
    }
}

void CCurveEditor::focusOutEvent(QFocusEvent* pEvent)
{
    if (m_optOutFlags & EOptOutControls)
    {
        return focusOutEvent(pEvent);
    }
    if (m_pMouseHandler)
    {
        m_pMouseHandler->focusOutEvent(pEvent);
        m_pMouseHandler.reset();
        update();
    }
}

void CCurveEditor::wheelEvent(QWheelEvent* pEvent)
{
    if (m_optOutFlags & EOptOutControls || m_optOutFlags & EOptOutZoomingAndPanning)
    {
        return QWidget::wheelEvent(pEvent);
    }
    Vec2 windowSize((float)size().width(), (float)size().height());
    windowSize.y = (windowSize.y > 0.0f) ? windowSize.y : 1.0f;

    const QRect curveArea = GetCurveArea();
    const float mouseXNormalized = (float)(pEvent->position().x() - curveArea.left()) / (float)curveArea.width();
    const float mouseYNormalized = (float)(pEvent->position().y() - curveArea.top()) / (float)curveArea.height();

    const float pivotX = (mouseXNormalized - m_translation.x) / m_zoom.x;
    const float pivotY = (mouseYNormalized - m_translation.y) / m_zoom.y;

    float zoomFactor = pow(1.2f, (float)pEvent->angleDelta().y() * 0.01f);

    if (!m_timeRangeEnforced)
    {
        m_zoom.x *= zoomFactor;
    }
    m_zoom.y *= zoomFactor;

    m_zoom.x = clamp_tpl(m_zoom.x, kMinZoom, kMaxZoom);
    m_zoom.y = clamp_tpl(m_zoom.y, kMinZoom, kMaxZoom);

    // Adjust translation so pivot point stays at same x and y position on screen
    m_translation.x += ((mouseXNormalized - m_translation.x) / m_zoom.x - pivotX) * m_zoom.x;
    m_translation.y += ((mouseYNormalized - m_translation.y) / m_zoom.y - pivotY) * m_zoom.y;

    update();
}

void CCurveEditor::keyPressEvent(QKeyEvent* pEvent)
{
    if (m_optOutFlags & EOptOutControls)
    {
        return QWidget::keyPressEvent(pEvent);
    }
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

CCurveEditorControl* CCurveEditor::GetSelectedCurveKey()
{
    for (CCurveEditorControl* pControlKey : m_pControlKeys)
    {
        if (pControlKey->IsSelected())
        {
            return pControlKey;
        }
    }
    return nullptr;
}


CCurveEditorControl* CCurveEditor::HitDetectKey(const QPoint point)
{
    for (CCurveEditorControl* pControlKey : m_pControlKeys)
    {
        if (pControlKey->IsMouseWithinControl(point))
        {
            return pControlKey;
        }
    }

    return NULL;
}

CCurveEditorTangentControl* CCurveEditor::HitDetectTangent(const QPoint point)
{
    if (m_optOutFlags & EOptOutSelectionInOutTangent)
    {
        return NULL;
    }

    for (CCurveEditorControl* pControlKey : m_pControlKeys)
    {
        if (pControlKey->GetInTangent().IsMouseWithinControl(point))
        {
            return &pControlKey->GetInTangent();
        }
        else if (pControlKey->GetOutTangent().IsMouseWithinControl(point))
        {
            return &pControlKey->GetOutTangent();
        }
    }

    return NULL;
}

void CCurveEditor::SelectKey(CCurveEditorControl* pControlToSelect, bool addToExistingSelection)
{
    bool wasSelected = pControlToSelect->IsSelected();

    if (!wasSelected)
    {
        if (!addToExistingSelection)
        {
            for (CCurveEditorControl* pControlKey : m_pControlKeys)
            {
                pControlKey->SetSelected(false);
            }
        }
        pControlToSelect->SetSelected(true);
        //update key selection style
        updateCurveKeyShapeColor();
        emit SignalKeySelected(pControlToSelect);
    }
}

void CCurveEditor::SelectTangent(CCurveEditorTangentControl* pTangentToSelect)
{
    bool wasSelected = pTangentToSelect->IsSelected();
    if (!wasSelected)
    {
        for (CCurveEditorControl* pControlKey : m_pControlKeys)
        {
            pControlKey->SetSelected(false);
            pControlKey->GetInTangent().SetSelected(false);
            pControlKey->GetOutTangent().SetSelected(false);
        }
        pTangentToSelect->GetControl().SetSelected(true);
        pTangentToSelect->SetSelected(true);
    }
}

void CCurveEditor::SelectInRect(const QRect& rect)
{
    if (!m_pContent || (m_optOutFlags & EOptOutSelectionKey))
    {
        return;
    }

    ForEachKey(*m_pContent, [&]([[maybe_unused]] SCurveEditorCurve& curve, SCurveEditorKey& key)
        {
            const Vec2 screenPoint = TransformPointToScreen(m_zoom, m_translation, GetCurveArea(), Vec2(key.m_time, key.m_value));
            key.m_bSelected = rect.contains((int)screenPoint.x, (int)screenPoint.y);
        });

    update();
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

    const Vec2 startKeyTransformed = transformFunc(Vec2(curve.m_keys.front().m_time, curve.m_keys.front().m_value));
    if (point.x < startKeyTransformed.x)
    {
        const float distanceToCurve = std::abs(point.y - startKeyTransformed.y);
        if (distanceToCurve < minDistance)
        {
            closestPoint = Vec2(point.x, startKeyTransformed.y);
            minDistance = distanceToCurve;
        }
    }

    const Vec2 endKeyTransformed = transformFunc(Vec2(curve.m_keys.back().m_time, curve.m_keys.back().m_value));
    if (point.x > endKeyTransformed.x)
    {
        const float distanceToCurve = std::abs(point.y - endKeyTransformed.y);
        if (distanceToCurve < minDistance)
        {
            closestPoint = Vec2(point.x, endKeyTransformed.y);
            minDistance = distanceToCurve;
        }
    }

    int numCustomKeys = curve.m_customInterpolator ? curve.m_customInterpolator->GetKeyCount() : 0;
    if (numCustomKeys > 1
        && transformFunc(Vec2(curve.m_customInterpolator->GetKeyTime(0), 0)).x <= point.x
        && transformFunc(Vec2(curve.m_customInterpolator->GetKeyTime(numCustomKeys - 1), 0)).x >= point.x)
    {
        const int sampleCount = 5;
        for (int sample = 0; sample < sampleCount; sample++)
        {
            float value;
            float offset = (float)sample - floorf((float)sampleCount / 2.0f);
            float t = TransformPointFromScreen(m_zoom, m_translation, GetCurveArea(), Vec2(point.x + offset, point.y)).x;
            curve.m_customInterpolator->InterpolateFloat(t, value);
            value = transformFunc(Vec2(0, value)).y;
            const Vec2 closestOnSegment = Vec2(point.x, value);
            const float distanceToSegment = (closestOnSegment - point).GetLength();
            if (distanceToSegment < minDistance)
            {
                closestPoint = closestOnSegment;
                minDistance = distanceToSegment;
            }
        }
    }
    else
    {
        const auto endIter = curve.m_keys.end() - 1;
        for (auto iter = curve.m_keys.begin(); iter != endIter; ++iter)
        {
            if (curveType == eCECT_Bezier)
            {
                const SCurveEditorKey* pKeyLeftOfSegment = (iter != curve.m_keys.begin()) ? &*(iter - 1) : nullptr;
                const SCurveEditorKey* pKeyRightOfSegment = (iter != (curve.m_keys.end() - 2)) ? &*(iter + 2) : nullptr;

                const SCurveEditorKey segmentStartKey = ApplyOutTangentFlags(*iter, pKeyLeftOfSegment, *(iter + 1));
                const SCurveEditorKey segmentEndKey = ApplyInTangentFlags(*(iter + 1), *iter, pKeyRightOfSegment);

                const Vec2 p0 = transformFunc(Vec2(segmentStartKey.m_time, segmentStartKey.m_value));
                const Vec2 p3 = transformFunc(Vec2(segmentEndKey.m_time, segmentEndKey.m_value));
                const Vec2 p1 = transformFunc(Vec2(0.0f, segmentStartKey.m_value + segmentStartKey.m_outTangent.y));
                const Vec2 p2 = transformFunc(Vec2(0.0f, segmentEndKey.m_value + segmentEndKey.m_inTangent.y));

                const Vec2 closestOnSegment = ClosestPointOnBezierSegment(point, p0.x, p3.x, p0.y, p1.y, p2.y, p3.y);
                const float distanceToSegment = (closestOnSegment - point).GetLength();
                if (distanceToSegment < minDistance)
                {
                    closestPoint = closestOnSegment;
                    minDistance = distanceToSegment;
                }
            }
        }
    }

    return closestPoint;
}

void CCurveEditor::ContentChanged()
{
    DeleteMarkedKeys();

    while (!m_pControlKeys.isEmpty())
    {
        delete m_pControlKeys.takeFirst();
    }

    for (auto iter = m_pContent->m_curves.begin(); iter != m_pContent->m_curves.end(); ++iter)
    {
        SCurveEditorCurve& curve = *iter;
        for (size_t i = 0; i < curve.m_keys.size(); ++i)
        {
            SCurveEditorKey& key = curve.m_keys[i];
            key.m_bModified = false;

            CCurveEditorControl* pControlKey = new CCurveEditorControl(*this, curve, key);
            pControlKey->SetSelected(key.m_bSelected);

            m_pControlKeys.append(pControlKey);
        }
    }

    UpdateTangents();

    update();

    SignalContentChanged();
}

void CCurveEditor::DeleteMarkedKeys()
{
    if (m_pContent)
    {
        bool changed = false;
        // just delete the underlying key from the data model - the UI controls will be automatically updated to match the new model
        for (auto iter = m_pContent->m_curves.begin(); iter != m_pContent->m_curves.end(); ++iter)
        {
            SCurveEditorCurve& curve = *iter;
            for (auto keyIter = curve.m_keys.begin(); keyIter != curve.m_keys.end(); )
            {
                if (keyIter->m_bDeleted)
                {
                    keyIter = curve.m_keys.erase(keyIter);
                    changed = true;
                }
                else
                {
                    ++keyIter;
                }
            }
        }
    }
}

bool CCurveEditor::AddPointToCurve(const Vec2 point, SCurveEditorCurve* pCurve)
{
    assert(pCurve);

    // Makes sure that a new point can be only added at a safe distance
    if (pCurve->m_customInterpolator)
    {
        const float w = aznumeric_cast<float>(kPointRectExtent.x() * 2);
        const float h = aznumeric_cast<float>(kPointRectExtent.y() * 2);
        const float minDist = w * w + h * h;
        auto keys = pCurve->m_keys;
        for (int i = 0; i < keys.size(); i++)
        {
            const SCurveEditorKey& k = keys[i];
            const Vec2 scrP0 = TransformPointToScreen(m_zoom, m_translation, GetCurveArea(), Vec2(k.m_time, k.m_value));
            const Vec2 scrP1 = TransformPointToScreen(m_zoom, m_translation, GetCurveArea(), point);
            const float sqrDist = (scrP1 - scrP0).GetLength2();
            if (sqrDist <= minDist)
            {
                return false;
            }
        }
    }

    SCurveEditorKey key;
    key.m_bAdded = true;
    key.m_time = point.x;
    float yValue;
    pCurve->m_customInterpolator->InterpolateFloat(point.x, yValue);
    key.m_value = pCurve->m_customInterpolator ? yValue : point.y;

    // Set in/out tangents based on neighboring keys
    const SCurveEditorKey* closestFromLeft = nullptr;
    float closestTimeFromLeft = -std::numeric_limits<float>::max();
    const SCurveEditorKey* closestFromRight = nullptr;
    float closestTimeFromRight = std::numeric_limits<float>::max();
    for (const SCurveEditorKey& k : pCurve->m_keys)
    {
        // Check left
        if (k.m_time > closestTimeFromLeft && k.m_time < key.m_time)
        {
            closestTimeFromLeft = k.m_time;
            closestFromLeft = &k;
        }

        // Check right
        if (k.m_time < closestTimeFromRight && k.m_time > key.m_time)
        {
            closestTimeFromRight = k.m_time;
            closestFromRight = &k;
        }
    }

    if (closestFromLeft)
    {
        key.m_inTangentType = SCurveEditorKey::eTangentType_Bezier;
        float value;
        pCurve->m_customInterpolator->EvalInTangentFloat(key.m_time, value);
        key.m_inTangent = Vec2(1.0f, value);

    }

    if (closestFromRight)
    {
        key.m_outTangentType = SCurveEditorKey::eTangentType_Bezier;
        float value;
        pCurve->m_customInterpolator->EvalOutTangentFloat(key.m_time, value);
        key.m_outTangent = Vec2(1.0f, value);
    }

    pCurve->m_keys.push_back(key);

    SortKeys(*pCurve);

    ContentChanged();
    return true;
}

void CCurveEditor::SortKeys(SCurveEditorCurve& curve)
{
    std::stable_sort(curve.m_keys.begin(), curve.m_keys.end(), [](const SCurveEditorKey& a, const SCurveEditorKey& b)
        {
            return a.m_time < b.m_time;
        });
}

QString CCurveEditor::TangentTypeToString(SCurveEditorKey::ETangentType type)
{
    switch (type)
    {
    case SCurveEditorKey::eTangentType_Standard: // Tangent freely rotates but will stay in sync with its pair if its pair is also standard
        return tr("Standard");
    case SCurveEditorKey::eTangentType_Free: // Tangent is completely free moving (does not sync with its pair)
        return tr("Free");
    case SCurveEditorKey::eTangentType_Step: // Step immediately to value of next control point in tangent direction
        return tr("Step");
    case SCurveEditorKey::eTangentType_Linear: // Tangent always points to next control point
        return tr("Linear");
    case SCurveEditorKey::eTangentType_Smooth: // Tangent is smoothed automatically based on direction/distance to neighboring controls
        return tr("Smooth");
    case SCurveEditorKey::eTangentType_Flat: // Tangent is flattened (y = 0) - will still sync with its pair if both are flat
        return tr("Flat");
    case SCurveEditorKey::eTangentType_Bezier: // Tangent is free moving and can be justified by user. Curve defined by Bz
        return tr("Bezier");
    default:
        break;
    }

    return tr("Undefined tangent!");
}

void CCurveEditor::OnDeleteSelectedKeys()
{
    ForEachKey(*m_pContent, []([[maybe_unused]] SCurveEditorCurve& curve, SCurveEditorKey& key)
        {
            key.m_bDeleted = key.m_bDeleted || key.m_bSelected;
        });

    ContentChanged();
}

void CCurveEditor::OnSetSelectedKeysTangentStandard()
{
    SetSelectedKeysTangentType(ETangent_In, SCurveEditorKey::eTangentType_Standard);
    SetSelectedKeysTangentType(ETangent_Out, SCurveEditorKey::eTangentType_Standard);

    SmoothSelectedKeys();
}

void CCurveEditor::OnSetSelectedKeysTangentSmooth()
{
    SetSelectedKeysTangentType(ETangent_In, SCurveEditorKey::eTangentType_Smooth);
    SetSelectedKeysTangentType(ETangent_Out, SCurveEditorKey::eTangentType_Smooth);
}

void CCurveEditor::OnSetSelectedKeysTangentFree()
{
    SetSelectedKeysTangentType(ETangent_In, SCurveEditorKey::eTangentType_Free);
    SetSelectedKeysTangentType(ETangent_Out, SCurveEditorKey::eTangentType_Free);
}

void CCurveEditor::OnSetSelectedKeysTangentBezier()
{
    SetSelectedKeysTangentType(ETangent_In, SCurveEditorKey::eTangentType_Bezier);
    SetSelectedKeysTangentType(ETangent_Out, SCurveEditorKey::eTangentType_Bezier);
}

void CCurveEditor::OnSetSelectedKeysTangentFlat()
{
    SetSelectedKeysTangentType(ETangent_In, SCurveEditorKey::eTangentType_Flat);
    SetSelectedKeysTangentType(ETangent_Out, SCurveEditorKey::eTangentType_Flat);
}

void CCurveEditor::OnSetSelectedKeysTangentLinear()
{
    SetSelectedKeysTangentType(ETangent_In, SCurveEditorKey::eTangentType_Linear);
    SetSelectedKeysTangentType(ETangent_Out, SCurveEditorKey::eTangentType_Linear);
}

void CCurveEditor::OnSetSelectedKeysInTangentFree()
{
    SetSelectedKeysTangentType(ETangent_In, SCurveEditorKey::eTangentType_Free);
}

void CCurveEditor::OnSetSelectedKeysInTangentFlat()
{
    SetSelectedKeysTangentType(ETangent_In, SCurveEditorKey::eTangentType_Flat);
}

void CCurveEditor::OnSetSelectedKeysInTangentLinear()
{
    SetSelectedKeysTangentType(ETangent_In, SCurveEditorKey::eTangentType_Linear);
}

void CCurveEditor::OnSetSelectedKeysInTangentStep()
{
    SetSelectedKeysTangentType(ETangent_In, SCurveEditorKey::eTangentType_Step);
}

void CCurveEditor::OnSetSelectedKeysInTangentBezier()
{
    SetSelectedKeysTangentType(ETangent_In, SCurveEditorKey::eTangentType_Bezier);
}

void CCurveEditor::OnSetSelectedKeysOutTangentFree()
{
    SetSelectedKeysTangentType(ETangent_Out, SCurveEditorKey::eTangentType_Free);
}

void CCurveEditor::OnSetSelectedKeysOutTangentFlat()
{
    SetSelectedKeysTangentType(ETangent_Out, SCurveEditorKey::eTangentType_Flat);
}

void CCurveEditor::OnSetSelectedKeysOutTangentStep()
{
    SetSelectedKeysTangentType(ETangent_Out, SCurveEditorKey::eTangentType_Step);
}

void CCurveEditor::OnSetSelectedKeysOutTangentLinear()
{
    SetSelectedKeysTangentType(ETangent_Out, SCurveEditorKey::eTangentType_Linear);
}

void CCurveEditor::OnSetSelectedKeysOutTangentBezier()
{
    SetSelectedKeysTangentType(ETangent_Out, SCurveEditorKey::eTangentType_Bezier);
}

void CCurveEditor::OnFitCurvesHorizontally()
{
    if (m_timeRangeEnforced)
    {
        return;
    }
    if (m_pContent)
    {
        bool bAnyKeyFound = false;
        float timeMin = std::numeric_limits<float>::max();
        float timeMax = -std::numeric_limits<float>::max();

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

        if (bAnyKeyFound)
        {
            ZoomToTimeRange(timeMin, timeMax);

            // Adjust zoom and translation depending on kFitMargin
            const float pivot = (0.5f - m_translation.x) / m_zoom.x;
            m_zoom.x /= 1.0f + 2.0f * (kFitMargin / GetCurveArea().width());
            m_translation.x += ((0.5f - m_translation.x) / m_zoom.x - pivot) * m_zoom.x;

            update();
        }
    }
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

            if (m_curveType == eCECT_Bezier && curve.m_keys.size() > 0 && !curve.m_customInterpolator)
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
        }

        if (bAnyKeyFound)
        {
            ZoomToValueRange(valueMin, valueMax);

            // Adjust zoom and translation depending on kFitMargin
            const float pivot = (0.5f - m_translation.y) / m_zoom.y;
            m_zoom.y /= 1.0f + 2.0f * (kFitMargin / GetCurveArea().height());
            m_translation.y += ((0.5f - m_translation.y) / m_zoom.y - pivot) * m_zoom.y;

            update();
        }
    }
}

void CCurveEditor::SetSelectedKeysTangentType(const ETangent tangent, const SCurveEditorKey::ETangentType type)
{
    if (m_pContent)
    {
        ForEachKey(*m_pContent, [&]([[maybe_unused]] SCurveEditorCurve& curve, SCurveEditorKey& key)
            {
                if (key.m_bSelected)
                {
                    if (tangent == ETangent_In)
                    {
                        key.m_inTangentType = type;
                    }
                    else
                    {
                        key.m_outTangentType = type;
                    }
                }
            });

        UpdateTangents();

        update();

        SignalContentChanged();
    }
}

void CCurveEditor::SmoothSelectedKeys()
{
    if (m_pContent)
    {
        for (SCurveEditorCurve& curve : m_pContent->m_curves)
        {
            for (int keyIx = 0; keyIx < curve.m_keys.size(); ++keyIx)
            {
                SCurveEditorKey& key = curve.m_keys[keyIx];
                if (key.m_bSelected)
                {
                    SCurveEditorKey* pLeftKey = (keyIx > 0) ? &curve.m_keys[keyIx - 1] : nullptr;
                    SCurveEditorKey* pRightKey = (keyIx + 1 < curve.m_keys.size()) ? &curve.m_keys[keyIx + 1] : nullptr;
                    SmoothTangents(key, key.m_inTangent, key.m_outTangent, pLeftKey, pRightKey, false);
                }
            }
        }

        UpdateTangents();

        update();
    }
}

void CCurveEditor::UpdateTangents()
{
    for (SCurveEditorCurve& curve : m_pContent->m_curves)
    {
        for (int keyIx = 0; keyIx < curve.m_keys.size(); ++keyIx)
        {
            SCurveEditorKey& key = curve.m_keys[keyIx];

            if (key.m_bAdded)
            {
                if (curve.m_keys.size() == 1)
                {
                    return;
                }
                if (keyIx == 0)
                {
                    SCurveEditorKey& nextKey = curve.m_keys[keyIx + 1];
                    key.m_outTangent = Vec2(nextKey.m_time - key.m_time, nextKey.m_value - key.m_value) / 3.0f;
                    key.m_inTangent = -key.m_outTangent;
                }
                else if (keyIx + 1 == curve.m_keys.size())
                {
                    SCurveEditorKey& prevKey = curve.m_keys[keyIx - 1];
                    key.m_inTangent = Vec2(prevKey.m_time - key.m_time, prevKey.m_value - key.m_value) / 3.0f;
                    key.m_outTangent = -key.m_outTangent;
                }
                else
                {
                    SCurveEditorKey& prevKey = curve.m_keys[keyIx - 1];
                    SCurveEditorKey& nextKey = curve.m_keys[keyIx + 1];

                }
                key.m_bAdded = false;
            }
            if (keyIx > 0)
            {
                SCurveEditorKey& prevKey = curve.m_keys[keyIx - 1];

                switch (key.m_inTangentType)
                {
                case SCurveEditorKey::eTangentType_Smooth:
                {
                    if (keyIx + 1 >= curve.m_keys.size())
                    {
                        key.m_inTangent = Vec2(prevKey.m_time - key.m_time, prevKey.m_value - key.m_value) / 3.0f;
                        break;
                    }
                    SCurveEditorKey& nextKey = curve.m_keys[keyIx + 1];
                    const float deltaTime = nextKey.m_time - prevKey.m_time;
                    if (deltaTime > 0.0f)
                    {
                        Vec2 normalizedIn = Vec2(prevKey.m_time - key.m_time, prevKey.m_value - key.m_value);
                        Vec2 normalizedOut = Vec2(nextKey.m_time - key.m_time, nextKey.m_value - key.m_value);

                        key.m_inTangent = GetSmoothInTangent(key, normalizedIn, normalizedOut, &prevKey, &nextKey, true);
                    }
                    break;
                }
                case SCurveEditorKey::eTangentType_Flat:
                {
                    key.m_inTangent = Vec2(prevKey.m_time - key.m_time, 0.0f) / 3.0f;
                    break;
                }
                case SCurveEditorKey::eTangentType_Step:
                {
                    //key.m_inTangent = Vec2(0.0f, 0.0f);
                    break;
                }
                case SCurveEditorKey::eTangentType_Linear:
                {
                    key.m_inTangent = Vec2(prevKey.m_time - key.m_time, prevKey.m_value - key.m_value) / 3.0f;
                    break;
                }
                default:
                {
                    const float oneThirdDeltaTime = (key.m_time - prevKey.m_time) / 3.0f;
                    float ratio = oneThirdDeltaTime / -key.m_inTangent.x;
                    key.m_inTangent *= ratio;
                    break;
                }
                }
            }
            if (keyIx + 1 < curve.m_keys.size())
            {
                SCurveEditorKey& nextKey = curve.m_keys[keyIx + 1];

                switch (key.m_outTangentType)
                {
                case SCurveEditorKey::eTangentType_Smooth:
                {
                    if (keyIx <= 0)
                    {
                        key.m_outTangent = Vec2(nextKey.m_time - key.m_time, nextKey.m_value - key.m_value) / 3.0f;
                    }
                    if (keyIx > 0)
                    {
                        SCurveEditorKey& prevKey = curve.m_keys[keyIx - 1];
                        const float deltaTime = nextKey.m_time - prevKey.m_time;
                        if (deltaTime > 0.0f)
                        {
                            Vec2 normalizedIn = Vec2(prevKey.m_time - key.m_time, prevKey.m_value - key.m_value);
                            Vec2 normalizedOut = Vec2(nextKey.m_time - key.m_time, nextKey.m_value - key.m_value);

                            key.m_outTangent = GetSmoothOutTangent(key, normalizedIn, normalizedOut, &prevKey, &nextKey, true);
                        }
                    }
                    break;
                }
                case SCurveEditorKey::eTangentType_Flat:
                {
                    key.m_outTangent = Vec2(nextKey.m_time - key.m_time, 0.0f) / 3.0f;
                    break;
                }
                case SCurveEditorKey::eTangentType_Step:
                {
                    //key.m_outTangent = Vec2(0.0f, 0.0f);
                    break;
                }
                case SCurveEditorKey::eTangentType_Linear:
                {
                    key.m_outTangent = Vec2(nextKey.m_time - key.m_time, nextKey.m_value - key.m_value) / 3.0f;
                    break;
                }
                default:
                {
                    const float oneThirdDeltaTime = (nextKey.m_time - key.m_time) / 3.0f;
                    float ratio = oneThirdDeltaTime / key.m_outTangent.x;
                    key.m_outTangent *= ratio;
                    break;
                }
                }
            }
        }
    }
}

QRect CCurveEditor::GetCurveArea()
{
    const uint rulerAreaHeight = m_bRulerVisible ? kRulerHeight : 0;
    return QRect(0, rulerAreaHeight, width(), height() - rulerAreaHeight);
}

Vec2 CCurveEditor::TransformToScreenCoordinates(Vec2 graphPoint)
{
    return TransformPointToScreen(m_zoom, m_translation, GetCurveArea(), graphPoint);
}

Vec2 CCurveEditor::TransformFromScreenCoordinates(Vec2 screenPoint)
{
    return TransformPointFromScreen(m_zoom, m_translation, GetCurveArea(), screenPoint);
}

void CCurveEditor::SetOptOutFlags(int flags)
{
    m_optOutFlags = flags;
    if (m_optOutFlags & EOptOutRuler)
    {
        m_bRulerVisible = false;
    }
}

void CCurveEditor::PopulateControlContextMenu(QMenu* pMenu)
{
    #define OPTOUT(BITS) ((m_optOutFlags & (BITS)))

    bool needsSeperator = false;

    pMenu->addAction("Delete selected keys", this, SLOT(OnDeleteSelectedKeys()));
    pMenu->addSeparator();

    // standard and smooth should only be available for both - flat, free, step, and linear should be available for all
    const int flagsFreeStep = EOptOutFree | EOptOutStep;
    if (OPTOUT(flagsFreeStep) != flagsFreeStep)
    {
        pMenu->addAction("Standard", this, SLOT(OnSetSelectedKeysTangentStandard()));
        pMenu->addAction("Auto Smooth", this, SLOT(OnSetSelectedKeysTangentSmooth()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutFree))
    {
        pMenu->addAction("Free", this, SLOT(OnSetSelectedKeysTangentFree()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutBezier))
    {
        pMenu->addAction("Bezier", this, SLOT(OnSetSelectedKeysTangentBezier()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutFlat))
    {
        pMenu->addAction("Flat", this, SLOT(OnSetSelectedKeysTangentFlat()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutLinear))
    {
        pMenu->addAction("Linear", this, SLOT(OnSetSelectedKeysTangentLinear()));
        needsSeperator = true;
    }

    if (needsSeperator)
    {
        pMenu->addSeparator();
    }
    needsSeperator = false;

    if (!OPTOUT(EOptOutBezier))
    {
        pMenu->addAction("IN Tangent - Bezier", this, SLOT(OnSetSelectedKeysInTangentBezier()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutFree))
    {
        pMenu->addAction("IN Tangent - Free", this, SLOT(OnSetSelectedKeysInTangentFree()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutFlat))
    {
        pMenu->addAction("IN Tangent - Flat", this, SLOT(OnSetSelectedKeysInTangentFlat()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutLinear))
    {
        pMenu->addAction("IN Tangent - Linear", this, SLOT(OnSetSelectedKeysInTangentLinear()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutStep))
    {
        pMenu->addAction("IN Tangent - Step", this, SLOT(OnSetSelectedKeysInTangentStep()));
        needsSeperator = true;
    }

    if (needsSeperator)
    {
        pMenu->addSeparator();
    }
    needsSeperator = false;

    if (!OPTOUT(EOptOutBezier))
    {
        pMenu->addAction("OUT Tangent - Bezier", this, SLOT(OnSetSelectedKeysOutTangentBezier()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutFree))
    {
        pMenu->addAction("OUT Tangent - Free", this, SLOT(OnSetSelectedKeysOutTangentFree()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutFlat))
    {
        pMenu->addAction("OUT Tangent - Flat", this, SLOT(OnSetSelectedKeysOutTangentFlat()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutLinear))
    {
        pMenu->addAction("OUT Tangent - Linear", this, SLOT(OnSetSelectedKeysOutTangentLinear()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutStep))
    {
        pMenu->addAction("OUT Tangent - Step", this, SLOT(OnSetSelectedKeysOutTangentStep()));
        needsSeperator = true;
    }

    if (!OPTOUT(EOptOutFitCurvesContextMenuOptions))
    {
        if (needsSeperator)
        {
            pMenu->addSeparator();
        }
        needsSeperator = false;
        pMenu->addAction("Fit curves horizontally", this, SLOT(OnFitCurvesHorizontally()));
        pMenu->addAction("Fit curves vertically", this, SLOT(OnFitCurvesVertically()));
    }
}

void CCurveEditor::setPenColor(QColor color)
{
    if (color.isValid())
    {
        m_penColor = color;
    }
    else if (!m_penColor.isValid())
    {
        m_penColor = palette().highlight().color();
    }
}

void CCurveEditor::updateCurveKeyShapeColor()
{
    for (CCurveEditorControl* key : m_pControlKeys)
    {
        QColor color = key->IsSelected() ? QColor(Qt::yellow) : QColor(Qt::white);
        key->SetIconShapeColor(color);
    }
}

void CCurveEditor::SetIconShapeColor(unsigned int key, QColor color)
{
    m_pControlKeys[key]->SetIconShapeColor(color);
}

void CCurveEditor::SetIconFillColor(unsigned int key, QColor color)
{
    m_pControlKeys[key]->SetIconFillColor(color);
}

void CCurveEditor::SetIconImage(QString str)
{
    for (auto* key : m_pControlKeys)
    {
        key->SetIconImage(str);
    }
}

void CCurveEditor::SetIconShapeMask(QColor color)
{
    for (auto* key : m_pControlKeys)
    {
        key->SetIconShapeMask(color);
    }
}

void CCurveEditor::SetIconFillMask(QColor color)
{
    for (auto* key : m_pControlKeys)
    {
        key->SetIconFillMask(color);
    }
}

void CCurveEditor::SetIconToolTip(unsigned int key, QString str)
{
    m_pControlKeys[key]->SetIconToolTip(str);
}

void CCurveEditor::SetIconSize(unsigned int key, unsigned int size)
{
    m_pControlKeys[key]->SetIconSize(size);
    m_pControlKeys[key]->SetVisualSize(size);
    m_pControlKeys[key]->SetClickableSize(size);
}


