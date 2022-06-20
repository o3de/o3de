/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_ANIMCOMPONENTNODE_H
#define CRYINCLUDE_CRYMOVIE_ANIMCOMPONENTNODE_H

#pragma once

#include "AnimNode.h"
#include "CharacterTrackAnimator.h"
#include <Maestro/Bus/EditorSequenceAgentComponentBus.h>

/**
 * CAnimComponentNode
 *
 * All animation on AZ::Entity nodes are keyed to tracks on CAnimComponentNodes.
 *
 */

class CAnimComponentNode
    : public CAnimNode
    , public Maestro::EditorSequenceAgentComponentNotificationBus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(CAnimComponentNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimComponentNode, "{722F3D0D-7AEB-46B7-BF13-D5C7A828E9BD}", CAnimNode);

    CAnimComponentNode(const int id);
    CAnimComponentNode();
    ~CAnimComponentNode();

    AZ::EntityId GetParentAzEntityId() const { return m_pParentNode ? m_pParentNode->GetAzEntityId() : AZ::EntityId(); }

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CAnimNode
    void Animate(SAnimContext& ac) override;
    
    void OnStart() override;
    void OnResume() override;

    void OnReset() override;
    void OnResetHard() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Overrides from IAnimNode
    // ComponentNodes use reflection for typing - return invalid for this pure virtual for the legacy system
    CAnimParamType GetParamType(unsigned int nIndex) const override;

    void     SetComponent(AZ::ComponentId componentId, const AZ::Uuid& typeId) override;

    // returns the componentId of the component the node is associate with, if applicable, or a AZ::InvalidComponentId otherwise
    AZ::ComponentId GetComponentId() const override { return m_componentId; }

    int SetKeysForChangedTrackValues(float time) override;

    void OnStartPlayInEditor() override;
    void OnStopPlayInEditor() override;

    void SetNodeOwner(IAnimNodeOwner* pOwner) override;

    void SetPos(float time, const Vec3& pos) override;
    void SetRotate(float time, const Quat& quat) override;
    void SetScale(float time, const Vec3& scale) override;

    Vec3 GetPos() override;
    Quat GetRotate() override;
    Quat GetRotate(float time) override;
    Vec3 GetScale() override;

    void Activate(bool bActivate) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Override CreateTrack to handle trackMultipliers for Component Tracks
    IAnimTrack* CreateTrack(const CAnimParamType& paramType) override;
    bool RemoveTrack(IAnimTrack* pTrack) override;

    //////////////////////////////////////////////////////////////////////////
    // EditorSequenceAgentComponentNotificationBus::Handler Interface
    void OnSequenceAgentConnected() override;

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;

    const AZ::Uuid& GetComponentTypeId() const { return m_componentTypeId; }

    // Skips Event Bus update of Components during animation - used for when another system is overriding a component's properties,
    // such as during camera interpolation between two Transforms. This will silently make the Animate() method do nothing - use only if you
    // know what you're doing!
    void SetSkipComponentAnimationUpdates(bool skipAnimationUpdates)
    {
        m_skipComponentAnimationUpdates = skipAnimationUpdates;
    }

    static void Reflect(AZ::ReflectContext* context);

protected:
    // functions involved in the process to parse and store component behavior context animated properties
    void UpdateDynamicParamsInternal() override;
    void UpdateDynamicParams_Editor();
    void UpdateDynamicParams_Game();
    
    void InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType) override;

    bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const override;


private:

    enum ETransformSpaceConversionDirection
    {
        eTransformConverstionDirection_toWorldSpace = 0,
        eTransformConverstionDirection_toLocalSpace
    };

    // methods to convert world transforms to local transforms to account for Transform Delegate bug working solely
    // in world space
    void GetParentWorldTransform(AZ::Transform& retTransform) const;
    void ConvertBetweenWorldAndLocalPosition(Vec3& position, ETransformSpaceConversionDirection conversionDirection) const;
    void ConvertBetweenWorldAndLocalRotation(Quat& rotation, ETransformSpaceConversionDirection conversionDirection) const;
    void ConvertBetweenWorldAndLocalScale(Vec3& scale, ETransformSpaceConversionDirection conversionDirection) const;

    // Utility function to query the units for a track and set the track multiplier if needed. Returns true if track multiplier was set.
    bool SetTrackMultiplier(IAnimTrack* track) const;

    void ForceAnimKeyChangeInCharacterTrackAnimator();

    // typed support functions for SetKeysForChangedTrackValues()
    int SetKeysForChangedBoolTrackValue(IAnimTrack* track, int keyIdx, float time);
    int SetKeysForChangedFloatTrackValue(IAnimTrack* track, int keyIdx, float time);
    int SetKeysForChangedVector3TrackValue(IAnimTrack* track, int keyIdx, float time, bool applyTrackMultiplier = true, float isChangedTolerance = AZ::Constants::Tolerance);
    int SetKeysForChangedQuaternionTrackValue(IAnimTrack* track, int keyIdx, float time);

    // Helper function to set individual properties on Simple Motion Component from an AssetBlend Track.
    void AnimateAssetBlendSubProperties(const Maestro::AssetBlends<AZ::Data::AssetData>& assetBlendValue);

    class BehaviorPropertyInfo
    {
    public:
        BehaviorPropertyInfo() {}
        BehaviorPropertyInfo(const AZStd::string& name)
        {
            *this = name;
        }
        BehaviorPropertyInfo(const BehaviorPropertyInfo& other)
        {
            m_displayName = other.m_displayName;
            m_animNodeParamInfo.paramType = other.m_displayName;
            m_animNodeParamInfo.name = m_displayName;
        }
        BehaviorPropertyInfo& operator=(const AZStd::string& str)
        {
            // TODO: clean this up - this weird memory sharing was copied from legacy Cry - could be better.
            m_displayName = str;
            m_animNodeParamInfo.paramType = str;   // set type to AnimParamType::ByString by assigning a string
            m_animNodeParamInfo.name = m_displayName;
            return *this;
        }

        AZStd::string m_displayName;
        SParamInfo m_animNodeParamInfo;
    };
    
    void AddPropertyToParamInfoMap(const CAnimParamType& paramType);

    int m_refCount;     // intrusive_ptr ref counter

    AZ::Uuid                                m_componentTypeId;
    AZ::ComponentId                         m_componentId;

    // a mapping of CAnimParmTypes to SBehaviorPropertyInfo structs for each virtual property
    AZStd::unordered_map<CAnimParamType, BehaviorPropertyInfo>   m_paramTypeToBehaviorPropertyInfoMap;

    // helper class responsible for animating Character Tracks (aka 'Animation' tracks in the TrackView UI)
    CCharacterTrackAnimator*   m_characterTrackAnimator = nullptr;

    bool m_skipComponentAnimationUpdates;
};
#endif // CRYINCLUDE_CRYMOVIE_ANIMCOMPONENTNODE_H
