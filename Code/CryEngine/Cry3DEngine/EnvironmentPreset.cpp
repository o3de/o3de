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

#include "Cry3DEngine_precompiled.h"

#include "ITimeOfDay.h"
#include "EnvironmentPreset.h"

#include <Serialization/IArchive.h>
#include <Serialization/IArchiveHost.h>
#include <Serialization/ClassFactory.h>
#include <Serialization/Enum.h>

#include <Bezier.h>
#include <Serialization/Enum.h>

SERIALIZATION_ENUM_BEGIN_NESTED(SBezierControlPoint, ETangentType, "TangentType")
SERIALIZATION_ENUM_VALUE_NESTED(SBezierControlPoint, eTangentType_Custom, "Custom")
SERIALIZATION_ENUM_VALUE_NESTED(SBezierControlPoint, eTangentType_Auto, "Smooth")
SERIALIZATION_ENUM_VALUE_NESTED(SBezierControlPoint, eTangentType_Zero, "Zero")
SERIALIZATION_ENUM_VALUE_NESTED(SBezierControlPoint, eTangentType_Step, "Step")
SERIALIZATION_ENUM_VALUE_NESTED(SBezierControlPoint, eTangentType_Linear, "Linear")
SERIALIZATION_ENUM_END()

namespace EnvironmentPresetDetails
{
    static const float sBezierSplineKeyValueEpsilon = 0.001f;

    //////////////////////////////////////////////////////////////////////////
    SBezierKey ApplyInTangent(const SBezierKey& key, const SBezierKey& leftKey, const SBezierKey* pRightKey)
    {
        SBezierKey newKey = key;

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
                    floatRightTime, pRightKey ? &pRightKey->m_controlPoint : NULL);
        }
        else
        {
            newKey.m_controlPoint.m_inTangent = Vec2(0.0f, 0.0f);
            newKey.m_controlPoint.m_value = leftKey.m_controlPoint.m_value;
        }

        return newKey;
    }

    SBezierKey ApplyOutTangent(const SBezierKey& key, const SBezierKey* pLeftKey, const SBezierKey& rightKey)
    {
        SBezierKey newKey = key;

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
                    floatLeftTime, pLeftKey ? &pLeftKey->m_controlPoint : NULL,
                    floatRightTime, &rightKey.m_controlPoint);
        }
        else
        {
            newKey.m_controlPoint.m_outTangent = Vec2(0.0f, 0.0f);
            newKey.m_controlPoint.m_value = rightKey.m_controlPoint.m_value;
        }

        return newKey;
    }
}

CBezierSpline::CBezierSpline()
{
    m_keys.reserve(2);
}

CBezierSpline::~CBezierSpline()
{
}

void CBezierSpline::Init(float fDefaultValue)
{
    m_keys.clear();
    InsertKey(SAnimTime(0.0f), fDefaultValue);
    InsertKey(SAnimTime(1.0f), fDefaultValue);
}

float CBezierSpline::Evaluate(float t) const
{
    if (m_keys.size() == 0)
    {
        return 0.0f;
    }

    if (m_keys.size() == 1)
    {
        return m_keys.front().m_controlPoint.m_value;
    }

    const SAnimTime time(t);

    if (time <= m_keys.front().m_time)
    {
        return m_keys.front().m_controlPoint.m_value;
    }
    else if (time >= m_keys.back().m_time)
    {
        return m_keys.back().m_controlPoint.m_value;
    }

    const TKeyContainer::const_iterator it = std::upper_bound(m_keys.begin(), m_keys.end(), time, SCompKeyTime());
    const TKeyContainer::const_iterator startIt = it - 1;

    if (startIt->m_controlPoint.m_outTangentType == SBezierControlPoint::eTangentType_Step)
    {
        return it->m_controlPoint.m_value;
    }

    if (it->m_controlPoint.m_inTangentType == SBezierControlPoint::eTangentType_Step)
    {
        return startIt->m_controlPoint.m_value;
    }

    const SAnimTime deltaTime = it->m_time - startIt->m_time;

    if (deltaTime == SAnimTime(0))
    {
        return startIt->m_controlPoint.m_value;
    }

    const float timeInSegment = (time - startIt->m_time).ToFloat();

    const SBezierKey* pKeyLeftOfSegment = (startIt != m_keys.begin()) ? &*(startIt - 1) : NULL;
    const SBezierKey* pKeyRightOfSegment = (startIt != (m_keys.end() - 2)) ? &*(startIt + 2) : NULL;
    const SBezierKey segmentStart = EnvironmentPresetDetails::ApplyOutTangent(*startIt, pKeyLeftOfSegment, *(startIt + 1));
    const SBezierKey segmentEnd = EnvironmentPresetDetails::ApplyInTangent(*(startIt + 1), *startIt, pKeyRightOfSegment);

    const float factor = Bezier::InterpolationFactorFromX(timeInSegment, deltaTime.ToFloat(), segmentStart.m_controlPoint, segmentEnd.m_controlPoint);
    const float fResult = Bezier::EvaluateY(factor, segmentStart.m_controlPoint, segmentEnd.m_controlPoint);

    return fResult;
}

