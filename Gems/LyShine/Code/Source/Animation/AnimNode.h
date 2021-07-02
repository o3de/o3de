/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Base of all Animation Nodes

#pragma once

#include <LyShine/Animation/IUiAnimation.h>
#include "UiAnimationSystem.h"


/*!
        Base class for all Animation nodes,
        can host multiple animation tracks, and execute them other time.
        Animation node is reference counted.
 */
class CUiAnimNode
    : public IUiAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CUiAnimNode, AZ::SystemAllocator, 0)
    AZ_RTTI(CUiAnimNode, "{1ECF3B73-FCED-464D-82E8-CFAF31BB63DC}", IUiAnimNode);

    struct SParamInfo
    {
        SParamInfo()
            : name("")
            , valueType(eUiAnimValue_Float)
            , flags(ESupportedParamFlags(0)) {};
        SParamInfo(const char* _name, CUiAnimParamType _paramType, EUiAnimValue _valueType, ESupportedParamFlags _flags)
            : name(_name)
            , paramType(_paramType)
            , valueType(_valueType)
            , flags(_flags) {};

        const char* name;               // parameter name.
        CUiAnimParamType paramType;     // parameter id.
        EUiAnimValue valueType;         // value type, defines type of track to use for animating this parameter.
        ESupportedParamFlags flags;     // combination of flags from ESupportedParamFlags.
    };

public:
    CUiAnimNode(const CUiAnimNode& other);
    CUiAnimNode(const int id, EUiAnimNodeType nodeType);
    CUiAnimNode();
    ~CUiAnimNode();
    EUiAnimNodeType GetType() const override { return m_nodeType; }

    //////////////////////////////////////////////////////////////////////////
    void add_ref() override;
    void release() override;
    //////////////////////////////////////////////////////////////////////////

    void SetName(const char* name) override { m_name = name; };
    const char* GetName() { return m_name.c_str(); };

    void SetSequence(IUiAnimSequence* pSequence) override { m_pSequence = pSequence; }
    // Return Animation Sequence that owns this node.
    IUiAnimSequence* GetSequence() const override { return m_pSequence; };

    void SetFlags(int flags);
    int GetFlags() const;

    IUiAnimationSystem* GetUiAnimationSystem() const { return m_pSequence->GetUiAnimationSystem(); };

    virtual void OnStart() {}
    void OnReset() override {}
    virtual void OnResetHard() { OnReset(); }
    virtual void OnPause() {}
    virtual void OnResume() {}
    virtual void OnStop() {}
    virtual void OnLoop() {}

    virtual Matrix34 GetReferenceMatrix() const;

    //////////////////////////////////////////////////////////////////////////
    bool IsParamValid(const CUiAnimParamType& paramType) const;
    virtual const char* GetParamName(const CUiAnimParamType& param) const;
    virtual EUiAnimValue GetParamValueType(const CUiAnimParamType& paramType) const;
    virtual IUiAnimNode::ESupportedParamFlags GetParamFlags(const CUiAnimParamType& paramType) const;
    virtual unsigned int GetParamCount() const { return 0; };

    bool SetParamValue(float time, CUiAnimParamType param, float val);
    bool SetParamValue(float time, CUiAnimParamType param, const Vec3& val);
    bool SetParamValue(float time, CUiAnimParamType param, const Vec4& val);
    bool GetParamValue(float time, CUiAnimParamType param, float& val);
    bool GetParamValue(float time, CUiAnimParamType param, Vec3& val);
    bool GetParamValue(float time, CUiAnimParamType param, Vec4& val);

    void SetTarget([[maybe_unused]] IUiAnimNode* node) {};
    IUiAnimNode* GetTarget() const { return 0; };

    void StillUpdate() {}
    void Animate(SUiAnimContext& ec) override;

    virtual void PrecacheStatic([[maybe_unused]] float startTime) {}
    virtual void PrecacheDynamic([[maybe_unused]] float time) {}

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;
    void SerializeUiAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;

    void SetNodeOwner(IUiAnimNodeOwner* pOwner) override;
    IUiAnimNodeOwner* GetNodeOwner() override { return m_pOwner; };

    // Called by sequence when needs to activate a node.
    virtual void Activate(bool bActivate);

    //////////////////////////////////////////////////////////////////////////
    void SetParent(IUiAnimNode* pParent) override;
    IUiAnimNode* GetParent() const override { return m_pParentNode; };
    IUiAnimNode* HasDirectorAsParent() const override;
    //////////////////////////////////////////////////////////////////////////

    void UpdateDynamicParams() override {};

    //////////////////////////////////////////////////////////////////////////
    // Track functions.
    //////////////////////////////////////////////////////////////////////////
    int  GetTrackCount() const override;
    IUiAnimTrack* GetTrackByIndex(int nIndex) const override;
    IUiAnimTrack* GetTrackForParameter(const CUiAnimParamType& paramType) const override;
    IUiAnimTrack* GetTrackForParameter(const CUiAnimParamType& paramType, uint32 index) const override;

    uint32 GetTrackParamIndex(const IUiAnimTrack* track) const override;

    IUiAnimTrack* GetTrackForAzField([[maybe_unused]] const UiAnimParamData& param) const override { return nullptr; }
    IUiAnimTrack* CreateTrackForAzField([[maybe_unused]] const UiAnimParamData& param) override { return nullptr; }

    virtual void SetTrack(const CUiAnimParamType& paramType, IUiAnimTrack* track);
    IUiAnimTrack* CreateTrack(const CUiAnimParamType& paramType) override;
    void SetTimeRange(Range timeRange) override;
    void AddTrack(IUiAnimTrack* track) override;
    bool RemoveTrack(IUiAnimTrack* track) override;
    void CreateDefaultTracks() override {};
    //////////////////////////////////////////////////////////////////////////

    void InitPostLoad(IUiAnimSequence* pSequence, bool remapIds, LyShine::EntityIdMap* entityIdMap) override;
    virtual void PostLoad();

    int GetId() const { return m_id; }
    void SetId(int id) { m_id = id; }
    const char* GetNameFast() const { return m_name.c_str(); }

    virtual void Render(){}

    static void Reflect(AZ::SerializeContext* serializeContext);

