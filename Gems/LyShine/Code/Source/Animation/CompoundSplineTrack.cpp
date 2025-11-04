/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CompoundSplineTrack.h"
#include "AnimSplineTrack.h"
#include <AzCore/Serialization/SerializeContext.h>

UiCompoundSplineTrack::UiCompoundSplineTrack(int nDims, EUiAnimValue inValueType, CUiAnimParamType subTrackParamTypes[MAX_SUBTRACKS])
    : m_refCount(0)
{
    assert(nDims > 0 && nDims <= MAX_SUBTRACKS);
    m_nDimensions = nDims;
    m_valueType = inValueType;

    m_nParamType = eUiAnimNodeType_Invalid;
    m_flags = 0;

    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i].reset(aznew C2DSplineTrack());
        m_subTracks[i]->SetParameterType(subTrackParamTypes[i]);

        if (inValueType == eUiAnimValue_RGB)
        {
            m_subTracks[i]->SetKeyValueRange(0.0f, 255.f);
        }
    }

    m_subTrackNames[0] = "X";
    m_subTrackNames[1] = "Y";
    m_subTrackNames[2] = "Z";
    m_subTrackNames[3] = "W";

#ifdef UI_ANIMATION_SYSTEM_SUPPORT_EDITING
    m_bCustomColorSet = false;
#endif
}

//////////////////////////////////////////////////////////////////////////
// Need default constructor for AZ Serialization
UiCompoundSplineTrack::UiCompoundSplineTrack()
    : m_refCount(0)
    , m_nDimensions(0)
#ifdef UI_ANIMATION_SYSTEM_SUPPORT_EDITING
    , m_bCustomColorSet(false)
