/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeData.h>
#include <MCore/Source/RefCounted.h>
#include <EMotionFX/Source/BlendSpace1DNode.h>
#include <EMotionFX/Source/BlendSpace2DNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MemoryCategories.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/MotionData/MotionDataFactory.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/MultiThreadManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionManager, MotionManagerAllocator)


    // constructor
    MotionManager::MotionManager()
        : MCore::RefCounted()
    {
        // reserve space for 400 motions
        m_motions.reserve(400);

        m_motionDataFactory = aznew MotionDataFactory();
    }

    MotionManager::~MotionManager()
    {
        delete m_motionDataFactory;
    }

    // create
    MotionManager* MotionManager::Create()
    {
        return aznew MotionManager();
    }


    // clear the motion manager
    void MotionManager::Clear(bool delFromMemory)
    {
        if (delFromMemory)
        {
            // destroy all motion sets, they will internally call RemoveMotionSetWithoutLock(this) in their destructor
            while (m_motionSets.size() > 0)
            {
                delete m_motionSets[0];
            }

            // destroy all motions, they will internally call RemoveMotionWithoutLock(this) in their destructor
            while (m_motions.size() > 0)
            {
                m_motions[0]->Destroy();
            }
        }
        else
        {
            // wait with execution until we can set the lock
            m_setLock.Lock();
            m_motionSets.clear();
            m_setLock.Unlock();

            // clear the arrays without destroying the memory of the entries
            m_lock.Lock();
            m_motions.clear();
            m_lock.Unlock();
        }
    }


    // find the motion and return a pointer, nullptr if the motion is not in
    Motion* MotionManager::FindMotionByName(const char* motionName, bool isTool) const
    {
        const auto foundMotion = AZStd::find_if(begin(m_motions), end(m_motions), [motionName, isTool](const auto& motion)
        {
            return motion->GetIsOwnedByRuntime() != isTool && motion->GetNameString() == motionName;
        });
        return foundMotion != end(m_motions) ? *foundMotion : nullptr;
    }


    // find the motion and return a pointer, nullptr if the motion is not in
    Motion* MotionManager::FindMotionByFileName(const char* fileName, bool isTool) const
    {
        const auto foundMotion = AZStd::find_if(begin(m_motions), end(m_motions), [fileName, isTool](const auto& motion)
        {
            return motion->GetIsOwnedByRuntime() != isTool &&
                AzFramework::StringFunc::Equal(motion->GetFileNameString().c_str(), fileName, false /* no case */);
        });
        return foundMotion != end(m_motions) ? *foundMotion : nullptr;
    }


    // find the motion set by filename and return a pointer, nullptr if the motion set is not in yet
    MotionSet* MotionManager::FindMotionSetByFileName(const char* fileName, bool isTool) const
    {
        const auto foundMotionSet = AZStd::find_if(begin(m_motionSets), end(m_motionSets), [fileName, isTool](const auto& motionSet)
        {
            return motionSet->GetIsOwnedByRuntime() != isTool &&
                AzFramework::StringFunc::Equal(motionSet->GetFilename(), fileName);
        });
        return foundMotionSet != end(m_motionSets) ? *foundMotionSet : nullptr;
    }


    // find the motion set and return a pointer, nullptr if the motion set has not been found
    MotionSet* MotionManager::FindMotionSetByName(const char* name, bool isOwnedByRuntime) const
    {
        const auto foundMotionSet = AZStd::find_if(begin(m_motionSets), end(m_motionSets), [name, isOwnedByRuntime](const auto& motionSet)
        {
            return motionSet->GetIsOwnedByRuntime() == isOwnedByRuntime &&
                AzFramework::StringFunc::Equal(motionSet->GetName(), name);
        });
        return foundMotionSet != end(m_motionSets) ? *foundMotionSet : nullptr;
    }


    // find the motion index for the given motion
    size_t MotionManager::FindMotionIndexByName(const char* motionName, bool isTool) const
    {
        const auto foundMotion = AZStd::find_if(begin(m_motions), end(m_motions), [motionName, isTool](const auto& motion)
        {
            return motion->GetIsOwnedByRuntime() != isTool && motion->GetNameString() == motionName;
        });
        return foundMotion != end(m_motions) ? AZStd::distance(begin(m_motions), foundMotion) : InvalidIndex;
    }


    // find the motion set index for the given motion
    size_t MotionManager::FindMotionSetIndexByName(const char* name, bool isTool) const
    {
        const auto foundMotionSet = AZStd::find_if(begin(m_motionSets), end(m_motionSets), [name, isTool](const MotionSet* motionSet)
        {
            return motionSet->GetIsOwnedByRuntime() != isTool &&
                AzFramework::StringFunc::Equal(motionSet->GetName(), name);
        });
        return foundMotionSet != end(m_motionSets) ? AZStd::distance(begin(m_motionSets), foundMotionSet) : InvalidIndex;
    }


    // find the motion index for the given motion
    size_t MotionManager::FindMotionIndexByID(uint32 id) const
    {
        const auto foundMotion = AZStd::find_if(begin(m_motions), end(m_motions), [id](const Motion* motion)
        {
            return motion->GetID() == id;
        });
        return foundMotion != end(m_motions) ? AZStd::distance(begin(m_motions), foundMotion) : InvalidIndex;
        // get the number of motions and iterate through them
    }


    // find the motion set index
    size_t MotionManager::FindMotionSetIndexByID(uint32 id) const
    {
        const auto foundMotionSet = AZStd::find_if(begin(m_motionSets), end(m_motionSets), [id](const MotionSet* motionSet)
        {
            return motionSet->GetID() == id;
        });
        return foundMotionSet != end(m_motionSets) ? AZStd::distance(begin(m_motionSets), foundMotionSet) : InvalidIndex;
    }


    // find the motion and return a pointer, nullptr if the motion is not in
    Motion* MotionManager::FindMotionByID(uint32 id) const
    {
        const auto foundMotion = AZStd::find_if(begin(m_motions), end(m_motions), [id](const Motion* motion)
        {
            return motion->GetID() == id;
        });
        return foundMotion != end(m_motions) ? *foundMotion : nullptr;
    }


    // find the motion set with the given and return it, nullptr if the motion set won't be found
    MotionSet* MotionManager::FindMotionSetByID(uint32 id) const
    {
        const auto foundMotionSet = AZStd::find_if(begin(m_motionSets), end(m_motionSets), [id](const MotionSet* motionSet)
        {
            return motionSet->GetID() == id;
        });
        return foundMotionSet != end(m_motionSets) ? *foundMotionSet : nullptr;
    }


    // find the motion set index and return it
    size_t MotionManager::FindMotionSetIndex(MotionSet* motionSet) const
    {
        const auto foundMotionSet = AZStd::find_if(begin(m_motionSets), end(m_motionSets), [motionSet](const MotionSet* ms)
        {
            return ms == motionSet;
        });
        return foundMotionSet != end(m_motionSets) ? AZStd::distance(begin(m_motionSets), foundMotionSet) : InvalidIndex;
    }


    // find the motion index for the given motion
    size_t MotionManager::FindMotionIndex(Motion* motion) const
    {
        const auto foundMotion = AZStd::find_if(begin(m_motions), end(m_motions), [motion](const Motion* m)
        {
            return m == motion;
        });
        return foundMotion != end(m_motions) ? AZStd::distance(begin(m_motions), foundMotion) : InvalidIndex;
    }


    // add the motion to the manager
    void MotionManager::AddMotion(Motion* motion)
    {
        // wait with execution until we can set the lock
        m_lock.Lock();
        m_motions.emplace_back(motion);
        m_lock.Unlock();
    }


    // find the motion based on the name and remove it
    bool MotionManager::RemoveMotionByName(const char* motionName, bool delFromMemory, bool isTool)
    {
        MCore::LockGuard lock(m_lock);
        return RemoveMotionWithoutLock(FindMotionIndexByName(motionName, isTool), delFromMemory);
    }


    // find the motion set based on the name and remove it
    bool MotionManager::RemoveMotionSetByName(const char* motionName, bool delFromMemory, bool isTool)
    {
        MCore::LockGuard lock(m_setLock);
        return RemoveMotionSetWithoutLock(FindMotionSetIndexByName(motionName, isTool), delFromMemory);
    }


    // find the motion based on the id and remove it
    bool MotionManager::RemoveMotionByID(uint32 id, bool delFromMemory)
    {
        MCore::LockGuard lock(m_lock);
        return RemoveMotionWithoutLock(FindMotionIndexByID(id), delFromMemory);
    }


    // find the motion set based on the id and remove it
    bool MotionManager::RemoveMotionSetByID(uint32 id, bool delFromMemory)
    {
        MCore::LockGuard lock(m_setLock);
        return RemoveMotionSetWithoutLock(FindMotionSetIndexByID(id), delFromMemory);
    }


    // find the index by filename
    size_t MotionManager::FindMotionIndexByFileName(const char* fileName, bool isTool) const
    {
        const auto foundMotion = AZStd::find_if(begin(m_motions), end(m_motions), [fileName, isTool](const Motion* motion)
        {
            return motion->GetIsOwnedByRuntime() != isTool && motion->GetFileNameString() == fileName;
        });
        return foundMotion != end(m_motions) ? AZStd::distance(begin(m_motions), foundMotion) : InvalidIndex;
    }


    // remove the motion by a given filename
    bool MotionManager::RemoveMotionByFileName(const char* fileName, bool delFromMemory, bool isTool)
    {
        MCore::LockGuard lock(m_lock);
        return RemoveMotionWithoutLock(FindMotionIndexByFileName(fileName, isTool), delFromMemory);
    }


    // recursively reset all motion nodes
    void MotionManager::ResetMotionNodes(AnimGraph* animGraph, Motion* motion)
    {
        const size_t numAnimGraphInstances = animGraph->GetNumAnimGraphInstances();
        for (size_t b = 0; b < numAnimGraphInstances; ++b)
        {
            AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(b);

            // reset all motion nodes that use this motion
            const size_t numNodes = animGraph->GetNumNodes();
            for (size_t m = 0; m < numNodes; ++m)
            {
                AnimGraphNode* node = animGraph->GetNode(m);
                AnimGraphNodeData* uniqueData = static_cast<AnimGraphNodeData*>(animGraphInstance->GetUniqueObjectData(node->GetObjectIndex()));
                if (uniqueData)
                {
                    if (motion->GetEventTable()->GetSyncTrack() == uniqueData->GetSyncTrack())
                    {
                        uniqueData->SetSyncTrack(nullptr);
                    }

                    if (azrtti_istypeof<AnimGraphMotionNode>(node))
                    {
                        AnimGraphMotionNode::UniqueData* motionNodeData = static_cast<AnimGraphMotionNode::UniqueData*>(uniqueData);
                        const MotionInstance* motionInstance = motionNodeData->m_motionInstance;
                        if (motionInstance && motionInstance->GetMotion() == motion)
                        {
                            motionNodeData->Reset();
                        }
                    }

                    if (azrtti_istypeof<BlendSpace1DNode>(node) ||
                        azrtti_istypeof<BlendSpace2DNode>(node))
                    {
                        uniqueData->Reset();
                    }
                }
            }
        }
    }


    // remove the motion with the given index from the motion manager
    bool MotionManager::RemoveMotionWithoutLock(size_t index, bool delFromMemory)
    {
        if (index == InvalidIndex)
        {
            return false;
        }

        Motion* motion = m_motions[index];

        // stop all motion instances of the motion to delete
        const size_t numActorInstances = GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            ActorInstance* actorInstance = GetActorManager().GetActorInstance(i);
            MotionSystem* motionSystem = actorInstance->GetMotionSystem();
            MCORE_ASSERT(actorInstance->GetMotionSystem());

            // instances and iterate through the motion instances
            for (size_t j = 0; j < motionSystem->GetNumMotionInstances(); )
            {
                MotionInstance* motionInstance = motionSystem->GetMotionInstance(j);

                // only stop instances that belong to our motion we want to remove
                if (motionInstance->GetMotion() == motion)
                {
                    // stop the motion instance and remove it directly from the motion system
                    motionInstance->Stop(0.0f);
                    actorInstance->GetMotionSystem()->RemoveMotionInstance(motionInstance);
                }
                else
                {
                    j++;
                }
            }
        }

        // Reset all motion entries in the motion sets of the current motion.
        for (const MotionSet* motionSet : m_motionSets)
        {
            const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
            for (const auto& item : motionEntries)
            {
                EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

                if (motionEntry->GetMotion() == motion)
                {
                    motionEntry->Reset();
                }
            }
        }

        // stop all motion instances of the motion to delete inside the motion nodes and reset their unique data
        const size_t numAnimGraphs = GetAnimGraphManager().GetNumAnimGraphs();
        for (size_t i = 0; i < numAnimGraphs; ++i)
        {
            AnimGraph* animGraph = GetAnimGraphManager().GetAnimGraph(i);
            ResetMotionNodes(animGraph, motion);
        }

        if (delFromMemory)
        {
            // destroy the memory of the motion, the destructor of the motion internally calls MotionManager::RemoveMotion(this)
            // which unregisters the motion from the motion manager
            motion->SetAutoUnregister(false);
            motion->Destroy();
            m_motions.erase(AZStd::next(begin(m_motions), index)); // only remove the motion from the motion manager without destroying its memory
        }
        else
        {
            m_motions.erase(AZStd::next(begin(m_motions), index)); // only remove the motion from the motion manager without destroying its memory
        }

        return true;
    }


    // add a new motion set
    void MotionManager::AddMotionSet(MotionSet* motionSet)
    {
        MCore::LockGuard lock(m_lock);
        m_motionSets.emplace_back(motionSet);
    }


    // remove the motion set with the given index from the motion manager
    bool MotionManager::RemoveMotionSetWithoutLock(size_t index, bool delFromMemory)
    {
        if (index == InvalidIndex)
        {
            return false;
        }

        MotionSet* motionSet = m_motionSets[index];

        // remove from the parent
        MotionSet* parentSet = motionSet->GetParentSet();
        if (parentSet)
        {
            parentSet->RemoveChildSetByID(motionSet->GetID());
        }

        // reset all anim graph instance motion sets that use the motion set to delete
        const size_t numAnimGraphInstances = GetAnimGraphManager().GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = GetAnimGraphManager().GetAnimGraphInstance(i);
            if (animGraphInstance->GetMotionSet() == motionSet)
            {
                animGraphInstance->SetMotionSet(nullptr);
            }
        }

        if (delFromMemory)
        {
            motionSet->SetAutoUnregister(false);
            delete motionSet;
        }

        m_motionSets.erase(AZStd::next(begin(m_motionSets), index));

        return true;
    }


    // remove the motion from the motion manager
    bool MotionManager::RemoveMotion(Motion* motion, bool delFromMemory)
    {
        MCore::LockGuard lock(m_lock);
        return RemoveMotionWithoutLock(FindMotionIndex(motion), delFromMemory);
    }


    // remove the motion set from the motion manager
    bool MotionManager::RemoveMotionSet(MotionSet* motionSet, bool delFromMemory)
    {
        MCore::LockGuard lock(m_setLock);
        return RemoveMotionSetWithoutLock(FindMotionSetIndex(motionSet), delFromMemory);
    }


    // calculate the number of root motion sets
    size_t MotionManager::CalcNumRootMotionSets() const
    {
        size_t result = 0;

        // get the number of motion sets and iterate through them
        for (const MotionSet* motionSet : m_motionSets)
        {
            // sum up the root motion sets
            if (motionSet->GetParentSet() == nullptr)
            {
                result++;
            }
        }

        return result;
    }


    // find the given root motion set
    MotionSet* MotionManager::FindRootMotionSet(size_t index)
    {
        auto foundRootMotionSet = AZStd::find_if(begin(m_motionSets), end(m_motionSets), [iter = index](const MotionSet* motionSet) mutable
        {
            return motionSet->GetParentSet() == nullptr && iter-- == 0;
        });
        return foundRootMotionSet != end(m_motionSets) ? *foundRootMotionSet : nullptr;
    }


    // wait with execution until we can set the lock
    void MotionManager::Lock()
    {
        m_lock.Lock();
    }


    // release the lock again
    void MotionManager::Unlock()
    {
        m_lock.Unlock();
    }

    MotionDataFactory& MotionManager::GetMotionDataFactory()
    {
        return *m_motionDataFactory;
    }

    const MotionDataFactory& MotionManager::GetMotionDataFactory() const
    {
        return *m_motionDataFactory;
    }
} // namespace EMotionFX
