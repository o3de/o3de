/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "LyShine_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "AnimNode.h"
#include "AnimTrack.h"
#include "AnimSequence.h"
#include "AnimSplineTrack.h"
#include "BoolTrack.h"
#include "TrackEventTrack.h"
#include "CompoundSplineTrack.h"

#include <AzCore/std/sort.h>
#include <ctime>

//////////////////////////////////////////////////////////////////////////
// Old deprecated IDs
//////////////////////////////////////////////////////////////////////////
#define APARAM_CHARACTER4 (eUiAnimParamType_User + 0x10)
#define APARAM_CHARACTER5 (eUiAnimParamType_User + 0x11)
#define APARAM_CHARACTER6 (eUiAnimParamType_User + 0x12)
#define APARAM_CHARACTER7 (eUiAnimParamType_User + 0x13)
#define APARAM_CHARACTER8 (eUiAnimParamType_User + 0x14)
#define APARAM_CHARACTER9 (eUiAnimParamType_User + 0x15)
#define APARAM_CHARACTER10 (eUiAnimParamType_User + 0x16)

#define APARAM_EXPRESSION4 (eUiAnimParamType_User + 0x20)
#define APARAM_EXPRESSION5 (eUiAnimParamType_User + 0x21)
#define APARAM_EXPRESSION6 (eUiAnimParamType_User + 0x22)
#define APARAM_EXPRESSION7 (eUiAnimParamType_User + 0x23)
#define APARAM_EXPRESSION8 (eUiAnimParamType_User + 0x24)
#define APARAM_EXPRESSION9 (eUiAnimParamType_User + 0x25)
#define APARAM_EXPRESSION10 (eUiAnimParamType_User + 0x26)

//////////////////////////////////////////////////////////////////////////
static const EUiAnimCurveType DEFAULT_TRACK_TYPE = eUiAnimCurveType_BezierFloat;

// Old serialization values that are no longer
// defined in IUiAnimationSystem.h, but needed for conversion:
static const int OLD_ACURVE_GOTO = 21;
static const int OLD_APARAM_PARTICLE_COUNT_SCALE = 95;
static const int OLD_APARAM_PARTICLE_PULSE_PERIOD = 96;
static const int OLD_APARAM_PARTICLE_SCALE = 97;
static const int OLD_APARAM_PARTICLE_SPEED_SCALE = 98;
static const int OLD_APARAM_PARTICLE_STRENGTH = 99;

