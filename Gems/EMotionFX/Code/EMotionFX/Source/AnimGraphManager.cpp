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
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphManager, AnimGraphManagerAllocator, 0)

    AnimGraphManager::AnimGraphManager()
        : BaseObject()
        , mBlendSpaceManager(nullptr)
    {
    }


    AnimGraphManager::~AnimGraphManager()
    {
        if (mBlendSpaceManager)
        {
            mBlendSpaceManager->Destroy();
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
        mAnimGraphInstances.reserve(1024);
        mAnimGraphs.reserve(128);

        mBlendSpaceManager = aznew BlendSpaceManager();

        // register custom attribute types
        MCore::GetAttributeFactory().RegisterAttribute(aznew AttributePose());
        MCore::GetAttributeFactory().RegisterAttribute(aznew AttributeMotionInstance());
    }


    void AnimGraphManager::RemoveAllAnimGraphs(bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        while (!mAnimGraphs.empty())
        {
            RemoveAnimGraph(mAnimGraphs.size() - 1, delFromMemory);
        }
    }


    void AnimGraphManager::RemoveAllAnimGraphInstances(bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        while (!mAnimGraphInstances.empty())
        {
            RemoveAnimGraphInstance(mAnimGraphInstances.size() - 1, delFromMemory);
        }
    }
    

    void AnimGraphManager::AddAnimGraph(AnimGraph* setup)
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);
        mAnimGraphs.push_back(setup);
    }


    // Remove a given anim graph by index.
    void AnimGraphManager::RemoveAnimGraph(size_t index, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        AnimGraph* animGraph = mAnimGraphs[index];
        const int animGraphInstanceCount = static_cast<int>(mAnimGraphInstances.size());
        for (int i = animGraphInstanceCount - 1; i >= 0; --i)
        {
            if (mAnimGraphInstances[i]->GetAnimGraph() == animGraph)
            {
                RemoveAnimGraphInstance(mAnimGraphInstances[i]);
            }
        }

        // Need to remove it from the list of anim graphs first since deleting it can cause assets to get unloaded and
        // this function to be called recursively (making the index to shift)
        mAnimGraphs.erase(mAnimGraphs.begin() + index);

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
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        // find the index of the anim graph and return false in case the pointer is not valid
        const uint32 animGraphIndex = FindAnimGraphIndex(animGraph);
        if (animGraphIndex == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        RemoveAnimGraph(animGraphIndex, delFromMemory);
        return true;
    }


    void AnimGraphManager::AddAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);
        mAnimGraphInstances.push_back(animGraphInstance);
    }


    void AnimGraphManager::RemoveAnimGraphInstance(size_t index, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        if (delFromMemory)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraphInstances[index];
            animGraphInstance->RemoveAllObjectData(true);

            // Remove all links to the anim graph instance that will get removed.
            const uint32 numActorInstances = GetActorManager().GetNumActorInstances();
            for (uint32 i = 0; i < numActorInstances; ++i)
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

        mAnimGraphInstances.erase(mAnimGraphInstances.begin() + index);
    }


    // remove a given anim graph instance by pointer
    bool AnimGraphManager::RemoveAnimGraphInstance(AnimGraphInstance* animGraphInstance, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        // find the index of the anim graph instance and return false in case the pointer is not valid
        const uint32 instanceIndex = FindAnimGraphInstanceIndex(animGraphInstance);
        if (instanceIndex == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // remove the anim graph instance and return success
        RemoveAnimGraphInstance(instanceIndex, delFromMemory);
        return true;
    }


    void AnimGraphManager::RemoveAnimGraphInstances(AnimGraph* animGraph, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        if (mAnimGraphInstances.empty())
        {
            return;
        }

        // Remove anim graph instances back to front in case they are linked to the given anim graph.
        const size_t numInstances = mAnimGraphInstances.size();
        for (size_t i = 0; i < numInstances; ++i)
        {
            const size_t reverseIndex = numInstances - 1 - i;

            AnimGraphInstance* instance = mAnimGraphInstances[reverseIndex];
            if (instance->GetAnimGraph() == animGraph)
            {
                RemoveAnimGraphInstance(reverseIndex, delFromMemory);
            }
        }
    }


    uint32 AnimGraphManager::FindAnimGraphIndex(AnimGraph* animGraph) const
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        auto iterator = AZStd::find(mAnimGraphs.begin(), mAnimGraphs.end(), animGraph);
        if (iterator == mAnimGraphs.end())
        {
            return MCORE_INVALIDINDEX32;
        }

        const size_t index = iterator - mAnimGraphs.begin();
        return static_cast<uint32>(index);
    }


    uint32 AnimGraphManager::FindAnimGraphInstanceIndex(AnimGraphInstance* animGraphInstance) const
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        auto iterator = AZStd::find(mAnimGraphInstances.begin(), mAnimGraphInstances.end(), animGraphInstance);
        if (iterator == mAnimGraphInstances.end())
        {
            return MCORE_INVALIDINDEX32;
        }

        const size_t index = iterator - mAnimGraphInstances.begin();
        return static_cast<uint32>(index);
    }


    // find a anim graph with a given filename
    AnimGraph* AnimGraphManager::FindAnimGraphByFileName(const char* filename, bool isTool) const
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        for (EMotionFX::AnimGraph* animGraph : mAnimGraphs)
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
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        for (EMotionFX::AnimGraph* animGraph : mAnimGraphs)
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
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        if (mAnimGraphs.size() > 0)
        {
            return mAnimGraphs[0];
        }
        return nullptr;
    }


    void AnimGraphManager::SetAnimGraphVisualizationEnabled(bool enabled)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        // Enable or disable anim graph visualization for all anim graph instances..
        for (AnimGraphInstance* animGraphInstance : mAnimGraphInstances)
        {
            animGraphInstance->SetVisualizationEnabled(enabled);
        }
    }


    void AnimGraphManager::RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<EMotionFX::AnimGraphObject*>& affectedObjects)
    {
        for (EMotionFX::AnimGraph* potentiallyAffected : mAnimGraphs)
        {
            if (potentiallyAffected != animGraph) // exclude the passed one since that will always be affected
            {
                potentiallyAffected->RecursiveCollectObjectsAffectedBy(animGraph, affectedObjects);
            }
        }
    }

    void AnimGraphManager::UpdateInstancesUniqueDataUsingMotionSet(EMotionFX::MotionSet* motionSet)
    {
        // Update unique datas for all anim graph instances that use the given motion set.
        for (EMotionFX::AnimGraphInstance* animGraphInstance : mAnimGraphInstances)
        {
            if (animGraphInstance->GetMotionSet() == motionSet)
            {
                animGraphInstance->RecursiveInvalidateUniqueDatas();
            }
        }
    }
} // namespace EMotionFX
