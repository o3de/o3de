/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "AzEntityNode.h"
#include "AnimSplineTrack.h"
#include "BoolTrack.h"
#include "ISystem.h"
#include "CompoundSplineTrack.h"
#include "UiAnimationSystem.h"
#include "AnimSequence.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <LyShine/Bus/UiAnimateEntityBus.h>

//////////////////////////////////////////////////////////////////////////
namespace
{
    const char* kScriptTablePrefix = "ScriptTable:";

    AZStd::array<CUiAnimNode::SParamInfo, 1> s_nodeParams{ { {
        /*.name =*/"Component Field float",
        /*.paramType =*/eUiAnimParamType_AzComponentField,
        /*.valueType =*/eUiAnimValue_Float,
        /*.flags =*/(IUiAnimNode::ESupportedParamFlags)0,
    } } };
}

//////////////////////////////////////////////////////////////////////////
CUiAnimAzEntityNode::CUiAnimAzEntityNode(const int id)
    : CUiAnimNode(id, eUiAnimNodeType_AzEntity)
{
    m_bWasTransRot = false;
    m_bInitialPhysicsStatus = false;

    m_pos(0, 0, 0);
    m_scale(1, 1, 1);
    m_rotate.SetIdentity();

    m_visible = true;

    m_time = 0.0f;

    m_lastEntityKey = -1;

    #ifdef CHECK_FOR_TOO_MANY_ONPROPERTY_SCRIPT_CALLS
    m_OnPropertyCalls = 0;
    #endif

    UiAnimNodeBus::Handler::BusConnect(this);
}

