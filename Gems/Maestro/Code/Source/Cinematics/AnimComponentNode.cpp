/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "AnimComponentNode.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Maestro/Bus/EditorSequenceComponentBus.h>
#include <Maestro/Bus/SequenceComponentBus.h>
#include <Maestro/Types/AnimNodeType.h>
#include <Maestro/Types/AnimValueType.h>
#include <Maestro/Types/AnimParamType.h>
#include <Maestro/Types/AssetBlends.h>

#include "CharacterTrack.h"

CAnimComponentNode::CAnimComponentNode(int id)
    : CAnimNode(id, AnimNodeType::Component)
    , m_componentTypeId(AZ::Uuid::CreateNull())
    , m_componentId(AZ::InvalidComponentId)
    , m_skipComponentAnimationUpdates(false)
{
}

CAnimComponentNode::CAnimComponentNode()
    : CAnimComponentNode(0)
{
}

CAnimComponentNode::~CAnimComponentNode()
{
    if (m_characterTrackAnimator)
    {
        delete m_characterTrackAnimator;
        m_characterTrackAnimator = nullptr;
    }
}

void CAnimComponentNode::OnStart()
{
}

void CAnimComponentNode::OnResume()
{
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::OnReset()
{
    // OnReset is called when sequences are loaded
    if (m_characterTrackAnimator)
    {
        m_characterTrackAnimator->OnReset(this);
    }
    UpdateDynamicParams();
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::OnResetHard()
{
    OnReset();
    if (m_pOwner)
    {
        m_pOwner->OnNodeReset(this);
    }
}

//////////////////////////////////////////////////////////////////////////
CAnimParamType CAnimComponentNode::GetParamType(unsigned int nIndex) const
{
    (void)nIndex;
    return AnimParamType::Invalid;
};

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::SetComponent(AZ::ComponentId componentId, const AZ::Uuid& componentTypeId)
{
    m_componentId = componentId;
    m_componentTypeId = componentTypeId;

    // call OnReset() to update dynamic params
    // (i.e. virtual properties from the exposed EBuses from the BehaviorContext)
    OnReset();
}

//////////////////////////////////////////////////////////////////////////
bool CAnimComponentNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    auto findIter = m_paramTypeToBehaviorPropertyInfoMap.find(paramId);
    if (findIter != m_paramTypeToBehaviorPropertyInfoMap.end())
    {
        info = findIter->second.m_animNodeParamInfo;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimComponentNode::SetTrackMultiplier(IAnimTrack* track) const
{
    bool trackMultiplierWasSet = false;

    CAnimParamType paramType(track->GetParameterType());

    if (paramType.GetType() == AnimParamType::ByString)
    {
        // check to see if we need to use a track multiplier

        // Get Property TypeId
        Maestro::SequenceComponentRequests::AnimatablePropertyAddress propertyAddress(m_componentId, paramType.GetName());
        AZ::Uuid propertyTypeId = AZ::Uuid::CreateNull();
        Maestro::SequenceComponentRequestBus::EventResult(propertyTypeId, m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedAddressTypeId,
            GetParentAzEntityId(), propertyAddress);

        if (propertyTypeId == AZ::Color::TYPEINFO_Uuid())
        {
            track->SetMultiplier(255.0f);
            trackMultiplierWasSet = true;
        }
    }

    return trackMultiplierWasSet;
}

int CAnimComponentNode::SetKeysForChangedBoolTrackValue(IAnimTrack* track, int keyIdx, float time)
{
    int retNumKeysSet = 0;
    bool currTrackValue;
    track->GetValue(time, currTrackValue);
    Maestro::SequenceComponentRequests::AnimatedBoolValue currValue(currTrackValue);
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, track->GetParameterType().GetName());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, currValue, GetParentAzEntityId(), animatableAddress);

    if (currTrackValue != currValue.GetBoolValue())
    {
        keyIdx = track->FindKey(time);
        if (keyIdx == -1)
        {
            keyIdx = track->CreateKey(time);
        }

        // no need to set a value of a Bool key - it's existence implies a Boolean toggle.
        retNumKeysSet++;
    }
    return retNumKeysSet;
}

int CAnimComponentNode::SetKeysForChangedFloatTrackValue(IAnimTrack* track, int keyIdx, float time)
{
    int retNumKeysSet = 0;
    float currTrackValue;
    track->GetValue(time, currTrackValue);
    Maestro::SequenceComponentRequests::AnimatedFloatValue currValue(currTrackValue);
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, track->GetParameterType().GetName());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, currValue, GetParentAzEntityId(), animatableAddress);

    if (currTrackValue != currValue.GetFloatValue())
    {
        keyIdx = track->FindKey(time);
        if (keyIdx == -1)
        {
            keyIdx = track->CreateKey(time);
        }

        if (track->GetValueType() == AnimValueType::DiscreteFloat)
        {
            IDiscreteFloatKey key;
            track->GetKey(keyIdx, &key);
            key.SetValue(currValue.GetFloatValue());
        }
        else
        {
            I2DBezierKey key;
            track->GetKey(keyIdx, &key);
            key.value.y = currValue.GetFloatValue();
            track->SetKey(keyIdx, &key);
        }
        retNumKeysSet++;
    }
    return retNumKeysSet;
}

