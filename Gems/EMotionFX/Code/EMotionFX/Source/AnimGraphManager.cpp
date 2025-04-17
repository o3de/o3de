/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StringFunc/StringFunc.h>
#include "EMotionFXConfig.h"
#include "AnimGraphManager.h"
#include "AnimGraph.h"
#include "AnimGraphObjectFactory.h"
#include "AnimGraphNode.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphInstance.h"
#include "BlendSpaceManager.h"
#include "Importer/Importer.h"
#include "ActorManager.h"
#include "EMotionFXManager.h"
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/AttributeFactory.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphManager, AnimGraphManagerAllocator)

    AnimGraphManager::AnimGraphManager()
        : MCore::RefCounted()
        , m_blendSpaceManager(nullptr)
    {
    }


    AnimGraphManager::~AnimGraphManager()
    {
        if (m_blendSpaceManager)
        {
            m_blendSpaceManager->Destroy();
        }
        // delete the anim graph instances and anim graphs
        //RemoveAllAnimGraphInstances(true);
        //RemoveAllAnimGraphs(true);
    }


    AnimGraphManager* AnimGraphManager::Create()
    {
        return aznew AnimGraphManager();
    }


    void AnimGraphManager::Init()
    {
        m_animGraphInstances.reserve(1024);
        m_animGraphs.reserve(128);

        m_blendSpaceManager = aznew BlendSpaceManager();

        // register custom attribute types
        MCore::GetAttributeFactory().RegisterAttribute(aznew AttributePose());
        MCore::GetAttributeFactory().RegisterAttribute(aznew AttributeMotionInstance());
    }


    void AnimGraphManager::RemoveAllAnimGraphs(bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(m_animGraphLock);

        while (!m_animGraphs.empty())
        {
            RemoveAnimGraph(m_animGraphs.size() - 1, delFromMemory);
        }
    }


    void AnimGraphManager::RemoveAllAnimGraphInstances(bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(m_animGraphInstanceLock);

        while (!m_animGraphInstances.empty())
        {
            RemoveAnimGraphInstance(m_animGraphInstances.size() - 1, delFromMemory);
        }
    }


    void AnimGraphManager::AddAnimGraph(AnimGraph* setup)
    {
        MCore::LockGuardRecursive lock(m_animGraphLock);
        m_animGraphs.push_back(setup);
    }


    // Remove a given anim graph by index.
    void AnimGraphManager::RemoveAnimGraph(size_t index, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(m_animGraphLock);

        AnimGraph* animGraph = m_animGraphs[index];
        const int animGraphInstanceCount = static_cast<int>(m_animGraphInstances.size());
        for (int i = animGraphInstanceCount - 1; i >= 0; --i)
        {
            if (m_animGraphInstances[i]->GetAnimGraph() == animGraph)
            {
                RemoveAnimGraphInstance(m_animGraphInstances[i]);
            }
        }

        // Need to remove it from the list of anim graphs first since deleting it can cause assets to get unloaded and
        // this function to be called recursively (making the index to shift)
        m_animGraphs.erase(m_animGraphs.begin() + index);

        if (delFromMemory)
        {
            // Destroy the memory of the anim graph and disable auto-unregister so that  the destructor of the anim graph does not
            // internally call AnimGraphManager::RemoveAnimGraph(this, false) to unregister the anim graph from the anim graph manager.
            animGraph->SetAutoUnregister(false);
            delete animGraph;
        }
    }


    // Remove a given anim graph by pointer.
    bool AnimGraphManager::RemoveAnimGraph(AnimGraph* animGraph, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(m_animGraphLock);

        // find the index of the anim graph and return false in case the pointer is not valid
        const size_t animGraphIndex = FindAnimGraphIndex(animGraph);
        if (animGraphIndex == InvalidIndex)
        {
            return false;
        }

        RemoveAnimGraph(animGraphIndex, delFromMemory);
        return true;
    }


    void AnimGraphManager::AddAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        MCore::LockGuardRecursive lock(m_animGraphInstanceLock);
        m_animGraphInstances.push_back(animGraphInstance);
    }


    void AnimGraphManager::RemoveAnimGraphInstance(size_t index, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(m_animGraphInstanceLock);

        if (delFromMemory)
        {
            AnimGraphInstance* animGraphInstance = m_animGraphInstances[index];
            animGraphInstance->RemoveAllObjectData(true);

            // Remove all links to the anim graph instance that will get removed.
            const size_t numActorInstances = GetActorManager().GetNumActorInstances();
            for (size_t i = 0; i < numActorInstances; ++i)
            {
                ActorInstance* actorInstance = GetActorManager().GetActorInstance(i);
                if (animGraphInstance == actorInstance->GetAnimGraphInstance())
                {
                    actorInstance->SetAnimGraphInstance(nullptr);
                }
            }

            // Disable automatic unregistration of the anim graph instance from the anim graph manager. If we don't disable it, the anim graph instance destructor
            // would call AnimGraphManager::RemoveAnimGraphInstance(this, false) while we are already removing it by index.
            animGraphInstance->SetAutoUnregisterEnabled(false);
            animGraphInstance->Destroy();
        }

        m_animGraphInstances.erase(m_animGraphInstances.begin() + index);
    }


    // remove a given anim graph instance by pointer
    bool AnimGraphManager::RemoveAnimGraphInstance(AnimGraphInstance* animGraphInstance, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(m_animGraphInstanceLock);

        // find the index of the anim graph instance and return false in case the pointer is not valid
        const size_t instanceIndex = FindAnimGraphInstanceIndex(animGraphInstance);
        if (instanceIndex == InvalidIndex)
        {
            return false;
        }

        // remove the anim graph instance and return success
        RemoveAnimGraphInstance(instanceIndex, delFromMemory);
        return true;
    }


    void AnimGraphManager::RemoveAnimGraphInstances(AnimGraph* animGraph, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(m_animGraphInstanceLock);

        if (m_animGraphInstances.empty())
        {
            return;
        }

        // Remove anim graph instances back to front in case they are linked to the given anim graph.
        const size_t numInstances = m_animGraphInstances.size();
        for (size_t i = 0; i < numInstances; ++i)
        {
            const size_t reverseIndex = numInstances - 1 - i;

            AnimGraphInstance* instance = m_animGraphInstances[reverseIndex];
            if (instance->GetAnimGraph() == animGraph)
            {
                RemoveAnimGraphInstance(reverseIndex, delFromMemory);
            }
        }
    }


    size_t AnimGraphManager::FindAnimGraphIndex(AnimGraph* animGraph) const
    {
        MCore::LockGuardRecursive lock(m_animGraphLock);

        auto iterator = AZStd::find(m_animGraphs.begin(), m_animGraphs.end(), animGraph);
        if (iterator == m_animGraphs.end())
        {
            return InvalidIndex;
        }

        const size_t index = iterator - m_animGraphs.begin();
        return index;
    }


    size_t AnimGraphManager::FindAnimGraphInstanceIndex(AnimGraphInstance* animGraphInstance) const
    {
        MCore::LockGuardRecursive lock(m_animGraphInstanceLock);

        auto iterator = AZStd::find(m_animGraphInstances.begin(), m_animGraphInstances.end(), animGraphInstance);
        if (iterator == m_animGraphInstances.end())
        {
            return InvalidIndex;
        }

        const size_t index = iterator - m_animGraphInstances.begin();
        return index;
    }


    // find a anim graph with a given filename
    AnimGraph* AnimGraphManager::FindAnimGraphByFileName(const char* filename, bool isTool) const
    {
        MCore::LockGuardRecursive lock(m_animGraphLock);

        for (EMotionFX::AnimGraph* animGraph : m_animGraphs)
        {
            if (animGraph->GetIsOwnedByRuntime() == isTool)
            {
                continue;
            }

            if (AzFramework::StringFunc::Equal(animGraph->GetFileName(), filename, false /* no case */))
            {
                return animGraph;
            }
        }

        return nullptr;
    }


    // Find anim graph with a given id.
    AnimGraph* AnimGraphManager::FindAnimGraphByID(uint32 animGraphID) const
    {
        MCore::LockGuardRecursive lock(m_animGraphLock);

        for (EMotionFX::AnimGraph* animGraph : m_animGraphs)
        {
            if (animGraph->GetID() == animGraphID)
            {
                return animGraph;
            }
        }

        return nullptr;
    }


    // Find the first available anim graph
    AnimGraph* AnimGraphManager::GetFirstAnimGraph() const
    {
        MCore::LockGuardRecursive lock(m_animGraphLock);

        if (m_animGraphs.size() > 0)
        {
            return m_animGraphs[0];
        }
        return nullptr;
    }


    void AnimGraphManager::SetAnimGraphVisualizationEnabled(bool enabled)
    {
        MCore::LockGuardRecursive lock(m_animGraphInstanceLock);

        // Enable or disable anim graph visualization for all anim graph instances..
        for (AnimGraphInstance* animGraphInstance : m_animGraphInstances)
        {
            animGraphInstance->SetVisualizationEnabled(enabled);
        }
    }


    void AnimGraphManager::RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<EMotionFX::AnimGraphObject*>& affectedObjects)
    {
        for (EMotionFX::AnimGraph* potentiallyAffected : m_animGraphs)
        {
            if (potentiallyAffected != animGraph) // exclude the passed one since that will always be affected
            {
                potentiallyAffected->RecursiveCollectObjectsAffectedBy(animGraph, affectedObjects);
            }
        }
    }

    void AnimGraphManager::InvalidateInstanceUniqueDataUsingMotionSet(EMotionFX::MotionSet* motionSet)
    {
        // Update unique datas for all anim graph instances that use the given motion set.
        for (EMotionFX::AnimGraphInstance* animGraphInstance : m_animGraphInstances)
        {
            if (animGraphInstance->GetMotionSet() == motionSet)
            {
                animGraphInstance->RecursiveInvalidateUniqueDatas();
            }
        }
    }
} // namespace EMotionFX
