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
#include <AzCore/Debug/Trace.h>
#include "TimeOfDay.h"
#include "Ocean.h"
#include "IPostEffectGroup.h"
#include <ISplines.h>
#include <IRemoteCommand.h>
#include "EnvironmentPreset.h"
#include <Environment/OceanEnvironmentBus.h>

// Maximum number of minutes in a day converted to a float value
static const float s_maxTime = ((24 * 60) - 1) / 60.0f;

class CBezierSplineFloat
    : public spline::CBaseSplineInterpolator<float, spline::BezierSpline<float> >
{
public:
    float m_fMinValue;
    float m_fMaxValue;

    virtual int GetNumDimensions() { return 1; };

    virtual void Interpolate(float time, ValueType& value)
    {
        value_type v;
        if (interpolate(time, v))
        {
            ToValueType(v, value);
        }
        // Clamp values
        //value[0] = clamp_tpl(value[0],m_fMinValue,m_fMaxValue);
    }

    //////////////////////////////////////////////////////////////////////////
    void SerializeSpline(XmlNodeRef& node, bool bLoading)
    {
        if (bLoading)
        {
            string keystr = node->getAttr("Keys");

            resize(0);
            int curPos = 0;
            uint32 nKeys = 0;
            string key = keystr.Tokenize(",", curPos);
            while (!key.empty())
            {
                ++nKeys;
                key = keystr.Tokenize(",", curPos);
            }
            reserve_keys(nKeys);

            curPos = 0;
            key = keystr.Tokenize(",", curPos);
            while (!key.empty())
            {
                float time, v;
                int flags = 0;

                int res = azsscanf(key, "%g:%g:%d", &time, &v, &flags);
                if (res != 3)
                {
                    res = azsscanf(key, "%g:%g", &time, &v);
                    if (res != 2)
                    {
                        continue;
                    }
                }
                ValueType val;
                val[0] = v;
                int keyIndex = InsertKey(time, val);
                SetKeyFlags(keyIndex, flags);
                key = keystr.Tokenize(",", curPos);
            }
        }
        else
        {
            string keystr;
            string skey;
            for (int i = 0; i < num_keys(); i++)
            {
                skey.Format("%g:%g:%d,", key(i).time, key(i).value, key(i).flags);
                keystr += skey;
            }
            node->setAttr("Keys", keystr);
        }
    }
};

//////////////////////////////////////////////////////////////////////////
class CBezierSplineVec3
    : public spline::CBaseSplineInterpolator<Vec3, spline::BezierSpline<Vec3> >
{
public:
    virtual int GetNumDimensions() { return 3; };

    virtual void Interpolate(float time, ValueType& value)
    {
        value_type v;
        if (interpolate(time, v))
        {
            ToValueType(v, value);
        }
        // Clamp for colors.
        //value[0] = clamp_tpl(value[0],0.0f,1.0f);
        //value[1] = clamp_tpl(value[1],0.0f,1.0f);
        //value[2] = clamp_tpl(value[2],0.0f,1.0f);
    }

    //////////////////////////////////////////////////////////////////////////
    void SerializeSpline(XmlNodeRef& node, bool bLoading)
    {
        if (bLoading)
        {
            string keystr = node->getAttr("Keys");

            resize(0);
            int curPos = 0;
            uint32 nKeys = 0;
            string key = keystr.Tokenize(",", curPos);
            while (!key.empty())
            {
                ++nKeys;
                key = keystr.Tokenize(",", curPos);
            }
            reserve_keys(nKeys);

            curPos = 0;
            key = keystr.Tokenize(",", curPos);
            while (!key.empty())
            {
                float time, val0, val1, val2;
                int flags = 0;
                int res = azsscanf(key, "%g:(%g:%g:%g):%d,", &time, &val0, &val1, &val2, &flags);
                if (res != 5)
                {
                    res = azsscanf(key, "%g:(%g:%g:%g),", &time, &val0, &val1, &val2);
                    if (res != 4)
                    {
                        continue;
                    }
                }
                ValueType val;
                val[0] = val0;
                val[1] = val1;
                val[2] = val2;
                int keyIndex = InsertKey(time, val);
                SetKeyFlags(keyIndex, flags);
                key = keystr.Tokenize(",", curPos);
            }
        }
        else
        {
            string keystr;
            string skey;
            for (int i = 0; i < num_keys(); i++)
            {
                skey.Format("%g:(%g:%g:%g):%d,", key(i).time, key(i).value.x, key(i).value.y, key(i).value.z, key(i).flags);
                keystr += skey;
            }
            node->setAttr("Keys", keystr);
        }
    }

    void ClampValues([[maybe_unused]] float fMinValue, [[maybe_unused]] float fMaxValue)
    {
        for (int i = 0, nkeys = num_keys(); i < nkeys; i++)
        {
            ValueType val;
            if (GetKeyValue(i, val))
            {
                //val[0] = clamp_tpl(val[0],fMinValue,fMaxValue);
                //val[1] = clamp_tpl(val[1],fMinValue,fMaxValue);
                //val[2] = clamp_tpl(val[2],fMinValue,fMaxValue);
                SetKeyValue(i, val);
            }
        }
    }
};

//////////////////////////////////////////////////////////////////////////
CTimeOfDay::CTimeOfDay()
{
    m_pTimer = 0;
    SetTimer(gEnv->pTimer);
    m_fTime = 12;
    m_fEditorTime = 12;
    m_bEditMode = false;

    m_advancedInfo.fAnimSpeed = 0;
    m_advancedInfo.fStartTime = 0;
    m_advancedInfo.fEndTime = 24;
    m_fHDRMultiplier = 1.f;
    m_pTimeOfDaySpeedCVar = gEnv->pConsole->GetCVar("e_TimeOfDaySpeed");
    m_pUpdateCallback = 0;
    m_sunRotationLatitude = 0;
    m_sunRotationLongitude = 0;
    m_bPaused = false;
    m_bSunLinkedToTOD = true;
    memset(m_vars, 0, sizeof(SVariableInfo) * ITimeOfDay::PARAM_TOTAL);

    // fill local var list so, sandbox can access var list without level being loaded

    // Cryengine supports the notion of environment presets which are set in code that is currently not
    // in lumberyard. Therefore, create a default preset here that is used as the only one.
    m_defaultPreset = new CEnvironmentPreset;
    for (int i = 0; i < PARAM_TOTAL; ++i)
    {
        const CTimeOfDayVariable* presetVar = m_defaultPreset->GetVar((ETimeOfDayParamID)i);
        SVariableInfo& var = m_vars[i];

        var.name = presetVar->GetName();
        var.displayName = presetVar->GetDisplayName();
        var.group = presetVar->GetGroupName();

        var.nParamId = i;
        var.type = presetVar->GetType();
        var.pInterpolator = NULL;

        Vec3 presetVal = presetVar->GetValue();
        var.fValue[0] = presetVal.x;
        const EVariableType varType = presetVar->GetType();
        if (varType == TYPE_FLOAT)
        {
            var.fValue[1] = presetVar->GetMinValue();
            var.fValue[2] = presetVar->GetMaxValue();
        }
        else if (varType == TYPE_COLOR)
        {
            var.fValue[1] = presetVal.y;
            var.fValue[2] = presetVal.z;
        }
    }

    m_pCurrentPreset = m_defaultPreset;

    ResetVariables();
}

//////////////////////////////////////////////////////////////////////////
CTimeOfDay::~CTimeOfDay()
{
    for (int i = 0; i < GetVariableCount(); i++)
    {
        switch (m_vars[i].type)
        {
        case TYPE_FLOAT:
            delete (CBezierSplineFloat*)(m_vars[i].pInterpolator);
            break;
        case TYPE_COLOR:
            delete (CBezierSplineVec3*)(m_vars[i].pInterpolator);
            break;
        }
    }

    SAFE_DELETE(m_defaultPreset);
}

