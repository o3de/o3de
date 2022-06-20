/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include "AnimGraphObjectData.h"
#include <AzCore/Memory/Memory.h>

namespace EMotionFX
{
    // forward declarations
    class AnimGraphObject;
    class AnimGraphInstance;
    class AnimGraphNode;
    class AnimGraphRefCountedData;
    class AnimGraphSyncTrack;


    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphNodeData
        : public AnimGraphObjectData
    {
        EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

    public:
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INHERITFLAGS_BACKWARD = 1 << 0
        };

        AnimGraphNodeData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
        virtual ~AnimGraphNodeData() = default;

        static AnimGraphNodeData* Create(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);

        void Clear();

        void Init(AnimGraphInstance* animGraphInstance, AnimGraphNode* node);
        void Init(AnimGraphNodeData* nodeData);

        AnimGraphNode* GetNode() const                            { return reinterpret_cast<AnimGraphNode*>(m_object); }
        void SetNode(AnimGraphNode* node)                         { m_object = reinterpret_cast<AnimGraphObject*>(node); }

        void SetSyncIndex(size_t syncIndex)                        { m_syncIndex = syncIndex; }
        size_t GetSyncIndex() const                                { return m_syncIndex; }

        void SetCurrentPlayTime(float absoluteTime)                { m_currentTime = absoluteTime; }
        float GetCurrentPlayTime() const                           { return m_currentTime; }

        void SetPlaySpeed(float speed)                             { m_playSpeed = speed; }
        float GetPlaySpeed() const                                 { return m_playSpeed; }

        void SetDuration(float durationInSeconds)                  { m_duration = durationInSeconds; }
        float GetDuration() const                                  { return m_duration; }

        void SetPreSyncTime(float timeInSeconds)                   { m_preSyncTime = timeInSeconds; }
        float GetPreSyncTime() const                               { return m_preSyncTime; }

        void SetGlobalWeight(float weight)                         { m_globalWeight = weight; }
        float GetGlobalWeight() const                              { return m_globalWeight; }

        void SetLocalWeight(float weight)                          { m_localWeight = weight; }
        float GetLocalWeight() const                               { return m_localWeight; }

        uint8 GetInheritFlags() const                              { return m_inheritFlags; }

        bool GetIsBackwardPlaying() const                          { return (m_inheritFlags & INHERITFLAGS_BACKWARD) != 0; }
        void SetBackwardFlag()                                     { m_inheritFlags |= INHERITFLAGS_BACKWARD; }
        void ClearInheritFlags()                                   { m_inheritFlags = 0; }

        uint8 GetPoseRefCount() const                              { return m_poseRefCount; }
        void IncreasePoseRefCount()                                { m_poseRefCount++; }
        void DecreasePoseRefCount()                                { m_poseRefCount--; }
        void SetPoseRefCount(uint8 refCount)                       { m_poseRefCount = refCount; }

        uint8 GetRefDataRefCount() const                           { return m_refDataRefCount; }
        void IncreaseRefDataRefCount()                             { m_refDataRefCount++; }
        void DecreaseRefDataRefCount()                             { m_refDataRefCount--; }
        void SetRefDataRefCount(uint8 refCount)                    { m_refDataRefCount = refCount; }

        void SetRefCountedData(AnimGraphRefCountedData* data)     { m_refCountedData = data; }
        AnimGraphRefCountedData* GetRefCountedData() const        { return m_refCountedData; }

        const AnimGraphSyncTrack* GetSyncTrack() const            { return m_syncTrack; }
        AnimGraphSyncTrack* GetSyncTrack()                        { return m_syncTrack; }
        void SetSyncTrack(AnimGraphSyncTrack* syncTrack)          { m_syncTrack = syncTrack; }

        bool GetIsMirrorMotion() const { return m_isMirrorMotion; }
        void SetIsMirrorMotion(bool newValue) { m_isMirrorMotion = newValue; }

    protected:
        float       m_duration;
        float       m_currentTime;
        float       m_playSpeed;
        float       m_preSyncTime;
        float       m_globalWeight;
        float       m_localWeight;
        size_t      m_syncIndex;             /**< The last used sync track index. */
        uint8       m_poseRefCount;
        uint8       m_refDataRefCount;
        uint8       m_inheritFlags;
        bool        m_isMirrorMotion;
        AnimGraphRefCountedData*   m_refCountedData;
        AnimGraphSyncTrack*        m_syncTrack;

        void Delete() override;
    };

    /**
     * This mixin can be used for unique datas on anim graph nodes that manually need to increase pose and data ref counts for nodes a hierarchy level up or
     * neighbor nodes with a risk of the node not being output. An example would be the state machine where the active nodes can change within the update
     * method due to ending or newly started transitions. We need some way to keep track of the nodes that increased the data and pose ref counts at a level
     * up in the hierarchy.
     */
    class EMFX_API NodeDataAutoRefCountMixin
    {
    public:
        void ClearRefCounts();

        void IncreaseDataRefCountForNode(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
        void DecreaseDataRefCounts(AnimGraphInstance* animGraphInstance);
        const AZStd::vector<AnimGraphNode*>& GetDataRefIncreasedNodes() const { return m_dataRefCountIncreasedNodes; }

        void IncreasePoseRefCountForNode(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
        void DecreasePoseRefCounts(AnimGraphInstance* animGraphInstance);
        const AZStd::vector<AnimGraphNode*>& GetPoseRefIncreasedNodes() const { return m_poseRefCountIncreasedNodes; }

    protected:
        AZStd::vector<AnimGraphNode*> m_dataRefCountIncreasedNodes;
        AZStd::vector<AnimGraphNode*> m_poseRefCountIncreasedNodes;
    };
} // namespace EMotionFX
