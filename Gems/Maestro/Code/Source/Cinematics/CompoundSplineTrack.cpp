/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "Maestro_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Transform.h>

#include "CompoundSplineTrack.h"
#include "AnimSplineTrack.h"
#include "Maestro/Types/AnimParamType.h"
#include "Maestro/Types/AnimValueType.h"

CCompoundSplineTrack::CCompoundSplineTrack(int nDims, AnimValueType inValueType, CAnimParamType subTrackParamTypes[MAX_SUBTRACKS], bool expanded)
    : m_refCount(0)
    , m_expanded(expanded)
{
    assert(nDims > 0 && nDims <= MAX_SUBTRACKS);
    m_node = nullptr;
    m_nDimensions = nDims;
    m_valueType = inValueType;

    m_nParamType = AnimParamType::Invalid;
    m_flags = 0;

    m_subTracks.resize(MAX_SUBTRACKS);
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i].reset(aznew C2DSplineTrack());
        m_subTracks[i]->SetParameterType(subTrackParamTypes[i]);

        if (inValueType == AnimValueType::RGB)
        {
            m_subTracks[i]->SetKeyValueRange(0.0f, 255.f);
        }
    }

    m_subTrackNames.resize(MAX_SUBTRACKS);
    m_subTrackNames[0] = "X";
    m_subTrackNames[1] = "Y";
    m_subTrackNames[2] = "Z";
    m_subTrackNames[3] = "W";

#ifdef MOVIESYSTEM_SUPPORT_EDITING
    m_bCustomColorSet = false;
#endif
}

//////////////////////////////////////////////////////////////////////////
// Need default constructor for AZ Serialization
CCompoundSplineTrack::CCompoundSplineTrack()
    : m_refCount(0)
    , m_nDimensions(0)
    , m_valueType(AnimValueType::Float)
#ifdef MOVIESYSTEM_SUPPORT_EDITING
    , m_bCustomColorSet(false)
#endif
    , m_expanded(false)
{
}

void CCompoundSplineTrack::SetNode(IAnimNode* node)
{
    m_node = node;
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetNode(node);
    }
}
//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetTimeRange(const Range& timeRange)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetTimeRange(timeRange);
    }
}

//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
bool CCompoundSplineTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks /*=true */)
{
#ifdef MOVIESYSTEM_SUPPORT_EDITING
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
        xmlNode->getAttr("Id", m_id);
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
        xmlNode->setAttr("Id", m_id);
    }