int CAnimComponentNode::SetKeysForChangedVector3TrackValue(IAnimTrack* track, [[maybe_unused]] int keyIdx, float time, bool applyTrackMultiplier, float isChangedTolerance)
{
    int retNumKeysSet = 0;
    AZ::Vector3 currTrackValue;
    track->GetValue(time, currTrackValue, applyTrackMultiplier);
    Maestro::SequenceComponentRequests::AnimatedVector3Value currValue(currTrackValue);
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, track->GetParameterType().GetName());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, currValue, GetParentAzEntityId(), animatableAddress);
    AZ::Vector3 currVector3Value;
    currValue.GetValue(currVector3Value);
    if (!currTrackValue.IsClose(currVector3Value, isChangedTolerance))
    {
        // track will be a CCompoundSplineTrack. For these we can simply call SetValue at the and keys will be added if needed.
        track->SetValue(time, currVector3Value, false, applyTrackMultiplier);
        retNumKeysSet++;    // we treat the compound vector as a single key for simplicity and speed - if needed, we can go through each component and count them up if this is important.
    }
    return retNumKeysSet;
}

int CAnimComponentNode::SetKeysForChangedQuaternionTrackValue(IAnimTrack* track, [[maybe_unused]] int keyIdx, float time)
{
    int retNumKeysSet = 0;
    AZ::Quaternion currTrackValue;
    track->GetValue(time, currTrackValue);
    Maestro::SequenceComponentRequests::AnimatedQuaternionValue currValue(currTrackValue);
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, track->GetParameterType().GetName());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, currValue, GetParentAzEntityId(), animatableAddress);
    AZ::Quaternion currQuaternionValue;
    currValue.GetValue(currQuaternionValue);

    if (!currTrackValue.IsClose(currQuaternionValue))
    {
        // track will be a CCompoundSplineTrack. For these we can simply call SetValue at the and keys will be added if needed.
        track->SetValue(time, currQuaternionValue, false);
        retNumKeysSet++;    // we treat the compound vector as a single key for simplicity and speed - if needed, we can go through each component and count them up if this is important.
    }
    return retNumKeysSet;
}

