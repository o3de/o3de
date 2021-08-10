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

        MCORE_INLINE AnimGraphNode* GetNode() const                            { return reinterpret_cast<AnimGraphNode*>(m_object); }
        MCORE_INLINE void SetNode(AnimGraphNode* node)                         { m_object = reinterpret_cast<AnimGraphObject*>(node); }

        MCORE_INLINE void SetSyncIndex(size_t syncIndex)                        { m_syncIndex = syncIndex; }
        MCORE_INLINE size_t GetSyncIndex() const                                { return m_syncIndex; }

        MCORE_INLINE void SetCurrentPlayTime(float absoluteTime)                { m_currentTime = absoluteTime; }
        MCORE_INLINE float GetCurrentPlayTime() const                           { return m_currentTime; }

        MCORE_INLINE void SetPlaySpeed(float speed)                             { m_playSpeed = speed; }
        MCORE_INLINE float GetPlaySpeed() const                                 { return m_playSpeed; }

        MCORE_INLINE void SetDuration(float durationInSeconds)                  { m_duration = durationInSeconds; }
        MCORE_INLINE float GetDuration() const                                  { return m_duration; }

        MCORE_INLINE void SetPreSyncTime(float timeInSeconds)                   { m_preSyncTime = timeInSeconds; }
        MCORE_INLINE float GetPreSyncTime() const                               { return m_preSyncTime; }

        MCORE_INLINE void SetGlobalWeight(float weight)                         { m_globalWeight = weight; }
        MCORE_INLINE float GetGlobalWeight() const                              { return m_globalWeight; }

        MCORE_INLINE void SetLocalWeight(float weight)                          { m_localWeight = weight; }
        MCORE_INLINE float GetLocalWeight() const                               { return m_localWeight; }

        MCORE_INLINE uint8 GetInheritFlags() const                              { return m_inheritFlags; }

        MCORE_INLINE bool GetIsBackwardPlaying() const                          { return (m_inheritFlags & INHERITFLAGS_BACKWARD) != 0; }
        MCORE_INLINE void SetBackwardFlag()                                     { m_inheritFlags |= INHERITFLAGS_BACKWARD; }
        MCORE_INLINE void ClearInheritFlags()                                   { m_inheritFlags = 0; }

        MCORE_INLINE uint8 GetPoseRefCount() const                              { return m_poseRefCount; }
        MCORE_INLINE void IncreasePoseRefCount()                                { m_poseRefCount++; }
        MCORE_INLINE void DecreasePoseRefCount()                                { m_poseRefCount--; }
        MCORE_INLINE void SetPoseRefCount(uint8 refCount)                       { m_poseRefCount = refCount; }

        MCORE_INLINE uint8 GetRefDataRefCount() const                           { return m_refDataRefCount; }
        MCORE_INLINE void IncreaseRefDataRefCount()                             { m_refDataRefCount++; }
        MCORE_INLINE void DecreaseRefDataRefCount()                             { m_refDataRefCount--; }
        MCORE_INLINE void SetRefDataRefCount(uint8 refCount)                    { m_refDataRefCount = refCount; }

        MCORE_INLINE void SetRefCountedData(AnimGraphRefCountedData* data)     { m_refCountedData = data; }
        MCORE_INLINE AnimGraphRefCountedData* GetRefCountedData() const        { return m_refCountedData; }

        MCORE_INLINE const AnimGraphSyncTrack* GetSyncTrack() const            { return m_syncTrack; }
        MCORE_INLINE AnimGraphSyncTrack* GetSyncTrack()                        { return m_syncTrack; }
        MCORE_INLINE void SetSyncTrack(AnimGraphSyncTrack* syncTrack)          { m_syncTrack = syncTrack; }

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