void CBezierSpline::InsertKey(SAnimTime time, float value)
{
    SBezierKey key;
    key.m_time = time;
    key.m_controlPoint.m_value = value;

    const size_t nKeyNum = m_keys.size();
    for (size_t i = 0; i < nKeyNum; ++i)
    {
        if (m_keys[i].m_time > time)
        {
            m_keys.insert(m_keys.begin() + i, key);
            return;
        }
    }

    m_keys.push_back(key);
}

void CBezierSpline::UpdateKeyForTime(float fTime, float value)
{
    const SAnimTime time(fTime);

    const size_t nKeyNum = m_keys.size();
    for (size_t i = 0; i < nKeyNum; ++i)
    {
        if (fabs(m_keys[i].m_time.ToFloat() - fTime) < EnvironmentPresetDetails::sBezierSplineKeyValueEpsilon)
        {
            m_keys[i].m_controlPoint.m_value = value;
            return;
        }
    }

    InsertKey(time, value);
}

void CBezierSpline::Serialize(Serialization::IArchive& ar)
{
    ar(m_keys, "keys");
}

//////////////////////////////////////////////////////////////////////////

CTimeOfDayVariable::CTimeOfDayVariable()
    : m_id(ITimeOfDay::PARAM_TOTAL)
    , m_type(ITimeOfDay::TYPE_FLOAT)
    , m_name(NULL)
    , m_displayName(NULL)
    , m_group(NULL)
    , m_minValue(0.0f)
    , m_maxValue(0.0f)
    , m_value(ZERO)
{
}

CTimeOfDayVariable::~CTimeOfDayVariable()
{
}

void CTimeOfDayVariable::Init(const char* group, const char* displayName, const char* name, ITimeOfDay::ETimeOfDayParamID nParamId, ITimeOfDay::EVariableType type, float defVal0, float defVal1, float defVal2)
{
    m_id = nParamId;
    m_type = type;
    m_name = name;
    m_displayName = (displayName && *displayName) ? displayName : name;
    m_group = (group && *group) ? group : "Default";

    if (ITimeOfDay::TYPE_FLOAT == type)
    {
        m_value.x = defVal0;
        m_minValue = defVal1;
        m_maxValue = defVal2;
        m_spline[0].Init(defVal0);
    }
    else if (ITimeOfDay::TYPE_COLOR == type)
    {
        m_value.x = defVal0;
        m_value.y = defVal1;
        m_value.z = defVal2;

        m_minValue = 0.0f;
        m_maxValue = 1.0f;

        m_spline[0].Init(defVal0);
        m_spline[1].Init(defVal1);
        m_spline[2].Init(defVal2);
    }
}

void CTimeOfDayVariable::Update(float time)
{
    m_value = GetInterpolatedAt(time);
}

Vec3 CTimeOfDayVariable::GetInterpolatedAt(float t) const
{
    Vec3 result;
    result.x = clamp_tpl(m_spline[0].Evaluate(t), m_minValue, m_maxValue);
    result.y = clamp_tpl(m_spline[1].Evaluate(t), m_minValue, m_maxValue);
    result.z = clamp_tpl(m_spline[2].Evaluate(t), m_minValue, m_maxValue);
    return result;
}

size_t CTimeOfDayVariable::GetSplineKeyCount(int nSpline) const
{
    if (const CBezierSpline* pSpline = GetSpline(nSpline))
    {
        return pSpline->GetKeyCount();
    }

    return 0;
}

bool CTimeOfDayVariable::GetSplineKeys(int nSpline, SBezierKey* keysArray, unsigned int keysArraySize) const
{
    if (const CBezierSpline* pSpline = GetSpline(nSpline))
    {
        if (keysArraySize < pSpline->GetKeyCount())
        {
            return false;
        }

        pSpline->GetKeys(keysArray);
        return true;
    }

    return false;
}

bool CTimeOfDayVariable::SetSplineKeys(int nSpline, const SBezierKey* keysArray, unsigned int keysArraySize)
{
    if (CBezierSpline* pSpline = GetSpline(nSpline))
    {
        pSpline->SetKeys(keysArray, keysArraySize);
        return true;
    }
    return false;
}

bool CTimeOfDayVariable::UpdateSplineKeyForTime(int nSpline, float fTime, float newKey)
{
    if (CBezierSpline* pSpline = GetSpline(nSpline))
    {
        pSpline->UpdateKeyForTime(fTime, newKey);
        return true;
    }
    return false;
}

void CTimeOfDayVariable::Serialize(Serialization::IArchive& ar)
{
    ITimeOfDay::ETimeOfDayParamID defID = m_id;
    ITimeOfDay::EVariableType   defType = m_type;

    ar(defID, "id");
    ar(defType, "type");
    if (!ar.IsInput() || (defID == m_id && defType == m_type))
    { // Write always, Read only when ID and Type are correct/Schema hasn't changed
        ar(m_minValue, "minValue");
        ar(m_maxValue, "maxValue");

        ar(m_spline[0], "spline0");
        ar(m_spline[1], "spline1");
        ar(m_spline[2], "spline2");
    }
}

