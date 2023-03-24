/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeData.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNodeData, AnimGraphObjectDataAllocator)

    // constructor
    AnimGraphNodeData::AnimGraphNodeData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphObjectData(reinterpret_cast<AnimGraphObject*>(node), animGraphInstance)
        , m_duration(0.0f)
        , m_currentTime(0.0f)
        , m_playSpeed(1.0f)
        , m_preSyncTime(0.0f)
        , m_globalWeight(1.0f)
        , m_localWeight(1.0f)
        , m_syncIndex(InvalidIndex)
        , m_poseRefCount(0)
        , m_refDataRefCount(0)
        , m_inheritFlags(0)
        , m_isMirrorMotion(false)
        , m_refCountedData(nullptr)
        , m_syncTrack(nullptr)
#if defined(EMFX_ANIMGRAPH_PROFILER_ENABLED)
        , mTotalUpdateTime(0)
        , mInputNodesUpdateTime(0)
#endif
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
        m_duration = 0.0f;
        m_currentTime = 0.0f;
        m_preSyncTime = 0.0f;
        m_playSpeed = 1.0f;
        m_globalWeight = 1.0f;
        m_localWeight = 1.0f;
        m_inheritFlags = 0;
        m_isMirrorMotion = false;
        m_syncIndex = InvalidIndex;
        m_syncTrack = nullptr;
        #if defined(EMFX_ANIMGRAPH_PROFILER_ENABLED)
            ClearUpdateTimes();
        #endif
    }


    // init the play related settings from a given node
    void AnimGraphNodeData::Init(AnimGraphInstance* animGraphInstance, AnimGraphNode* node)
    {
        Init(node->FindOrCreateUniqueNodeData(animGraphInstance));
    }


    // init from existing node data
    void AnimGraphNodeData::Init(AnimGraphNodeData* nodeData)
    {
        m_duration = nodeData->m_duration;
        m_currentTime = nodeData->m_currentTime;
        m_preSyncTime = nodeData->m_preSyncTime;
        m_playSpeed = nodeData->m_playSpeed;
        m_syncIndex = nodeData->m_syncIndex;
        m_globalWeight = nodeData->m_globalWeight;
        m_inheritFlags = nodeData->m_inheritFlags;
        m_isMirrorMotion = nodeData->m_isMirrorMotion;
        m_syncTrack = nodeData->m_syncTrack;
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