//////////////////////////////////////////////////////////////////////////
// CUiAnimNode.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::Activate([[maybe_unused]] bool bActivate)
{
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimNode::GetTrackCount() const
{
    return m_tracks.size();
}

const char* CUiAnimNode::GetParamName(const CUiAnimParamType& paramType) const
{
    SParamInfo info;
    if (GetParamInfoFromType(paramType, info))
    {
        return info.name;
    }

    return "Unknown";
}

EUiAnimValue CUiAnimNode::GetParamValueType(const CUiAnimParamType& paramType) const
{
    SParamInfo info;
    if (GetParamInfoFromType(paramType, info))
    {
        return info.valueType;
    }

    return eUiAnimValue_Unknown;
}

IUiAnimNode::ESupportedParamFlags CUiAnimNode::GetParamFlags(const CUiAnimParamType& paramType) const
{
    SParamInfo info;
    if (GetParamInfoFromType(paramType, info))
    {
        return info.flags;
    }

    return IUiAnimNode::ESupportedParamFlags(0);
}

IUiAnimTrack* CUiAnimNode::GetTrackForParameter(const CUiAnimParamType& paramType) const
{
    for (int i = 0, num = (int)m_tracks.size(); i < num; i++)
    {
        if (m_tracks[i]->GetParameterType() == paramType)
        {
            return m_tracks[i].get();
        }

        // Search the sub-tracks also if any.
        for (int k = 0; k < m_tracks[i]->GetSubTrackCount(); ++k)
        {
            if (m_tracks[i]->GetSubTrack(k)->GetParameterType() == paramType)
            {
                return m_tracks[i]->GetSubTrack(k);
            }
        }
    }
    return 0;
}

IUiAnimTrack* CUiAnimNode::GetTrackForParameter(const CUiAnimParamType& paramType, uint32 index) const
{
    SParamInfo paramInfo;
    GetParamInfoFromType(paramType, paramInfo);

    if ((paramInfo.flags & IUiAnimNode::eSupportedParamFlags_MultipleTracks) == 0)
    {
        return GetTrackForParameter(paramType);
    }

    uint32 count = 0;
    for (int i = 0, num = (int)m_tracks.size(); i < num; i++)
    {
        if (m_tracks[i]->GetParameterType() == paramType && count++ == index)
        {
            return m_tracks[i].get();
        }

        // For this case, no subtracks are considered.
    }
    return 0;
}

uint32 CUiAnimNode::GetTrackParamIndex(const IUiAnimTrack* track) const
{
    AZ_Assert(track, "Track is nullptr.");
    uint32 index = 0;
    CUiAnimParamType paramType = track->GetParameterType();

    SParamInfo paramInfo;
    GetParamInfoFromType(paramType, paramInfo);

    if ((paramInfo.flags & IUiAnimNode::eSupportedParamFlags_MultipleTracks) == 0)
    {
        return 0;
    }

    for (int i = 0, num = (int)m_tracks.size(); i < num; i++)
    {
        if (m_tracks[i].get() == track)
        {
            return index;
        }

        if (m_tracks[i]->GetParameterType() == paramType)
        {
            ++index;
        }

        // For this case, no subtracks are considered.
    }
    AZ_Assert(false, "CUiAnimNode::GetTrackParamIndex() called with an invalid argument!");
    return 0;
}

IUiAnimTrack* CUiAnimNode::GetTrackByIndex(int nIndex) const
{
    AZ_Assert(nIndex >= 0 && nIndex < static_cast<int>(m_tracks.size()), "Track index out of range.");
    if (nIndex < 0 || nIndex >= static_cast<int>(m_tracks.size()))
    {
        return NULL;
    }
    return m_tracks[nIndex].get();
}

void CUiAnimNode::SetTrack(const CUiAnimParamType& paramType, IUiAnimTrack* track)
{
    if (track)
    {
        for (unsigned int i = 0; i < m_tracks.size(); i++)
        {
            if (m_tracks[i]->GetParameterType() == paramType)
            {
                m_tracks[i].reset(track);
                return;
            }
        }

        AddTrack(track);
    }
    else
    {
        // Remove track at this id.
        for (unsigned int i = 0; i < m_tracks.size(); i++)
        {
            if (m_tracks[i]->GetParameterType() == paramType)
            {
                m_tracks.erase(m_tracks.begin() + i);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::TrackOrder(const AZStd::intrusive_ptr<IUiAnimTrack>& left, const AZStd::intrusive_ptr<IUiAnimTrack>& right)
{
    return left->GetParameterType() < right->GetParameterType();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::AddTrack(IUiAnimTrack* track)
{
    RegisterTrack(track);
    m_tracks.push_back(AZStd::intrusive_ptr<IUiAnimTrack>(track));
    SortTracks();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::RegisterTrack(IUiAnimTrack* track)
{
    track->SetTimeRange(GetSequence()->GetTimeRange());
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::SortTracks()
{
    AZStd::insertion_sort(m_tracks.begin(), m_tracks.end(), TrackOrder);
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::RemoveTrack(IUiAnimTrack* track)
{
    for (unsigned int i = 0; i < m_tracks.size(); i++)
    {
        if (m_tracks[i].get() == track)
        {
            m_tracks.erase(m_tracks.begin() + i);
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::Reflect(AZ::SerializeContext* serializeContext)
{
    // we do not currently serialize node type because all nodes are the same type (AzEntityNode)

    serializeContext->Class<CUiAnimNode>()
        ->Version(2)
        ->Field("ID", &CUiAnimNode::m_id)
        ->Field("Parent", &CUiAnimNode::m_parentNodeId)
        ->Field("Name", &CUiAnimNode::m_name)
        ->Field("Flags", &CUiAnimNode::m_flags)
        ->Field("Tracks", &CUiAnimNode::m_tracks)
        ->Field("Type", &CUiAnimNode::m_nodeType);
}

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* CUiAnimNode::CreateTrackInternal(const CUiAnimParamType& paramType, EUiAnimCurveType trackType, EUiAnimValue valueType)
{

    if (valueType == eUiAnimValue_Unknown)
    {
        SParamInfo info;

        // Try to get info from paramType, else we can't determine the track data type
        if (!GetParamInfoFromType(paramType, info))
        {
            return 0;
        }

        valueType = info.valueType;
    }

    IUiAnimTrack* track = NULL;

    switch (paramType.GetType())
    {
    // Create sub-classed tracks
    case eUiAnimParamType_TrackEvent:
        track = aznew CUiTrackEventTrack(m_pSequence->GetTrackEventStringTable());
        break;
    case eUiAnimParamType_Float:
        track = CreateTrackInternalFloat(trackType);
        break;

    default:
        // Create standard tracks
        switch (valueType)
        {
        case eUiAnimValue_Float:
            track = CreateTrackInternalFloat(trackType);
            break;
        case eUiAnimValue_RGB:
        case eUiAnimValue_Vector:
            track = CreateTrackInternalVector(trackType, paramType, valueType);
            break;
        case eUiAnimValue_Quat:
            track = CreateTrackInternalQuat(trackType, paramType);
            break;
        case eUiAnimValue_Bool:
            track = aznew UiBoolTrack;
            break;
        case eUiAnimValue_Vector2:
            track = CreateTrackInternalVector2(paramType);
            break;
        case eUiAnimValue_Vector3:
            track = CreateTrackInternalVector3(paramType);
            break;
        case eUiAnimValue_Vector4:
            track = CreateTrackInternalVector4(paramType);
            break;
        }
    }

    if (track)
    {
        track->SetParameterType(paramType);
        AddTrack(track);
    }

    return track;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* CUiAnimNode::CreateTrack(const CUiAnimParamType& paramType)
{
    IUiAnimTrack* track = CreateTrackInternal(paramType, DEFAULT_TRACK_TYPE, eUiAnimValue_Unknown);
    return track;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::SerializeUiAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    if (bLoading)
    {
        // Delete all tracks.
        stl::free_container(m_tracks);

        CUiAnimNode::SParamInfo info;
        // Loading.
        int paramTypeVersion = 0;
        xmlNode->getAttr("paramIdVersion", paramTypeVersion);
        CUiAnimParamType paramType;
        const int num = xmlNode->getChildCount();
        for (int i = 0; i < num; i++)
        {
            XmlNodeRef trackNode = xmlNode->getChild(i);
            paramType.Serialize(GetUiAnimationSystem(), trackNode, bLoading, paramTypeVersion);

            int curveType = eUiAnimCurveType_Unknown;
            trackNode->getAttr("Type", curveType);


            int valueType = eUiAnimValue_Unknown;
            trackNode->getAttr("ValueType", valueType);

            IUiAnimTrack* track = CreateTrackInternal(paramType, (EUiAnimCurveType)curveType, (EUiAnimValue)valueType);
            if (track)
            {
                UiAnimParamData paramData;
                paramData.Serialize(GetUiAnimationSystem(), trackNode, bLoading);
                track->SetParamData(paramData);

                if (!track->Serialize(GetUiAnimationSystem(), trackNode, bLoading, bLoadEmptyTracks))
                {
                    // Boolean tracks must always be loaded even if empty.
                    if (track->GetValueType() != eUiAnimValue_Bool)
                    {
                        RemoveTrack(track);
                    }
                }
            }
        }
    }
    else
    {
        // Saving.
        xmlNode->setAttr("paramIdVersion", CUiAnimParamType::kParamTypeVersion);
        for (unsigned int i = 0; i < m_tracks.size(); i++)
        {
            IUiAnimTrack* track = m_tracks[i].get();
            if (track)
            {
                CUiAnimParamType paramType = track->GetParameterType();
                XmlNodeRef trackNode = xmlNode->newChild("Track");
                paramType.Serialize(GetUiAnimationSystem(), trackNode, bLoading);

                UiAnimParamData paramData = track->GetParamData();
                paramData.Serialize(GetUiAnimationSystem(), trackNode, bLoading);

                int nTrackType = track->GetCurveType();
                trackNode->setAttr("Type", nTrackType);
                track->Serialize(GetUiAnimationSystem(), trackNode, bLoading);
                int valueType = track->GetValueType();
                trackNode->setAttr("ValueType", valueType);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::SetTimeRange(Range timeRange)
{
    for (unsigned int i = 0; i < m_tracks.size(); i++)
    {
        if (m_tracks[i])
        {
            m_tracks[i]->SetTimeRange(timeRange);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// AZ::Serialization requires a default constructor
CUiAnimNode::CUiAnimNode()
    : CUiAnimNode(0, eUiAnimNodeType_Invalid)
{
}

//////////////////////////////////////////////////////////////////////////
// explicit copy constructor is required to prevent compiler's generated copy constructor
// from calling AZStd::mutex's private copy constructor
CUiAnimNode::CUiAnimNode(const CUiAnimNode& other)
    : m_refCount(0)
    , m_id(0)                                   // don't copy id - these should be unique
    , m_parentNodeId(other.m_parentNodeId)
    , m_nodeType(other.m_nodeType)
    , m_pOwner(other.m_pOwner)
    , m_pSequence(other.m_pSequence)
    , m_flags(other.m_flags)
    , m_pParentNode(other.m_pParentNode)
    , m_nLoadedParentNodeId(other.m_nLoadedParentNodeId)
{
    // m_bIgnoreSetParam not copied
}

//////////////////////////////////////////////////////////////////////////
CUiAnimNode::CUiAnimNode(const int id, EUiAnimNodeType nodeType)
    : m_refCount(0)
    , m_id(id)
    , m_parentNodeId(0)
    , m_nodeType(nodeType)
{
    m_pOwner = 0;
    m_pSequence = 0;
    m_flags = 0;
    m_bIgnoreSetParam = false;
    m_pParentNode = 0;
    m_nLoadedParentNodeId = 0;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimNode::~CUiAnimNode()
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::SetFlags(int flags)
{
    m_flags = flags;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimNode::GetFlags() const
{
    return m_flags;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::Animate([[maybe_unused]] SUiAnimContext& ec)
{
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::IsParamValid(const CUiAnimParamType& paramType) const
{
    SParamInfo info;

    if (GetParamInfoFromType(paramType, info))
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::SetParamValue(float time, CUiAnimParamType param, float value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    IUiAnimTrack* track = GetTrackForParameter(param);
    if (track && track->GetValueType() == eUiAnimValue_Float)
    {
        // Float track.
        bool bDefault = !(GetUiAnimationSystem()->IsRecording() && (m_flags & eUiAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
        track->SetValue(time, value, bDefault);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::SetParamValue(float time, CUiAnimParamType param, const Vec3& value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    UiCompoundSplineTrack* track = static_cast<UiCompoundSplineTrack*>(GetTrackForParameter(param));
    if (track && track->GetValueType() == eUiAnimValue_Vector)
    {
        // Vec3 track.
        bool bDefault = !(GetUiAnimationSystem()->IsRecording() && (m_flags & eUiAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
        track->SetValue(time, value, bDefault);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::SetParamValue(float time, CUiAnimParamType param, const Vec4& value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    UiCompoundSplineTrack* track = static_cast<UiCompoundSplineTrack*>(GetTrackForParameter(param));
    if (track && track->GetValueType() == eUiAnimValue_Vector4)
    {
        // Vec4 track.
        bool bDefault = !(GetUiAnimationSystem()->IsRecording() && (m_flags & eUiAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
        track->SetValue(time, value, bDefault);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::GetParamValue(float time, CUiAnimParamType param, float& value)
{
    IUiAnimTrack* track = GetTrackForParameter(param);
    if (track && track->GetValueType() == eUiAnimValue_Float && track->GetNumKeys() > 0)
    {
        // Float track.
        track->GetValue(time, value);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::GetParamValue(float time, CUiAnimParamType param, Vec3& value)
{
    UiCompoundSplineTrack* track = static_cast<UiCompoundSplineTrack*>(GetTrackForParameter(param));
    if (track && track->GetValueType() == eUiAnimValue_Vector && track->GetNumKeys() > 0)
    {
        // Vec3 track.
        track->GetValue(time, value);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::GetParamValue(float time, CUiAnimParamType param, Vec4& value)
{
    UiCompoundSplineTrack* track = static_cast<UiCompoundSplineTrack*>(GetTrackForParameter(param));
    if (track && track->GetValueType() == eUiAnimValue_Vector4 && track->GetNumKeys() > 0)
    {
        // Vec4 track.
        track->GetValue(time, value);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    if (bLoading)
    {
        xmlNode->getAttr("Id", m_id);
        const char* name = xmlNode->getAttr("Name");
        int flags;
        if (xmlNode->getAttr("Flags", flags))
        {
            // Don't load expanded or selected flags
            flags = flags & ~(eUiAnimNodeFlags_Expanded | eUiAnimNodeFlags_EntitySelected);
            SetFlags(flags);
        }

        SetName(name);

        m_nLoadedParentNodeId = 0;
        xmlNode->getAttr("ParentNode", m_nLoadedParentNodeId);
    }
    else
    {
        m_nLoadedParentNodeId = 0;
        xmlNode->setAttr("Id", m_id);

        EUiAnimNodeType nodeType = GetType();
        static_cast<UiAnimationSystem*>(GetUiAnimationSystem())->SerializeNodeType(nodeType, xmlNode, bLoading, IUiAnimSequence::kSequenceVersion, m_flags);

        xmlNode->setAttr("Name", GetName());

        // Don't store expanded or selected flags
        int flags = GetFlags() & ~(eUiAnimNodeFlags_Expanded | eUiAnimNodeFlags_EntitySelected);
        xmlNode->setAttr("Flags", flags);

        if (m_pParentNode)
        {
            xmlNode->setAttr("ParentNode", static_cast<CUiAnimNode*>(m_pParentNode)->GetId());
        }
    }

    SerializeUiAnims(xmlNode, bLoading, bLoadEmptyTracks);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::InitPostLoad(IUiAnimSequence* pSequence, [[maybe_unused]] bool remapIds, [[maybe_unused]] LyShine::EntityIdMap* entityIdMap)
{
    m_pSequence = pSequence;
    m_pParentNode = ((CUiAnimSequence*)m_pSequence)->FindNodeById(m_parentNodeId);

    // fix up animNode pointers and time ranges on tracks, then sort them
    for (unsigned int i = 0; i < m_tracks.size(); i++)
    {
        IUiAnimTrack* track = m_tracks[i].get();
        RegisterTrack(track);
        track->InitPostLoad(pSequence);
    }
    SortTracks();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::SetNodeOwner(IUiAnimNodeOwner* pOwner)
{
    m_pOwner = pOwner;

    if (pOwner)
    {
        pOwner->OnNodeUiAnimated(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::PostLoad()
{
    if (m_nLoadedParentNodeId)
    {
        IUiAnimNode* pParentNode = ((CUiAnimSequence*)m_pSequence)->FindNodeById(m_nLoadedParentNodeId);
        m_pParentNode = pParentNode;
        m_parentNodeId = m_nLoadedParentNodeId; // adding as a temporary fix while we support both serialization methods
        m_nLoadedParentNodeId = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
Matrix34 CUiAnimNode::GetReferenceMatrix() const
{
    static Matrix34 tm(IDENTITY);
    return tm;
}

IUiAnimTrack* CUiAnimNode::CreateTrackInternalFloat([[maybe_unused]] int trackType) const
{
    // Don't need trackType any more
    return aznew C2DSplineTrack;
}

IUiAnimTrack* CUiAnimNode::CreateTrackInternalVector([[maybe_unused]] EUiAnimCurveType trackType, [[maybe_unused]] const CUiAnimParamType& paramType, [[maybe_unused]] const EUiAnimValue animValue) const
{
    // Don't need trackType any more

    CUiAnimParamType subTrackParamTypes[MAX_SUBTRACKS];
    for (unsigned int i = 0; i < MAX_SUBTRACKS; ++i)
    {
        subTrackParamTypes[i] = eUiAnimParamType_AzComponentField;
    }

    return aznew UiCompoundSplineTrack(3, eUiAnimValue_Vector, subTrackParamTypes);
}

IUiAnimTrack* CUiAnimNode::CreateTrackInternalQuat([[maybe_unused]] EUiAnimCurveType trackType, [[maybe_unused]] const CUiAnimParamType& paramType) const
{
    // UI_ANIMATION_REVISIT - my may want quat support at some point
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* CUiAnimNode::CreateTrackInternalVector2([[maybe_unused]] const CUiAnimParamType& paramType) const
{
    IUiAnimTrack* track;

    CUiAnimParamType subTrackParamTypes[MAX_SUBTRACKS];
    for (unsigned int i = 0; i < MAX_SUBTRACKS; ++i)
    {
        subTrackParamTypes[i] = eUiAnimParamType_Float;
    }


    track = aznew UiCompoundSplineTrack(2, eUiAnimValue_Vector2, subTrackParamTypes);


    return track;
}

IUiAnimTrack* CUiAnimNode::CreateTrackInternalVector3([[maybe_unused]] const CUiAnimParamType& paramType) const
{
    IUiAnimTrack* track;

    CUiAnimParamType subTrackParamTypes[MAX_SUBTRACKS];
    for (unsigned int i = 0; i < MAX_SUBTRACKS; ++i)
    {
        subTrackParamTypes[i] = eUiAnimParamType_Float;
    }

    track = aznew UiCompoundSplineTrack(3, eUiAnimValue_Vector3, subTrackParamTypes);

    return track;
}


//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* CUiAnimNode::CreateTrackInternalVector4([[maybe_unused]] const CUiAnimParamType& paramType) const
{
    IUiAnimTrack* track;

    CUiAnimParamType subTrackParamTypes[MAX_SUBTRACKS];
    for (unsigned int i = 0; i < MAX_SUBTRACKS; ++i)
    {
        subTrackParamTypes[i] = eUiAnimParamType_Float;
    }

    track = aznew UiCompoundSplineTrack(4, eUiAnimValue_Vector4, subTrackParamTypes);
    return track;
}
//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::SetParent(IUiAnimNode* pParent)
{
    m_pParentNode = pParent;
    if (pParent)
    {
        m_parentNodeId = static_cast<CUiAnimNode*>(m_pParentNode)->GetId();
    }
    else
    {
        m_parentNodeId = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimNode::HasDirectorAsParent() const
{
    IUiAnimNode* pParent = GetParent();
    while (pParent)
    {
        if (pParent->GetType() == eUiAnimNodeType_Director)
        {
            return pParent;
        }
        // There are some invalid data.
        if (pParent->GetParent() == pParent)
        {
            pParent->SetParent(NULL);
            return NULL;
        }
        pParent = pParent->GetParent();
    }
    return NULL;
}
