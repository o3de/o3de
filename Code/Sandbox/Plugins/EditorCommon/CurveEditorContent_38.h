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
#pragma once

#include <Cry_Math.h>
#include <Cry_Color.h>
#include <Bezier.h>
#include <AnimTime.h>

#include "Serialization/Strings.h"
#include "Serialization/SmartPtr.h"
#include "Serialization/Color.h"
#include "Serialization.h"

struct SCurveEditorKey
{
    SCurveEditorKey()
        : m_time(0)
        , m_bSelected(false)
        , m_bModified(false)
        , m_bAdded(false)
        , m_bDeleted(false)
    {
    }

    void Serialize(IArchive& ar)
    {
        ar(m_time);
        ar(m_controlPoint);
    }

    SAnimTime m_time;
    SBezierControlPoint m_controlPoint;

    bool m_bSelected : 1;
    bool m_bModified : 1;
    bool m_bAdded : 1;
    bool m_bDeleted : 1;
};

struct SCurveEditorCurve
{
    SCurveEditorCurve()
        : m_bModified(false)
        , m_defaultValue(0.0f)
        , m_color(255, 255, 255)
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

    DynArray<char> userSideLoad;

    std::vector<SCurveEditorKey> m_keys;
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
