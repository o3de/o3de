/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        MCORE_INLINE AnimGraphNode* GetNode() const                            { return reinterpret_cast<AnimGraphNode*>(mObject); }
        MCORE_INLINE void SetNode(AnimGraphNode* node)                         { mObject = reinterpret_cast<AnimGraphObject*>(node); }

        MCORE_INLINE void SetSyncIndex(uint32 syncIndex)                        { mSyncIndex = syncIndex; }
        MCORE_INLINE uint32 GetSyncIndex() const                                { return mSyncIndex; }

        MCORE_INLINE void SetCurrentPlayTime(float absoluteTime)                { mCurrentTime = absoluteTime; }
        MCORE_INLINE float GetCurrentPlayTime() const                           { return mCurrentTime; }

        MCORE_INLINE void SetPlaySpeed(float speed)                             { mPlaySpeed = speed; }
        MCORE_INLINE float GetPlaySpeed() const                                 { return mPlaySpeed; }

        MCORE_INLINE void SetDuration(float durationInSeconds)                  { mDuration = durationInSeconds; }
        MCORE_INLINE float GetDuration() const                                  { return mDuration; }

        MCORE_INLINE void SetPreSyncTime(float timeInSeconds)                   { mPreSyncTime = timeInSeconds; }
        MCORE_INLINE float GetPreSyncTime() const                               { return mPreSyncTime; }

        MCORE_INLINE void SetGlobalWeight(float weight)                         { mGlobalWeight = weight; }
        MCORE_INLINE float GetGlobalWeight() const                              { return mGlobalWeight; }

        MCORE_INLINE void SetLocalWeight(float weight)                          { mLocalWeight = weight; }
        MCORE_INLINE float GetLocalWeight() const                               { return mLocalWeight; }

        MCORE_INLINE uint8 GetInheritFlags() const                              { return mInheritFlags; }

        MCORE_INLINE bool GetIsBackwardPlaying() const                          { return (mInheritFlags & INHERITFLAGS_BACKWARD) != 0; }
        MCORE_INLINE void SetBackwardFlag()                                     { mInheritFlags |= INHERITFLAGS_BACKWARD; }
        MCORE_INLINE void ClearInheritFlags()                                   { mInheritFlags = 0; }

        MCORE_INLINE uint8 GetPoseRefCount() const                              { return mPoseRefCount; }
        MCORE_INLINE void IncreasePoseRefCount()                                { mPoseRefCount++; }
        MCORE_INLINE void DecreasePoseRefCount()                                { mPoseRefCount--; }
        MCORE_INLINE void SetPoseRefCount(uint8 refCount)                       { mPoseRefCount = refCount; }

        MCORE_INLINE uint8 GetRefDataRefCount() const                           { return mRefDataRefCount; }
        MCORE_INLINE void IncreaseRefDataRefCount()                             { mRefDataRefCount++; }
        MCORE_INLINE void DecreaseRefDataRefCount()                             { mRefDataRefCount--; }
        MCORE_INLINE void SetRefDataRefCount(uint8 refCount)                    { mRefDataRefCount = refCount; }

        MCORE_INLINE void SetRefCountedData(AnimGraphRefCountedData* data)     { mRefCountedData = data; }
        MCORE_INLINE AnimGraphRefCountedData* GetRefCountedData() const        { return mRefCountedData; }

        MCORE_INLINE const AnimGraphSyncTrack* GetSyncTrack() const            { return mSyncTrack; }
        MCORE_INLINE AnimGraphSyncTrack* GetSyncTrack()                        { return mSyncTrack; }
        MCORE_INLINE void SetSyncTrack(AnimGraphSyncTrack* syncTrack)          { mSyncTrack = syncTrack; }

        bool GetIsMirrorMotion() const { return m_isMirrorMotion; }
        void SetIsMirrorMotion(bool newValue) { m_isMirrorMotion = newValue; }

    protected:
        float       mDuration;
        float       mCurrentTime;
        float       mPlaySpeed;
        float       mPreSyncTime;
        float       mGlobalWeight;
        float       mLocalWeight;
        uint32      mSyncIndex;             /**< The last used sync track index. */
        uint8       mPoseRefCount;
        uint8       mRefDataRefCount;
        uint8       mInheritFlags;
        bool        m_isMirrorMotion;
        AnimGraphRefCountedData*   mRefCountedData;
        AnimGraphSyncTrack*        mSyncTrack;

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