//////////////////////////////////////////////////////////////////////////
CUiAnimAzEntityNode::CUiAnimAzEntityNode()
    : CUiAnimAzEntityNode(0)
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::AddTrack(IUiAnimTrack* track)
{
    CUiAnimNode::AddTrack(track);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::UpdateDynamicParams()
{
    m_entityScriptPropertiesParamInfos.clear();
    m_nameToScriptPropertyParamInfo.clear();

    // editor stores *all* properties of *every* entity used in an AnimAzEntityNode, including to-display names, full lua paths, string maps for fast access, etc.
    // in pure game mode we just need to store the properties that we know are going to be used in a track, so we can save a lot of memory.
    if (gEnv->IsEditor())
    {
        UpdateDynamicParams_Editor();
    }
    else
    {
        UpdateDynamicParams_PureGame();
    }
}


//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::UpdateDynamicParams_Editor()
{
}


//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::UpdateDynamicParams_PureGame()
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::CreateDefaultTracks()
{
}

//////////////////////////////////////////////////////////////////////////
CUiAnimAzEntityNode::~CUiAnimAzEntityNode()
{
    ReleaseSounds();

    UiAnimNodeBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
unsigned int CUiAnimAzEntityNode::GetParamCount() const
{
    return static_cast<unsigned int>(CUiAnimAzEntityNode::GetParamCountStatic() + m_entityScriptPropertiesParamInfos.size());
}

//////////////////////////////////////////////////////////////////////////
CUiAnimParamType CUiAnimAzEntityNode::GetParamType(unsigned int nIndex) const
{
    SParamInfo info;

    if (!CUiAnimAzEntityNode::GetParamInfoStatic(nIndex, info))
    {
        const uint scriptParamsOffset = (uint)s_nodeParams.size();
        const uint end = (uint)s_nodeParams.size() + (uint)m_entityScriptPropertiesParamInfos.size();

        if (nIndex >= scriptParamsOffset && nIndex < end)
        {
            return m_entityScriptPropertiesParamInfos[nIndex - scriptParamsOffset].animNodeParamInfo.paramType;
        }

        return eUiAnimParamType_Invalid;
    }

    return info.paramType;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimAzEntityNode::GetParamCountStatic()
{
    return static_cast<int>(s_nodeParams.size());
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimAzEntityNode::GetParamInfoStatic(int nIndex, SParamInfo& info)
{
    if (nIndex >= 0 && nIndex < (int)s_nodeParams.size())
    {
        info = s_nodeParams[nIndex];
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CUiAnimAzEntityNode, CUiAnimNode>()
        ->Version(1)
        ->Field("Entity", &CUiAnimAzEntityNode::m_entityId);
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimAzEntityNode::GetParamInfoFromType(const CUiAnimParamType& paramId, SParamInfo& info) const
{
    for (int i = 0; i < (int)s_nodeParams.size(); i++)
    {
        if (s_nodeParams[i].paramType == paramId)
        {
            info = s_nodeParams[i];
            return true;
        }
    }

    for (size_t i = 0; i < m_entityScriptPropertiesParamInfos.size(); ++i)
    {
        if (m_entityScriptPropertiesParamInfos[i].animNodeParamInfo.paramType == paramId)
        {
            info = m_entityScriptPropertiesParamInfos[i].animNodeParamInfo;
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
const AZ::SerializeContext::ClassElement* CUiAnimAzEntityNode::ComputeOffsetFromElementName(
    const AZ::SerializeContext::ClassData* classData,
    IUiAnimTrack* track,
    size_t baseOffset)
{
    const UiAnimParamData& paramData = track->GetParamData();

    // find the data element in the class data that matches the name in the paramData
    AZ::Crc32 nameCrc = AZ_CRC(paramData.GetName());
    const AZ::SerializeContext::ClassElement* element = nullptr;
    for (const AZ::SerializeContext::ClassElement& classElement : classData->m_elements)
    {
        if (classElement.m_nameCrc == nameCrc)
        {
            element = &classElement;
            break;
        }
    }

    // if the name doesn't exist or is of the wrong type then the animation data
    // no longer matches the component definition. This could happen if the serialization format of
    // a component is changed. We don't want to assert in that case. Ideally we would have
    // some way of converting the animation data. We do not have that yet. So we will output a warning
    // and recover.
    if (!element || element->m_typeId != paramData.GetTypeId())
    {
        bool mismatch = true;

        if (element)
        {
            // Allow AZ::Vector2 types to be assigned Vec2 animation data and AZ::Color types
            // to be assiged AZ::Vector3 animation data
            if (((element->m_typeId == AZ::SerializeTypeInfo<AZ::Vector2>::GetUuid()) && (paramData.GetTypeId() == AZ::SerializeTypeInfo<Vec2>::GetUuid()))
                || ((element->m_typeId == AZ::SerializeTypeInfo<AZ::Color>::GetUuid()) && (paramData.GetTypeId() == AZ::SerializeTypeInfo<AZ::Vector3>::GetUuid())))
            {
                mismatch = false;
            }
        }

        if (mismatch)
        {
            CryWarning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING,
                "Data mismatch reading animation data for type %s. The field \"%s\" %s. This part of the animation data will be ignored.",
                classData->m_typeId.ToString<AZStd::string>().c_str(),
                paramData.GetName(),
                (!element ? "cannot be found" : "has a different type to that in the animation data")
            );
            return nullptr;
        }
    }

    // Set the correct offset in the param data for the track
    UiAnimParamData newParamData(paramData.GetComponentId(), paramData.GetName(), element->m_typeId, baseOffset + element->m_offset);
    track->SetParamData(newParamData);

    return element;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::ComputeOffsetsFromElementNames()
{
    // Get the serialize context for the application
    AZ::SerializeContext* context = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    AZ_Assert(context, "No serialization context found");

    // Get the AZ entity that this node is animating
    AZ::Entity* entity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_entityId);
    if (!entity)
    {
        // This can happen, if a UI element is deleted we do not delete all AnimNodes
        // that reference it (which could be in multiple sequences). Instead we leave
        // them in the sequences and draw them in red and they have no effect.
        // If the canvas is saved like that and reloaded we come through here. The
        // AnimNode can't be made functional again at this point but is there so that
        // the user can see that their sequence was animating something that has now
        // been deleted.
        return;
    }

    // go through all its tracks and update the offsets
    for (int i = 0, num = (int)m_tracks.size(); i < num; i++)
    {
        IUiAnimTrack* track = m_tracks[i].get();

        if (track->GetParameterType() == eUiAnimParamType_AzComponentField)
        {
            // Get the class data for the component that this track is animating
            const UiAnimParamData& paramData = track->GetParamData();
            AZ::Component* component = paramData.GetComponent(entity);
            const AZ::Uuid& classId = AZ::SerializeTypeInfo<AZ::Component>::GetUuid(component);
            const AZ::SerializeContext::ClassData* classData = context->FindClassData(classId);

            // update the offset for the field this track is animating
            const AZ::SerializeContext::ClassElement* element = ComputeOffsetFromElementName(classData, track, 0);

            bool deleteTrack = false;
            if (element)
            {
                // the field is a valid field in the component, proceed with sub tracks if any
                size_t baseOffset = element->m_offset;
                const AZ::SerializeContext::ClassData* baseElementClassData = context->FindClassData(element->m_typeId);

                // Search the sub-tracks also if any.
                if (baseElementClassData && !baseElementClassData->m_elements.empty())
                {
                    for (int k = 0; k < track->GetSubTrackCount(); ++k)
                    {
                        IUiAnimTrack* pSubTrack = track->GetSubTrack(k);

                        if (pSubTrack->GetParameterType() == eUiAnimParamType_AzComponentField)
                        {
                            // update the offset for this subtrack
                            if (!ComputeOffsetFromElementName(baseElementClassData, pSubTrack, baseOffset))
                            {
                                deleteTrack = true; // animation data is no longer valid
                            }
                        }
                    }
                }
            }
            else
            {
                deleteTrack = true; // animation data is no longer valid
            }

            if (deleteTrack)
            {
                // delete track and leave as nullptr to be cleaned up after loop
                m_tracks[i].reset();
            }
        }
    }

    // remove any null entries from m_tracks (only happens if we found invalid animation data)
    stl::find_and_erase_if(m_tracks, [](const AZStd::intrusive_ptr<IUiAnimTrack>& sp) { return !sp; });
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CUiAnimAzEntityNode::GetParamName(const CUiAnimParamType& param) const
{
    SParamInfo info;
    if (GetParamInfoFromType(param, info))
    {
        return info.name;
    }

    const char* pName = param.GetName();
    if (param.GetType() == eUiAnimParamType_ByString && pName && strncmp(pName, kScriptTablePrefix, strlen(kScriptTablePrefix)) == 0)
    {
        return pName + strlen(kScriptTablePrefix);
    }

    return "Unknown Entity Parameter";
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CUiAnimAzEntityNode::GetParamNameForTrack(const CUiAnimParamType& param, const IUiAnimTrack* track) const
{
    // for Az Component Fields we use the name from the ClassElement
    if (param == eUiAnimParamType_AzComponentField)
    {
        // if the edit context is available it would be better to use the EditContext to get
        // the name? If so then we should pass than in as the name when creating the track
        return track->GetParamData().GetName();
    }

    SParamInfo info;
    if (GetParamInfoFromType(param, info))
    {
        return info.name;
    }

    const char* pName = param.GetName();
    if (param.GetType() == eUiAnimParamType_ByString && pName && strncmp(pName, kScriptTablePrefix, strlen(kScriptTablePrefix)) == 0)
    {
        return pName + strlen(kScriptTablePrefix);
    }

    return "Unknown Entity Parameter";
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::StillUpdate()
{
    // used to handle LookAt
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::EnableEntityPhysics([[maybe_unused]] bool bEnable)
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::Animate(SUiAnimContext& ec)
{
    if (!m_entityId.IsValid())
    {
        return;
    }

    AZ::Entity* entity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_entityId);
    if (!entity)
    {
        // This can happen, if a UI element is deleted we do not delete all AnimNodes
        // that reference it (which could be in multiple sequences). Instead we leave
        // them in the sequences and draw them in red and they have no effect. If the
        // delete is undone they will go back to working.
        return;
    }

    int trackCount = NumTracks();
    for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
    {
        IUiAnimTrack* track = m_tracks[paramIndex].get();

        CUiAnimParamType paramType = m_tracks[paramIndex]->GetParameterType();
        if ((track->HasKeys() == false)
            || (track->GetFlags() & IUiAnimTrack::eUiAnimTrackFlags_Disabled)
            || track->IsMasked(ec.trackMask))
        {
            continue;
        }

        AZ_Assert(paramType.GetType() == eUiAnimParamType_AzComponentField, "Invalid param type");

        const UiAnimParamData& paramData = track->GetParamData();

        AZ::Component* component = paramData.GetComponent(entity);

        if (!component)
        {
            continue;
        }

        void* elementData = reinterpret_cast<char*>(component) + paramData.GetOffset();

        if (paramData.GetTypeId() == AZ::SerializeTypeInfo<float>::GetUuid())
        {
            float* elementFloat = reinterpret_cast<float*>(elementData);

            float trackValue;
            track->GetValue(ec.time, trackValue);

            *elementFloat = trackValue;
        }
        else if (paramData.GetTypeId() == AZ::SerializeTypeInfo<bool>::GetUuid())
        {
            bool* elementValue = reinterpret_cast<bool*>(elementData);

            bool trackValue;
            track->GetValue(ec.time, trackValue);

            *elementValue = trackValue;
        }
        else if (paramData.GetTypeId() == AZ::SerializeTypeInfo<AZ::Vector2>::GetUuid())
        {
            AZ::Vector2* elementValue = reinterpret_cast<AZ::Vector2*>(elementData);

            AZ::Vector2 trackValue;
            track->GetValue(ec.time, trackValue);

            *elementValue = trackValue;
        }
        else if (paramData.GetTypeId() == AZ::SerializeTypeInfo<AZ::Vector3>::GetUuid())
        {
            AZ::Vector3* elementValue = reinterpret_cast<AZ::Vector3*>(elementData);

            AZ::Vector3 trackValue;
            track->GetValue(ec.time, trackValue);

            *elementValue = trackValue;
        }
        else if (paramData.GetTypeId() == AZ::SerializeTypeInfo<AZ::Vector4>::GetUuid())
        {
            AZ::Vector4* elementValue = reinterpret_cast<AZ::Vector4*>(elementData);

            AZ::Vector4 trackValue;
            track->GetValue(ec.time, trackValue);

            *elementValue = trackValue;
        }
        else if (paramData.GetTypeId() == AZ::SerializeTypeInfo<AZ::Color>::GetUuid())
        {
            AZ::Color* elementValue = reinterpret_cast<AZ::Color*>(elementData);

            AZ::Color trackValue = AZ::Color::CreateOne(); // Initialize alpha
            track->GetValue(ec.time, trackValue);

            *elementValue = trackValue;
        }
        else
        {
            // Animate the sub-tracks also if any.
            for (int k = 0; k < track->GetSubTrackCount(); ++k)
            {
                IUiAnimTrack* pSubTrack = track->GetSubTrack(k);

                const UiAnimParamData& subTrackParamData = pSubTrack->GetParamData();

                if (pSubTrack->GetParameterType() == eUiAnimParamType_AzComponentField)
                {
                    elementData = reinterpret_cast<char*>(component) + subTrackParamData.GetOffset();

                    if (subTrackParamData.GetTypeId() == AZ::SerializeTypeInfo<float>::GetUuid())
                    {
                        float* elementFloat = reinterpret_cast<float*>(elementData);

                        float trackValue;
                        pSubTrack->GetValue(ec.time, trackValue);

                        *elementFloat = trackValue;
                    }
                    else if (subTrackParamData.GetTypeId() == AZ::SerializeTypeInfo<bool>::GetUuid())
                    {
                        bool* elementValue = reinterpret_cast<bool*>(elementData);

                        bool trackValue;
                        pSubTrack->GetValue(ec.time, trackValue);

                        *elementValue = trackValue;
                    }
                }
            }
        }
    }

    m_time = ec.time;

    if (m_pOwner)
    {
        m_bIgnoreSetParam = true; // Prevents feedback change of track.
        m_pOwner->OnNodeUiAnimated(this);
        m_bIgnoreSetParam = false;
    }

    UiAnimateEntityBus::Event(m_entityId, &UiAnimateEntityBus::Events::PropertyValuesChanged);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::ReleaseSounds()
{
    // Audio: Stop all playing sounds
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::OnReset()
{
    m_lastEntityKey = -1;

    ReleaseSounds();
    UpdateDynamicParams();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::OnResetHard()
{
    OnReset();
    if (m_pOwner)
    {
        m_pOwner->OnNodeReset(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::Activate(bool bActivate)
{
    CUiAnimNode::Activate(bActivate);
    if (bActivate)
    {
#ifdef CHECK_FOR_TOO_MANY_ONPROPERTY_SCRIPT_CALLS
        m_OnPropertyCalls = 0;
#endif
    }
    else
    {
#ifdef CHECK_FOR_TOO_MANY_ONPROPERTY_SCRIPT_CALLS
        IEntity* pEntity = GetEntity();
        if (m_OnPropertyCalls > 30) // arbitrary amount
        {
            CryWarning(VALIDATOR_MODULE_SHINE, VALIDATOR_ERROR, "Entity: %s. A UI animation has called lua function 'OnPropertyChange' too many (%d) times .This is a performance issue. Adding Some custom management in the entity lua code will fix the issue", pEntity ? pEntity->GetName() : "<UNKNOWN", m_OnPropertyCalls);
        }
#endif
    }
};

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* CUiAnimAzEntityNode::GetTrackForAzField(const UiAnimParamData& param) const
{
    for (int i = 0, num = (int)m_tracks.size(); i < num; i++)
    {
        IUiAnimTrack* track = m_tracks[i].get();

        if (track->GetParameterType() == eUiAnimParamType_AzComponentField)
        {
            if (track->GetParamData() == param)
            {
                return track;
            }
        }

        // Search the sub-tracks also if any.
        for (int k = 0; k < track->GetSubTrackCount(); ++k)
        {
            IUiAnimTrack* pSubTrack = track->GetSubTrack(k);

            if (pSubTrack->GetParameterType() == eUiAnimParamType_AzComponentField)
            {
                if (pSubTrack->GetParamData() == param)
                {
                    return pSubTrack;
                }
            }
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* CUiAnimAzEntityNode::CreateTrackForAzField(const UiAnimParamData& param)
{
    AZ::SerializeContext* context = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    AZ_Assert(context, "No serialization context found");

    IUiAnimTrack* track = nullptr;

    const AZ::SerializeContext::ClassData* classData = context->FindClassData(param.GetTypeId());
    if (classData && !classData->m_elements.empty())
    {
        // this is a compound type, create a compound track

        // We only support compound tracks with 2, 3 or 4 subtracks
        int numElements = static_cast<int>(classData->m_elements.size());
        if (numElements < 2 || numElements > 4)
        {
            return nullptr;
        }

        EUiAnimValue valueType = eUiAnimValue_Unknown;
        switch (numElements)
        {
        case 2:
            valueType = eUiAnimValue_Vector2;
            break;
        case 3:
            valueType = eUiAnimValue_Vector3;
            break;
        case 4:
            valueType = eUiAnimValue_Vector4;
            break;
        }

        track = CreateTrackInternal(eUiAnimParamType_AzComponentField, eUiAnimCurveType_BezierFloat, valueType);

        track->SetParamData(param);

        int numSubTracks = track->GetSubTrackCount();
        int curSubTrack = 0;

        for (const AZ::SerializeContext::ClassElement& element : classData->m_elements)
        {
            if (element.m_typeId == AZ::SerializeTypeInfo<float>::GetUuid() && curSubTrack < numSubTracks)
            {
                IUiAnimTrack* pSubTrack = track->GetSubTrack(curSubTrack);

                pSubTrack->SetParameterType(eUiAnimParamType_AzComponentField);

                UiAnimParamData subTrackParam(param.GetComponentId(), element.m_name,
                    element.m_typeId, param.GetOffset() + element.m_offset);
                pSubTrack->SetParamData(subTrackParam);

                track->SetSubTrackName(curSubTrack, element.m_name);
                curSubTrack++;
            }
        }

        for (int i = curSubTrack; i < numElements; ++i)
        {
            track->SetSubTrackName(i, "_unused");  // only happens if some elements were not floats
        }
    }
    else if (param.GetTypeId() == AZ::SerializeTypeInfo<AZ::Vector2>::GetUuid())
    {
        track = CreateVectorTrack(param, eUiAnimValue_Vector2, 2);
    }
    else if (param.GetTypeId() == AZ::SerializeTypeInfo<AZ::Vector3>::GetUuid())
    {
        track = CreateVectorTrack(param, eUiAnimValue_Vector3, 3);
    }
    else if (param.GetTypeId() == AZ::SerializeTypeInfo<AZ::Vector4>::GetUuid())
    {
        track = CreateVectorTrack(param, eUiAnimValue_Vector4, 4);
    }
    else if (param.GetTypeId() == AZ::SerializeTypeInfo<AZ::Color>::GetUuid())
    {
        // this is a compound type, create a compound track
        track = CreateTrackInternal(eUiAnimParamType_AzComponentField, eUiAnimCurveType_BezierFloat, eUiAnimValue_Vector3);

        track->SetParamData(param);

        track->SetSubTrackName(0, "R");
        track->SetSubTrackName(1, "G");
        track->SetSubTrackName(2, "B");

        int numSubTracks = track->GetSubTrackCount();
        for (int i = 0; i < numSubTracks; ++i)
        {
            IUiAnimTrack* pSubTrack = track->GetSubTrack(i);

            pSubTrack->SetParameterType(eUiAnimParamType_Float);    // subtracks are not actual component properties
        }

        return track;
    }
    else
    {
        if (param.GetTypeId() == AZ::SerializeTypeInfo<float>::GetUuid())
        {
            track = CreateTrackInternal(eUiAnimParamType_AzComponentField, eUiAnimCurveType_BezierFloat, eUiAnimValue_Unknown);
        }
        else if (param.GetTypeId() == AZ::SerializeTypeInfo<bool>::GetUuid())
        {
            track = CreateTrackInternal(eUiAnimParamType_AzComponentField, eUiAnimCurveType_BezierFloat, eUiAnimValue_Bool);
        }
        else if (param.GetTypeId() == AZ::SerializeTypeInfo<int>::GetUuid())
        {
            // no support for int yet
            track = CreateTrackInternal(eUiAnimParamType_AzComponentField, eUiAnimCurveType_BezierFloat, eUiAnimValue_Bool);
        }
        else if (param.GetTypeId() == AZ::SerializeTypeInfo<unsigned int>::GetUuid())
        {
            // no support for int yet
            track = CreateTrackInternal(eUiAnimParamType_AzComponentField, eUiAnimCurveType_BezierFloat, eUiAnimValue_Bool);
        }

        track->SetParamData(param);
    }

    return track;
}

void CUiAnimAzEntityNode::OnStart()
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::OnPause()
{
    ReleaseSounds();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::OnStop()
{
    ReleaseSounds();
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimAzEntityNode::SetParamValueAz(float time, const UiAnimParamData& param, float value)
{
    IUiAnimTrack* track = GetTrackForAzField(param);
    if (track)
    {
        track->SetValue(time, value);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimAzEntityNode::SetParamValueAz(float time, const UiAnimParamData& param, bool value)
{
    IUiAnimTrack* track = GetTrackForAzField(param);
    if (track)
    {
        track->CreateKey(time);
        track->SetValue(time, value);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimAzEntityNode::SetParamValueAz(float time, const UiAnimParamData& param, [[maybe_unused]] int value)
{
    IUiAnimTrack* track = GetTrackForAzField(param);
    if (track)
    {
        track->CreateKey(time);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimAzEntityNode::SetParamValueAz(float time, const UiAnimParamData& param, [[maybe_unused]] unsigned int value)
{
    IUiAnimTrack* track = GetTrackForAzField(param);
    if (track)
    {
        track->CreateKey(time);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimAzEntityNode::SetParamValueAz(float time, const UiAnimParamData& param, const AZ::Vector2& value)
{
    IUiAnimTrack* track = GetTrackForAzField(param);
    if (track)
    {
        track->SetValue(time, value);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimAzEntityNode::SetParamValueAz(float time, const UiAnimParamData& param, const AZ::Vector3& value)
{
    IUiAnimTrack* track = GetTrackForAzField(param);
    if (track)
    {
        track->SetValue(time, value);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimAzEntityNode::SetParamValueAz(float time, const UiAnimParamData& param, const AZ::Vector4& value)
{
    IUiAnimTrack* track = GetTrackForAzField(param);
    if (track)
    {
        track->SetValue(time, value);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimAzEntityNode::SetParamValueAz(float time, const UiAnimParamData& param, const AZ::Color& value)
{
    IUiAnimTrack* track = GetTrackForAzField(param);
    if (track)
    {
        track->SetValue(time, value);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimAzEntityNode::GetParamValueAz(float time, const UiAnimParamData& param, float& value)
{
    IUiAnimTrack* track = GetTrackForAzField(param);
    if (track)
    {
        track->GetValue(time, value);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    CUiAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
    if (bLoading)
    {
        unsigned long idHi;
        unsigned long idLo;

        xmlNode->getAttr("EntityIdHi", idHi);
        xmlNode->getAttr("EntityIdLo", idLo);
        AZ::u64 id64 = ((AZ::u64)idHi) << 32 | idLo;
        m_entityId = AZ::EntityId(id64);
    }
    else
    {
        AZ::u64 id64 = static_cast<AZ::u64>(m_entityId);
        unsigned long idHi = id64 >> 32;
        unsigned long idLo = id64 & 0xFFFFFFFF;
        xmlNode->setAttr("EntityIdHi", idHi);
        xmlNode->setAttr("EntityIdLo", idLo);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimAzEntityNode::InitPostLoad(IUiAnimSequence* pSequence, bool remapIds, LyShine::EntityIdMap* entityIdMap)
{
    // do base class init first
    CUiAnimNode::InitPostLoad(pSequence, remapIds, entityIdMap);

    if (remapIds)
    {
        // the UI element entityIDs were changed on load, so update the entityId of the
        // entity this node is animating using the given map
        AZ::EntityId newId = (*entityIdMap)[m_entityId];
        if (newId.IsValid())
        {
            m_entityId = newId;
        }
    }

    // We don't save the offset for each track in serialized data because, if they added or removed
    // fields in a component, the offset would be invalid. So we compute the offset on load using the
    // field name and type to find it in the class data
    ComputeOffsetsFromElementNames();
}

void CUiAnimAzEntityNode::PrecacheStatic([[maybe_unused]] float time)
{
}

void CUiAnimAzEntityNode::PrecacheDynamic([[maybe_unused]] float time)
{
    // Used to update durations of all character animations.
}

IUiAnimTrack* CUiAnimAzEntityNode::CreateVectorTrack(const UiAnimParamData& param, EUiAnimValue valueType, int numElements)
{
    // this is a compound type, create a compound track
    IUiAnimTrack* track = CreateTrackInternal(eUiAnimParamType_AzComponentField, eUiAnimCurveType_BezierFloat, valueType);

    track->SetParamData(param);

    track->SetSubTrackName(0, "X");
    track->SetSubTrackName(1, "Y");
    if (numElements > 2)
    {
        track->SetSubTrackName(2, "Z");
        if (numElements > 3)
        {
            track->SetSubTrackName(3, "W");
        }
    }

    for (int i = 0; i < numElements; ++i)
    {
        IUiAnimTrack* pSubTrack = track->GetSubTrack(i);

        pSubTrack->SetParameterType(eUiAnimParamType_Float);    // subtracks are not actual component properties
    }

    return track;
}
