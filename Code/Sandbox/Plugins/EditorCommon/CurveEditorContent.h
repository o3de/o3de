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

#pragma once

#include <Cry_Math.h>
#include <Cry_Color.h>

#include "Serialization/Strings.h"
#include "Serialization/SmartPtr.h"
#include "Serialization/Color.h"
#include "Serialization.h"

struct ISplineInterpolator;

struct SCurveEditorKey
{
    SCurveEditorKey()
        : m_bSelected(false)
        , m_bModified(false)
        , m_bAdded(false)
        , m_bDeleted(false)
        , m_time(0.0f)
        , m_value(0.0f)
        , m_inTangentType(eTangentType_Standard)
        , m_outTangentType(eTangentType_Standard)
        , m_inTangent(ZERO)
        , m_outTangent(ZERO)
    {
    }

    enum ETangentType
    {
        eTangentType_Standard, // Tangent freely rotates but will stay in sync with its pair if its pair is also standard
        eTangentType_Free, // Tangent is completely free moving (does not sync with its pair)
        eTangentType_Step, // Step immediately to value of next control point in tangent direction
        eTangentType_Linear, // Tangent always points to next control point
        eTangentType_Bezier, // Tangent is free moving and can be justified by user
        eTangentType_Smooth, // Tangent is smoothed automatically based on direction/distance to neighboring controls
        eTangentType_Flat, // Tangent is flattened (y = 0) - will still sync with its pair if both are flat
    };

    void Serialize(IArchive& ar)
    {
        ar(m_inTangentType, "inTangentType");
        ar(m_outTangentType, "outTangentType");
        ar(m_time, "time");
        ar(m_value, "value");
        ar(m_inTangent, "inTangent");
        ar(m_outTangent, "outTangent");
    }

    bool m_bSelected : 1;
    bool m_bModified : 1;
    bool m_bAdded : 1;
    bool m_bDeleted : 1;

    ETangentType m_inTangentType : 4;
    ETangentType m_outTangentType : 4;

    float m_time;
    float m_value;

    // For 1D Bezier only the Y component is used
    Vec2 m_inTangent;
    Vec2 m_outTangent;


    bool operator==(const SCurveEditorKey& rhs) const
    {
        return (m_inTangentType == rhs.m_inTangentType
            && m_outTangentType == rhs.m_outTangentType
            && m_time == rhs.m_time
            && m_value == rhs.m_value
            && m_inTangent == rhs.m_inTangent
            && m_outTangent == rhs.m_outTangent);
    }

    bool operator!=(const SCurveEditorKey& rhs) const
    {
        return !(*this == rhs);
    }
};

struct SCurveEditorCurve
{
    SCurveEditorCurve()
        : m_bModified(false)
        , m_defaultValue(0.0f)
        , m_color(255, 255, 255)
        , m_customInterpolator(nullptr)
    {}

    void Serialize(IArchive& ar)
    {
        ar(m_keys, "keys");
        ar(m_defaultValue, "defaultValue");
        ar(m_color, "color");
    }

    bool m_bModified : 1;
    float m_defaultValue;
    ColorB m_color;

    // Setting m_customInterpolator will override spline-draw code.
    // When used, its up to developer to fill and update all necessary keys.
    ISplineInterpolator* m_customInterpolator;

    std::vector<SCurveEditorKey> m_keys;

    bool operator==(const SCurveEditorCurve& rhs) const
    {
        if (m_defaultValue != rhs.m_defaultValue
            || m_color != rhs.m_color
            || m_keys.size() != rhs.m_keys.size())
        {
            return false;
        }

        for (int i = 0; i < m_keys.size(); i++)
        {
            if (m_keys[i] != rhs.m_keys[i])
                return false;
        }

        return true;
    }

    bool operator!=(const SCurveEditorCurve& rhs) const
    {
        return !(*this == rhs);
    }
};

typedef std::vector<SCurveEditorCurve> TCurveEditorCurves;

struct SCurveEditorContent
{
    void Serialize(IArchive& ar)
    {
        ar(m_curves, "curves");
    }

    TCurveEditorCurves m_curves;
};