//////////////////////////////////////////////////////////////////////////
int CAnimComponentNode::SetKeysForChangedTrackValues(float time)
{
    int retNumKeysSet = 0;

    for (int i = GetTrackCount(); --i >= 0;)
    {
        IAnimTrack* track = GetTrackByIndex(i);
        int keyIdx = -1;

        switch (track->GetValueType())
        {
            case AnimValueType::Bool:
                retNumKeysSet += SetKeysForChangedBoolTrackValue(track, keyIdx, time);
                break;
            case AnimValueType::Float:
            case AnimValueType::DiscreteFloat:
                retNumKeysSet += SetKeysForChangedFloatTrackValue(track, keyIdx, time);
                break;
            case AnimValueType::RGB:
                retNumKeysSet += SetKeysForChangedVector3TrackValue(track, keyIdx, time, true, (1.0f) / 255.0f);
                break;
            case AnimValueType::Vector:
                retNumKeysSet += SetKeysForChangedVector3TrackValue(track, keyIdx, time, true);
                break;
            case AnimValueType::Quat:
                retNumKeysSet += SetKeysForChangedQuaternionTrackValue(track, keyIdx, time);
                break;
            case AnimValueType::Vector4:
                AZ_Warning("TrackView", false, "Vector4's are not supported for recording.");
                break;
        }
    }

    return retNumKeysSet;
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::OnStartPlayInEditor()
{
    // reset key states for entering AI/Physics SIM mode
    ForceAnimKeyChangeInCharacterTrackAnimator();
}

void CAnimComponentNode::OnStopPlayInEditor()
{
    // reset key states for returning to Editor mode
    ForceAnimKeyChangeInCharacterTrackAnimator();
}

void CAnimComponentNode::SetNodeOwner(IAnimNodeOwner* pOwner)
{
    CAnimNode::SetNodeOwner(pOwner);
    if (pOwner && gEnv->IsEditor())
    {
        // SetNodeOwner is called when a node is added on undo/redo - we have to update dynamic params in such a case
        UpdateDynamicParams();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::GetParentWorldTransform(AZ::Transform& retTransform) const
{
    AZ::EntityId parentId;
    AZ::TransformBus::EventResult(parentId, GetParentAzEntityId(), &AZ::TransformBus::Events::GetParentId);

    if (parentId.IsValid())
    {
        AZ::TransformBus::EventResult(retTransform, parentId, &AZ::TransformBus::Events::GetWorldTM);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::ConvertBetweenWorldAndLocalPosition(Vec3& position, ETransformSpaceConversionDirection conversionDirection) const
{
    AZ::Vector3 pos(position.x, position.y, position.z);
    AZ::Transform parentTransform = AZ::Transform::Identity();

    GetParentWorldTransform(parentTransform);
    if (conversionDirection == eTransformConverstionDirection_toLocalSpace)
    {
        parentTransform.Invert();
    }
    pos = parentTransform.TransformPoint(pos);

    position.Set(pos.GetX(), pos.GetY(), pos.GetZ());
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::ConvertBetweenWorldAndLocalRotation(Quat& rotation, ETransformSpaceConversionDirection conversionDirection) const
{
    AZ::Quaternion rot(rotation.v.x, rotation.v.y, rotation.v.z, rotation.w);
    AZ::Transform rotTransform = AZ::Transform::CreateFromQuaternion(rot);
    rotTransform.ExtractUniformScale();

    AZ::Transform parentTransform = AZ::Transform::Identity();
    GetParentWorldTransform(parentTransform);
    parentTransform.ExtractUniformScale();
    if (conversionDirection == eTransformConverstionDirection_toLocalSpace)
    {
        parentTransform.Invert();
    }

    rotTransform = parentTransform * rotTransform;
    rot = rotTransform.GetRotation();

    rotation = Quat(rot);
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::ConvertBetweenWorldAndLocalScale(Vec3& scale, ETransformSpaceConversionDirection conversionDirection) const
{
    AZ::Transform parentTransform = AZ::Transform::Identity();
    AZ::Transform scaleTransform = AZ::Transform::CreateUniformScale(AZ::Vector3(scale.x, scale.y, scale.z).GetMaxElement());

    GetParentWorldTransform(parentTransform);
    if (conversionDirection == eTransformConverstionDirection_toLocalSpace)
    {
        parentTransform.Invert();
    }
    scaleTransform = parentTransform * scaleTransform;

    const float uniformScale = scaleTransform.GetUniformScale();
    scale.Set(uniformScale, uniformScale, uniformScale);
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::SetPos(float time, const Vec3& pos)
{
    if (m_componentTypeId == AZ::Uuid(AZ::EditorTransformComponentTypeId) || m_componentTypeId == AzFramework::TransformComponent::TYPEINFO_Uuid())
    {
        bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (GetParent()->GetFlags() & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded

        IAnimTrack* posTrack = GetTrackForParameter(AnimParamType::Position);
        if (posTrack)
        {
            // pos is in world position, even if the entity is parented - because Component Entity AZ::Transforms do not correctly set
            // CBaseObject parenting. This should probably be fixed, but for now, we explicitly change from World to Local space here
            Vec3 localPos(pos);
            ConvertBetweenWorldAndLocalPosition(localPos, eTransformConverstionDirection_toLocalSpace);

            posTrack->SetValue(time, localPos, bDefault);
        }

        if (!bDefault)
        {
            GetCMovieSystem()->Callback(IMovieCallback::CBR_CHANGETRACK, this);
        }
    }
}

Vec3 CAnimComponentNode::GetPos()
{
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, "Position");
    Maestro::SequenceComponentRequests::AnimatedVector3Value posValue(AZ::Vector3::CreateZero());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, posValue, GetParentAzEntityId(), animatableAddress);

    // Always return world position because Component Entity AZ::Transforms do not correctly set
    // CBaseObject parenting. This should probably be fixed, but for now, we explicitly change from Local to World space here.
    Vec3 worldPos(posValue.GetVector3Value());
    ConvertBetweenWorldAndLocalPosition(worldPos, eTransformConverstionDirection_toWorldSpace);

    return worldPos;
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::SetRotate(float time, const Quat& rotation)
{
    if (m_componentTypeId == AZ::Uuid(AZ::EditorTransformComponentTypeId) || m_componentTypeId == AzFramework::TransformComponent::TYPEINFO_Uuid())
    {
        bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (GetParent()->GetFlags() & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded

        IAnimTrack* rotTrack = GetTrackForParameter(AnimParamType::Rotation);
        if (rotTrack)
        {
            // Rotation is in world space, even if the entity is parented - because Component Entity AZ::Transforms do not correctly set
            // CBaseObject parenting, so we convert it to Local space here. This should probably be fixed, but for now, we explicitly change from World to Local space here.
            Quat localRot(rotation);
            ConvertBetweenWorldAndLocalRotation(localRot, eTransformConverstionDirection_toLocalSpace);
            rotTrack->SetValue(time, localRot, bDefault);
        }

        if (!bDefault)
        {
            GetCMovieSystem()->Callback(IMovieCallback::CBR_CHANGETRACK, this);
        }
    }
}

Quat CAnimComponentNode::GetRotate(float time)
{
    Quat worldRot;

    // If there is rotation track data, get the rotation from there.
    // Otherwise just use the current entity rotation value.
    IAnimTrack* rotTrack = GetTrackForParameter(AnimParamType::Rotation);
    if (rotTrack != nullptr && rotTrack->GetNumKeys() > 0)
    {
        rotTrack->GetValue(time, worldRot);

        // Track values are always stored as relative to the parent (local), so convert to world.
        ConvertBetweenWorldAndLocalRotation(worldRot, eTransformConverstionDirection_toWorldSpace);
    }
    else
    {
        worldRot = GetRotate();
    }

    return worldRot;
}

Quat CAnimComponentNode::GetRotate()
{
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, "Rotation");
    Maestro::SequenceComponentRequests::AnimatedQuaternionValue rotValue(AZ::Quaternion::CreateIdentity());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, rotValue, GetParentAzEntityId(), animatableAddress);

    // Always return world rotation because Component Entity AZ::Transforms do not correctly set
    // CBaseObject parenting. This should probably be fixed, but for now, we explicitly change from Local to World space here.
    Quat worldRot(rotValue.GetQuaternionValue());
    ConvertBetweenWorldAndLocalRotation(worldRot, eTransformConverstionDirection_toWorldSpace);

    return worldRot;
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::SetScale(float time, const Vec3& scale)
{
    if (m_componentTypeId == AZ::Uuid(AZ::EditorTransformComponentTypeId) || m_componentTypeId == AzFramework::TransformComponent::TYPEINFO_Uuid())
    {
        bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (GetParent()->GetFlags() & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded

        IAnimTrack* scaleTrack = GetTrackForParameter(AnimParamType::Scale);
        if (scaleTrack)
        {
            // Scale is in World space, even if the entity is parented - because Component Entity AZ::Transforms do not correctly set
            // CBaseObject parenting, so we convert it to Local space here. This should probably be fixed, but for now, we explicitly change from World to Local space here.
            Vec3 localScale(scale);
            ConvertBetweenWorldAndLocalScale(localScale, eTransformConverstionDirection_toLocalSpace);
            scaleTrack->SetValue(time, localScale, bDefault);
        }

        if (!bDefault)
        {
            GetCMovieSystem()->Callback(IMovieCallback::CBR_CHANGETRACK, this);
        }
    }
}

Vec3 CAnimComponentNode::GetScale()
{
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, "Scale");
    Maestro::SequenceComponentRequests::AnimatedVector3Value scaleValue(AZ::Vector3::CreateZero());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, scaleValue, GetParentAzEntityId(), animatableAddress);

    // Always return World scale because Component Entity AZ::Transforms do not correctly set
    // CBaseObject parenting. This should probably be fixed, but for now, we explicitly change from Local to World space here.
    Vec3 worldScale(scaleValue.GetVector3Value());
    ConvertBetweenWorldAndLocalScale(worldScale, eTransformConverstionDirection_toWorldSpace);

    return worldScale;
}

void CAnimComponentNode::Activate(bool bActivate)
{
    // Connect to EditorSequenceAgentComponentNotificationBus. The Sequence Agent Component
    // is always added to the Entity that is being animated aka the entity at GetParentAzEntityId().
    if (bActivate)
    {
        Maestro::EditorSequenceAgentComponentNotificationBus::Handler::BusConnect(GetParentAzEntityId());
    }
    else
    {
        Maestro::EditorSequenceAgentComponentNotificationBus::Handler::BusDisconnect();
    }
}

///////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::OnSequenceAgentConnected()
{
    // Whenever the Sequence Agent is connected to the Sequence, refresh the params.
    // This is redundant in most cases, but sometimes depending on the order of entity activation
    // a slice may be activated while a animated entity with the Sequence Agent is not active
    // (and thus not connected to the Sequence). This happens during save slice overrides.
    UpdateDynamicParamsInternal();
}

///////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::ForceAnimKeyChangeInCharacterTrackAnimator()
{
    if (m_characterTrackAnimator)
    {
        IAnimTrack* animTrack = GetTrackForParameter(AnimParamType::Animation);
        if (animTrack && animTrack->HasKeys())
        {
            // resets anim key change states so animation will update correctly on the next Animate()
            m_characterTrackAnimator->ForceAnimKeyChange();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CAnimComponentNode::CreateTrack(const CAnimParamType& paramType)
{
    IAnimTrack* retTrack = CAnimNode::CreateTrack(paramType);

    if (retTrack)
    {
        SetTrackMultiplier(retTrack);
        if (paramType.GetType() == AnimParamType::Animation && !m_characterTrackAnimator)
        {
            m_characterTrackAnimator = new CCharacterTrackAnimator();
        }
    }

    return retTrack;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimComponentNode::RemoveTrack(IAnimTrack* pTrack)
{
    if (pTrack && pTrack->GetParameterType().GetType() == AnimParamType::Animation && m_characterTrackAnimator)
    {
        delete m_characterTrackAnimator;
        m_characterTrackAnimator = nullptr;
    }

    return CAnimNode::RemoveTrack(pTrack);
}

//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
void CAnimComponentNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
    if (bLoading)
    {
        XmlString uuidString;

        xmlNode->getAttr("ComponentId", m_componentId);
        if (xmlNode->getAttr("ComponentTypeId", uuidString))
        {
            m_componentTypeId = AZ::Uuid::CreateString(uuidString);
        }
        else
        {
            m_componentTypeId = AZ::Uuid::CreateNull();
        }
    }
    else
    {
        // saving
        xmlNode->setAttr("ComponentId", m_componentId);
        xmlNode->setAttr("ComponentTypeId", m_componentTypeId.ToFixedString().c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
// Property Value types are detected in this function
void CAnimComponentNode::AddPropertyToParamInfoMap(const CAnimParamType& paramType)
{
    BehaviorPropertyInfo propertyInfo;                  // the default value type is AnimValueType::Float
    {
        // property is handled by Component animation (Behavior Context getter/setters). Regardless of the param Type, it must have a non-empty name
        // (the VirtualProperty name)
        AZ_Assert(paramType.GetName() && strlen(paramType.GetName()), "All AnimParamTypes animated on Components must have a name for its VirtualProperty");

        // Initialize property name string, which sets to AnimParamType::ByString by default
        propertyInfo = paramType.GetName();

        if (paramType.GetType() != AnimParamType::ByString)
        {
            // This sets the eAnimParamType enumeration but leaves the string name untouched
            propertyInfo.m_animNodeParamInfo.paramType = paramType.GetType();
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////
        //! Detect the value type from reflection in the Behavior Context
        //
        // Query the property type Id from the Sequence Component and set it if a supported type is found
        AZ::Uuid propertyTypeId = AZ::Uuid::CreateNull();
        Maestro::SequenceComponentRequests::AnimatablePropertyAddress propertyAddress(m_componentId, propertyInfo.m_displayName.c_str());

        Maestro::SequenceComponentRequestBus::EventResult(propertyTypeId, m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedAddressTypeId,
            GetParentAzEntityId(), propertyAddress);

        if (propertyTypeId == AZ::Vector3::TYPEINFO_Uuid())
        {
            propertyInfo.m_animNodeParamInfo.valueType = AnimValueType::Vector;
        }
        else if (propertyTypeId == AZ::Color::TYPEINFO_Uuid())
        {
            propertyInfo.m_animNodeParamInfo.valueType = AnimValueType::RGB;
        }
        else if (propertyTypeId == AZ::Quaternion::TYPEINFO_Uuid())
        {
            propertyInfo.m_animNodeParamInfo.valueType = AnimValueType::Quat;
        }
        else if (propertyTypeId == AZ::AzTypeInfo<bool>::Uuid())
        {
            propertyInfo.m_animNodeParamInfo.valueType = AnimValueType::Bool;
        }
        // Special case, if an AssetId property named "Motion" is found, create an AssetBlend.
        // The Simple Motion Component exposes a virtual property named "motion" of type AssetId.
        // We it is detected here create an AssetBlend type in Track View. The Asset Blend has special
        // UI and will be used to drive mulitple properties on this component, not just the motion AssetId.
        else if (propertyTypeId == AZ::Data::AssetId::TYPEINFO_Uuid() && 0 == azstricmp(paramType.GetName(), "motion"))
        {
            propertyInfo.m_animNodeParamInfo.valueType = AnimValueType::AssetBlend;
        }
        // the fall-through default type is propertyInfo.m_animNodeParamInfo.valueType = AnimValueType::Float
    }

    m_paramTypeToBehaviorPropertyInfoMap[paramType] = propertyInfo;
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<CAnimComponentNode, CAnimNode>()
            ->Version(1)
            ->Field("ComponentID", &CAnimComponentNode::m_componentId)
            ->Field("ComponentTypeID", &CAnimComponentNode::m_componentTypeId);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::UpdateDynamicParams_Editor()
{
    IAnimNode::AnimParamInfos animatableParams;

    // add all parameters supported by the component
    Maestro::EditorSequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::EditorSequenceComponentRequestBus::Events::GetAllAnimatablePropertiesForComponent,
                                                          animatableParams, GetParentAzEntityId(), m_componentId);

    for (int i = 0; i < animatableParams.size(); i++)
    {
        AddPropertyToParamInfoMap(animatableParams[i].paramType);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::UpdateDynamicParams_Game()
{
    // Fill m_paramTypeToBehaviorPropertyInfoMap based on our animated tracks
    for (uint32 i = 0; i < m_tracks.size(); ++i)
    {
        AddPropertyToParamInfoMap(m_tracks[i]->GetParameterType());
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::UpdateDynamicParamsInternal()
{
    m_paramTypeToBehaviorPropertyInfoMap.clear();

    // editor stores *all* properties of *every* entity used in an AnimEntityNode.
    // In pure game mode we just need to store the properties that we know are going to be used in a track, so we can save a lot of memory.
    if (gEnv->IsEditor() && !gEnv->IsEditorSimulationMode() && !gEnv->IsEditorGameMode())
    {
        UpdateDynamicParams_Editor();
    }
    else
    {
        UpdateDynamicParams_Game();
    }

    // Go through all tracks and set Multipliers if required
    for (uint32 i = 0; i < m_tracks.size(); ++i)
    {
        AZStd::intrusive_ptr<IAnimTrack> track = m_tracks[i];
        SetTrackMultiplier(track.get());
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType)
{
    // Initialize new track to property value
    if (paramType.GetType() == AnimParamType::ByString && pTrack)
    {
        auto findIter = m_paramTypeToBehaviorPropertyInfoMap.find(paramType);
        if (findIter != m_paramTypeToBehaviorPropertyInfoMap.end())
        {
            BehaviorPropertyInfo& propertyInfo = findIter->second;

            Maestro::SequenceComponentRequests::AnimatablePropertyAddress address(m_componentId, propertyInfo.m_animNodeParamInfo.name);

            switch (pTrack->GetValueType())
            {
                case AnimValueType::Float:
                {
                    Maestro::SequenceComponentRequests::AnimatedFloatValue defaultValue(.0f);

                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, defaultValue, GetParentAzEntityId(), address);
                    pTrack->SetValue(0, defaultValue.GetFloatValue(), true);
                    break;
                }
                case AnimValueType::Vector:
                {
                    Maestro::SequenceComponentRequests::AnimatedVector3Value defaultValue(AZ::Vector3::CreateZero());
                    AZ::Vector3 vector3Value = AZ::Vector3::CreateZero();

                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, defaultValue, GetParentAzEntityId(), address);
                    defaultValue.GetValue(vector3Value);

                    pTrack->SetValue(0, Vec3(vector3Value.GetX(), vector3Value.GetY(), vector3Value.GetZ()), true);
                    break;
                }
                case AnimValueType::Quat:
                {
                    Maestro::SequenceComponentRequests::AnimatedQuaternionValue defaultValue(AZ::Quaternion::CreateIdentity());
                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, defaultValue, GetParentAzEntityId(), address);
                    pTrack->SetValue(0, Quat(defaultValue.GetQuaternionValue()), true);
                    break;
                }
                case AnimValueType::RGB:
                {
                    Maestro::SequenceComponentRequests::AnimatedVector3Value defaultValue(AZ::Vector3::CreateOne());
                    AZ::Vector3 vector3Value = AZ::Vector3::CreateOne();

                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, defaultValue, GetParentAzEntityId(), address);
                    defaultValue.GetValue(vector3Value);

                    pTrack->SetValue(0, Vec3(clamp_tpl((float)vector3Value.GetX(), .0f, 1.0f), clamp_tpl((float)vector3Value.GetY(), .0f, 1.0f), clamp_tpl((float)vector3Value.GetZ(), .0f, 1.0f)), /*setDefault=*/ true, /*applyMultiplier=*/ true);
                    break;
                }
                case AnimValueType::Bool:
                {
                    Maestro::SequenceComponentRequests::AnimatedBoolValue defaultValue(true);

                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, defaultValue, GetParentAzEntityId(), address);

                    pTrack->SetValue(0, defaultValue.GetBoolValue(), true);
                    break;
                }
                case AnimValueType::AssetBlend:
                {
                    // Just init to an empty value.
                    Maestro::AssetBlends<AZ::Data::AssetData> assetData;
                    pTrack->SetValue(0, assetData, true);
                    break;
                }
                default:
                {
                    AZ_Warning("TrackView", false, "Unsupported value type requested for Component Node Track %s, skipping...", paramType.GetName());
                    break;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::Animate(SAnimContext& ac)
{
    if (m_skipComponentAnimationUpdates)
    {
        return;
    }

    // Evaluate all tracks

    // indices used for character animation (SimpleAnimationComponent)
    int characterAnimationLayer = 0;
    int characterAnimationTrackIdx = 0;

    int trackCount = NumTracks();
    for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
    {
        CAnimParamType paramType = m_tracks[paramIndex]->GetParameterType();
        IAnimTrack* pTrack = m_tracks[paramIndex].get();

        if ((pTrack->HasKeys() == false) || (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled) || pTrack->IsMasked(ac.trackMask))
        {
            continue;
        }

        if (!ac.resetting)
        {
            if (paramType.GetType() == AnimParamType::Animation)
            {
                // special handling for Character Animation. We short-circuit the SimpleAnimation behavior using m_characterTrackAnimator
                if (!m_characterTrackAnimator)
                {
                    m_characterTrackAnimator = new CCharacterTrackAnimator;
                }

                if (characterAnimationLayer < MAX_CHARACTER_TRACKS + ADDITIVE_LAYERS_OFFSET)
                {
                    int index = characterAnimationLayer;
                    CCharacterTrack* pCharTrack = (CCharacterTrack*)pTrack;
                    if (pCharTrack->GetAnimationLayerIndex() >= 0)   // If the track has an animation layer specified,
                    {
                        assert(pCharTrack->GetAnimationLayerIndex() < 16);
                        index = pCharTrack->GetAnimationLayerIndex();  // use it instead.
                    }

                    m_characterTrackAnimator->AnimateTrack(pCharTrack, ac, index, characterAnimationTrackIdx);

                    if (characterAnimationLayer == 0)
                    {
                        characterAnimationLayer += ADDITIVE_LAYERS_OFFSET;
                    }
                    ++characterAnimationLayer;
                    ++characterAnimationTrackIdx;
                }
            }
            else
            {
                // handle all other non-specialized Components
                auto findIter = m_paramTypeToBehaviorPropertyInfoMap.find(paramType);
                if (findIter != m_paramTypeToBehaviorPropertyInfoMap.end())
                {
                    BehaviorPropertyInfo& propertyInfo = findIter->second;
                    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, propertyInfo.m_animNodeParamInfo.name);

                    switch (pTrack->GetValueType())
                    {
                        case AnimValueType::Float:
                        {
                            if (pTrack->HasKeys())
                            {
                                float floatValue = .0f;
                                pTrack->GetValue(ac.time, floatValue, /*applyMultiplier= */ true);
                                Maestro::SequenceComponentRequests::AnimatedFloatValue value(floatValue);

                                Maestro::SequenceComponentRequests::AnimatedFloatValue prevValue(floatValue);
                                Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevValue, GetParentAzEntityId(), animatableAddress);
                                if (!value.IsClose(prevValue))
                                {
                                    // only set the value if it's changed
                                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), animatableAddress, value);
                                }
                            }
                            break;
                        }
                        case AnimValueType::Vector:     // fall-through
                        case AnimValueType::RGB:
                        {
                            float tolerance = AZ::Constants::FloatEpsilon;
                            Vec3 vec3Value(.0f, .0f, .0f);
                            pTrack->GetValue(ac.time, vec3Value, /*applyMultiplier= */ true);
                            AZ::Vector3 vector3Value(vec3Value.x, vec3Value.y, vec3Value.z);

                            if (pTrack->GetValueType() == AnimValueType::RGB)
                            {
                                vec3Value.x = clamp_tpl(vec3Value.x, 0.0f, 1.0f);
                                vec3Value.y = clamp_tpl(vec3Value.y, 0.0f, 1.0f);
                                vec3Value.z = clamp_tpl(vec3Value.z, 0.0f, 1.0f);

                                // set tolerance to just under 1 unit in normalized RGB space
                                tolerance = (1.0f - AZ::Constants::FloatEpsilon) / 255.0f;
                            }

                            Maestro::SequenceComponentRequests::AnimatedVector3Value value(AZ::Vector3(vec3Value.x, vec3Value.y, vec3Value.z));

                            Maestro::SequenceComponentRequests::AnimatedVector3Value prevValue(AZ::Vector3(vec3Value.x, vec3Value.y, vec3Value.z));
                            Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevValue, GetParentAzEntityId(), animatableAddress);
                            AZ::Vector3 vector3PrevValue;
                            prevValue.GetValue(vector3PrevValue);

                            // Check sub-tracks for keys. If there are none, use the prevValue for that track (essentially making a non-keyed track a no-op)
                            vector3Value.Set(pTrack->GetSubTrack(0)->HasKeys() ? vector3Value.GetX() : vector3PrevValue.GetX(),
                                pTrack->GetSubTrack(1)->HasKeys() ? vector3Value.GetY() : vector3PrevValue.GetY(),
                                pTrack->GetSubTrack(2)->HasKeys() ? vector3Value.GetZ() : vector3PrevValue.GetZ());
                            value.SetValue(vector3Value);

                            if (!value.IsClose(prevValue, tolerance))
                            {
                                // only set the value if it's changed
                                Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), animatableAddress, value);
                            }
                            break;
                        }
                        case AnimValueType::Quat:
                        {
                            if (pTrack->HasKeys())
                            {
                                float tolerance = AZ::Constants::FloatEpsilon;

                                AZ::Quaternion quaternionValue(AZ::Quaternion::CreateIdentity());
                                pTrack->GetValue(ac.time, quaternionValue);
                                Maestro::SequenceComponentRequests::AnimatedQuaternionValue value(quaternionValue);
                                Maestro::SequenceComponentRequests::AnimatedQuaternionValue prevValue(quaternionValue);
                                Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevValue, GetParentAzEntityId(), animatableAddress);
                                AZ::Quaternion prevQuaternionValue;
                                prevValue.GetValue(prevQuaternionValue);

                                if (!prevQuaternionValue.IsClose(quaternionValue, tolerance))
                                {
                                    // only set the value if it's changed
                                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), animatableAddress, value);
                                }
                            }
                            break;
                        }
                        case AnimValueType::Bool:
                        {
                            if (pTrack->HasKeys())
                            {
                                bool boolValue = true;
                                pTrack->GetValue(ac.time, boolValue);
                                Maestro::SequenceComponentRequests::AnimatedBoolValue value(boolValue);

                                Maestro::SequenceComponentRequests::AnimatedBoolValue prevValue(boolValue);
                                Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevValue, GetParentAzEntityId(), animatableAddress);
                                if (!value.IsClose(prevValue))
                                {
                                    // only set the value if it's changed
                                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), animatableAddress, value);
                                }
                            }
                            break;
                        }
                        case AnimValueType::AssetBlend:
                        {
                            if (pTrack->HasKeys())
                            {
                                // Get the AssetBlends from the Track and update Properties on Component
                                Maestro::AssetBlends<AZ::Data::AssetData> assetBlendValue;
                                pTrack->GetValue(ac.time, assetBlendValue);
                                AnimateAssetBlendSubProperties(assetBlendValue);
                            }
                            break;
                        }
                        default:
                        {
                            AZ_Warning("TrackView", false, "Unsupported value type %d requested for Component Node Track %s, skipping...", pTrack->GetValueType(), paramType.GetName());
                            break;
                        }
                    }
                }
            }
        }
    }

    if (m_pOwner)
    {
        m_bIgnoreSetParam = true; // Prevents feedback change of track.
        m_pOwner->OnNodeAnimated(this);
        m_bIgnoreSetParam = false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::AnimateAssetBlendSubProperties(const Maestro::AssetBlends<AZ::Data::AssetData>& assetBlendValue)
{
    // These are the params to set for the Simple Motion Component
    bool previewInEditor = true;
    float playTime = 0.0f;
    float playSpeed = 0.0f;
    AZ::Data::AssetId assetId;
    float blendInTime = 0.0f;
    float blendOutTime = 0.0f;

    // Populate params based on the last AssetBlend found.
    // So new keys will be picked up and played on top of currently
    // playing animations (resulting in a blend).
    if (assetBlendValue.m_assetBlends.size() > 0)
    {
        Maestro::AssetBlend assetData = assetBlendValue.m_assetBlends.back();
        playTime = assetData.m_time;
        assetId = assetData.m_assetId;
        blendInTime = assetData.m_blendInTime;
        blendOutTime = assetData.m_blendOutTime;
    }

    // Set Preview in Editor
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress previewInEditorAnimatableAddress(m_componentId, "PreviewInEditor");
    Maestro::SequenceComponentRequests::AnimatedFloatValue prevPreviewInEditorValue(previewInEditor);
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevPreviewInEditorValue, GetParentAzEntityId(), previewInEditorAnimatableAddress);
    Maestro::SequenceComponentRequests::AnimatedFloatValue previewInEditorValue(previewInEditor);
    if (!previewInEditorValue.IsClose(prevPreviewInEditorValue))
    {
        Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), previewInEditorAnimatableAddress, previewInEditorValue);
    }

    // Set Blend In Time before Motion so the Blend In will be used on the Motion that is about to Play.
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress blendInTimeAnimatableAddress(m_componentId, "BlendInTime");
    Maestro::SequenceComponentRequests::AnimatedFloatValue prevBlendInTimeValue(blendInTime);
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevBlendInTimeValue, GetParentAzEntityId(), blendInTimeAnimatableAddress);
    Maestro::SequenceComponentRequests::AnimatedFloatValue blendInTimeValue(blendInTime);
    if (!blendInTimeValue.IsClose(prevBlendInTimeValue))
    {
        Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), blendInTimeAnimatableAddress, blendInTimeValue);
    }

    // Set Motion
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress motionAnimatableAddress(m_componentId, "Motion");
    Maestro::SequenceComponentRequests::AnimatedAssetIdValue prevMotionValue(assetId);
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevMotionValue, GetParentAzEntityId(), motionAnimatableAddress);
    Maestro::SequenceComponentRequests::AnimatedAssetIdValue motionValue(assetId);
    if (!motionValue.IsClose(prevMotionValue))
    {
        Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), motionAnimatableAddress, motionValue);
    }

    // Set Blend Out Time after Motion so the Blend Out will not be used on Play, but instead used on that 'last' Motion 'Stop' as fade out.
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress blendOutTimeAnimatableAddress(m_componentId, "BlendOutTime");
    Maestro::SequenceComponentRequests::AnimatedFloatValue prevBlendOutTimeValue(blendOutTime);
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevBlendOutTimeValue, GetParentAzEntityId(), blendOutTimeAnimatableAddress);
    Maestro::SequenceComponentRequests::AnimatedFloatValue blendOutTimeValue(blendOutTime);
    if (!blendOutTimeValue.IsClose(prevBlendOutTimeValue))
    {
        Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), blendOutTimeAnimatableAddress, blendOutTimeValue);
    }

    // Set Play Time
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress playTimeAnimatableAddress(m_componentId, "PlayTime");
    Maestro::SequenceComponentRequests::AnimatedFloatValue prevPlayTimeValue(playTime);
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevPlayTimeValue, GetParentAzEntityId(), playTimeAnimatableAddress);
    Maestro::SequenceComponentRequests::AnimatedFloatValue playTimeValue(playTime);
    if (!playTimeValue.IsClose(prevPlayTimeValue))
    {
        Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), playTimeAnimatableAddress, playTimeValue);
    }

    // Set Play Speed
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress playSpeedAnimatableAddress(m_componentId, "PlaySpeed");
    Maestro::SequenceComponentRequests::AnimatedFloatValue prevPlaySpeedValue(playSpeed);
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevPlaySpeedValue, GetParentAzEntityId(), playSpeedAnimatableAddress);
    Maestro::SequenceComponentRequests::AnimatedFloatValue playSpeedValue(playSpeed);
    if (!playSpeedValue.IsClose(prevPlaySpeedValue))
    {
        Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), playSpeedAnimatableAddress, playSpeedValue);
    }
}