//////////////////////////////////////////////////////////////////////////

CEnvironmentPreset::CEnvironmentPreset()
{
    ResetVariables();
}

CEnvironmentPreset::~CEnvironmentPreset()
{
}

void CEnvironmentPreset::ResetVariables()
{
    const float fRecip255 = 1.0f / 255.0f;

    AddVar("Sun", "", "Sun color", ITimeOfDay::PARAM_SUN_COLOR, ITimeOfDay::TYPE_COLOR, 255.0f * fRecip255, 248.0f * fRecip255, 248.0f * fRecip255);
    AddVar("Sun", "Sun intensity (lux)", "Sun intensity", ITimeOfDay::PARAM_SUN_INTENSITY, ITimeOfDay::TYPE_FLOAT, 119000.0f, 0.0f, 550000.0f);
    AddVar("Sun", "", "Sun specular multiplier", ITimeOfDay::PARAM_SUN_SPECULAR_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 4.0f);

    AddVar("Fog", "Color (bottom)", "Fog color", ITimeOfDay::PARAM_FOG_COLOR, ITimeOfDay::TYPE_COLOR, 0.0f, 0.0f, 0.0f);
    AddVar("Fog", "Color (bottom) multiplier", "Fog color multiplier", ITimeOfDay::PARAM_FOG_COLOR_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 16.0f);
    AddVar("Fog", "Height (bottom)", "Fog height (bottom)", ITimeOfDay::PARAM_VOLFOG_HEIGHT, ITimeOfDay::TYPE_FLOAT, 0.0f, -5000.0f, 30000.0f);
    AddVar("Fog", "Density (bottom)", "Fog layer density (bottom)", ITimeOfDay::PARAM_VOLFOG_DENSITY, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 1.0f);
    AddVar("Fog", "Color (top)", "Fog color (top)", ITimeOfDay::PARAM_FOG_COLOR2, ITimeOfDay::TYPE_COLOR, 0.0f, 0.0f, 0.0f);
    AddVar("Fog", "Color (top) multiplier", "Fog color (top) multiplier", ITimeOfDay::PARAM_FOG_COLOR2_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 16.0f);
    AddVar("Fog", "Height (top)", "Fog height (top)", ITimeOfDay::PARAM_VOLFOG_HEIGHT2, ITimeOfDay::TYPE_FLOAT, 4000.0f, -5000.0f, 30000.0f);
    AddVar("Fog", "Density (top)", "Fog layer density (top)", ITimeOfDay::PARAM_VOLFOG_DENSITY2, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 1.0f);
    AddVar("Fog", "Color height offset", "Fog color height offset", ITimeOfDay::PARAM_VOLFOG_HEIGHT_OFFSET, ITimeOfDay::TYPE_FLOAT, 0.0f, -1.0f, 1.0f);

    AddVar("Fog", "Color (radial)", "Fog color (radial)", ITimeOfDay::PARAM_FOG_RADIAL_COLOR, ITimeOfDay::TYPE_COLOR, 0.0f, 0.0f, 0.0f);
    AddVar("Fog", "Color (radial) multiplier", "Fog color (radial) multiplier", ITimeOfDay::PARAM_FOG_RADIAL_COLOR_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 16.0f);
    AddVar("Fog", "Radial size", "Fog radial size", ITimeOfDay::PARAM_VOLFOG_RADIAL_SIZE, ITimeOfDay::TYPE_FLOAT, 0.75f, 0.0f, 1.0f);
    AddVar("Fog", "Radial lobe", "Fog radial lobe", ITimeOfDay::PARAM_VOLFOG_RADIAL_LOBE, ITimeOfDay::TYPE_FLOAT, 0.5f, 0.0f, 1.0f);

    AddVar("Fog", "Global density", "Volumetric fog: Global density", ITimeOfDay::PARAM_VOLFOG_GLOBAL_DENSITY, ITimeOfDay::TYPE_FLOAT, 0.02f, 0.0f, 100.0f);
    AddVar("Fog", "Final density clamp", "Volumetric fog: Final density clamp", ITimeOfDay::PARAM_VOLFOG_FINAL_DENSITY_CLAMP, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 1.0f);

    AddVar("Fog", "Ramp start", "Volumetric fog: Ramp start", ITimeOfDay::PARAM_VOLFOG_RAMP_START, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 30000.0f);
    AddVar("Fog", "Ramp end", "Volumetric fog: Ramp end", ITimeOfDay::PARAM_VOLFOG_RAMP_END, ITimeOfDay::TYPE_FLOAT, 100.0f, 0.0f, 30000.0f);
    AddVar("Fog", "Ramp influence", "Volumetric fog: Ramp influence", ITimeOfDay::PARAM_VOLFOG_RAMP_INFLUENCE, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 1.0f);

    AddVar("Fog", "Shadow darkening", "Volumetric fog: Shadow darkening", ITimeOfDay::PARAM_VOLFOG_SHADOW_DARKENING, ITimeOfDay::TYPE_FLOAT, 0.25f, 0.0f, 1.0f);
    AddVar("Fog", "Shadow darkening sun", "Volumetric fog: Shadow darkening sun", ITimeOfDay::PARAM_VOLFOG_SHADOW_DARKENING_SUN, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 1.0f);
    AddVar("Fog", "Shadow darkening ambient", "Volumetric fog: Shadow darkening ambient", ITimeOfDay::PARAM_VOLFOG_SHADOW_DARKENING_AMBIENT, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 1.0f);
    AddVar("Fog", "Shadow range", "Volumetric fog: Shadow range", ITimeOfDay::PARAM_VOLFOG_SHADOW_RANGE, ITimeOfDay::TYPE_FLOAT, 0.1f, 0.0f, 1.0f);

    AddVar("Volumetric fog", "Height (bottom)", "Volumetric fog 2: Fog height (bottom)", ITimeOfDay::PARAM_VOLFOG2_HEIGHT, ITimeOfDay::TYPE_FLOAT, 0.0f, -5000.0f, 30000.0f);
    AddVar("Volumetric fog", "Density (bottom)", "Volumetric fog 2: Fog layer density (bottom)", ITimeOfDay::PARAM_VOLFOG2_DENSITY, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 1.0f);
    AddVar("Volumetric fog", "Height (top)", "Volumetric fog 2: Fog height (top)", ITimeOfDay::PARAM_VOLFOG2_HEIGHT2, ITimeOfDay::TYPE_FLOAT, 4000.0f, -5000.0f, 30000.0f);
    AddVar("Volumetric fog", "Density (top)", "Volumetric fog 2: Fog layer density (top)", ITimeOfDay::PARAM_VOLFOG2_DENSITY2, ITimeOfDay::TYPE_FLOAT, 0.0001f, 0.0f, 1.0f);
    AddVar("Volumetric fog", "Global density", "Volumetric fog 2: Global fog density", ITimeOfDay::PARAM_VOLFOG2_GLOBAL_DENSITY, ITimeOfDay::TYPE_FLOAT, 0.1f, 0.0f, 100.0f);
    AddVar("Volumetric fog", "Ramp start", "Volumetric fog 2: Ramp start", ITimeOfDay::PARAM_VOLFOG2_RAMP_START, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 30000.0f);
    AddVar("Volumetric fog", "Ramp end", "Volumetric fog 2: Ramp end", ITimeOfDay::PARAM_VOLFOG2_RAMP_END, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 30000.0f);
    AddVar("Volumetric fog", "Color (atmosphere)", "Volumetric fog 2: Fog albedo color (atmosphere)", ITimeOfDay::PARAM_VOLFOG2_COLOR1, ITimeOfDay::TYPE_COLOR, 1.0f, 1.0f, 1.0f);
    AddVar("Volumetric fog", "Anisotropy (atmosphere)", "Volumetric fog 2: Anisotropy factor (atmosphere)", ITimeOfDay::PARAM_VOLFOG2_ANISOTROPIC1, ITimeOfDay::TYPE_FLOAT, 0.2f, -1.0f, 1.0f);
    AddVar("Volumetric fog", "Color (sun radial)", "Volumetric fog 2: Fog albedo color (sun radial)", ITimeOfDay::PARAM_VOLFOG2_COLOR2, ITimeOfDay::TYPE_COLOR, 1.0f, 1.0f, 1.0f);
    AddVar("Volumetric fog", "Anisotropy (sun radial)", "Volumetric fog 2: Anisotropy factor (sun radial)", ITimeOfDay::PARAM_VOLFOG2_ANISOTROPIC2, ITimeOfDay::TYPE_FLOAT, 0.95f, -1.0f, 1.0f);
    AddVar("Volumetric fog", "Radial blend factor", "Volumetric fog 2: Blend factor for sun scattering", ITimeOfDay::PARAM_VOLFOG2_BLEND_FACTOR, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 1.0f);
    AddVar("Volumetric fog", "Radial blend mode", "Volumetric fog 2: Blend mode for sun scattering", ITimeOfDay::PARAM_VOLFOG2_BLEND_MODE, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 1.0f);
    AddVar("Volumetric fog", "Range", "Volumetric fog 2: Maximum range of ray-marching", ITimeOfDay::PARAM_VOLFOG2_RANGE, ITimeOfDay::TYPE_FLOAT, 64.0f, 0.0f, 8192.0f);
    AddVar("Volumetric fog", "In-scattering", "Volumetric fog 2: In-scattering factor", ITimeOfDay::PARAM_VOLFOG2_INSCATTER, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 100.0f);
    AddVar("Volumetric fog", "Extinction", "Volumetric fog 2: Extinction factor", ITimeOfDay::PARAM_VOLFOG2_EXTINCTION, ITimeOfDay::TYPE_FLOAT, 0.3f, 0.0f, 100.0f);
    AddVar("Volumetric fog", "Color (entities)", "Volumetric fog 2: Fog albedo color (entities)", ITimeOfDay::PARAM_VOLFOG2_COLOR, ITimeOfDay::TYPE_COLOR, 1.0f, 1.0f, 1.0f);
    AddVar("Volumetric fog", "Anisotropy (entities)", "Volumetric fog 2: Anisotropy factor (entities)", ITimeOfDay::PARAM_VOLFOG2_ANISOTROPIC, ITimeOfDay::TYPE_FLOAT, 0.6f, -1.0f, 1.0f);
    AddVar("Volumetric fog", "Analytical fog visibility", "Volumetric fog 2: Analytical volumetric fog visibility", ITimeOfDay::PARAM_VOLFOG2_GLOBAL_FOG_VISIBILITY, ITimeOfDay::TYPE_FLOAT, 0.5f, 0.0f, 1.0f);
    AddVar("Volumetric fog", "Final density clamp", "Volumetric fog 2: Final density clamp", ITimeOfDay::PARAM_VOLFOG2_FINAL_DENSITY_CLAMP, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 1.0f);

    AddVar("Sky Light", "Sun intensity", "Sky light: Sun intensity", ITimeOfDay::PARAM_SKYLIGHT_SUN_INTENSITY, ITimeOfDay::TYPE_COLOR, 1.0f, 1.0f, 1.0f);
    AddVar("Sky Light", "Sun intensity multiplier", "Sky light: Sun intensity multiplier", ITimeOfDay::PARAM_SKYLIGHT_SUN_INTENSITY_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 50.0f, 0.0f, 1000.0f);
    AddVar("Sky Light", "Mie scattering", "Sky light: Mie scattering", ITimeOfDay::PARAM_SKYLIGHT_KM, ITimeOfDay::TYPE_FLOAT, 4.8f, 0.0f, 1000.0f);
    AddVar("Sky Light", "Rayleigh scattering", "Sky light: Rayleigh scattering", ITimeOfDay::PARAM_SKYLIGHT_KR, ITimeOfDay::TYPE_FLOAT, 2.0f, 0.0f, 1000.0f);
    AddVar("Sky Light", "Sun anisotropy factor", "Sky light: Sun anisotropy factor", ITimeOfDay::PARAM_SKYLIGHT_G, ITimeOfDay::TYPE_FLOAT, -0.997f, -0.9999f, 0.9999f);
    AddVar("Sky Light", "Wavelength (R)", "Sky light: Wavelength (R)", ITimeOfDay::PARAM_SKYLIGHT_WAVELENGTH_R, ITimeOfDay::TYPE_FLOAT, 694.0f, 380.0f, 780.0f);
    AddVar("Sky Light", "Wavelength (G)", "Sky light: Wavelength (G)", ITimeOfDay::PARAM_SKYLIGHT_WAVELENGTH_G, ITimeOfDay::TYPE_FLOAT, 597.0f, 380.0f, 780.0f);
    AddVar("Sky Light", "Wavelength (B)", "Sky light: Wavelength (B)", ITimeOfDay::PARAM_SKYLIGHT_WAVELENGTH_B, ITimeOfDay::TYPE_FLOAT, 488.0f, 380.0f, 780.0f);

    AddVar("Night Sky", "Horizon color", "Night sky: Horizon color", ITimeOfDay::PARAM_NIGHSKY_HORIZON_COLOR, ITimeOfDay::TYPE_COLOR, 222.0f * fRecip255, 148.0f * fRecip255, 47.0f * fRecip255);
    AddVar("Night Sky", "Zenith color", "Night sky: Zenith color", ITimeOfDay::PARAM_NIGHSKY_ZENITH_COLOR, ITimeOfDay::TYPE_COLOR, 17.0f * fRecip255, 38.0f * fRecip255, 78.0f * fRecip255);
    AddVar("Night Sky", "Zenith shift", "Night sky: Zenith shift", ITimeOfDay::PARAM_NIGHSKY_ZENITH_SHIFT, ITimeOfDay::TYPE_FLOAT, 0.25f, 0.0f, 16.0f);
    AddVar("Night Sky", "Star intensity", "Night sky: Star intensity", ITimeOfDay::PARAM_NIGHSKY_START_INTENSITY, ITimeOfDay::TYPE_FLOAT, 0.01f, 0.0f, 16.0f);
    AddVar("Night Sky", "Moon color", "Night sky: Moon color", ITimeOfDay::PARAM_NIGHSKY_MOON_COLOR, ITimeOfDay::TYPE_COLOR, 255.0f * fRecip255, 255.0f * fRecip255, 255.0f * fRecip255);
    AddVar("Night Sky", "Moon inner corona color", "Night sky: Moon inner corona color", ITimeOfDay::PARAM_NIGHSKY_MOON_INNERCORONA_COLOR, ITimeOfDay::TYPE_COLOR, 230.0f * fRecip255, 255.0f * fRecip255, 255.0f * fRecip255);
    AddVar("Night Sky", "Moon inner corona scale", "Night sky: Moon inner corona scale", ITimeOfDay::PARAM_NIGHSKY_MOON_INNERCORONA_SCALE, ITimeOfDay::TYPE_FLOAT, 0.499f, 0.0f, 2.0f);
    AddVar("Night Sky", "Moon outer corona color", "Night sky: Moon outer corona color", ITimeOfDay::PARAM_NIGHSKY_MOON_OUTERCORONA_COLOR, ITimeOfDay::TYPE_COLOR, 128.0f * fRecip255, 200.0f * fRecip255, 255.0f * fRecip255);
    AddVar("Night Sky", "Moon outer corona scale", "Night sky: Moon outer corona scale", ITimeOfDay::PARAM_NIGHSKY_MOON_OUTERCORONA_SCALE, ITimeOfDay::TYPE_FLOAT, 0.006f, 0.0f, 2.0f);

    AddVar("Night Sky Multiplier", "Horizon color", "Night sky: Horizon color multiplier", ITimeOfDay::PARAM_NIGHSKY_HORIZON_COLOR_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 0.0001f, 0.0f, 1.0f);
    AddVar("Night Sky Multiplier", "Zenith color", "Night sky: Zenith color multiplier", ITimeOfDay::PARAM_NIGHSKY_ZENITH_COLOR_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 0.00002f, 0.0f, 1.0f);
    AddVar("Night Sky Multiplier", "Moon color", "Night sky: Moon color multiplier", ITimeOfDay::PARAM_NIGHSKY_MOON_COLOR_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 0.01f, 0.0f, 1.0f);
    AddVar("Night Sky Multiplier", "Moon inner corona color", "Night sky: Moon inner corona color multiplier", ITimeOfDay::PARAM_NIGHSKY_MOON_INNERCORONA_COLOR_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 0.0001f, 0.0f, 1.0f);
    AddVar("Night Sky Multiplier", "Moon outer corona color", "Night sky: Moon outer corona color multiplier", ITimeOfDay::PARAM_NIGHSKY_MOON_OUTERCORONA_COLOR_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 0.00005f, 0.0f, 1.0f);

    AddVar("Cloud Shading", "Sun contribution", "Cloud shading: Sun light multiplier", ITimeOfDay::PARAM_CLOUDSHADING_SUNLIGHT_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 1.96f, 0.0f, 16.0f);
    AddVar("Cloud Shading", "Sun custom color", "Cloud shading: Sun custom color", ITimeOfDay::PARAM_CLOUDSHADING_SUNLIGHT_CUSTOM_COLOR, ITimeOfDay::TYPE_COLOR, 215.0f * fRecip255, 200.0f * fRecip255, 170.0f * fRecip255);
    AddVar("Cloud Shading", "Sun custom color multiplier", "Cloud shading: Sun custom color multiplier", ITimeOfDay::PARAM_CLOUDSHADING_SUNLIGHT_CUSTOM_COLOR_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 16.0f);
    AddVar("Cloud Shading", "Sun custom color influence", "Cloud shading: Sun custom color influence", ITimeOfDay::PARAM_CLOUDSHADING_SUNLIGHT_CUSTOM_COLOR_INFLUENCE, ITimeOfDay::TYPE_FLOAT, 0.0f,  0.0f, 1.0f);

    AddVar("Sun Rays Effect", "", "Sun shafts visibility", ITimeOfDay::PARAM_SUN_SHAFTS_VISIBILITY, ITimeOfDay::TYPE_FLOAT, 0.25f,  0.0f, 1.0f);
    AddVar("Sun Rays Effect", "", "Sun rays visibility", ITimeOfDay::PARAM_SUN_RAYS_VISIBILITY, ITimeOfDay::TYPE_FLOAT, 1.0f,  0.0f, 10.0f);
    AddVar("Sun Rays Effect", "", "Sun rays attenuation", ITimeOfDay::PARAM_SUN_RAYS_ATTENUATION, ITimeOfDay::TYPE_FLOAT, 5.0f,  0.0f, 10.0f);
    AddVar("Sun Rays Effect", "", "Sun rays suncolor influence", ITimeOfDay::PARAM_SUN_RAYS_SUNCOLORINFLUENCE, ITimeOfDay::TYPE_FLOAT, 1.0f,  0.0f, 1.0f);
    AddVar("Sun Rays Effect", "", "Sun rays custom color", ITimeOfDay::PARAM_SUN_RAYS_CUSTOMCOLOR, ITimeOfDay::TYPE_COLOR, 1.0f, 1.0f, 1.0f);

    AddVar("HDR", "", "Film curve shoulder scale", ITimeOfDay::PARAM_HDR_FILMCURVE_SHOULDER_SCALE, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 10.0f);
    AddVar("HDR", "", "Film curve midtones scale", ITimeOfDay::PARAM_HDR_FILMCURVE_LINEAR_SCALE, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 10.0f);
    AddVar("HDR", "", "Film curve toe scale", ITimeOfDay::PARAM_HDR_FILMCURVE_TOE_SCALE, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 10.0f);
    AddVar("HDR", "", "Film curve whitepoint", ITimeOfDay::PARAM_HDR_FILMCURVE_WHITEPOINT, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 10.0f);
    AddVar("HDR", "", "Saturation", ITimeOfDay::PARAM_HDR_COLORGRADING_COLOR_SATURATION, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 2.0f);
    AddVar("HDR", "", "Color balance", ITimeOfDay::PARAM_HDR_COLORGRADING_COLOR_BALANCE, ITimeOfDay::TYPE_COLOR, 1.0f, 1.0f, 1.0f);
    AddVar("HDR", "(Dep) Scene key", "Scene key", ITimeOfDay::PARAM_HDR_EYEADAPTATION_SCENEKEY, ITimeOfDay::TYPE_FLOAT, 0.18f, 0.0f, 1.0f);
    AddVar("HDR", "(Dep) Min exposure", "Min exposure", ITimeOfDay::PARAM_HDR_EYEADAPTATION_MIN_EXPOSURE, ITimeOfDay::TYPE_FLOAT, 0.36f, 0.0f, 10.0f);
    AddVar("HDR", "(Dep) Max exposure", "Max exposure", ITimeOfDay::PARAM_HDR_EYEADAPTATION_MAX_EXPOSURE, ITimeOfDay::TYPE_FLOAT, 2.8f, 0.0f, 10.0f);
    AddVar("HDR", "", "EV Min", ITimeOfDay::PARAM_HDR_EYEADAPTATION_EV_MIN, ITimeOfDay::TYPE_FLOAT, 4.5f, -10.0f, 20.0f);
    AddVar("HDR", "", "EV Max", ITimeOfDay::PARAM_HDR_EYEADAPTATION_EV_MAX, ITimeOfDay::TYPE_FLOAT, 17.0f, -10.0f, 20.0f);
    AddVar("HDR", "", "EV Auto compensation", ITimeOfDay::PARAM_HDR_EYEADAPTATION_EV_AUTO_COMPENSATION, ITimeOfDay::TYPE_FLOAT, 1.5f, -5.0f, 5.0f);
    AddVar("HDR", "", "Bloom amount", ITimeOfDay::PARAM_HDR_BLOOM_AMOUNT, ITimeOfDay::TYPE_FLOAT, 0.1f, 0.0f, 10.0f);

    AddVar("Filters", "Grain", "Filters: grain", ITimeOfDay::PARAM_COLORGRADING_FILTERS_GRAIN, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 8.0f);   // deprecated
    AddVar("Filters", "Photofilter color", "Filters: photofilter color", ITimeOfDay::PARAM_COLORGRADING_FILTERS_PHOTOFILTER_COLOR, ITimeOfDay::TYPE_COLOR, 0.952f, 0.517f, 0.09f);   // deprecated
    AddVar("Filters", "Photofilter density", "Filters: photofilter density", ITimeOfDay::PARAM_COLORGRADING_FILTERS_PHOTOFILTER_DENSITY, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 1.0f); // deprecated

    AddVar("Depth Of Field", "Focus range", "Dof: focus range", ITimeOfDay::PARAM_COLORGRADING_DOF_FOCUSRANGE, ITimeOfDay::TYPE_FLOAT, 1000.0f, 0.0f, 10000.0f);
    AddVar("Depth Of Field", "Blur amount", "Dof: blur amount", ITimeOfDay::PARAM_COLORGRADING_DOF_BLURAMOUNT, ITimeOfDay::TYPE_FLOAT, 0.0f, 0.0f, 1.0f);
    
    AddVar("Advanced", "", "Ocean fog color", ITimeOfDay::PARAM_OCEANFOG_COLOR, ITimeOfDay::TYPE_COLOR, 29.0f * fRecip255, 102.0f * fRecip255, 141.0f * fRecip255);
    AddVar("Advanced", "", "Ocean fog color multiplier", ITimeOfDay::PARAM_OCEANFOG_COLOR_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 1.0f);
    AddVar("Advanced", "", "Ocean fog density", ITimeOfDay::PARAM_OCEANFOG_DENSITY, ITimeOfDay::TYPE_FLOAT, 0.2f, 0.0f, 1.0f);

    AddVar("Advanced", "", "Static skybox multiplier", ITimeOfDay::PARAM_SKYBOX_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 1.0f);

    const float arrDepthConstBias[] = {1.0f, 1.0f, 1.9f, 3.0f, 2.0f, 2.0f, 2.0f, 2.0f};
    const float arrDepthSlopeBias[] = {4.0f, 2.0f, 0.24f, 0.24f, 0.5f, 0.5f, 0.5f, 0.5f};
    AddVar("Shadows", "", "Cascade 0: Bias", ITimeOfDay::PARAM_SHADOWSC0_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthConstBias[0], 0.0f, 10.0f);
    AddVar("Shadows", "", "Cascade 0: Slope Bias", ITimeOfDay::PARAM_SHADOWSC0_SLOPE_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthSlopeBias[0], 0.0f, 500.0f);
    AddVar("Shadows", "", "Cascade 1: Bias", ITimeOfDay::PARAM_SHADOWSC1_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthConstBias[1], 0.0f, 10.0f);
    AddVar("Shadows", "", "Cascade 1: Slope Bias", ITimeOfDay::PARAM_SHADOWSC1_SLOPE_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthSlopeBias[1], 0.0f, 500.0f);
    AddVar("Shadows", "", "Cascade 2: Bias", ITimeOfDay::PARAM_SHADOWSC2_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthConstBias[2], 0.0f, 10.0f);
    AddVar("Shadows", "", "Cascade 2: Slope Bias", ITimeOfDay::PARAM_SHADOWSC2_SLOPE_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthSlopeBias[2], 0.0f, 500.0f);
    AddVar("Shadows", "", "Cascade 3: Bias", ITimeOfDay::PARAM_SHADOWSC3_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthConstBias[3], 0.0f, 10.0f);
    AddVar("Shadows", "", "Cascade 3: Slope Bias", ITimeOfDay::PARAM_SHADOWSC3_SLOPE_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthSlopeBias[3], 0.0f, 500.0f);
    AddVar("Shadows", "", "Cascade 4: Bias", ITimeOfDay::PARAM_SHADOWSC4_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthConstBias[4], 0.0f, 10.0f);
    AddVar("Shadows", "", "Cascade 4: Slope Bias", ITimeOfDay::PARAM_SHADOWSC4_SLOPE_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthSlopeBias[4], 0.0f, 500.0f);
    AddVar("Shadows", "", "Cascade 5: Bias", ITimeOfDay::PARAM_SHADOWSC5_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthConstBias[5], 0.0f, 10.0f);
    AddVar("Shadows", "", "Cascade 5: Slope Bias", ITimeOfDay::PARAM_SHADOWSC5_SLOPE_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthSlopeBias[5], 0.0f, 500.0f);
    AddVar("Shadows", "", "Cascade 6: Bias", ITimeOfDay::PARAM_SHADOWSC6_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthConstBias[6], 0.0f, 10.0f);
    AddVar("Shadows", "", "Cascade 6: Slope Bias", ITimeOfDay::PARAM_SHADOWSC6_SLOPE_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthSlopeBias[6], 0.0f, 500.0f);
    AddVar("Shadows", "", "Cascade 7: Bias", ITimeOfDay::PARAM_SHADOWSC7_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthConstBias[7], 0.0f, 10.0f);
    AddVar("Shadows", "", "Cascade 7: Slope Bias", ITimeOfDay::PARAM_SHADOWSC7_SLOPE_BIAS, ITimeOfDay::TYPE_FLOAT, arrDepthSlopeBias[7], 0.0f, 500.0f);

    AddVar("Shadows", "", "Shadow jittering", ITimeOfDay::PARAM_SHADOW_JITTERING, ITimeOfDay::TYPE_FLOAT, 2.5f, 0.f, 10.f);

    AddVar("Obsolete", "", "HDR dynamic power factor", ITimeOfDay::PARAM_HDR_DYNAMIC_POWER_FACTOR, ITimeOfDay::TYPE_FLOAT, 0.0f, -4.0f, 4.0f);
    AddVar("Obsolete", "", "Sky brightening (terrain occlusion)", ITimeOfDay::PARAM_TERRAIN_OCCL_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 0.3f, 0.f, 1.f);
    AddVar("Obsolete", "", "Sun color multiplier", ITimeOfDay::PARAM_SUN_COLOR_MULTIPLIER, ITimeOfDay::TYPE_FLOAT, 1.0f, 0.0f, 16.0f);
}

