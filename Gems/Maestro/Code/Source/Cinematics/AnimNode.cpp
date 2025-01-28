/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include "AnimNode.h"
#include "AnimTrack.h"
#include "AnimSequence.h"
#include <Maestro/Types/AssetBlendKey.h>
#include "AssetBlendTrack.h"
#include "CharacterTrack.h"
#include "AnimSplineTrack.h"
#include "BoolTrack.h"
#include "MathConversion.h"
#include "SelectTrack.h"
#include "EventTrack.h"
#include "SoundTrack.h"
#include "ConsoleTrack.h"
#include "LookAtTrack.h"
#include "TrackEventTrack.h"
#include "SequenceTrack.h"
#include "CompoundSplineTrack.h"
#include "GotoTrack.h"
#include "CaptureTrack.h"
#include "CommentTrack.h"
#include "ScreenFaderTrack.h"
#include "TimeRangesTrack.h"

#include <AzCore/std/sort.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Component/TickBus.h>
#include <ctime>
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimParamType.h"


namespace Maestro
{

    namespace
    {
        constexpr bool IsSoundRange(AnimParamType animParamType)
        {
            return animParamType >= AnimParamType::Sound &&
                animParamType <= static_cast<AnimParamType>(static_cast<int>(AnimParamType::Sound) + 2);
        }
        constexpr bool IsAnimRange(AnimParamType animParamType)
        {
            return animParamType >= AnimParamType::Animation && animParamType <=
                static_cast<AnimParamType>(static_cast<int>(AnimParamType::Animation) + 2);
        }
        constexpr bool IsUserAnimRange(AnimParamType animParamType)
        {
            return animParamType >= static_cast<AnimParamType>(static_cast<int>(AnimParamType::User) + 0x10) &&
                animParamType <= static_cast<AnimParamType>(static_cast<int>(AnimParamType::User) + 0x16);
        }
    }

    static const EAnimCurveType DEFAULT_TRACK_TYPE = eAnimCurveType_BezierFloat;

    // Old serialization values that are no longer
    // defined in IMovieSystem.h, but needed for conversion:
    static const int OLD_ACURVE_GOTO = 21;
    static const int OLD_APARAM_PARTICLE_COUNT_SCALE = 95;
    static const int OLD_APARAM_PARTICLE_PULSE_PERIOD = 96;
    static const int OLD_APARAM_PARTICLE_SCALE = 97;
    static const int OLD_APARAM_PARTICLE_SPEED_SCALE = 98;
    static const int OLD_APARAM_PARTICLE_STRENGTH = 99;

    // CAnimNode.
    void CAnimNode::Activate([[maybe_unused]] bool bActivate)
    {
    }

    int CAnimNode::GetTrackCount() const
    {
        return static_cast<int>(m_tracks.size());
    }

    AZStd::string CAnimNode::GetParamName(const CAnimParamType& paramType) const
    {
        SParamInfo info;
        if (GetParamInfoFromType(paramType, info))
        {
            return info.name;
        }

        return "Unknown";
    }

    AnimValueType CAnimNode::GetParamValueType(const CAnimParamType& paramType) const
    {
        SParamInfo info;
        if (GetParamInfoFromType(paramType, info))
        {
            return info.valueType;
        }

        return AnimValueType::Unknown;
    }

    IAnimNode::ESupportedParamFlags CAnimNode::GetParamFlags(const CAnimParamType& paramType) const
    {
        SParamInfo info;
        if (GetParamInfoFromType(paramType, info))
        {
            return info.flags;
        }

        return IAnimNode::ESupportedParamFlags(0);
    }

    IAnimTrack* CAnimNode::GetTrackForParameter(const CAnimParamType& paramType) const
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

