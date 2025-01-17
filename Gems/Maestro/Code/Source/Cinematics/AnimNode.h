/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Base of all Animation Nodes

#pragma once

#include <IMovieSystem.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include "Movie.h"

namespace Maestro
{

    // forward declaration
    struct SSoundInfo;

    /*!
            Base class for all Animation nodes,
            can host multiple animation tracks, and execute them other time.
            Animation node is reference counted.
     */
    class CAnimNode : public IAnimNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CAnimNode, AZ::SystemAllocator);
        AZ_RTTI(CAnimNode, "{57736B48-5EE7-4530-8051-657ACC9BA1EE}", IAnimNode);

        typedef AZStd::vector<AZStd::intrusive_ptr<IAnimTrack>> AnimTracks;

        CAnimNode();
        CAnimNode(const CAnimNode& other);
        CAnimNode(const int id, AnimNodeType nodeType);
        ~CAnimNode();

        AnimNodeType GetType() const override
        {
            return m_nodeType;
        }

        //////////////////////////////////////////////////////////////////////////
        void add_ref() override;
        void release() override;
        //////////////////////////////////////////////////////////////////////////

        void SetName(const char* name) override
        {
            m_name = name;
        }

        const char* GetName() const override
        {
            return m_name.c_str();
        }

        void SetSequence(IAnimSequence* sequence) override
        {
            m_pSequence = sequence;
        }

        // Return Animation Sequence that owns this node.
        IAnimSequence* GetSequence() const override
        {
            return m_pSequence;
        }

        // CAnimNode's aren't bound to AZ::Entities, CAnimAzEntityNodes are. return InvalidEntityId by default
        void SetAzEntityId([[maybe_unused]] const AZ::EntityId& id) override
        {
        }

        AZ::EntityId GetAzEntityId() const override
        {
            return AZ::EntityId();
        }

        void SetFlags(int flags) override;
        int GetFlags() const override;
        bool AreFlagsSetOnNodeOrAnyParent(EAnimNodeFlags flagsToCheck) const override;

        IMovieSystem* GetMovieSystem() const override;

        virtual void OnStart()
        {
        }

        void OnReset() override
        {
        }

        virtual void OnResetHard()
        {
            OnReset();
        }

        virtual void OnPause()
        {
        }

        virtual void OnResume()
        {
        }

        virtual void OnStop()
        {
        }

        virtual void OnLoop()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        // Space position/orientation scale.
        //////////////////////////////////////////////////////////////////////////
        void SetPos([[maybe_unused]] float time, [[maybe_unused]] const AZ::Vector3& pos) override {}
        void SetRotate([[maybe_unused]] float time, [[maybe_unused]] const AZ::Quaternion& quat) override {}
        void SetScale([[maybe_unused]] float time, [[maybe_unused]] const AZ::Vector3& scale) override {}

        Vec3 GetPos() override
        {
            return Vec3(0, 0, 0);
        }

        Quat GetRotate() override
        {
            return Quat(0, 0, 0, 0);
        }

        Quat GetRotate(float /*time*/) override
        {
            return Quat(0, 0, 0, 0);
        }

        Vec3 GetScale() override
        {
            return Vec3(0, 0, 0);
        }

        virtual Matrix34 GetReferenceMatrix() const;

        //////////////////////////////////////////////////////////////////////////
        bool IsParamValid(const CAnimParamType& paramType) const override;
        AZStd::string GetParamName(const CAnimParamType& param) const override;
        AnimValueType GetParamValueType(const CAnimParamType& paramType) const override;
        IAnimNode::ESupportedParamFlags GetParamFlags(const CAnimParamType& paramType) const override;
        unsigned int GetParamCount() const override
        {
            return 0;
        }

        bool SetParamValue(float time, CAnimParamType param, float val) override;
        bool SetParamValue(float time, CAnimParamType param, const AZ::Vector3& val) override;
        bool SetParamValue(float time, CAnimParamType param, const AZ::Vector4& val) override;
        bool GetParamValue(float time, CAnimParamType param, float& val) override;
        bool GetParamValue(float time, CAnimParamType param, AZ::Vector3& val) override;
        bool GetParamValue(float time, CAnimParamType param, AZ::Vector4& val) override;

        void SetTarget([[maybe_unused]] IAnimNode* node) {}

        IAnimNode* GetTarget() const
        {
            return 0;
        }

        void StillUpdate() override
        {
        }

        void Animate(SAnimContext& ec) override;

        virtual void PrecacheStatic([[maybe_unused]] float startTime)
        {
        }

        virtual void PrecacheDynamic([[maybe_unused]] float time)
        {
        }

        void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;
        void InitPostLoad(IAnimSequence* sequence) override;

        void SetNodeOwner(IAnimNodeOwner* pOwner) override;
        IAnimNodeOwner* GetNodeOwner() override
        {
            return m_pOwner;
        }

        // Called by sequence when needs to activate a node.
        void Activate(bool bActivate) override;

        //////////////////////////////////////////////////////////////////////////
        void SetParent(IAnimNode* parent) override;
        IAnimNode* GetParent() const override
        {
            return m_pParentNode;
        }

        IAnimNode* HasDirectorAsParent() const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Track functions.
        //////////////////////////////////////////////////////////////////////////
        int GetTrackCount() const override;
        IAnimTrack* GetTrackByIndex(int nIndex) const override;
        IAnimTrack* GetTrackForParameter(const CAnimParamType& paramType) const override;
        IAnimTrack* GetTrackForParameter(const CAnimParamType& paramType, uint32 index) const override;

        uint32 GetTrackParamIndex(const IAnimTrack* pTrack) const override;

        void SetTrack(const CAnimParamType& paramType, IAnimTrack* track) override;
        IAnimTrack* CreateTrack(const CAnimParamType& paramType) override;
        void InitializeTrackDefaultValue([[maybe_unused]] IAnimTrack* pTrack, [[maybe_unused]] const CAnimParamType& paramType) override
        {
        }
        void SetTimeRange(Range timeRange) override;
        void AddTrack(IAnimTrack* pTrack) override;
        bool RemoveTrack(IAnimTrack* pTrack) override;
        void CreateDefaultTracks() override {}

        void SerializeAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;
        //////////////////////////////////////////////////////////////////////////

        virtual void PostLoad();

        int GetId() const override
        {
            return m_id;
        }

        void SetId(int id)
        {
            m_id = id;
        }

        void Render() override
        {
        }

        void UpdateDynamicParams() final;

        void TimeChanged(float newTime) override;

        void SetExpanded(bool expanded) override;
        bool GetExpanded() const override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        virtual void UpdateDynamicParamsInternal() {}
        virtual bool GetParamInfoFromType([[maybe_unused]] const CAnimParamType& paramType, [[maybe_unused]] SParamInfo& info) const
        {
            return false;
        }

        int NumTracks() const
        {
            return (int)m_tracks.size();
        }

        IAnimTrack* CreateTrackInternal(const CAnimParamType& paramType, EAnimCurveType trackType, AnimValueType valueType);

        IAnimTrack* CreateTrackInternalVector4(const CAnimParamType& paramType) const;
        IAnimTrack* CreateTrackInternalQuat(EAnimCurveType trackType, const CAnimParamType& paramType) const;
        IAnimTrack* CreateTrackInternalVector(
            EAnimCurveType trackType, const CAnimParamType& paramType, const AnimValueType animValue) const;
        IAnimTrack* CreateTrackInternalFloat(int trackType) const;

        // sets track animNode pointer to this node and sorts tracks
        void RegisterTrack(IAnimTrack* pTrack);

        CMovieSystem* GetCMovieSystem() const
        {
            return static_cast<CMovieSystem*>(m_movieSystem);
        }

        bool NeedToRender() const override
        {
            return false;
        }

        // nodes which support sounds should override this to reset their start/stop sound states
        virtual void ResetSounds()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        // AnimateSound() calls ApplyAudioKey() to trigger audio on sound key frames. Nodes which support audio must override
        // this to trigger audio
        virtual void ApplyAudioKey([[maybe_unused]] char const* const sTriggerName, [[maybe_unused]] bool const bPlay = true) {}
        void AnimateSound(AZStd::vector<SSoundInfo>& nodeSoundInfo, SAnimContext& ec, IAnimTrack* pTrack, size_t numAudioTracks);
        //////////////////////////////////////////////////////////////////////////

        AnimTracks m_tracks;
        AnimNodeType m_nodeType;
        AZStd::string m_name;
        IAnimSequence* m_pSequence;
        IAnimNodeOwner* m_pOwner;
        IAnimNode* m_pParentNode;
        int m_refCount;
        int m_id;
        int m_nLoadedParentNodeId; // only used in legacy Serialize()
        int m_parentNodeId;
        int m_flags;
        unsigned int m_bIgnoreSetParam : 1; // Internal flags.
        bool m_expanded;
        IMovieSystem* m_movieSystem;

    private:
        void SortTracks();
        bool IsTimeOnSoundKey(float queryTime) const;

        static bool TrackOrder(const AZStd::intrusive_ptr<IAnimTrack>& left, const AZStd::intrusive_ptr<IAnimTrack>& right);

        AZStd::mutex m_updateDynamicParamsLock;
    };

} // namespace Maestro
