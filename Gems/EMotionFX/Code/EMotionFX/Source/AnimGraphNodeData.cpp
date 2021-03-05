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

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeData.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNodeData, AnimGraphObjectDataAllocator, 0)

    // constructor
    AnimGraphNodeData::AnimGraphNodeData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphObjectData(reinterpret_cast<AnimGraphObject*>(node), animGraphInstance)
        , mDuration(0.0f)
        , mCurrentTime(0.0f)
        , mPlaySpeed(1.0f)
        , mPreSyncTime(0.0f)
        , mGlobalWeight(1.0f)
        , mLocalWeight(1.0f)
        , mSyncIndex(MCORE_INVALIDINDEX32)
        , mPoseRefCount(0)
        , mRefDataRefCount(0)
        , mInheritFlags(0)
        , m_isMirrorMotion(false)
        , mRefCountedData(nullptr)
        , mSyncTrack(nullptr)
    {
    }


    // static create
    AnimGraphNodeData* AnimGraphNodeData::Create(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
    {
        return aznew AnimGraphNodeData(node, animGraphInstance);
    }
    

    // reset the sync related data
    void AnimGraphNodeData::Clear()
    {
        mDuration = 0.0f;
        mCurrentTime = 0.0f;
        mPreSyncTime = 0.0f;
        mPlaySpeed = 1.0f;
        mGlobalWeight = 1.0f;
        mLocalWeight = 1.0f;
        mInheritFlags = 0;
        m_isMirrorMotion = false;
        mSyncIndex = MCORE_INVALIDINDEX32;
        mSyncTrack = nullptr;
    }


    // init the play related settings from a given node
    void AnimGraphNodeData::Init(AnimGraphInstance* animGraphInstance, AnimGraphNode* node)
    {
        Init(node->FindOrCreateUniqueNodeData(animGraphInstance));
    }


    // init from existing node data
    void AnimGraphNodeData::Init(AnimGraphNodeData* nodeData)
    {
        mDuration = nodeData->mDuration;
        mCurrentTime = nodeData->mCurrentTime;
        mPreSyncTime = nodeData->mPreSyncTime;
        mPlaySpeed = nodeData->mPlaySpeed;
        mSyncIndex = nodeData->mSyncIndex;
        mGlobalWeight = nodeData->mGlobalWeight;
        mInheritFlags = nodeData->mInheritFlags;
        m_isMirrorMotion = nodeData->m_isMirrorMotion;
        mSyncTrack = nodeData->mSyncTrack;
    }


    void AnimGraphNodeData::Delete()
    {
        delete this;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void NodeDataAutoRefCountMixin::ClearRefCounts()
    {
        m_dataRefCountIncreasedNodes.clear();
        m_poseRefCountIncreasedNodes.clear();
    }

    void NodeDataAutoRefCountMixin::IncreaseDataRefCountForNode(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
    {
        if (AZStd::find(m_dataRefCountIncreasedNodes.begin(), m_dataRefCountIncreasedNodes.end(), node) == m_dataRefCountIncreasedNodes.end())
        {
            node->IncreaseRefDataRefCount(animGraphInstance);
            m_dataRefCountIncreasedNodes.emplace_back(node);
        }
    }

    void NodeDataAutoRefCountMixin::DecreaseDataRefCounts(AnimGraphInstance* animGraphInstance)
    {
        for (AnimGraphNode* node : m_dataRefCountIncreasedNodes)
        {
            node->DecreaseRefDataRef(animGraphInstance);
        }
        m_dataRefCountIncreasedNodes.clear();
    }

    void NodeDataAutoRefCountMixin::IncreasePoseRefCountForNode(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
    {
        if (AZStd::find(m_poseRefCountIncreasedNodes.begin(), m_poseRefCountIncreasedNodes.end(), node) == m_poseRefCountIncreasedNodes.end())
        {
            node->IncreasePoseRefCount(animGraphInstance);
            m_poseRefCountIncreasedNodes.emplace_back(node);
        }
    }

    void NodeDataAutoRefCountMixin::DecreasePoseRefCounts(AnimGraphInstance* animGraphInstance)
    {
        for (AnimGraphNode* node : m_poseRefCountIncreasedNodes)
        {
            node->DecreaseRef(animGraphInstance);
        }
        m_poseRefCountIncreasedNodes.clear();
    }
} // namespace EMotionFX