    IAnimTrack* CAnimNode::GetTrackForParameter(const CAnimParamType& paramType, uint32 index) const
    {
        SParamInfo paramInfo;
        GetParamInfoFromType(paramType, paramInfo);

        if ((paramInfo.flags & IAnimNode::eSupportedParamFlags_MultipleTracks) == 0)
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

    uint32 CAnimNode::GetTrackParamIndex(const IAnimTrack* pTrack) const
    {
        AZ_Assert(pTrack, "pTrack is null");
        uint32 index = 0;
        CAnimParamType paramType = pTrack->GetParameterType();

        SParamInfo paramInfo;
        GetParamInfoFromType(paramType, paramInfo);

        if ((paramInfo.flags & IAnimNode::eSupportedParamFlags_MultipleTracks) == 0)
        {
            return 0;
        }

        for (int i = 0, num = (int)m_tracks.size(); i < num; i++)
        {
            if (m_tracks[i].get() == pTrack)
            {
                return index;
            }

            if (m_tracks[i]->GetParameterType() == paramType)
            {
                ++index;
            }

            // For this case, no subtracks are considered.
        }
        AZ_Assert(false, "CAnimNode::GetTrackParamIndex() called with an invalid argument");
        return 0;
    }

    IAnimTrack* CAnimNode::GetTrackByIndex(int nIndex) const
    {
        if (nIndex >= (int)m_tracks.size())
        {
            AZ_Assert(false, "Track index %i is out of range", nIndex);
            return nullptr;
        }
        return m_tracks[nIndex].get();
    }

    void CAnimNode::SetTrack(const CAnimParamType& paramType, IAnimTrack* pTrack)
    {
        if (pTrack)
        {
            for (unsigned int i = 0; i < m_tracks.size(); i++)
            {
                if (m_tracks[i]->GetParameterType() == paramType)
                {
                    m_tracks[i].reset(pTrack);
                    return;
                }
            }

            AddTrack(pTrack);
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

    bool CAnimNode::TrackOrder(const AZStd::intrusive_ptr<IAnimTrack>& left, const AZStd::intrusive_ptr<IAnimTrack>& right)
    {
        return left->GetParameterType() < right->GetParameterType();
    }

    void CAnimNode::AddTrack(IAnimTrack* pTrack)
    {
        RegisterTrack(pTrack);
        m_tracks.push_back(AZStd::intrusive_ptr<IAnimTrack>(pTrack));
        SortTracks();
    }

    void CAnimNode::RegisterTrack(IAnimTrack* pTrack)
    {
        pTrack->SetTimeRange(GetSequence()->GetTimeRange());
        pTrack->SetNode(this);
    }

    void CAnimNode::SortTracks()
    {
        AZStd::insertion_sort(m_tracks.begin(), m_tracks.end(), TrackOrder);
    }

    bool CAnimNode::RemoveTrack(IAnimTrack* pTrack)
    {
        for (unsigned int i = 0; i < m_tracks.size(); i++)
        {
            if (m_tracks[i].get() == pTrack)
            {
                m_tracks.erase(m_tracks.begin() + i);
                return true;
            }
        }
        return false;
    }

    static bool AnimNodeVersionConverter(
        AZ::SerializeContext& serializeContext,
        AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimNode>());
        }

        if (rootElement.GetVersion() < 4)
        {
            // remove vector scale tracks from transform animation nodes
            AZStd::string name;
            if (rootElement.FindSubElementAndGetData<AZStd::string>(AZ_CRC_CE("Name"), name) && name == "Transform")
            {
                auto tracksElement = rootElement.FindSubElement(AZ_CRC_CE("Tracks"));
                if (tracksElement)
                {
                    for (int trackIndex = tracksElement->GetNumSubElements() - 1; trackIndex >= 0; trackIndex--)
                    {
                        AZ::Serialize::DataElementNode& trackElement = tracksElement->GetSubElement(trackIndex);
                        bool isScale = false;

                        // trackElement should be an intrusive_ptr with one child
                        if (trackElement.GetNumSubElements() == 1)
                        {
                            auto ptrElement = trackElement.GetSubElement(0);
                            auto paramTypeElement = ptrElement.FindSubElement(AZ_CRC_CE("ParamType"));
                            if (paramTypeElement)
                            {
                                AZStd::string paramName;
                                if (paramTypeElement->FindSubElementAndGetData<AZStd::string>(AZ_CRC_CE("Name"), paramName) && paramName == "Scale")
                                {
                                    isScale = true;
                                }
                            }
                        }

                        if (isScale)
                        {
                            tracksElement->RemoveElement(trackIndex);
                        }
                    }
                }
            }

        }

        return true;
    }

    void CAnimNode::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CAnimNode, IAnimNode>()
                ->Version(4, &AnimNodeVersionConverter)
                ->Field("ID", &CAnimNode::m_id)
                ->Field("Name", &CAnimNode::m_name)
                ->Field("Flags", &CAnimNode::m_flags)
                ->Field("Tracks", &CAnimNode::m_tracks)
                ->Field("Parent", &CAnimNode::m_parentNodeId)
                ->Field("Type", &CAnimNode::m_nodeType)
                ->Field("Expanded", &CAnimNode::m_expanded);
        }
    }

    IAnimTrack* CAnimNode::CreateTrackInternal(const CAnimParamType& paramType, EAnimCurveType trackType, AnimValueType valueType)
    {
        if (valueType == AnimValueType::Unknown)
        {
            SParamInfo info;

            // Try to get info from paramType, else we can't determine the track data type
            if (!GetParamInfoFromType(paramType, info))
            {
                return 0;
            }

            valueType = info.valueType;
        }

        IAnimTrack* pTrack = nullptr;

        switch (paramType.GetType())
        {
        // Create sub-classed tracks
        case AnimParamType::Event:
            pTrack = aznew CEventTrack(m_pSequence->GetTrackEventStringTable());
            break;
        case AnimParamType::Sound:
            pTrack = aznew CSoundTrack;
            break;
        case AnimParamType::Animation:
            pTrack = aznew CCharacterTrack;
            break;
        case AnimParamType::Console:
            pTrack = aznew CConsoleTrack;
            break;
        case AnimParamType::LookAt:
            pTrack = aznew CLookAtTrack;
            break;
        case AnimParamType::TrackEvent:
            pTrack = aznew CTrackEventTrack(m_pSequence->GetTrackEventStringTable());
            break;
        case AnimParamType::Sequence:
            pTrack = aznew CSequenceTrack;
            break;
        case AnimParamType::Capture:
            pTrack = aznew CCaptureTrack;
            break;
        case AnimParamType::CommentText:
            pTrack = aznew CCommentTrack;
            break;
        case AnimParamType::ScreenFader:
            pTrack = aznew CScreenFaderTrack;
            break;
        case AnimParamType::Goto:
            pTrack = aznew CGotoTrack;
            break;
        case AnimParamType::TimeRanges:
            pTrack = aznew CTimeRangesTrack;
            break;
        case AnimParamType::Float:
            pTrack = CreateTrackInternalFloat(trackType);
            break;

        default:
            // Create standard tracks
            switch (valueType)
            {
            case AnimValueType::Float:
                pTrack = CreateTrackInternalFloat(trackType);
                break;
            case AnimValueType::RGB:
            case AnimValueType::Vector:
                pTrack = CreateTrackInternalVector(trackType, paramType, valueType);
                break;
            case AnimValueType::Quat:
                pTrack = CreateTrackInternalQuat(trackType, paramType);
                break;
            case AnimValueType::Bool:
                pTrack = aznew CBoolTrack;
                break;
            case AnimValueType::Select:
                pTrack = aznew CSelectTrack;
                break;
            case AnimValueType::Vector4:
                pTrack = CreateTrackInternalVector4(paramType);
                break;
            case AnimValueType::CharacterAnim:
                pTrack = aznew CCharacterTrack;
                break;
            case AnimValueType::AssetBlend:
                pTrack = aznew CAssetBlendTrack;
                break;
            }
        }

        if (pTrack)
        {
            pTrack->SetParameterType(paramType);

            // Assign a unique id for every track.
            pTrack->SetId(m_pSequence->GetUniqueTrackIdAndGenerateNext());
            int subTrackCount = pTrack->GetSubTrackCount();
            for (int subTrackIndex = 0; subTrackIndex < subTrackCount; subTrackIndex++)
            {
                IAnimTrack* subTrack = pTrack->GetSubTrack(subTrackIndex);
                subTrack->SetId(m_pSequence->GetUniqueTrackIdAndGenerateNext());
            }

            AddTrack(pTrack);
        }

        return pTrack;
    }

    IAnimTrack* CAnimNode::CreateTrack(const CAnimParamType& paramType)
    {
        IAnimTrack* pTrack = CreateTrackInternal(paramType, DEFAULT_TRACK_TYPE, AnimValueType::Unknown);
        InitializeTrackDefaultValue(pTrack, paramType);
        return pTrack;
    }

    void CAnimNode::SerializeAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
    {
        if (bLoading)
        {
            // Delete all tracks.
            stl::free_container(m_tracks);

            CAnimNode::SParamInfo info;
            // Loading.
            int paramTypeVersion = 0;
            xmlNode->getAttr("paramIdVersion", paramTypeVersion);
            CAnimParamType paramType;
            int num = xmlNode->getChildCount();
            for (int i = 0; i < num; i++)
            {
                XmlNodeRef trackNode = xmlNode->getChild(i);
                paramType.Serialize(trackNode, bLoading, paramTypeVersion);

                if (paramType.GetType() == AnimParamType::Music)
                {
                    // skip loading AnimParamType::Music - it's deprecated
                    continue;
                }

                if (paramTypeVersion == 0) // for old version with sound and animation param ids swapped
                {
                    AnimParamType APARAM_ANIMATION_OLD = AnimParamType::Sound;
                    AnimParamType APARAM_SOUND_OLD = AnimParamType::Animation;
                    if (paramType.GetType() == APARAM_ANIMATION_OLD)
                    {
                        paramType = AnimParamType::Animation;
                    }
                    else if (paramType.GetType() == APARAM_SOUND_OLD)
                    {
                        paramType = AnimParamType::Sound;
                    }
                }

                int curveType = eAnimCurveType_Unknown;
                trackNode->getAttr("Type", curveType);
                if (curveType == eAnimCurveType_Unknown)
                {
                    if (paramTypeVersion == 0)
                    {
                        // Backward compatibility code
                        //
                        // Legacy animation track.
                        // Collapse parameter ID to the single type
                        const auto animParamType = paramType.GetType();
                        if (IsSoundRange(animParamType))
                        {
                            paramType = AnimParamType::Sound;
                        }
                        else if (IsAnimRange(animParamType) || IsUserAnimRange(animParamType))
                        {
                            paramType = AnimParamType::Animation;
                        }

                        // Old tracks always used TCB tracks.
                        // Backward compatibility to the CryEngine2 for track type (will make TCB controller)
                        curveType = eAnimCurveType_TCBVector;
                    }
                }

                if (paramTypeVersion <= 1)
                {
                    // In old versions goto tracks were identified by a curve id
                    if (curveType == OLD_ACURVE_GOTO)
                    {
                        paramType = AnimParamType::Goto;
                        curveType = eAnimCurveType_Unknown;
                    }
                }

                if (paramTypeVersion <= 3 && paramType.GetType() >= static_cast<AnimParamType>(OLD_APARAM_USER))
                {
                    // APARAM_USER 100 => 100000
                    paramType = static_cast<AnimParamType>(static_cast<int>(paramType.GetType()) + static_cast<int>(AnimParamType::User) - OLD_APARAM_USER);
                }

                if (paramTypeVersion <= 4)
                {
                    // In old versions there was special code for particles
                    // that is now handles by generic entity node code
                    switch (paramType.GetType())
                    {
                    case static_cast<AnimParamType>(OLD_APARAM_PARTICLE_COUNT_SCALE) :
                        paramType = CAnimParamType("ScriptTable:Properties/CountScale");
                        break;
                    case static_cast<AnimParamType>(OLD_APARAM_PARTICLE_PULSE_PERIOD) :
                        paramType = CAnimParamType("ScriptTable:Properties/PulsePeriod");
                        break;
                    case static_cast<AnimParamType>(OLD_APARAM_PARTICLE_SCALE) :
                        paramType = CAnimParamType("ScriptTable:Properties/Scale");
                        break;
                    case static_cast<AnimParamType>(OLD_APARAM_PARTICLE_SPEED_SCALE) :
                        paramType = CAnimParamType("ScriptTable:Properties/SpeedScale");
                        break;
                    case static_cast<AnimParamType>(OLD_APARAM_PARTICLE_STRENGTH):
                        paramType = CAnimParamType("ScriptTable:Properties/Strength");
                        break;
                    }
                }

                if (paramTypeVersion <= 5 && !(GetSequence()->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet))
                {
                    // In old versions there was special code for lights that is now handled
                    // by generic entity node code if this is not a light animation set sequence
                    switch (paramType.GetType())
                    {
                    case AnimParamType::LightDiffuse:
                        paramType = CAnimParamType("ScriptTable:Properties/Color/clrDiffuse");
                        break;
                    case AnimParamType::LightRadius:
                        paramType = CAnimParamType("ScriptTable:Properties/Radius");
                        break;
                    case AnimParamType::LightDiffuseMult:
                        paramType = CAnimParamType("ScriptTable:Properties/Color/fDiffuseMultiplier");
                        break;
                    case AnimParamType::LightHDRDynamic:
                        paramType = CAnimParamType("ScriptTable:Properties/Color/fHDRDynamic");
                        break;
                    case AnimParamType::LightSpecularMult:
                        paramType = CAnimParamType("ScriptTable:Properties/Color/fSpecularMultiplier");
                        break;
                    case AnimParamType::LightSpecPercentage:
                        paramType = CAnimParamType("ScriptTable:Properties/Color/fSpecularPercentage");
                        break;
                    }
                }

                if (paramTypeVersion <= 7 && paramType.GetType() == AnimParamType::Physics)
                {
                    paramType = AnimParamType::PhysicsDriven;
                }

                int valueType = static_cast<int>(AnimValueType::Unknown);
                trackNode->getAttr("ValueType", valueType);

                IAnimTrack* pTrack = CreateTrackInternal(paramType, (EAnimCurveType)curveType, static_cast<AnimValueType>(valueType));
                bool trackRemoved = false;
                if (pTrack)
                {
                    if (!pTrack->Serialize(trackNode, bLoading, bLoadEmptyTracks))
                    {
                        // Boolean tracks must always be loaded even if empty.
                        if (pTrack->GetValueType() != AnimValueType::Bool)
                        {
                            RemoveTrack(pTrack);
                            trackRemoved = true;
                        }
                    }
                }

                if (!trackRemoved && gEnv->IsEditor())
                {
                    InitializeTrackDefaultValue(pTrack, paramType);
                }
            }
        }
        else
        {
            // Saving.
            xmlNode->setAttr("paramIdVersion", CAnimParamType::kParamTypeVersion);
            for (unsigned int i = 0; i < m_tracks.size(); i++)
            {
                IAnimTrack* pTrack = m_tracks[i].get();
                if (pTrack)
                {
                    CAnimParamType paramType = m_tracks[i]->GetParameterType();
                    XmlNodeRef trackNode = xmlNode->newChild("Track");
                    paramType.Serialize(trackNode, bLoading);
                    int nTrackType = pTrack->GetCurveType();
                    trackNode->setAttr("Type", nTrackType);
                    pTrack->Serialize(trackNode, bLoading);
                    int valueType = static_cast<int>(pTrack->GetValueType());
                    trackNode->setAttr("ValueType", valueType);
                }
            }
        }
    }

    void CAnimNode::SetTimeRange(Range timeRange)
    {
        for (unsigned int i = 0; i < m_tracks.size(); i++)
        {
            if (m_tracks[i])
            {
                m_tracks[i]->SetTimeRange(timeRange);
            }
        }
    }

    // AZ::Serialization requires a default constructor
    CAnimNode::CAnimNode()
        : CAnimNode(0, AnimNodeType::Invalid)
    {
    }

    // explicit copy constructor is required to prevent compiler's generated copy constructor
    // from calling AZStd::mutex's private copy constructor
    CAnimNode::CAnimNode(const CAnimNode& other)
        : m_refCount(0)
        , m_id(0)                                   // don't copy id - these should be unique
        , m_parentNodeId(other.m_parentNodeId)
        , m_nodeType(other.m_nodeType)
        , m_pOwner(other.m_pOwner)
        , m_pSequence(other.m_pSequence)
        , m_flags(other.m_flags)
        , m_pParentNode(other.m_pParentNode)
        , m_nLoadedParentNodeId(other.m_nLoadedParentNodeId)
        , m_expanded(other.m_expanded)
        , m_movieSystem(other.m_movieSystem)
    {
        // m_bIgnoreSetParam not copied
    }

    CAnimNode::CAnimNode(const int id, AnimNodeType nodeType)
        : m_refCount(0)
        , m_id(id)
        , m_parentNodeId(0)
        , m_nodeType(nodeType)
        , m_movieSystem(AZ::Interface<IMovieSystem>::Get())
    {
        m_pOwner = 0;
        m_pSequence = 0;
        m_flags = 0;
        m_bIgnoreSetParam = false;
        m_pParentNode = 0;
        m_nLoadedParentNodeId = 0;
        m_expanded = true;

        AZ_Trace("CAnimNode", "CAnimNode type %i", static_cast<int>(nodeType));
    }

    CAnimNode::~CAnimNode()
    {
        AZ_Trace("CAnimNode", "~CAnimNode %i", static_cast<int>(m_nodeType));
    }

    void CAnimNode::add_ref()
    {
        ++m_refCount;
    }

    void CAnimNode::release()
    {
        if (--m_refCount <= 0)
        {
            delete this;
        }
    }

    void CAnimNode::SetFlags(int flags)
    {
        m_flags = flags;
    }

    int CAnimNode::GetFlags() const
    {
        return m_flags;
    }

    bool CAnimNode::AreFlagsSetOnNodeOrAnyParent(EAnimNodeFlags flagsToCheck) const
    {
        if (m_pParentNode)
        {
            // recurse up parent chain until we find the flagsToCheck set or get to the top of the chain
            return ((GetFlags() & flagsToCheck) != 0) || m_pParentNode->AreFlagsSetOnNodeOrAnyParent(flagsToCheck);
        }

        // top of parent chain
        return ((GetFlags() & flagsToCheck) != 0);
    }

    void CAnimNode::Animate([[maybe_unused]] SAnimContext& ec)
    {
    }

    bool CAnimNode::IsParamValid(const CAnimParamType& paramType) const
    {
        SParamInfo info;

        if (GetParamInfoFromType(paramType, info))
        {
            return true;
        }

        return false;
    }

    bool CAnimNode::SetParamValue(float time, CAnimParamType param, float value)
    {
        if (m_bIgnoreSetParam)
        {
            return true;
        }

        IAnimTrack* pTrack = GetTrackForParameter(param);
        if (pTrack && pTrack->GetValueType() == AnimValueType::Float)
        {
            // Float track.
            bool bDefault = !(m_movieSystem->IsRecording() && (m_flags & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
            pTrack->SetValue(time, value, bDefault);
            return true;
        }
        return false;
    }

    bool CAnimNode::SetParamValue(float time, CAnimParamType param, const AZ::Vector3& value)
    {
        if (m_bIgnoreSetParam)
        {
            return true;
        }

        CCompoundSplineTrack* pTrack = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
        if (pTrack && pTrack->GetValueType() == AnimValueType::Vector)
        {
            // Vec3 track.
            bool bDefault = !(m_movieSystem->IsRecording() && (m_flags & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
            pTrack->SetValue(time, value, bDefault);
            return true;
        }
        return false;
    }

    bool CAnimNode::SetParamValue(float time, CAnimParamType param, const AZ::Vector4& value)
    {
        if (m_bIgnoreSetParam)
        {
            return true;
        }

        CCompoundSplineTrack* pTrack = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
        if (pTrack && pTrack->GetValueType() == AnimValueType::Vector4)
        {
            // Vec4 track.
            bool bDefault = !(m_movieSystem->IsRecording() && (m_flags & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
            pTrack->SetValue(time, value, bDefault);
            return true;
        }
        return false;
    }

    bool CAnimNode::GetParamValue(float time, CAnimParamType param, float& value)
    {
        IAnimTrack* pTrack = GetTrackForParameter(param);
        if (pTrack && pTrack->GetValueType() == AnimValueType::Float && pTrack->GetNumKeys() > 0)
        {
            // Float track.
            pTrack->GetValue(time, value);
            return true;
        }
        return false;
    }

    bool CAnimNode::GetParamValue(float time, CAnimParamType param, AZ::Vector3& value)
    {
        CCompoundSplineTrack* pTrack = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
        if (pTrack && pTrack->GetValueType() == AnimValueType::Vector && pTrack->GetNumKeys() > 0)
        {
            // Vec3 track.
            pTrack->GetValue(time, value);
            return true;
        }
        return false;
    }

    bool CAnimNode::GetParamValue(float time, CAnimParamType param, AZ::Vector4& value)
    {
        CCompoundSplineTrack* pTrack = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
        if (pTrack && pTrack->GetValueType() == AnimValueType::Vector4 && pTrack->GetNumKeys() > 0)
        {
            // Vec4 track.
            pTrack->GetValue(time, value);
            return true;
        }
        return false;
    }

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
    void CAnimNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
    {
        if (bLoading)
        {
            xmlNode->getAttr("Id", m_id);
            const char* name = xmlNode->getAttr("Name");
            int flags;
            if (xmlNode->getAttr("Flags", flags))
            {
                // Don't load expanded or selected flags
                flags = flags & ~(eAnimNodeFlags_Expanded | eAnimNodeFlags_EntitySelected);
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

            AnimNodeType nodeType = GetType();
            GetMovieSystem()->SerializeNodeType(nodeType, xmlNode, bLoading, IAnimSequence::kSequenceVersion, m_flags);

            xmlNode->setAttr("Name", GetName());

            // Don't store expanded or selected flags
            int flags = GetFlags() & ~(eAnimNodeFlags_Expanded | eAnimNodeFlags_EntitySelected);
            xmlNode->setAttr("Flags", flags);

            if (m_pParentNode)
            {
                xmlNode->setAttr("ParentNode", static_cast<CAnimNode*>(m_pParentNode)->GetId());
            }
        }

        SerializeAnims(xmlNode, bLoading, bLoadEmptyTracks);
    }

    void CAnimNode::InitPostLoad(IAnimSequence* sequence)
    {
        [[maybe_unused]] const AZ::EntityId& sequenceEntityId = sequence->GetSequenceEntityId();
        AZ_Trace("CAnimNode::InitPostLoad", "IAnimSequence is %s", sequenceEntityId.ToString().c_str());

        m_pSequence = sequence;
        m_pParentNode = ((CAnimSequence*)m_pSequence)->FindNodeById(m_parentNodeId);

        // fix up animNode pointers and time ranges on tracks, then sort them
        for (unsigned int i = 0; i < m_tracks.size(); i++)
        {
            RegisterTrack(m_tracks[i].get());
            m_tracks[i].get()->InitPostLoad(sequence);
        }
        SortTracks();
    }

    void CAnimNode::SetNodeOwner(IAnimNodeOwner* pOwner)
    {
        m_pOwner = pOwner;

        if (pOwner)
        {
            pOwner->OnNodeAnimated(this);
        }
    }

    void CAnimNode::PostLoad()
    {
        if (m_nLoadedParentNodeId)
        {
            IAnimNode* pParentNode = ((CAnimSequence*)m_pSequence)->FindNodeById(m_nLoadedParentNodeId);
            m_pParentNode = pParentNode;
            m_parentNodeId = m_nLoadedParentNodeId; // adding as a temporary fix while we support both serialization methods
            m_nLoadedParentNodeId = 0;
        }
    }

    Matrix34 CAnimNode::GetReferenceMatrix() const
    {
        static Matrix34 tm(IDENTITY);
        return tm;
    }

    IAnimTrack* CAnimNode::CreateTrackInternalFloat([[maybe_unused]] int trackType) const
    {
        return aznew C2DSplineTrack;
    }

    IAnimTrack* CAnimNode::CreateTrackInternalVector([[maybe_unused]] EAnimCurveType trackType, const CAnimParamType& paramType, const AnimValueType animValue) const
    {
        CAnimParamType subTrackParamTypes[CCompoundSplineTrack::MaxSubtracks];
        for (unsigned int i = 0; i < CCompoundSplineTrack::MaxSubtracks; ++i)
        {
            subTrackParamTypes[i] = AnimParamType::Float;
        }

        if (paramType == AnimParamType::Position)
        {
            subTrackParamTypes[0] = AnimParamType::PositionX;
            subTrackParamTypes[1] = AnimParamType::PositionY;
            subTrackParamTypes[2] = AnimParamType::PositionZ;
        }
        else if (paramType == AnimParamType::Scale)
        {
            subTrackParamTypes[0] = AnimParamType::ScaleX;
            subTrackParamTypes[1] = AnimParamType::ScaleY;
            subTrackParamTypes[2] = AnimParamType::ScaleZ;
        }
        else if (paramType == AnimParamType::Rotation)
        {
            subTrackParamTypes[0] = AnimParamType::RotationX;
            subTrackParamTypes[1] = AnimParamType::RotationY;
            subTrackParamTypes[2] = AnimParamType::RotationZ;
            IAnimTrack* pTrack = aznew CCompoundSplineTrack(3, AnimValueType::Quat, subTrackParamTypes, false);
            return pTrack;
        }
        else if (paramType == AnimParamType::DepthOfField)
        {
            subTrackParamTypes[0] = AnimParamType::FocusDistance;
            subTrackParamTypes[1] = AnimParamType::FocusRange;
            subTrackParamTypes[2] = AnimParamType::BlurAmount;
            IAnimTrack* pTrack = aznew CCompoundSplineTrack(3, AnimValueType::Vector, subTrackParamTypes, false);
            pTrack->SetSubTrackName(0, "FocusDist");
            pTrack->SetSubTrackName(1, "FocusRange");
            pTrack->SetSubTrackName(2, "BlurAmount");
            return pTrack;
        }
        else if (animValue == AnimValueType::RGB || paramType == AnimParamType::LightDiffuse ||
                 paramType == AnimParamType::MaterialDiffuse || paramType == AnimParamType::MaterialSpecular
                 || paramType == AnimParamType::MaterialEmissive)
        {
            subTrackParamTypes[0] = AnimParamType::ColorR;
            subTrackParamTypes[1] = AnimParamType::ColorG;
            subTrackParamTypes[2] = AnimParamType::ColorB;
            IAnimTrack* pTrack = aznew CCompoundSplineTrack(3, AnimValueType::RGB, subTrackParamTypes, false);
            pTrack->SetSubTrackName(0, "Red");
            pTrack->SetSubTrackName(1, "Green");
            pTrack->SetSubTrackName(2, "Blue");
            return pTrack;
        }

        return aznew CCompoundSplineTrack(3, AnimValueType::Vector, subTrackParamTypes, false);
    }

    IAnimTrack* CAnimNode::CreateTrackInternalQuat([[maybe_unused]] EAnimCurveType trackType, const CAnimParamType& paramType) const
    {
        CAnimParamType subTrackParamTypes[CCompoundSplineTrack::MaxSubtracks];
        if (paramType == AnimParamType::Rotation)
        {
            subTrackParamTypes[0] = AnimParamType::RotationX;
            subTrackParamTypes[1] = AnimParamType::RotationY;
            subTrackParamTypes[2] = AnimParamType::RotationZ;
        }
        else
        {
            AZ_Assert(false, "Unknown param type");
        }

        return aznew CCompoundSplineTrack(3, AnimValueType::Quat, subTrackParamTypes, false);
    }

    IAnimTrack* CAnimNode::CreateTrackInternalVector4(const CAnimParamType& paramType) const
    {
        IAnimTrack* pTrack;

        CAnimParamType subTrackParamTypes[CCompoundSplineTrack::MaxSubtracks];

        // set up track subtypes
        if (paramType == AnimParamType::TransformNoise
            || paramType == AnimParamType::ShakeMultiplier)
        {
            subTrackParamTypes[0] = AnimParamType::ShakeAmpAMult;
            subTrackParamTypes[1] = AnimParamType::ShakeAmpBMult;
            subTrackParamTypes[2] = AnimParamType::ShakeFreqAMult;
            subTrackParamTypes[3] = AnimParamType::ShakeFreqBMult;
        }
        else
        {
            // default to a Vector4 of floats
            for (unsigned int i = 0; i < CCompoundSplineTrack::MaxSubtracks; ++i)
            {
                subTrackParamTypes[i] = AnimParamType::Float;
            }
        }

        // create track
        pTrack = aznew CCompoundSplineTrack(4, AnimValueType::Vector4, subTrackParamTypes, true);

        // label subtypes
        if (paramType == AnimParamType::TransformNoise)
        {
            pTrack->SetSubTrackName(0, "Pos Noise Amp");
            pTrack->SetSubTrackName(1, "Pos Noise Freq");
            pTrack->SetSubTrackName(2, "Rot Noise Amp");
            pTrack->SetSubTrackName(3, "Rot Noise Freq");
        }
        else if (paramType == AnimParamType::ShakeMultiplier)
        {
            pTrack->SetSubTrackName(0, "Amplitude A");
            pTrack->SetSubTrackName(1, "Amplitude B");
            pTrack->SetSubTrackName(2, "Frequency A");
            pTrack->SetSubTrackName(3, "Frequency B");
        }

        return pTrack;
    }

    void CAnimNode::TimeChanged(float newTime)
    {
        // if the newTime is on a sound key, then reset sounds so sound will playback on next call to Animate()
        if (IsTimeOnSoundKey(newTime))
        {
            ResetSounds();
        }
    }

    bool CAnimNode::IsTimeOnSoundKey(float queryTime) const
    {
        bool retIsTimeOnSoundKey = false;
        const float tolerance = 0.0333f;        // one frame at 30 fps

        int trackCount = NumTracks();
        for (int trackIndex = 0; trackIndex < trackCount; trackIndex++)
        {
            CAnimParamType paramType = m_tracks[trackIndex]->GetParameterType();
            IAnimTrack* pTrack = m_tracks[trackIndex].get();
            if ((paramType.GetType() != AnimParamType::Sound)
                || (pTrack->HasKeys() == false && pTrack->GetParameterType() != AnimParamType::Visibility)
                || (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
            {
                continue;
            }

            // if we're here, pTrack points to a AnimParamType::Sound track
            ISoundKey oSoundKey;
            int const nSoundKey = static_cast<CSoundTrack*>(pTrack)->GetActiveKey(queryTime, &oSoundKey);
            if (nSoundKey >= 0)
            {
                retIsTimeOnSoundKey = AZ::IsClose(queryTime, oSoundKey.time, tolerance);
                if (retIsTimeOnSoundKey)
                {
                    break;      // no need to search further, we have a hit
                }
            }
        }

        return retIsTimeOnSoundKey;
    }

    void CAnimNode::AnimateSound(AZStd::vector<SSoundInfo>& nodeSoundInfo, SAnimContext& ec, IAnimTrack* pTrack, size_t numAudioTracks)
    {
        bool const bMute = gEnv->IsEditor() && (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Muted);

        if (!bMute && ec.time >= 0.0f)
        {
            ISoundKey oSoundKey;
            int const nSoundKey = static_cast<CSoundTrack*>(pTrack)->GetActiveKey(ec.time, &oSoundKey);
            SSoundInfo& rSoundInfo = nodeSoundInfo[numAudioTracks - 1];

            if (nSoundKey >= 0)
            {
                float const fSoundKeyTime = (ec.time - oSoundKey.time);

                if (rSoundInfo.nSoundKeyStart < nSoundKey && fSoundKeyTime < oSoundKey.fDuration)
                {
                    ApplyAudioKey(oSoundKey.sStartTrigger.c_str());
                }

                if (rSoundInfo.nSoundKeyStart > nSoundKey)
                {
                    rSoundInfo.nSoundKeyStop = nSoundKey;
                }

                rSoundInfo.nSoundKeyStart = nSoundKey;

                if (fSoundKeyTime >= oSoundKey.fDuration)
                {
                    if (rSoundInfo.nSoundKeyStop < nSoundKey)
                    {
                        rSoundInfo.nSoundKeyStop = nSoundKey;

                        if (oSoundKey.sStopTrigger.empty())
                        {
                            ApplyAudioKey(oSoundKey.sStartTrigger.c_str(), false);
                        }
                        else
                        {
                            ApplyAudioKey(oSoundKey.sStopTrigger.c_str());
                        }
                    }
                }
                else
                {
                    rSoundInfo.nSoundKeyStop = -1;
                }
            }
            else
            {
                rSoundInfo.Reset();
            }
        }
    }

    void CAnimNode::SetParent(IAnimNode* parent)
    {
        m_pParentNode = parent;
        if (parent)
        {
            m_parentNodeId = static_cast<CAnimNode*>(m_pParentNode)->GetId();
        }
        else
        {
            m_parentNodeId = 0;
        }
    }

    IAnimNode* CAnimNode::HasDirectorAsParent() const
    {
        IAnimNode* pParent = GetParent();
        while (pParent)
        {
            if (pParent->GetType() == AnimNodeType::Director)
            {
                return pParent;
            }
            // There are some invalid data.
            if (pParent->GetParent() == pParent)
            {
                pParent->SetParent(nullptr);
                return nullptr;
            }
            pParent = pParent->GetParent();
        }
        return nullptr;
    }

    void CAnimNode::UpdateDynamicParams()
    {
        if (gEnv->IsEditor())
        {
            // UpdateDynamicParams is called as the result of an editor event that is fired when a material is loaded,
            // which could happen from multiple threads. Lock to avoid a crash iterating over the lua stack
            AZStd::lock_guard<AZStd::mutex> lock(m_updateDynamicParamsLock);

            // run this on the main thread to prevent further threading issues downstream in
            // AnimNodes that may use EBuses that are not thread safe
            if (gEnv && gEnv->mMainThreadId == CryGetCurrentThreadId())
            {
                UpdateDynamicParamsInternal();
            }
            else
            {
                AZ::TickBus::QueueFunction([this] {
                    UpdateDynamicParamsInternal();
                });
            }
        }
        else
        {
            UpdateDynamicParamsInternal();
        }
    }

    void CAnimNode::SetExpanded(bool expanded)
    {
        m_expanded = expanded;
    }

    bool CAnimNode::GetExpanded() const
    {
        return m_expanded;
    }

    IMovieSystem* CAnimNode::GetMovieSystem() const
    {
        return AZ::Interface<IMovieSystem>::Get();
    }

} // namespace Maestro
