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

#ifndef __BEZIER_H__
#define __BEZIER_H__

#include <AnimTime.h>
#include <Serialization/IArchive.h>
#include <Serialization/Math.h>

struct SBezierControlPoint
{
    SBezierControlPoint()
        : m_value(0.0f)
        , m_inTangent(ZERO)
        , m_outTangent(ZERO)
        , m_inTangentType(eTangentType_Auto)
        , m_outTangentType(eTangentType_Auto)
        , m_bBreakTangents(false)
    {
    }

    enum ETangentType
    {
        eTangentType_Custom,
        eTangentType_Auto,
        eTangentType_Zero,
        eTangentType_Step,
        eTangentType_Linear,
    };

    void Serialize(Serialization::IArchive& ar)
    {
        ar(m_value, "value", "Value");

        if (ar.IsOutput())
        {
            bool breakTangents = m_bBreakTangents;
            ar(breakTangents, "breakTangents", "Break Tangents");
        }
        else
        {
            bool breakTangents = false;
            ar(breakTangents, "breakTangents", "Break Tangents");
            m_bBreakTangents = breakTangents;
        }

        if (ar.IsOutput())
        {
            ETangentType inTangentType = m_inTangentType;
            ar(inTangentType, "inTangentType", "Incoming tangent type");
        }
        else
        {
            ETangentType inTangentType = eTangentType_Auto;
            ar(inTangentType, "inTangentType", "Incoming tangent type");
            m_inTangentType = inTangentType;
        }

        ar(m_inTangent, "inTangent", (m_inTangentType == eTangentType_Custom) ? "Incoming Tangent" : NULL);

        if (ar.IsOutput())
        {
            ETangentType outTangentType = m_outTangentType;
            ar(outTangentType, "outTangentType", "Outgoing tangent type");
        }
        else
        {
            ETangentType outTangentType = eTangentType_Auto;
            ar(outTangentType, "outTangentType", "Outgoing tangent type");
            m_outTangentType = outTangentType;
        }

        ar(m_outTangent, "outTangent", (m_outTangentType == eTangentType_Custom) ? "Outgoing Tangent" : NULL);
    }

    float m_value;

    // For 1D Bezier only the Y component is used
    Vec2 m_inTangent;
    Vec2 m_outTangent;

    ETangentType m_inTangentType : 4;
    ETangentType m_outTangentType : 4;
    bool m_bBreakTangents : 1;
};

struct SBezierKey
{
    SBezierKey()
        : m_time(0) {}

    void Serialize(Serialization::IArchive& ar)
    {
        ar(m_time, "time", "Time");
        ar(m_controlPoint, "controlPoint", "Control Point");
    }

    SAnimTime m_time;
    SBezierControlPoint m_controlPoint;
};

namespace Bezier
{
    inline float Evaluate(float t, float p0, float p1, float p2, float p3)
    {
        const float a = 1 - t;
        const float aSq = a * a;
        const float tSq = t * t;
        return (aSq * a * p0) + (3.0f * aSq * t * p1) + (3.0f * a * tSq * p2) + (tSq * t * p3);
    }

    inline float EvaluateDeriv(float t, float p0, float p1, float p2, float p3)
    {
        const float a = 1 - t;
        const float ta = t * a;
        const float aSq = a * a;
        const float tSq = t * t;
        return 3.0f * ((-p2 * tSq) + (p3 * tSq) - (p0 * aSq) + (p1 * aSq) + 2.0f * ((-p1 * ta) + (p2 * ta)));
    }

    inline float EvaluateX(const float t, const float duration, const SBezierControlPoint& start, const SBezierControlPoint& end)
    {
        const float p0 = 0.0f;
        const float p1 = p0 + start.m_outTangent.x;
        const float p3 = duration;
        const float p2 = p3 + end.m_inTangent.x;
        return Evaluate(t, p0, p1, p2, p3);
    }

    inline float EvaluateY(const float t, const SBezierControlPoint& start, const SBezierControlPoint& end)
    {
        const float p0 = start.m_value;
        const float p1 = p0 + start.m_outTangent.y;
        const float p3 = end.m_value;
        const float p2 = p3 + end.m_inTangent.y;
        return Evaluate(t, p0, p1, p2, p3);
    }

    // Duration = (time at end key) - (time at start key)
    inline float EvaluateDerivX(const float t, const float duration, const SBezierControlPoint& start, const SBezierControlPoint& end)
    {
        const float p0 = 0.0f;
        const float p1 = p0 + start.m_outTangent.x;
        const float p3 = duration;
        const float p2 = p3 + end.m_inTangent.x;
        return EvaluateDeriv(t, p0, p1, p2, p3);
    }

    inline float EvaluateDerivY(const float t, const SBezierControlPoint& start, const SBezierControlPoint& end)
    {
        const float p0 = start.m_value;
        const float p1 = p0 + start.m_outTangent.y;
        const float p3 = end.m_value;
        const float p2 = p3 + end.m_inTangent.y;
        return EvaluateDeriv(t, p0, p1, p2, p3);
    }