#endif

    for (int i = 0; i < m_nDimensions; i++)
    {
        XmlNodeRef subTrackNode;
        if (bLoading)
        {
            subTrackNode = xmlNode->getChild(i);
        }
        else
        {
            subTrackNode = xmlNode->newChild("NewSubTrack");
        }
        m_subTracks[i]->Serialize(subTrackNode, bLoading, bLoadEmptyTracks);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CCompoundSplineTrack::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected /*=false*/, float fTimeOffset /*=0*/)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        XmlNodeRef subTrackNode;
        if (bLoading)
        {
            subTrackNode = xmlNode->getChild(i);
        }
        else
        {
            subTrackNode = xmlNode->newChild("NewSubTrack");
        }
        m_subTracks[i]->SerializeSelection(subTrackNode, bLoading, bCopySelected, fTimeOffset);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::GetValue(float time, float& value, bool applyMultiplier)
{
    for (int i = 0; i < 1 && i < m_nDimensions; i++)
    {
        m_subTracks[i]->GetValue(time, value, applyMultiplier);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::GetValue(float time, Vec3& value, bool applyMultiplier)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        float v = value[i];
        m_subTracks[i]->GetValue(time, v, applyMultiplier);
        value[i] = v;
    }
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::GetValue(float time, Vec4& value, bool applyMultiplier)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        float v = value[i];
        m_subTracks[i]->GetValue(time, v, applyMultiplier);
        value[i] = v;
    }
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::GetValue(float time, Quat& value)
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
void CCompoundSplineTrack::SetValue(float time, const float& value, bool bDefault, bool applyMultiplier)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetValue(time, value, bDefault, applyMultiplier);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetValue(float time, const Vec3& value, bool bDefault, bool applyMultiplier)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetValue(time, value[i], bDefault, applyMultiplier);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetValue(float time, const Vec4& value, bool bDefault, bool applyMultiplier)
{
    for (int i = 0; i < m_nDimensions; i++)
    {
        m_subTracks[i]->SetValue(time, value[i], bDefault, applyMultiplier);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetValue(float time, const Quat& value, bool bDefault)
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
void CCompoundSplineTrack::OffsetKeyPosition(const Vec3& offset)
{
    if (m_nDimensions == 3)
    {
        for (int i = 0; i < 3; i++)
        {
            IAnimTrack* pSubTrack = m_subTracks[i].get();
            // Iterate over all keys.
            for (int k = 0, num = pSubTrack->GetNumKeys(); k < num; k++)
            {
                // Offset each key.
                float time = pSubTrack->GetKeyTime(k);
                float value = 0;
                pSubTrack->GetValue(time, value);
                value = value + offset[i];
                pSubTrack->SetValue(time, value);
            }
        }
    }
    else
    {
        assert(0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM)
{
    // Only update the position tracks
    if (m_nParamType.GetType() != AnimParamType::Position)
    {
        return;
    }

    AZ_Assert(m_nDimensions == 3, "Expected 3 dimensions, position, rotation or scale.");

    struct KeyValues
    {
        KeyValues(int i, float t, float v) : index(i), time(t), value(v) {};
        int index;
        float time;
        float value;           
    };

    // Don't actually set the key data until we are done calculating all the new values.
    // In the case where not all 3 tracks have key data, data from other keys may be referenced
    // and used. So we don't want to muck with those other keys until we are done transforming all of
    // the key data.
    AZStd::vector<KeyValues> newKeyValues;

    // Collect all times that have key data on any track.
    for (int subTrackIndex = 0; subTrackIndex < 3; subTrackIndex++)
    {
        IAnimTrack* subTrack = m_subTracks[subTrackIndex].get();
        for (int k = 0, num = subTrack->GetNumKeys(); k < num; k++)
        {
            // If this key time is not already in the list, add it.
            float time = subTrack->GetKeyTime(k);

            // Create a 3 float vector with values from the 3 tracks.
                AZ::Vector3 vector;
            for (int i = 0; i < 3; i++)
            {
                    float value = 0;
                m_subTracks[i]->GetValue(time, value);
                vector.SetElement(i, value);
            }

            // Use the old parent world transform to get the current key data into world space.
            AZ::Vector3 worldPosition = oldParentWorldTM.GetTranslation() + vector;

            // Get the world location into local space relative to the new parent.
            vector = worldPosition - newParentWorldTM.GetTranslation();

            // Store the new key data in a list to be set to keys later.
            newKeyValues.push_back(KeyValues(subTrackIndex, time, vector.GetElement(subTrackIndex)));
        }
    }

    // Set key data for each time gathered from the keys.
    for (auto valuePair : newKeyValues)
    {
        m_subTracks[valuePair.index]->SetValue(valuePair.time, valuePair.value);
    }
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CCompoundSplineTrack::GetSubTrack(int nIndex) const
{
    assert(nIndex >= 0 && nIndex < m_nDimensions);
    return m_subTracks[nIndex].get();
}

//////////////////////////////////////////////////////////////////////////
const char* CCompoundSplineTrack::GetSubTrackName(int nIndex) const
{
    assert(nIndex >= 0 && nIndex < m_nDimensions);
    return m_subTrackNames[nIndex].c_str();
}


//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetSubTrackName(int nIndex, const char* name)
{
    assert(nIndex >= 0 && nIndex < m_nDimensions);
    assert(name);
    m_subTrackNames[nIndex] = name;
}

//////////////////////////////////////////////////////////////////////////
int CCompoundSplineTrack::GetNumKeys() const
{
    int nKeys = 0;
    for (int i = 0; i < m_nDimensions; i++)
    {
        nKeys += m_subTracks[i]->GetNumKeys();
    }
    return nKeys;
}

//////////////////////////////////////////////////////////////////////////
bool CCompoundSplineTrack::HasKeys() const
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

float CCompoundSplineTrack::PreferShortestRotPath(float degree, float degree0) const
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

int CCompoundSplineTrack::GetSubTrackIndex(int& key) const
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

void CCompoundSplineTrack::RemoveKey(int num)
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

void CCompoundSplineTrack::GetKeyInfo(int key, const char*& description, float& duration)
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
            cry_strcat(str, subDesc);
            break;
        }
    }
    if (m == m_subTracks[0]->GetNumKeys())
    {
        cry_strcat(str, m_subTrackNames[0].c_str());
    }
    // Tail cases
    for (int i = 1; i < GetSubTrackCount(); ++i)
    {
        cry_strcat(str, ",");
        for (m = 0; m < m_subTracks[i]->GetNumKeys(); ++m)
        {
            if (m_subTracks[i]->GetKeyTime(m) == time)
            {
                float dummy;
                m_subTracks[i]->GetKeyInfo(m, subDesc, dummy);
                cry_strcat(str, subDesc);
                break;
            }
        }
        if (m == m_subTracks[i]->GetNumKeys())
        {
            cry_strcat(str, m_subTrackNames[i].c_str());
        }
    }
}

float CCompoundSplineTrack::GetKeyTime(int index) const
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

void CCompoundSplineTrack::SetKeyTime(int index, float time)
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

bool CCompoundSplineTrack::IsKeySelected(int key) const
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

void CCompoundSplineTrack::SelectKey(int key, bool select)
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

int CCompoundSplineTrack::NextKeyByTime(int key) const
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
void CCompoundSplineTrack::SetExpanded(bool expanded)
{
    m_expanded = expanded;
}

//////////////////////////////////////////////////////////////////////////
bool CCompoundSplineTrack::GetExpanded() const
{
    return m_expanded;
}
 
//////////////////////////////////////////////////////////////////////////
unsigned int CCompoundSplineTrack::GetId() const
{
    return m_id;
}

//////////////////////////////////////////////////////////////////////////
void CCompoundSplineTrack::SetId(unsigned int id)
{
    m_id = id;
}

//////////////////////////////////////////////////////////////////////////
static bool CompoundSplineTrackVersionConverter(
    AZ::SerializeContext& serializeContext,
    AZ::SerializeContext::DataElementNode& rootElement)
{
    if (rootElement.GetVersion() < 4)
    {
        rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
    }

    return true;
}

void CCompoundSplineTrack::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<CCompoundSplineTrack, IAnimTrack>()
            ->Version(4, &CompoundSplineTrackVersionConverter)
            ->Field("Flags", &CCompoundSplineTrack::m_flags)
            ->Field("ParamType", &CCompoundSplineTrack::m_nParamType)
            ->Field("NumSubTracks", &CCompoundSplineTrack::m_nDimensions)
            ->Field("SubTracks", &CCompoundSplineTrack::m_subTracks)
            ->Field("SubTrackNames", &CCompoundSplineTrack::m_subTrackNames)
            ->Field("ValueType", &CCompoundSplineTrack::m_valueType)
            ->Field("Expanded", &CCompoundSplineTrack::m_expanded)
            ->Field("Id", &CCompoundSplineTrack::m_id);
    }
}