void CTimeOfDay::SetTimer(ITimer* pTimer)
{
    AZ_Assert(pTimer, "Null pointer access in CTimeOfDay::SetTimer!");
    m_pTimer = pTimer;

    // Update timer for ocean also - Craig
    COcean::SetTimer(pTimer);
}

ITimeOfDay::SVariableInfo& CTimeOfDay::GetVar(ETimeOfDayParamID id)
{
    AZ_Assert(id == m_vars[id].nParamId, "Wrong ID in CTimeOfDay::GetVar!");
    return(m_vars[ id ]);
}

//////////////////////////////////////////////////////////////////////////
bool CTimeOfDay::GetVariableInfo(int nIndex, SVariableInfo& varInfo) const
{
    if (nIndex < 0 || nIndex >= GetVariableCount())
    {
        return false;
    }

    varInfo = m_vars[nIndex];
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::SetVariableValue(int nIndex, float fValue[3])
{
    if (nIndex < 0 || nIndex >= GetVariableCount())
    {
        return;
    }

    m_vars[nIndex].fValue[0] = fValue[0];
    m_vars[nIndex].fValue[1] = fValue[1];
    m_vars[nIndex].fValue[2] = fValue[2];
}
//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::ResetVariables()
{
    if (!m_pCurrentPreset)
    {
        return;
    }

    m_pCurrentPreset->ResetVariables();

    m_varsMap.clear();
    for (int i = 0; i < PARAM_TOTAL; ++i)
    {
        const CTimeOfDayVariable* presetVar = m_pCurrentPreset->GetVar((ETimeOfDayParamID)i);
        SVariableInfo& var = m_vars[i];

        var.name = presetVar->GetName();
        var.displayName = presetVar->GetDisplayName();
        var.group = presetVar->GetGroupName();

        var.nParamId = i;
        var.type = presetVar->GetType();
        SAFE_DELETE(var.pInterpolator);

        Vec3 presetVal = presetVar->GetValue();
        var.fValue[0] = presetVal.x;
        const EVariableType varType = presetVar->GetType();
        if (varType == TYPE_FLOAT)
        {
            var.fValue[1] = presetVar->GetMinValue();
            var.fValue[2] = presetVar->GetMaxValue();

            CBezierSplineFloat* pSpline = new CBezierSplineFloat;
            pSpline->m_fMinValue = var.fValue[1];
            pSpline->m_fMaxValue = var.fValue[2];
            pSpline->reserve_keys(2);
            pSpline->InsertKeyFloat(0, var.fValue[0]);
            pSpline->InsertKeyFloat(1, var.fValue[0]);
            var.pInterpolator = pSpline;
        }
        else if (varType == TYPE_COLOR)
        {
            var.fValue[1] = presetVal.y;
            var.fValue[2] = presetVal.z;

            CBezierSplineVec3* pSpline = new CBezierSplineVec3;
            pSpline->reserve_keys(2);
            pSpline->InsertKeyFloat3(0, var.fValue);
            pSpline->InsertKeyFloat3(1, var.fValue);
            var.pInterpolator = pSpline;
        }

        m_varsMap[var.name] = var.nParamId;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::SetEnvironmentSettings(const SEnvironmentInfo& envInfo)
{
    m_sunRotationLongitude = envInfo.sunRotationLongitude;
    m_sunRotationLatitude = envInfo.sunRotationLatitude;
    m_bSunLinkedToTOD = envInfo.bSunLinkedToTOD;
}


//////////////////////////////////////////////////////////////////////////
// Time of day is specified in hours.
void CTimeOfDay::SetTime(float fHour, bool bForceUpdate, bool bEnvUpdate)
{
    // set new time
    m_fTime = fHour;

    // Change time variable.
    Cry3DEngineBase::GetCVars()->e_TimeOfDay = m_fTime;

    Update(true, bForceUpdate, bEnvUpdate);

    gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_TIME_OF_DAY_SET, 0, 0);
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::SetSunPos(float longitude, float latitude)
{
    m_sunRotationLongitude = longitude;
    m_sunRotationLatitude = latitude;
}


//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::Update(bool bInterpolate, bool bForceUpdate, bool bEnvUpdate)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_3DENGINE);

    if (bInterpolate)
    {
        if (m_pUpdateCallback)
        {
            m_pUpdateCallback->BeginUpdate();
        }

        // normalized time for interpolation
        float t = m_fTime / s_maxTime;

        // interpolate all values
        for (uint32 i = 0; i < static_cast<uint32>(GetVariableCount()); i++)
        {
            SVariableInfo& var = m_vars[i];
            if (var.pInterpolator)
            {
                if (var.pInterpolator->GetNumDimensions() == 1)
                {
                    var.pInterpolator->InterpolateFloat(t, var.fValue[0]);
                }
                else if (var.pInterpolator->GetNumDimensions() == 3)
                {
                    var.pInterpolator->InterpolateFloat3(t, var.fValue);
                }

                if (m_pUpdateCallback)
                {
                    const int dim = var.pInterpolator->GetNumDimensions();
                    float customValues[3] = {0, 0, 0};
                    float blendWeight = 0;
                    if (m_pUpdateCallback->GetCustomValue((ETimeOfDayParamID) var.nParamId, dim, customValues, blendWeight))
                    {
                        AZ_Assert(blendWeight >= 0 && blendWeight <= 1, "blendweight outside 0 and 1 in CTimeOfDay::Update!");
                        blendWeight = clamp_tpl(blendWeight, 0.0f, 1.0f);
                        for (int j = 0; j < dim; ++j)
                        {
                            var.fValue[j] = var.fValue[j] + blendWeight * (customValues[j] - var.fValue[j]);
                        }
                    }
                }

                switch (var.type)
                {
                case TYPE_FLOAT:
                {
                    var.fValue[0] = clamp_tpl(var.fValue[0], var.fValue[1], var.fValue[2]);
                    if (fabs(var.fValue[0]) < 1e-10f)
                    {
                        var.fValue[0] = 0.0f;
                    }
                    break;
                }
                case TYPE_COLOR:
                {
                    var.fValue[0] = clamp_tpl(var.fValue[0], 0.0f, 1.0f);
                    var.fValue[1] = clamp_tpl(var.fValue[1], 0.0f, 1.0f);
                    var.fValue[2] = clamp_tpl(var.fValue[2], 0.0f, 1.0f);
                    break;
                }
                default:
                {
                    AZ_Error("TimeOfDay", false, "Invalid TimeOfDay object during CTimeOfDay::Update!");
                }
                }
            }
        }

        if (m_pUpdateCallback)
        {
            m_pUpdateCallback->EndUpdate();
        }
    }

    // update environment lighting according to new interpolated values
    if (bEnvUpdate)
    {
        UpdateEnvLighting(bForceUpdate);
    }
}


Vec3 ConvertIlluminanceToLightColor(float illuminance, Vec3 colorRGB)
{
    illuminance /= RENDERER_LIGHT_UNIT_SCALE;

    ColorF color(colorRGB);
    color.adjust_luminance(illuminance);

    // Divide by PI as this is not done in the BRDF at the moment
    Vec3 finalColor = color.toVec3() / gf_PI;

    return finalColor;
}
//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::UpdateEnvLighting(bool forceUpdate)
{
    C3DEngine* p3DEngine((C3DEngine*)gEnv->p3DEngine);
    IRenderer* pRenderer(gEnv->pRenderer);
    IPostEffectGroup* postEffectGroup = p3DEngine->GetPostEffectBaseGroup();
    const float fRecip255 = 1.0f / 255.0f;

    bool bHDRModeEnabled = false;
    pRenderer->EF_Query(EFQ_HDRModeEnabled, bHDRModeEnabled);
    if (bHDRModeEnabled)
    {
        const Vec3 vEyeAdaptationParams(GetVar(PARAM_HDR_EYEADAPTATION_EV_MIN).fValue[ 0 ],
            GetVar(PARAM_HDR_EYEADAPTATION_EV_MAX).fValue[ 0 ],
            GetVar(PARAM_HDR_EYEADAPTATION_EV_AUTO_COMPENSATION).fValue[ 0 ]);
        p3DEngine->SetGlobalParameter(E3DPARAM_HDR_EYEADAPTATION_PARAMS, vEyeAdaptationParams);

        const Vec3 vEyeAdaptationParamsLegacy(GetVar(PARAM_HDR_EYEADAPTATION_SCENEKEY).fValue[ 0 ],
            GetVar(PARAM_HDR_EYEADAPTATION_MIN_EXPOSURE).fValue[ 0 ],
            GetVar(PARAM_HDR_EYEADAPTATION_MAX_EXPOSURE).fValue[ 0 ]);
        p3DEngine->SetGlobalParameter(E3DPARAM_HDR_EYEADAPTATION_PARAMS_LEGACY, vEyeAdaptationParamsLegacy);

        float fHDRShoulderScale(GetVar(PARAM_HDR_FILMCURVE_SHOULDER_SCALE).fValue[ 0 ]);
        float fHDRMidtonesScale(GetVar(PARAM_HDR_FILMCURVE_LINEAR_SCALE).fValue[ 0 ]);
        float fHDRToeScale(GetVar(PARAM_HDR_FILMCURVE_TOE_SCALE).fValue[ 0 ]);
        float fHDRWhitePoint(GetVar(PARAM_HDR_FILMCURVE_WHITEPOINT).fValue[ 0 ]);

        p3DEngine->SetGlobalParameter(E3DPARAM_HDR_FILMCURVE_SHOULDER_SCALE, Vec3(fHDRShoulderScale, 0, 0));
        p3DEngine->SetGlobalParameter(E3DPARAM_HDR_FILMCURVE_LINEAR_SCALE, Vec3(fHDRMidtonesScale, 0, 0));
        p3DEngine->SetGlobalParameter(E3DPARAM_HDR_FILMCURVE_TOE_SCALE, Vec3(fHDRToeScale, 0, 0));
        p3DEngine->SetGlobalParameter(E3DPARAM_HDR_FILMCURVE_WHITEPOINT, Vec3(fHDRWhitePoint, 0, 0));

        float fHDRBloomAmount(GetVar(PARAM_HDR_BLOOM_AMOUNT).fValue[ 0 ]);
        postEffectGroup->SetParam("Global_User_HDRBloom", fHDRBloomAmount);

        float fHDRSaturation(GetVar(PARAM_HDR_COLORGRADING_COLOR_SATURATION).fValue[ 0 ]);
        p3DEngine->SetGlobalParameter(E3DPARAM_HDR_COLORGRADING_COLOR_SATURATION, Vec3(fHDRSaturation, 0, 0));

        Vec3 vColorBalance(GetVar(PARAM_HDR_COLORGRADING_COLOR_BALANCE).fValue[ 0 ],
            GetVar(PARAM_HDR_COLORGRADING_COLOR_BALANCE).fValue[ 1 ],
            GetVar(PARAM_HDR_COLORGRADING_COLOR_BALANCE).fValue[ 2 ]);
        p3DEngine->SetGlobalParameter(E3DPARAM_HDR_COLORGRADING_COLOR_BALANCE, vColorBalance);
    }

    pRenderer->SetShadowJittering(GetVar(PARAM_SHADOW_JITTERING).fValue[ 0 ]);

    float sunMultiplier(1.0f);
    float sunSpecMultiplier(GetVar(PARAM_SUN_SPECULAR_MULTIPLIER).fValue[ 0 ]);
    float fogMultiplier(GetVar(PARAM_FOG_COLOR_MULTIPLIER).fValue[ 0 ]);
    float fogMultiplier2(GetVar(PARAM_FOG_COLOR2_MULTIPLIER).fValue[ 0 ]);
    float fogMultiplierRadial(GetVar(PARAM_FOG_RADIAL_COLOR_MULTIPLIER).fValue[ 0 ]);
    float nightSkyHorizonMultiplier(GetVar(PARAM_NIGHSKY_HORIZON_COLOR_MULTIPLIER).fValue[ 0 ]);
    float nightSkyZenithMultiplier(GetVar(PARAM_NIGHSKY_ZENITH_COLOR_MULTIPLIER).fValue[ 0 ]);
    float nightSkyMoonMultiplier(GetVar(PARAM_NIGHSKY_MOON_COLOR_MULTIPLIER).fValue[ 0 ]);
    float nightSkyMoonInnerCoronaMultiplier(GetVar(PARAM_NIGHSKY_MOON_INNERCORONA_COLOR_MULTIPLIER).fValue[ 0 ]);
    float nightSkyMoonOuterCoronaMultiplier(GetVar(PARAM_NIGHSKY_MOON_OUTERCORONA_COLOR_MULTIPLIER).fValue[ 0 ]);

    // set sun position
    Vec3 sunPos;


    if (m_bSunLinkedToTOD)
    {
        float timeAng(((m_fTime + 12.0f) / s_maxTime) * gf_PI * 2.0f);
        float sunRot = gf_PI * (-m_sunRotationLatitude) / 180.0f;
        float longitude = 0.5f * gf_PI - gf_PI * m_sunRotationLongitude / 180.0f;

        Matrix33 a, b, c, m;

        a.SetRotationZ(timeAng);
        b.SetRotationX(longitude);
        c.SetRotationY(sunRot);

        m = a * b * c;
        sunPos = Vec3(0, 1, 0) * m;

        float h = sunPos.z;
        sunPos.z = sunPos.y;
        sunPos.y = -h;
    }
    else // when not linked, it behaves like the moon
    {
        float sunLati(-gf_PI + gf_PI* m_sunRotationLatitude / 180.0f);
        float sunLong(0.5f * gf_PI - gf_PI* m_sunRotationLongitude / 180.0f);

        float sinLon(sinf(sunLong));
        float cosLon(cosf(sunLong));
        float sinLat(sinf(sunLati));
        float cosLat(cosf(sunLati));

        sunPos = Vec3(sinLon * cosLat, sinLon * sinLat, cosLon);
    }


    Vec3 sunPosOrig = sunPos;

    // transition phase for sun/moon lighting
    AZ_Assert(p3DEngine->m_dawnStart <= p3DEngine->m_dawnEnd, "Invalid sun/moon transition parameters in CTimeOfDay::UpdateEnvLighting!");
    AZ_Assert(p3DEngine->m_duskStart <= p3DEngine->m_duskEnd, "Invalid sun/moon transition parameters in CTimeOfDay::UpdateEnvLighting!");
    AZ_Assert(p3DEngine->m_dawnEnd <= p3DEngine->m_duskStart, "Invalid sun/moon transition parameters in CTimeOfDay::UpdateEnvLighting!");
    float sunIntensityMultiplier(1.0f);

    // The ratio between night and day for adjusting luminance.   Day = 1, Night = 0, transitions = [0..1]
    float dayNightIndicator(1.0);   
    // The ratio [0..1] relative to high noon which represents maximum luminance.
    float midDayIndicator(1.0);

    if (m_fTime < p3DEngine->m_dawnStart || m_fTime >= p3DEngine->m_duskEnd)
    {   // Night time
        p3DEngine->GetGlobalParameter(E3DPARAM_NIGHSKY_MOON_DIRECTION, sunPos);
        sunIntensityMultiplier = 0.0f;
        midDayIndicator = 0.0f;
        dayNightIndicator = 0.0f;
    }
    else
    {   // Dawn, day and dusk time

        // Calculating the energy multiplier where mid-day sun is 1.0 and night is 0
        const float noonTime = 12.0f;
        float   dayTime = p3DEngine->m_duskEnd - p3DEngine->m_dawnStart;

        if (m_fTime <= noonTime)
        {
            float   dawnToNoon = noonTime - p3DEngine->m_dawnStart;
            midDayIndicator = (m_fTime - p3DEngine->m_dawnStart) / dawnToNoon;
        }
        else
        {
            float   noonToDusk = p3DEngine->m_duskEnd - noonTime;
            midDayIndicator = (m_fTime - noonTime) / noonToDusk;
        }
        midDayIndicator = cos( 0.5f * midDayIndicator * 3.14159265f );     // Converting to the cosine to represent energy distribution

        // Calculation of sun intensity and day-to-night indicator
        if (m_fTime < p3DEngine->m_dawnEnd)
        {
            // dawn
            AZ_Assert(p3DEngine->m_dawnStart < p3DEngine->m_dawnEnd, "Invalid sun/moon transition parameters in CTimeOfDay::UpdateEnvLighting!");
            float b(0.5f * (p3DEngine->m_dawnStart + p3DEngine->m_dawnEnd));
            if (m_fTime < b)
            {
                // fade out moon
                sunMultiplier *= (b - m_fTime) / (b - p3DEngine->m_dawnStart);
                sunIntensityMultiplier = 0.0f;
                p3DEngine->GetGlobalParameter(E3DPARAM_NIGHSKY_MOON_DIRECTION, sunPos);
            }
            else
            {
                // fade in sun
                float t((m_fTime - b) / (p3DEngine->m_dawnEnd - b));
                sunMultiplier *= t;
                sunIntensityMultiplier = t;
            }
            dayNightIndicator = (m_fTime - p3DEngine->m_dawnStart) / (p3DEngine->m_dawnEnd - p3DEngine->m_dawnStart);
        }
        else if (m_fTime < p3DEngine->m_duskStart)
        {
            // day
            dayNightIndicator = 1.0;
        }
        else if (m_fTime < p3DEngine->m_duskEnd)
        {
            // dusk
            AZ_Assert(p3DEngine->m_duskStart < p3DEngine->m_duskEnd, "Invalid sun/moon transition parameters in CTimeOfDay::UpdateEnvLighting!");
            float b(0.5f * (p3DEngine->m_duskStart + p3DEngine->m_duskEnd));
            if (m_fTime < b)
            {
                // fade out sun
                float t((b - m_fTime) / (b - p3DEngine->m_duskStart));
                sunMultiplier *= t;
                sunIntensityMultiplier = t;
            }
            else
            {
                // fade in moon
                sunMultiplier *= (m_fTime - b) / (p3DEngine->m_duskEnd - b);
                sunIntensityMultiplier = 0.0;
                p3DEngine->GetGlobalParameter(E3DPARAM_NIGHSKY_MOON_DIRECTION, sunPos);
            }

            dayNightIndicator = (p3DEngine->m_duskEnd - m_fTime) / (p3DEngine->m_duskEnd - p3DEngine->m_duskStart);
        }
    }

    sunIntensityMultiplier = max(GetVar(PARAM_SKYLIGHT_SUN_INTENSITY_MULTIPLIER).fValue[0], 0.0f);
    p3DEngine->SetGlobalParameter(E3DPARAM_DAY_NIGHT_INDICATOR, Vec3(dayNightIndicator, midDayIndicator, 0));

    p3DEngine->SetSunDir(sunPos);

    // set sun, sky, and fog color
    Vec3 sunColor(Vec3(GetVar(PARAM_SUN_COLOR).fValue[ 0 ],
            GetVar(PARAM_SUN_COLOR).fValue[ 1 ], GetVar(PARAM_SUN_COLOR).fValue[ 2 ]));
    float sunIntensityLux(GetVar(PARAM_SUN_INTENSITY).fValue[ 0 ] * sunMultiplier);
    p3DEngine->SetSunColor(ConvertIlluminanceToLightColor(sunIntensityLux, sunColor));

    p3DEngine->SetGlobalParameter(E3DPARAM_SUN_SPECULAR_MULTIPLIER, Vec3(sunSpecMultiplier, 0, 0));

    Vec3 fogColor(fogMultiplier* Vec3(GetVar(PARAM_FOG_COLOR).fValue[ 0 ],
            GetVar(PARAM_FOG_COLOR).fValue[ 1 ], GetVar(PARAM_FOG_COLOR).fValue[ 2 ]));
    p3DEngine->SetFogColor(fogColor);

    const Vec3 fogColor2 = fogMultiplier2 * Vec3(GetVar(PARAM_FOG_COLOR2).fValue[0], GetVar(PARAM_FOG_COLOR2).fValue[1], GetVar(PARAM_FOG_COLOR2).fValue[2]);
    p3DEngine->SetGlobalParameter(E3DPARAM_FOG_COLOR2, fogColor2);

    const Vec3 fogColorRadial = fogMultiplierRadial * Vec3(GetVar(PARAM_FOG_RADIAL_COLOR).fValue[0], GetVar(PARAM_FOG_RADIAL_COLOR).fValue[1], GetVar(PARAM_FOG_RADIAL_COLOR).fValue[2]);
    p3DEngine->SetGlobalParameter(E3DPARAM_FOG_RADIAL_COLOR, fogColorRadial);

    const Vec3 volFogHeightDensity = Vec3(GetVar(PARAM_VOLFOG_HEIGHT).fValue[0], GetVar(PARAM_VOLFOG_DENSITY).fValue[0], 0);
    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG_HEIGHT_DENSITY, volFogHeightDensity);

    const Vec3 volFogHeightDensity2 = Vec3(GetVar(PARAM_VOLFOG_HEIGHT2).fValue[0], GetVar(PARAM_VOLFOG_DENSITY2).fValue[0], 0);
    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG_HEIGHT_DENSITY2, volFogHeightDensity2);

    const Vec3 volFogGradientCtrl = Vec3(GetVar(PARAM_VOLFOG_HEIGHT_OFFSET).fValue[0], GetVar(PARAM_VOLFOG_RADIAL_SIZE).fValue[0], GetVar(PARAM_VOLFOG_RADIAL_LOBE).fValue[0]);
    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG_GRADIENT_CTRL, volFogGradientCtrl);

    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG_GLOBAL_DENSITY, Vec3(GetVar(PARAM_VOLFOG_GLOBAL_DENSITY).fValue[0], 0, GetVar(PARAM_VOLFOG_FINAL_DENSITY_CLAMP).fValue[0]));

    // set volumetric fog ramp
    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG_RAMP, Vec3(GetVar(PARAM_VOLFOG_RAMP_START).fValue[0], GetVar(PARAM_VOLFOG_RAMP_END).fValue[0], GetVar(PARAM_VOLFOG_RAMP_INFLUENCE).fValue[0]));

    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_RANGE, Vec3(GetVar(PARAM_VOLFOG_SHADOW_RANGE).fValue[0], 0, 0));
    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_DARKENING, Vec3(GetVar(PARAM_VOLFOG_SHADOW_DARKENING).fValue[0], GetVar(PARAM_VOLFOG_SHADOW_DARKENING_SUN).fValue[0], GetVar(PARAM_VOLFOG_SHADOW_DARKENING_AMBIENT).fValue[0]));

    // set HDR sky lighting properties
    Vec3 sunIntensity(sunIntensityMultiplier* Vec3(GetVar(PARAM_SKYLIGHT_SUN_INTENSITY).fValue[ 0 ],
            GetVar(PARAM_SKYLIGHT_SUN_INTENSITY).fValue[ 1 ], GetVar(PARAM_SKYLIGHT_SUN_INTENSITY).fValue[ 2 ]));

    Vec3 rgbWaveLengths(GetVar(PARAM_SKYLIGHT_WAVELENGTH_R).fValue[ 0 ],
        GetVar(PARAM_SKYLIGHT_WAVELENGTH_G).fValue[ 0 ], GetVar(PARAM_SKYLIGHT_WAVELENGTH_B).fValue[ 0 ]);

    p3DEngine->SetSkyLightParameters(sunPosOrig, sunIntensity, GetVar(PARAM_SKYLIGHT_KM).fValue[ 0 ],
        GetVar(PARAM_SKYLIGHT_KR).fValue[ 0 ], GetVar(PARAM_SKYLIGHT_G).fValue[ 0 ], rgbWaveLengths, forceUpdate);

    // set night sky color properties
    Vec3 nightSkyHorizonColor(nightSkyHorizonMultiplier* Vec3(GetVar(PARAM_NIGHSKY_HORIZON_COLOR).fValue[ 0 ],
            GetVar(PARAM_NIGHSKY_HORIZON_COLOR).fValue[ 1 ], GetVar(PARAM_NIGHSKY_HORIZON_COLOR).fValue[ 2 ]));
    p3DEngine->SetGlobalParameter(E3DPARAM_NIGHSKY_HORIZON_COLOR, nightSkyHorizonColor);

    Vec3 nightSkyZenithColor(nightSkyZenithMultiplier* Vec3(GetVar(PARAM_NIGHSKY_ZENITH_COLOR).fValue[ 0 ],
            GetVar(PARAM_NIGHSKY_ZENITH_COLOR).fValue[ 1 ], GetVar(PARAM_NIGHSKY_ZENITH_COLOR).fValue[ 2 ]));
    p3DEngine->SetGlobalParameter(E3DPARAM_NIGHSKY_ZENITH_COLOR, nightSkyZenithColor);

    float nightSkyZenithColorShift(GetVar(PARAM_NIGHSKY_ZENITH_SHIFT).fValue[ 0 ]);
    p3DEngine->SetGlobalParameter(E3DPARAM_NIGHSKY_ZENITH_SHIFT, Vec3(nightSkyZenithColorShift, 0, 0));

    float nightSkyStarIntensity(GetVar(PARAM_NIGHSKY_START_INTENSITY).fValue[ 0 ]);
    p3DEngine->SetGlobalParameter(E3DPARAM_NIGHSKY_STAR_INTENSITY, Vec3(nightSkyStarIntensity, 0, 0));

    Vec3 nightSkyMoonColor(nightSkyMoonMultiplier* Vec3(GetVar(PARAM_NIGHSKY_MOON_COLOR).fValue[ 0 ],
            GetVar(PARAM_NIGHSKY_MOON_COLOR).fValue[ 1 ], GetVar(PARAM_NIGHSKY_MOON_COLOR).fValue[ 2 ]));
    p3DEngine->SetGlobalParameter(E3DPARAM_NIGHSKY_MOON_COLOR, nightSkyMoonColor);

    Vec3 nightSkyMoonInnerCoronaColor(nightSkyMoonInnerCoronaMultiplier* Vec3(GetVar(PARAM_NIGHSKY_MOON_INNERCORONA_COLOR).fValue[ 0 ],
            GetVar(PARAM_NIGHSKY_MOON_INNERCORONA_COLOR).fValue[ 1 ], GetVar(PARAM_NIGHSKY_MOON_INNERCORONA_COLOR).fValue[ 2 ]));
    p3DEngine->SetGlobalParameter(E3DPARAM_NIGHSKY_MOON_INNERCORONA_COLOR, nightSkyMoonInnerCoronaColor);

    float nightSkyMoonInnerCoronaScale(GetVar(PARAM_NIGHSKY_MOON_INNERCORONA_SCALE).fValue[ 0 ]);
    p3DEngine->SetGlobalParameter(E3DPARAM_NIGHSKY_MOON_INNERCORONA_SCALE, Vec3(nightSkyMoonInnerCoronaScale, 0, 0));

    Vec3 nightSkyMoonOuterCoronaColor(nightSkyMoonOuterCoronaMultiplier* Vec3(GetVar(PARAM_NIGHSKY_MOON_OUTERCORONA_COLOR).fValue[ 0 ],
            GetVar(PARAM_NIGHSKY_MOON_OUTERCORONA_COLOR).fValue[ 1 ], GetVar(PARAM_NIGHSKY_MOON_OUTERCORONA_COLOR).fValue[ 2 ]));
    p3DEngine->SetGlobalParameter(E3DPARAM_NIGHSKY_MOON_OUTERCORONA_COLOR, nightSkyMoonOuterCoronaColor);

    float nightSkyMoonOuterCoronaScale(GetVar(PARAM_NIGHSKY_MOON_OUTERCORONA_SCALE).fValue[ 0 ]);
    p3DEngine->SetGlobalParameter(E3DPARAM_NIGHSKY_MOON_OUTERCORONA_SCALE, Vec3(nightSkyMoonOuterCoronaScale, 0, 0));

    // set sun shafts visibility and activate if required

    float fSunShaftsVis = GetVar(PARAM_SUN_SHAFTS_VISIBILITY).fValue[ 0 ];
    fSunShaftsVis = clamp_tpl<float>(fSunShaftsVis, 0.0f, 0.3f);
    float fSunRaysVis = GetVar(PARAM_SUN_RAYS_VISIBILITY).fValue[ 0 ];
    float fSunRaysAtten = GetVar(PARAM_SUN_RAYS_ATTENUATION).fValue[ 0 ];
    float fSunRaySunColInfluence = GetVar(PARAM_SUN_RAYS_SUNCOLORINFLUENCE).fValue[ 0 ];

    float* pSunRaysCustomColorVar = GetVar(PARAM_SUN_RAYS_CUSTOMCOLOR).fValue;
    Vec4 pSunRaysCustomColor = Vec4(pSunRaysCustomColorVar[0], pSunRaysCustomColorVar[1], pSunRaysCustomColorVar[2], 1.0f);

    postEffectGroup->SetParam("SunShafts_Active", (fSunShaftsVis > 0.05f || fSunRaysVis > 0.05f) ? 1.f : 0.f);
    postEffectGroup->SetParam("SunShafts_Amount", fSunShaftsVis);
    postEffectGroup->SetParam("SunShafts_RaysAmount", fSunRaysVis);
    postEffectGroup->SetParam("SunShafts_RaysAttenuation", fSunRaysAtten);
    postEffectGroup->SetParam("SunShafts_RaysSunColInfluence", fSunRaySunColInfluence);
    postEffectGroup->SetParam("SunShafts_RaysCustomColor", pSunRaysCustomColor);

    {
        const Vec3 cloudShadingMultipliers = Vec3(GetVar(PARAM_CLOUDSHADING_SUNLIGHT_MULTIPLIER).fValue[0], 0, 0);
        p3DEngine->SetGlobalParameter(E3DPARAM_CLOUDSHADING_MULTIPLIERS, cloudShadingMultipliers);

        const float cloudShadingCustomSunColorMult = GetVar(PARAM_CLOUDSHADING_SUNLIGHT_CUSTOM_COLOR_MULTIPLIER).fValue[0];
        const Vec3 cloudShadingCustomSunColor = cloudShadingCustomSunColorMult * Vec3(GetVar(PARAM_CLOUDSHADING_SUNLIGHT_CUSTOM_COLOR).fValue[0], GetVar(PARAM_CLOUDSHADING_SUNLIGHT_CUSTOM_COLOR).fValue[1], GetVar(PARAM_CLOUDSHADING_SUNLIGHT_CUSTOM_COLOR).fValue[2]);
        const float cloudShadingCustomSunColorInfluence = GetVar(PARAM_CLOUDSHADING_SUNLIGHT_CUSTOM_COLOR_INFLUENCE).fValue[0];

        IObjManager* pObjMan = p3DEngine->GetObjectManager();
        const Vec3 cloudShadingSunColor = pObjMan ? cloudShadingMultipliers.x* pObjMan->GetSunColor() : Vec3(0, 0, 0);

        p3DEngine->SetGlobalParameter(E3DPARAM_CLOUDSHADING_SUNCOLOR, cloudShadingSunColor + (cloudShadingCustomSunColor - cloudShadingSunColor) * cloudShadingCustomSunColorInfluence);
    }

    bool bHasOceanFeature = false;
    AZ::OceanFeatureToggleBus::BroadcastResult(bHasOceanFeature, &AZ::OceanFeatureToggleBus::Events::OceanComponentEnabled);
    if (!bHasOceanFeature)
    {
        // set ocean fog color multiplier
        const float oceanFogColorMultiplier = GetVar(PARAM_OCEANFOG_COLOR_MULTIPLIER).fValue[0];
        const Vec3 oceanFogColor = Vec3(GetVar(PARAM_OCEANFOG_COLOR).fValue[0], GetVar(PARAM_OCEANFOG_COLOR).fValue[1], GetVar(PARAM_OCEANFOG_COLOR).fValue[2]);
        p3DEngine->SetGlobalParameter(E3DPARAM_OCEANFOG_COLOR, oceanFogColor * oceanFogColorMultiplier);

        // legacy style: set ocean color density
        const float oceanFogColorDensity = GetVar(PARAM_OCEANFOG_DENSITY).fValue[0];
        p3DEngine->SetGlobalParameter(E3DPARAM_OCEANFOG_DENSITY, Vec3(oceanFogColorDensity, 0, 0));
    }

    // set skybox multiplier
    float skyBoxMultiplier(GetVar(PARAM_SKYBOX_MULTIPLIER).fValue[ 0 ] * m_fHDRMultiplier);
    p3DEngine->SetGlobalParameter(E3DPARAM_SKYBOX_MULTIPLIER, Vec3(skyBoxMultiplier, 0, 0));

    // Set color grading stuff
    float fValue = GetVar(PARAM_COLORGRADING_FILTERS_GRAIN).fValue[ 0 ];
    p3DEngine->SetGlobalParameter(E3DPARAM_COLORGRADING_FILTERS_GRAIN, Vec3(fValue, 0, 0));

    Vec4 pColor = Vec4(GetVar(PARAM_COLORGRADING_FILTERS_PHOTOFILTER_COLOR).fValue[ 0 ],
            GetVar(PARAM_COLORGRADING_FILTERS_PHOTOFILTER_COLOR).fValue[ 1 ],
            GetVar(PARAM_COLORGRADING_FILTERS_PHOTOFILTER_COLOR).fValue[ 2 ], 1.0f);
    p3DEngine->SetGlobalParameter(E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_COLOR, Vec3(pColor.x, pColor.y, pColor.z));
    fValue = GetVar(PARAM_COLORGRADING_FILTERS_PHOTOFILTER_DENSITY).fValue[ 0 ];
    p3DEngine->SetGlobalParameter(E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_DENSITY, Vec3(fValue, 0, 0));

    fValue = GetVar(PARAM_COLORGRADING_DOF_FOCUSRANGE).fValue[ 0 ];
    postEffectGroup->SetParam("Dof_Tod_FocusRange", fValue);

    fValue = GetVar(PARAM_COLORGRADING_DOF_BLURAMOUNT).fValue[ 0 ];
    postEffectGroup->SetParam("Dof_Tod_BlurAmount", fValue);

    const float arrDepthConstBias[MAX_SHADOW_CASCADES_NUM] =
    {
        GetVar(PARAM_SHADOWSC0_BIAS).fValue[ 0 ], GetVar(PARAM_SHADOWSC1_BIAS).fValue[ 0 ], GetVar(PARAM_SHADOWSC2_BIAS).fValue[ 0 ],  GetVar(PARAM_SHADOWSC3_BIAS).fValue[ 0 ],
        GetVar(PARAM_SHADOWSC4_BIAS).fValue[ 0 ], GetVar(PARAM_SHADOWSC5_BIAS).fValue[ 0 ], GetVar(PARAM_SHADOWSC6_BIAS).fValue[ 0 ],  GetVar(PARAM_SHADOWSC7_BIAS).fValue[ 0 ],
        2.0f, 2.0f, 2.0f, 2.0f,
        2.0f, 2.0f, 2.0f, 2.0f,
        2.0f, 2.0f, 2.0f, 2.0f
    };

    const float arrDepthSlopeBias[MAX_SHADOW_CASCADES_NUM] =
    {
        GetVar(PARAM_SHADOWSC0_SLOPE_BIAS).fValue[ 0 ], GetVar(PARAM_SHADOWSC1_SLOPE_BIAS).fValue[ 0 ], GetVar(PARAM_SHADOWSC2_SLOPE_BIAS).fValue[ 0 ], GetVar(PARAM_SHADOWSC3_SLOPE_BIAS).fValue[ 0 ],
        GetVar(PARAM_SHADOWSC4_SLOPE_BIAS).fValue[ 0 ], GetVar(PARAM_SHADOWSC5_SLOPE_BIAS).fValue[ 0 ], GetVar(PARAM_SHADOWSC6_SLOPE_BIAS).fValue[ 0 ], GetVar(PARAM_SHADOWSC7_SLOPE_BIAS).fValue[ 0 ],
        0.5f, 0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f, 0.5f
    };

    p3DEngine->SetShadowsCascadesBias(arrDepthConstBias, arrDepthSlopeBias);

    if (gEnv->IsEditing())
    {
        p3DEngine->SetRecomputeCachedShadows();
    }

    // set volumetric fog 2 params
    const Vec3 volFogCtrlParams = Vec3(GetVar(PARAM_VOLFOG2_RANGE).fValue[0], GetVar(PARAM_VOLFOG2_BLEND_FACTOR).fValue[0], GetVar(PARAM_VOLFOG2_BLEND_MODE).fValue[0]);
    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG2_CTRL_PARAMS, volFogCtrlParams);

    const Vec3 volFogScatteringParams = Vec3(GetVar(PARAM_VOLFOG2_INSCATTER).fValue[0], GetVar(PARAM_VOLFOG2_EXTINCTION).fValue[0], GetVar(PARAM_VOLFOG2_ANISOTROPIC).fValue[0]);
    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG2_SCATTERING_PARAMS, volFogScatteringParams);

    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG2_RAMP, Vec3(GetVar(PARAM_VOLFOG2_RAMP_START).fValue[0], GetVar(PARAM_VOLFOG2_RAMP_END).fValue[0], 0.0f));

    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG2_COLOR, Vec3(GetVar(PARAM_VOLFOG2_COLOR).fValue[0], GetVar(PARAM_VOLFOG2_COLOR).fValue[1], GetVar(PARAM_VOLFOG2_COLOR).fValue[2]));

    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG2_GLOBAL_DENSITY, Vec3(GetVar(PARAM_VOLFOG2_GLOBAL_DENSITY).fValue[0], GetVar(PARAM_VOLFOG2_FINAL_DENSITY_CLAMP).fValue[0], GetVar(PARAM_VOLFOG2_GLOBAL_FOG_VISIBILITY).fValue[0]));

    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG2_HEIGHT_DENSITY, Vec3(GetVar(PARAM_VOLFOG2_HEIGHT).fValue[0], GetVar(PARAM_VOLFOG2_DENSITY).fValue[0], GetVar(PARAM_VOLFOG2_ANISOTROPIC1).fValue[0]));

    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG2_HEIGHT_DENSITY2, Vec3(GetVar(PARAM_VOLFOG2_HEIGHT2).fValue[0], GetVar(PARAM_VOLFOG2_DENSITY2).fValue[0], GetVar(PARAM_VOLFOG2_ANISOTROPIC2).fValue[0]));

    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG2_COLOR1, Vec3(GetVar(PARAM_VOLFOG2_COLOR1).fValue[0], GetVar(PARAM_VOLFOG2_COLOR1).fValue[1], GetVar(PARAM_VOLFOG2_COLOR1).fValue[2]));

    p3DEngine->SetGlobalParameter(E3DPARAM_VOLFOG2_COLOR2, Vec3(GetVar(PARAM_VOLFOG2_COLOR2).fValue[0], GetVar(PARAM_VOLFOG2_COLOR2).fValue[1], GetVar(PARAM_VOLFOG2_COLOR2).fValue[2]));
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::SetAdvancedInfo(const SAdvancedInfo& advInfo)
{
    m_advancedInfo = advInfo;
    if (m_pTimeOfDaySpeedCVar->GetFVal() != m_advancedInfo.fAnimSpeed)
    {
        m_pTimeOfDaySpeedCVar->Set(m_advancedInfo.fAnimSpeed);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::GetAdvancedInfo(SAdvancedInfo& advInfo)
{
    advInfo = m_advancedInfo;
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::MigrateLegacyData(bool bSunIntensity, const XmlNodeRef& node)
{
    if (bSunIntensity)  // Convert sun intensity as specified up to CE 3.8.2 to illuminance
    {
        SVariableInfo& varSunIntensity = GetVar(PARAM_SUN_INTENSITY);
        SVariableInfo& varSunMult = GetVar(PARAM_SUN_COLOR_MULTIPLIER);
        SVariableInfo& varSunColor = GetVar(PARAM_SUN_COLOR);
        SVariableInfo& varHDRPower = GetVar(PARAM_HDR_DYNAMIC_POWER_FACTOR);

        int numKeys = varSunMult.pInterpolator->GetKeyCount();
        for (int key = 0; key < numKeys; ++key)
        {
            float time = varSunMult.pInterpolator->GetKeyTime(key);

            float sunMult, sunColor[3], hdrPower;
            varSunMult.pInterpolator->GetKeyValueFloat(key, sunMult);
            varSunColor.pInterpolator->InterpolateFloat3(time, sunColor);
            varHDRPower.pInterpolator->InterpolateFloat(time, hdrPower);

            const float HDRDynamicMultiplier = 2.0f;
            float hdrMult = powf(HDRDynamicMultiplier, hdrPower);
            float sunColorLum = sunColor[0] * 0.2126f + sunColor[1] * 0.7152f + sunColor[2] * 0.0722f;
            float sunIntensity = sunMult * sunColorLum * 10000.0f * gf_PI;

            varSunIntensity.pInterpolator->InsertKeyFloat(time, sunIntensity);
        }
    }

    // copy data from old node to new node if old nodes exist.
    // this needs to maintain compatibility from CE 3.8.2 to above.
    // this should be removed in the future.
    string paramOldNameFogAlbedoColor = "Volumetric fog 2: Fog albedo color";
    string paramOldNameAnisotropic = "Volumetric fog 2: Anisotropic factor";
    for (int i = 0; i < node->getChildCount(); i++)
    {
        XmlNodeRef varNode = node->getChild(i);
        const char* name = varNode->getAttr("Name");
        if (paramOldNameFogAlbedoColor.compare(name) == 0)
        {
            LoadValueFromXmlNode(PARAM_VOLFOG2_COLOR1, varNode);
            LoadValueFromXmlNode(PARAM_VOLFOG2_COLOR2, varNode);
            LoadValueFromXmlNode(PARAM_VOLFOG2_COLOR, varNode);
        }
        else if (paramOldNameAnisotropic.compare(name) == 0)
        {
            LoadValueFromXmlNode(PARAM_VOLFOG2_ANISOTROPIC1, varNode);
            LoadValueFromXmlNode(PARAM_VOLFOG2_ANISOTROPIC, varNode);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::LoadValueFromXmlNode(ETimeOfDayParamID destId, const XmlNodeRef& varNode)
{
    if (destId < 0 || destId >= PARAM_TOTAL)
    {
        return;
    }

    XmlNodeRef splineNode = varNode->findChild("Spline");
    SVariableInfo& var = GetVar(destId);
    switch (var.type)
    {
    case TYPE_FLOAT:
        varNode->getAttr("Value", var.fValue[0]);
        if (var.pInterpolator && splineNode != 0)
        {
            ((CBezierSplineFloat*)var.pInterpolator)->SerializeSpline(splineNode, true);
        }
        break;
    case TYPE_COLOR:
    {
        Vec3 v(var.fValue[0], var.fValue[1], var.fValue[2]);
        varNode->getAttr("Color", v);
        var.fValue[0] = v.x;
        var.fValue[1] = v.y;
        var.fValue[2] = v.z;

        if (var.pInterpolator && splineNode != 0)
        {
            CBezierSplineVec3* pBezierSpline = ((CBezierSplineVec3*)var.pInterpolator);
            pBezierSpline->SerializeSpline(splineNode, true);
            // Clamp colors in case too big colors are provided.
            pBezierSpline->ClampValues(-100, 100);
        }
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::Serialize(XmlNodeRef& node, bool bLoading)
{
    if (bLoading)
    {
        node->getAttr("Time", m_fTime);

        node->getAttr("TimeStart", m_advancedInfo.fStartTime);
        node->getAttr("TimeEnd", m_advancedInfo.fEndTime);
        node->getAttr("TimeAnimSpeed", m_advancedInfo.fAnimSpeed);

        if (m_pTimeOfDaySpeedCVar->GetFVal() != m_advancedInfo.fAnimSpeed)
        {
            m_pTimeOfDaySpeedCVar->Set(m_advancedInfo.fAnimSpeed);
        }

        bool bSunIntensityFound = false;

        // Load.
        for (int i = 0; i < node->getChildCount(); i++)
        {
            XmlNodeRef varNode = node->getChild(i);
            int nParamId = stl::find_in_map(m_varsMap, varNode->getAttr("Name"), -1);
            if (nParamId < 0 || nParamId >= PARAM_TOTAL)
            {
                continue;
            }

            if (nParamId == PARAM_SUN_INTENSITY)
            {
                bSunIntensityFound = true;
            }

            LoadValueFromXmlNode((ETimeOfDayParamID)nParamId, varNode);
        }
        MigrateLegacyData(!bSunIntensityFound, node);
        SetTime(m_fTime, false);
    }
    else
    {
        node->setAttr("Time", m_fTime);
        node->setAttr("TimeStart", m_advancedInfo.fStartTime);
        node->setAttr("TimeEnd", m_advancedInfo.fEndTime);
        node->setAttr("TimeAnimSpeed", m_advancedInfo.fAnimSpeed);
        // Save.
        for (uint32 i = 0; i < static_cast<uint32>(GetVariableCount()); i++)
        {
            SVariableInfo& var = m_vars[i];
            XmlNodeRef varNode = node->newChild("Variable");
            varNode->setAttr("Name", var.name);
            switch (var.type)
            {
            case TYPE_FLOAT:
                varNode->setAttr("Value", var.fValue[0]);
                if (var.pInterpolator)
                {
                    XmlNodeRef splineNode = varNode->newChild("Spline");
                    ((CBezierSplineFloat*)var.pInterpolator)->SerializeSpline(splineNode, bLoading);
                }
                break;
            case TYPE_COLOR:
                varNode->setAttr("Color", Vec3(var.fValue[0], var.fValue[1], var.fValue[2]));
                if (var.pInterpolator)
                {
                    XmlNodeRef splineNode = varNode->newChild("Spline");
                    ((CBezierSplineVec3*)var.pInterpolator)->SerializeSpline(splineNode, bLoading);
                }
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::Serialize(TSerialize ser)
{
    AZ_Assert(ser.GetSerializationTarget() != eST_Network, "Cannot serialize to network error in CTimeOfDay::Serialize");

    string tempName;

    ser.Value("time", m_fTime);
    ser.Value("mode", m_bEditMode);
    ser.Value("m_sunRotationLatitude", m_sunRotationLatitude);
    ser.Value("m_sunRotationLongitude", m_sunRotationLongitude);
    int size = GetVariableCount();
    ser.BeginGroup("VariableValues");
    for (int v = 0; v < size; v++)
    {
        tempName = m_vars[v].name;
        tempName.replace(' ', '_');
        tempName.replace('(', '_');
        tempName.replace(')', '_');
        tempName.replace(':', '_');
        ser.BeginGroup(tempName);
        ser.Value("Val0", m_vars[v].fValue[0]);
        ser.Value("Val1", m_vars[v].fValue[1]);
        ser.Value("Val2", m_vars[v].fValue[2]);
        ser.EndGroup();
    }
    ser.EndGroup();

    ser.Value("AdvInfoSpeed", m_advancedInfo.fAnimSpeed);
    ser.Value("AdvInfoStart", m_advancedInfo.fStartTime);
    ser.Value("AdvInfoEnd", m_advancedInfo.fEndTime);

    if (ser.IsReading())
    {
        SetTime(m_fTime, true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::NetSerialize(TSerialize ser, float lag, uint32 flags)
{
    if (0 == (flags & NETSER_STATICPROPS))
    {
        if (ser.IsWriting())
        {
            ser.Value("time", m_fTime, 'tod');
        }
        else
        {
            float serializedTime;
            ser.Value("time", serializedTime, 'tod');
            float remoteTime = serializedTime + ((flags & NETSER_COMPENSATELAG) != 0) * m_advancedInfo.fAnimSpeed * lag;
            float setTime = remoteTime;
            if (0 == (flags & NETSER_FORCESET))
            {
                const float adjustmentFactor = 0.05f;
                const float wraparoundGuardHours = 2.0f;

                float localTime = m_fTime;
                // handle wraparound
                if (localTime < wraparoundGuardHours && remoteTime > (s_maxTime - wraparoundGuardHours))
                {
                    localTime += s_maxTime;
                }
                else if (remoteTime < wraparoundGuardHours && localTime > (s_maxTime - wraparoundGuardHours))
                {
                    remoteTime += s_maxTime;
                }
                // don't blend times if they're very different
                if (fabsf(remoteTime - localTime) < 1.0f)
                {
                    setTime = adjustmentFactor * remoteTime + (1.0f - adjustmentFactor) * m_fTime;
                    if (setTime > s_maxTime)
                    {
                        setTime -= s_maxTime;
                    }
                }
            }
            SetTime(setTime, (flags & NETSER_FORCESET) != 0);
        }
    }
    else
    {
        // no static serialization (yet)
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDay::Tick()
{
    AZ_TRACE_METHOD();
    //  if(!gEnv->bServer)
    //      return;
    if (!m_bEditMode && !m_bPaused)
    {
        if (fabs(m_advancedInfo.fAnimSpeed) > 0.0001f)
        {
            // advance (forward or backward)
            float fTime = m_fTime + m_advancedInfo.fAnimSpeed * m_pTimer->GetFrameTime();

            // full cycle mode
            if (m_advancedInfo.fStartTime <= 0.05f && m_advancedInfo.fEndTime >= 23.5f)
            {
                if (fTime > m_advancedInfo.fEndTime)
                {
                    fTime = m_advancedInfo.fStartTime;
                }
                if (fTime < m_advancedInfo.fStartTime)
                {
                    fTime = m_advancedInfo.fEndTime;
                }
            }
            else if (fabs(m_advancedInfo.fStartTime - m_advancedInfo.fEndTime) <= 0.05f)//full cycle mode
            {
                if (fTime > s_maxTime)
                {
                    fTime -= s_maxTime;
                }
                else if (fTime < 0.0f)
                {
                    fTime += s_maxTime;
                }
            }
            else
            {
                // clamp advancing time
                if (fTime > m_advancedInfo.fEndTime)
                {
                    fTime = m_advancedInfo.fEndTime;
                }
                if (fTime < m_advancedInfo.fStartTime)
                {
                    fTime = m_advancedInfo.fStartTime;
                }
            }

            SetTime(fTime);
        }
    }
}

void CTimeOfDay::SetUpdateCallback(ITimeOfDayUpdateCallback* pCallback)
{
    m_pUpdateCallback = pCallback;
}

void CTimeOfDay::LerpWith(const ITimeOfDay& other, float lerpValue, ITimeOfDay& output) const
{
    AZ_Assert(GetVariableCount() == other.GetVariableCount() && GetVariableCount() == output.GetVariableCount(), "Attempting to lerp mismatching TimeOfDay objects!");
    CTimeOfDay& todOutput = static_cast<CTimeOfDay&>(output);
    const CTimeOfDay& todOther = static_cast<const CTimeOfDay&>(other);

    for (uint32 i = 0; i < static_cast<uint32>(GetVariableCount()); i++)
    {
        SVariableInfo& outvar = todOutput.m_vars[i];
        const SVariableInfo& var0 = m_vars[i];
        const SVariableInfo& var1 = todOther.m_vars[i];
        if (outvar.nParamId == var0.nParamId && outvar.nParamId == var1.nParamId)
        {
            switch (outvar.type)
            {
            case TYPE_FLOAT:
            {
                outvar.fValue[0] = Lerp(var0.fValue[0], var1.fValue[0], lerpValue);
                break;
            }
            case TYPE_COLOR:
            {
                outvar.fValue[0] = Lerp(var0.fValue[0], var1.fValue[0], lerpValue);
                outvar.fValue[1] = Lerp(var0.fValue[1], var1.fValue[1], lerpValue);
                outvar.fValue[2] = Lerp(var0.fValue[2], var1.fValue[2], lerpValue);
                break;
            }
            default:
            {
                AZ_Error("TimeOfDay", false, "Attempting to lerp mismatching TimeOfDay objects!");
            }
            }
        }
        else
        {
            CryWarning(EValidatorModule::VALIDATOR_MODULE_3DENGINE, EValidatorSeverity::VALIDATOR_WARNING, "Lerping mismatched time of day settings!");
        }
    }
}

void CTimeOfDay::BeginEditMode()
{
    m_bEditMode = true;
    SetTime(m_fEditorTime);
}

void CTimeOfDay::EndEditMode()
{
    m_bEditMode = false;
    m_fEditorTime = m_fTime;
}