#endif
{
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::SetTimeRange(const Range& timeRange)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetTimeRange(timeRange);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::PrepareNodeForSubTrackSerialization(XmlNodeRef& subTrackNode, XmlNodeRef& xmlNode, int i, bool bLoading)
{
    assert(!bLoading || xmlNode->getChildCount() == m_nDimensions);

    if (bLoading)
    {
        subTrackNode = xmlNode->getChild(i);
        // First, check its version.
        if (strcmp(subTrackNode->getTag(), "SubTrack") == 0)
        // So, it's an old format.
        {
#if 0
            CUiAnimParamType paramType = m_subTracks[i]->GetParameterType();
            // Recreate sub tracks as the old format.
            m_subTracks[i] = new CTcbFloatTrack;
            m_subTracks[i]->SetParameterType(paramType);
#endif
        }
    }
    else
    {
        if (m_subTracks[i]->GetCurveType() == eUiAnimCurveType_BezierFloat)
        {
            // It's a new 2D Bezier curve.
            subTrackNode = xmlNode->newChild("NewSubTrack");
        }
#if 0
        else
        // Old TCB spline
        {
            assert(m_subTracks[i]->GetCurveType() == eUiAnimCurveType_TCBFloat);
            subTrackNode = xmlNode->newChild("SubTrack");
        }
#endif
    }
}

//////////////////////////////////////////////////////////////////////////
bool UiCompoundSplineTrack::Serialize(IUiAnimationSystem* uiAnimationSystem, XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks /*=true */)
{
#ifdef UI_ANIMATION_SYSTEM_SUPPORT_EDITING
    if (bLoading)
    {
        int flags = m_flags;
        xmlNode->getAttr("Flags", flags);
        SetFlags(flags);
        xmlNode->getAttr("HasCustomColor", m_bCustomColorSet);
        if (m_bCustomColorSet)
        {
            unsigned int abgr;
            xmlNode->getAttr("CustomColor", abgr);
            m_customColor = ColorB(abgr);
        }
    }
    else
    {
        int flags = GetFlags();
        xmlNode->setAttr("Flags", flags);
        xmlNode->setAttr("HasCustomColor", m_bCustomColorSet);
        if (m_bCustomColorSet)
        {
            xmlNode->setAttr("CustomColor", m_customColor.pack_abgr8888());
        }
    }
#endif

    for (int i = 0; i < m_nDimensions; i++)
    {
        XmlNodeRef subTrackNode;
        PrepareNodeForSubTrackSerialization(subTrackNode, xmlNode, i, bLoading);

        if (bLoading)
        {
            CUiAnimParamType paramType;
            paramType.Serialize(uiAnimationSystem, subTrackNode, bLoading);
            m_subTracks[i]->SetParameterType(paramType);

            UiAnimParamData paramData;
            paramData.Serialize(uiAnimationSystem, subTrackNode, bLoading);
            m_subTracks[i]->SetParamData(paramData);
        }
        else
        {
            CUiAnimParamType paramType = m_subTracks[i]->GetParameterType();
            paramType.Serialize(uiAnimationSystem, subTrackNode, bLoading);

            UiAnimParamData paramData = m_subTracks[i]->GetParamData();
            paramData.Serialize(uiAnimationSystem, subTrackNode, bLoading);
        }
        m_subTracks[i]->Serialize(uiAnimationSystem, subTrackNode, bLoading, bLoadEmptyTracks);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool UiCompoundSplineTrack::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected /*=false*/, float fTimeOffset /*=0*/)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        XmlNodeRef subTrackNode;
        PrepareNodeForSubTrackSerialization(subTrackNode, xmlNode, i, bLoading);
        m_subTracks[i]->SerializeSelection(subTrackNode, bLoading, bCopySelected, fTimeOffset);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::GetValue(float time, float& value)
{
    for (int i = 0; i < 1 && i < m_nDimensions; i++)
    {
        m_subTracks[i]->GetValue(time, value);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::GetValue(float time, Vec3& value)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        float v = value[i];
        m_subTracks[i]->GetValue(time, v);
        value[i] = v;
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::GetValue(float time, Vec4& value)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        float v = value[i];
        m_subTracks[i]->GetValue(time, v);
        value[i] = v;
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::GetValue(float time, Quat& value)
{
    if (m_nDimensions == 3)
    {
        // Assume Euler Angles XYZ
        float angles[3] = {0, 0, 0};
        for (int i = 0; i < m_nDimensions; i++)
        {
            m_subTracks[i]->GetValue(time, angles[i]);
        }
        value = Quat::CreateRotationXYZ(Ang3(DEG2RAD(angles[0]), DEG2RAD(angles[1]), DEG2RAD(angles[2])));
    }
    else
    {
        assert(0);
        value.SetIdentity();
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::GetValue(float time, AZ::Vector2& value)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        float v = value.GetElement(i);
        m_subTracks[i]->GetValue(time, v);
        value.SetElement(i, v);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::GetValue(float time, AZ::Vector3& value)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        float v = value.GetElement(i);
        m_subTracks[i]->GetValue(time, v);
        value.SetElement(i, v);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::GetValue(float time, AZ::Vector4& value)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        float v = value.GetElement(i);
        m_subTracks[i]->GetValue(time, v);
        value.SetElement(i, v);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::GetValue(float time, AZ::Color& value)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        float v = value.GetElement(i);
        m_subTracks[i]->GetValue(time, v);
        value.SetElement(i, v);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::SetValue(float time, const float& value, bool bDefault)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetValue(time, value, bDefault);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::SetValue(float time, const Vec3& value, bool bDefault)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetValue(time, value[i], bDefault);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::SetValue(float time, const Vec4& value, bool bDefault)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetValue(time, value[i], bDefault);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::SetValue(float time, const Quat& value, bool bDefault)
{
    if (m_nDimensions == 3)
    {
        // Assume Euler Angles XYZ
        Ang3 angles = Ang3::GetAnglesXYZ(value);
        for (int i = 0; i < 3; i++)
        {
            float degree = RAD2DEG(angles[i]);
            if (false == bDefault)
            {
                // Try to prefer the shortest path of rotation.
                float degree0 = 0.0f;
                m_subTracks[i]->GetValue(time, degree0);
                degree = PreferShortestRotPath(degree, degree0);
            }
            m_subTracks[i]->SetValue(time, degree, bDefault);
        }
    }
    else
    {
        assert(0);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::SetValue(float time, const AZ::Vector2& value, bool bDefault)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetValue(time, value.GetElement(i), bDefault);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::SetValue(float time, const AZ::Vector3& value, bool bDefault)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetValue(time, value.GetElement(i), bDefault);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::SetValue(float time, const AZ::Vector4& value, bool bDefault)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetValue(time, value.GetElement(i), bDefault);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::SetValue(float time, const AZ::Color& value, bool bDefault)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetValue(time, value.GetElement(i), bDefault);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::OffsetKeyPosition(const AZ::Vector3& offset)
{
    AZ_Assert(m_nDimensions == 3, "expect 3 subtracks found %d", m_nDimensions);
    if (m_nDimensions == 3)
    {
        for (int i = 0; i < 3; i++)
        {
            IUiAnimTrack* pSubTrack = m_subTracks[i].get();
            // Iterate over all keys.
            for (int k = 0, num = pSubTrack->GetNumKeys(); k < num; k++)
            {
                // Offset each key.
                float time = pSubTrack->GetKeyTime(k);
                float value = 0;
                pSubTrack->GetValue(time, value);
                value = value + offset.GetElement(i);
                pSubTrack->SetValue(time, value);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* UiCompoundSplineTrack::GetSubTrack(int nIndex) const
{
    assert(nIndex >= 0 && nIndex < m_nDimensions);
    return m_subTracks[nIndex].get();
}

//////////////////////////////////////////////////////////////////////////
AZStd::string UiCompoundSplineTrack::GetSubTrackName(int nIndex) const
{
    assert(nIndex >= 0 && nIndex < m_nDimensions);
    return m_subTrackNames[nIndex];
}


//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::SetSubTrackName(int nIndex, const char* name)
{
    assert(nIndex >= 0 && nIndex < m_nDimensions);
    assert(name);
    m_subTrackNames[nIndex] = name;
}

//////////////////////////////////////////////////////////////////////////
int UiCompoundSplineTrack::GetNumKeys() const
{
    int nKeys = 0;
    for (int i = 0; i < m_nDimensions; i++)
    {
        nKeys += m_subTracks[i]->GetNumKeys();
    }
    return nKeys;
}

//////////////////////////////////////////////////////////////////////////
bool UiCompoundSplineTrack::HasKeys() const
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        if (m_subTracks[i]->GetNumKeys())
        {
            return true;
        }
    }
    return false;
}

float UiCompoundSplineTrack::PreferShortestRotPath(float degree, float degree0) const
{
    // Assumes the degree is in (-PI, PI).
    assert(-181.0f < degree && degree < 181.0f);
    float degree00 = degree0;
    degree0 = fmod_tpl(degree0, 360.0f);
    float n = (degree00 - degree0) / 360.0f;
    float degreeAlt;
    if (degree >= 0)
    {
        degreeAlt = degree - 360.0f;
    }
    else
    {
        degreeAlt = degree + 360.0f;
    }
    if (fabs(degreeAlt - degree0) < fabs(degree - degree0))
    {
        return degreeAlt + n * 360.0f;
    }
    else
    {
        return degree + n * 360.0f;
    }
}

int UiCompoundSplineTrack::GetSubTrackIndex(int& key) const
{
    assert(key >= 0 && key < GetNumKeys());
    int count = 0;
    for (int i = 0; i < m_nDimensions; i++)
    {
        if (key < count + m_subTracks[i]->GetNumKeys())
        {
            key = key - count;
            return i;
        }
        count += m_subTracks[i]->GetNumKeys();
    }
    return -1;
}

void UiCompoundSplineTrack::RemoveKey(int num)
{
    assert(num >= 0 && num < GetNumKeys());
    int i = GetSubTrackIndex(num);
    assert(i >= 0);
    if (i < 0)
    {
        return;
    }
    m_subTracks[i]->RemoveKey(num);
}

void UiCompoundSplineTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    static char str[64];
    duration = 0;
    description = str;
    const char* subDesc = NULL;
    float time = GetKeyTime(key);
    int m = 0;
    /// Using the time obtained, combine descriptions from keys of the same time
    /// in sub-tracks if any into one compound description.
    str[0] = 0;
    // A head case
    for (m = 0; m < m_subTracks[0]->GetNumKeys(); ++m)
    {
        if (m_subTracks[0]->GetKeyTime(m) == time)
        {
            float dummy;
            m_subTracks[0]->GetKeyInfo(m, subDesc, dummy);
            azstrcat(str, AZ_ARRAY_SIZE(str), subDesc);
            break;
        }
    }
    if (m == m_subTracks[0]->GetNumKeys())
    {
        azstrcat(str, AZ_ARRAY_SIZE(str), m_subTrackNames[0].c_str());
    }
    // Tail cases
    for (int i = 1; i < GetSubTrackCount(); ++i)
    {
        azstrcat(str, AZ_ARRAY_SIZE(str), ",");
        for (m = 0; m < m_subTracks[i]->GetNumKeys(); ++m)
        {
            if (m_subTracks[i]->GetKeyTime(m) == time)
            {
                float dummy;
                m_subTracks[i]->GetKeyInfo(m, subDesc, dummy);
                azstrcat(str, AZ_ARRAY_SIZE(str), subDesc);
                break;
            }
        }
        if (m == m_subTracks[i]->GetNumKeys())
        {
            azstrcat(str, AZ_ARRAY_SIZE(str), m_subTrackNames[i].c_str());
        }
    }
}

float UiCompoundSplineTrack::GetKeyTime(int index) const
{
    assert(index >= 0 && index < GetNumKeys());
    int i = GetSubTrackIndex(index);
    assert(i >= 0);
    if (i < 0)
    {
        return 0;
    }
    return m_subTracks[i]->GetKeyTime(index);
}

void UiCompoundSplineTrack::SetKeyTime(int index, float time)
{
    assert(index >= 0 && index < GetNumKeys());
    int i = GetSubTrackIndex(index);
    assert(i >= 0);
    if (i < 0)
    {
        return;
    }
    m_subTracks[i]->SetKeyTime(index, time);
}

bool UiCompoundSplineTrack::IsKeySelected(int key) const
{
    assert(key >= 0 && key < GetNumKeys());
    int i = GetSubTrackIndex(key);
    assert(i >= 0);
    if (i < 0)
    {
        return false;
    }
    return m_subTracks[i]->IsKeySelected(key);
}

void UiCompoundSplineTrack::SelectKey(int key, bool select)
{
    assert(key >= 0 && key < GetNumKeys());
    int i = GetSubTrackIndex(key);
    assert(i >= 0);
    if (i < 0)
    {
        return;
    }
    float keyTime = m_subTracks[i]->GetKeyTime(key);
    // In the case of compound tracks, animators want to
    // select all keys of the same time in the sub-tracks together.
    const float timeEpsilon = 0.001f;
    for (int k = 0; k < m_nDimensions; ++k)
    {
        for (int m = 0; m < m_subTracks[k]->GetNumKeys(); ++m)
        {
            if (fabs(m_subTracks[k]->GetKeyTime(m) - keyTime) < timeEpsilon)
            {
                m_subTracks[k]->SelectKey(m, select);
                break;
            }
        }
    }
}

int UiCompoundSplineTrack::NextKeyByTime(int key) const
{
    assert(key >= 0 && key < GetNumKeys());
    float time = GetKeyTime(key);
    int count = 0, result = -1;
    float timeNext = FLT_MAX;
    for (int i = 0; i < GetSubTrackCount(); ++i)
    {
        for (int k = 0; k < m_subTracks[i]->GetNumKeys(); ++k)
        {
            float t = m_subTracks[i]->GetKeyTime(k);
            if (t > time)
            {
                if (t < timeNext)
                {
                    timeNext = t;
                    result = count + k;
                }
                break;
            }
        }
        count += m_subTracks[i]->GetNumKeys();
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////
void UiCompoundSplineTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<UiCompoundSplineTrack>()
        ->Version(1)
        ->Field("Flags", &UiCompoundSplineTrack::m_flags)
        ->Field("ParamType", &UiCompoundSplineTrack::m_nParamType)
        ->Field("ParamData", &UiCompoundSplineTrack::m_componentParamData)
        ->Field("NumSubTracks", &UiCompoundSplineTrack::m_nDimensions)
        ->Field("SubTracks", &UiCompoundSplineTrack::m_subTracks)
        ->Field("SubTrackNames", &UiCompoundSplineTrack::m_subTrackNames);
}