void CEnvironmentPreset::Serialize(Serialization::IArchive& ar)
{
    for (size_t i = 0; i < ITimeOfDay::PARAM_TOTAL; ++i)
    {
        ar(m_vars[i], "var");
    }
}

void CEnvironmentPreset::Update(float t)
{
    for (size_t i = 0; i < ITimeOfDay::PARAM_TOTAL; ++i)
    {
        m_vars[i].Update(t);
    }
}

CTimeOfDayVariable* CEnvironmentPreset::GetVar(const char* varName)
{
    for (size_t i = 0; i < ITimeOfDay::PARAM_TOTAL; ++i)
    {
        if (strcmp(m_vars[i].GetName(), varName) == 0)
        {
            return &m_vars[i];
        }
    }
    return NULL;
}

bool CEnvironmentPreset::InterpolateVarInRange(ITimeOfDay::ETimeOfDayParamID id, float fMin, float fMax, unsigned int nCount, Vec3* resultArray) const
{
    const float fdx = 1.0f / float(nCount);
    float normX = 0.0f;
    for (unsigned int i = 0; i < nCount; ++i)
    {
        const float time = Lerp(fMin, fMax, normX);
        resultArray[i] = m_vars[id].GetInterpolatedAt(time);
        normX += fdx;
    }

    return true;
}

void CEnvironmentPreset::AddVar(const char* group, const char* displayName, const char* name, ITimeOfDay::ETimeOfDayParamID nParamId, ITimeOfDay::EVariableType type, float defVal0, float defVal1, float defVal2)
{
    CTimeOfDayVariable& var = m_vars[nParamId];
    var.Init(group, displayName, name, nParamId, type, defVal0, defVal1, defVal2);
}