protected:
    virtual bool GetParamInfoFromType([[maybe_unused]] const CUiAnimParamType& paramType, [[maybe_unused]] SParamInfo& info) const { return false; };

    int  NumTracks() const { return (int)m_tracks.size(); }
    IUiAnimTrack* CreateTrackInternal(const CUiAnimParamType& paramType, EUiAnimCurveType trackType, EUiAnimValue valueType);

    IUiAnimTrack* CreateTrackInternalVector2(const CUiAnimParamType& paramType) const;
    IUiAnimTrack* CreateTrackInternalVector3(const CUiAnimParamType& paramType) const;
    IUiAnimTrack* CreateTrackInternalVector4(const CUiAnimParamType& paramType) const;
    IUiAnimTrack* CreateTrackInternalQuat(EUiAnimCurveType trackType, const CUiAnimParamType& paramType) const;
    IUiAnimTrack* CreateTrackInternalVector(EUiAnimCurveType trackType, const CUiAnimParamType& paramType, const EUiAnimValue animValue) const;
    IUiAnimTrack* CreateTrackInternalFloat(int trackType) const;
    UiAnimationSystem* GetUiAnimationSystemImpl() const { return (UiAnimationSystem*)GetUiAnimationSystem(); }

    // sets track animNode pointer to this node and sorts tracks
    void RegisterTrack(IUiAnimTrack* track);

    virtual bool NeedToRender() const { return false; }

protected:
    int m_refCount;
    EUiAnimNodeType m_nodeType;
    int m_id;
    AZStd::string m_name;
    IUiAnimSequence* m_pSequence;
    IUiAnimNodeOwner* m_pOwner;
    IUiAnimNode* m_pParentNode;
    int m_parentNodeId;
    int m_nLoadedParentNodeId;  // only used by old serialize
    int m_flags;
    unsigned int m_bIgnoreSetParam : 1; // Internal flags.

    typedef AZStd::vector<AZStd::intrusive_ptr<IUiAnimTrack>> AnimTracks;
    AnimTracks m_tracks;

private:
    void SortTracks();

    static bool TrackOrder(const AZStd::intrusive_ptr<IUiAnimTrack>& left, const AZStd::intrusive_ptr<IUiAnimTrack>& right);
};

//////////////////////////////////////////////////////////////////////////
class CUiAnimNodeGroup
    : public CUiAnimNode
{
public:
    CUiAnimNodeGroup(const int id)
        : CUiAnimNode(id, eUiAnimNodeType_Group) { SetFlags(GetFlags() | eUiAnimNodeFlags_CanChangeName); }
    EUiAnimNodeType GetType() const { return eUiAnimNodeType_Group; }

    virtual CUiAnimParamType GetParamType([[maybe_unused]] unsigned int nIndex) const { return eUiAnimParamType_Invalid; }
};