    // Find interpolation factor where 2D bezier curve has the given x value. Works only for curves where x is monotonically increasing.
    // The passed x must be in range [0, duration]. Uses the Newton-Raphson root finding method. Usually takes 2 or 3 iterations.
    //
    // Note: This is for "1D" 2D bezier curves as used in TrackView. The curves are restricted by the curve editor to be monotonically increasing.
    //
    inline float InterpolationFactorFromX(const float x, const float duration, const SBezierControlPoint& start, const SBezierControlPoint& end)
    {
        float t = (x / duration);

        const float epsilon = 0.00001f;
        const uint maxSteps = 10;

        for (uint i  = 0; i < maxSteps; ++i)
        {
            const float currentX = EvaluateX(t, duration, start, end) - x;
            if (fabs(currentX) <= epsilon)
            {
                break;
            }

            const float currentXDeriv = EvaluateDerivX(t, duration, start, end);
            t -= currentX / currentXDeriv;
        }

        return t;
    }

    inline SBezierControlPoint CalculateInTangent(
        float time, const SBezierControlPoint& point,
        float leftTime, const SBezierControlPoint* pLeftPoint,
        float rightTime, const SBezierControlPoint* pRightPoint)
    {
        SBezierControlPoint newPoint = point;

        // In tangent X can never be positive
        newPoint.m_inTangent.x = std::min(point.m_inTangent.x, 0.0f);

        if (pLeftPoint)
        {
            switch (point.m_inTangentType)
            {
            case SBezierControlPoint::eTangentType_Custom:
            {
                // Need to clamp tangent if it is reaching over last point
                const float deltaTime = time - leftTime;
                if (deltaTime < -newPoint.m_inTangent.x)
                {
                    if (newPoint.m_inTangent.x == 0)
                    {
                        newPoint.m_inTangent = Vec2(ZERO);
                    }
                    else
                    {
                        float scaleFactor = deltaTime / -newPoint.m_inTangent.x;
                        newPoint.m_inTangent.x = -deltaTime;
                        newPoint.m_inTangent.y *= scaleFactor;
                    }
                }
            }
            break;
            case SBezierControlPoint::eTangentType_Zero:
            // Fall through. Zero for y is same as Auto, x is set to 0.0f
            case SBezierControlPoint::eTangentType_Auto:
            {
                const SBezierControlPoint& rightPoint = pRightPoint ? *pRightPoint : point;
                const float deltaTime = (pRightPoint ? rightTime : time) - leftTime;
                if (deltaTime > 0.0f)
                {
                    const float ratio = (time - leftTime) / deltaTime;
                    const float deltaValue = rightPoint.m_value - pLeftPoint->m_value;
                    const bool bIsZeroTangent = (point.m_inTangentType == SBezierControlPoint::eTangentType_Zero);
                    newPoint.m_inTangent = Vec2(-(deltaTime * ratio) / 3.0f, bIsZeroTangent ? 0.0f : -(deltaValue * ratio) / 3.0f);
                }
                else
                {
                    newPoint.m_inTangent = Vec2(ZERO);
                }
            }
            break;
            case SBezierControlPoint::eTangentType_Linear:
                newPoint.m_inTangent = Vec2((leftTime - time) / 3.0f,
                        (pLeftPoint->m_value - point.m_value) / 3.0f);
                break;
            }
        }

        return newPoint;
    }

    inline SBezierControlPoint CalculateOutTangent(
        float time, const SBezierControlPoint& point,
        float leftTime, const SBezierControlPoint* pLeftPoint,
        float rightTime, const SBezierControlPoint* pRightPoint)
    {
        SBezierControlPoint newPoint = point;

        // Out tangent X can never be negative
        newPoint.m_outTangent.x = std::max(point.m_outTangent.x, 0.0f);

        if (pRightPoint)
        {
            switch (point.m_outTangentType)
            {
            case SBezierControlPoint::eTangentType_Custom:
            {
                // Need to clamp tangent if it is reaching over next point
                const float deltaTime = rightTime - time;
                if (deltaTime < newPoint.m_outTangent.x)
                {
                    if (newPoint.m_outTangent.x == 0)
                    {
                        newPoint.m_outTangent = Vec2(ZERO);
                    }
                    else
                    {
                        float scaleFactor = deltaTime / newPoint.m_outTangent.x;
                        newPoint.m_outTangent.x = deltaTime;
                        newPoint.m_outTangent.y *= scaleFactor;
                    }
                }
            }
            break;
            case SBezierControlPoint::eTangentType_Zero:
            // Fall through. Zero for y is same as Auto, x is set to 0.0f
            case SBezierControlPoint::eTangentType_Auto:
            {
                const SBezierControlPoint& leftPoint = pLeftPoint ? *pLeftPoint : point;
                const float deltaTime = rightTime - (pLeftPoint ? leftTime : time);
                if (deltaTime > 0.0f)
                {
                    const float ratio = (rightTime - time) / deltaTime;
                    const float deltaValue = pRightPoint->m_value - leftPoint.m_value;
                    const bool bIsZeroTangent = (point.m_outTangentType == SBezierControlPoint::eTangentType_Zero);
                    newPoint.m_outTangent = Vec2((deltaTime * ratio) / 3.0f, bIsZeroTangent ? 0.0f : (deltaValue * ratio) / 3.0f);
                }
                else
                {
                    newPoint.m_outTangent = Vec2(ZERO);
                }
            }
            break;
            case SBezierControlPoint::eTangentType_Linear:
                newPoint.m_outTangent = Vec2((rightTime - time) / 3.0f,
                        (pRightPoint->m_value - point.m_value) / 3.0f);
                break;
            }
        }

        return newPoint;
    }
}

#endif