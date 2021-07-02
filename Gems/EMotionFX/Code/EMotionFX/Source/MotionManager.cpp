/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <EMotionFX/Source/BaseObject.h>
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
#include <MCore/Source/Array.h>
#include <MCore/Source/MultiThreadManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionManager, MotionManagerAllocator, 0)


    // constructor
    MotionManager::MotionManager()
        : BaseObject()
    {
        mMotions.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONMANAGER);
        mMotionSets.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONMANAGER);

        // reserve space for 400 motions
        mMotions.Reserve(400);

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
            while (mMotionSets.GetLength() > 0)
            {
                delete mMotionSets[0];
            }

            // destroy all motions, they will internally call RemoveMotionWithoutLock(this) in their destructor
            while (mMotions.GetLength() > 0)
            {
                mMotions[0]->Destroy();
            }
        }
        else
        {
            // wait with execution until we can set the lock
            mSetLock.Lock();
            mMotionSets.Clear();
            mSetLock.Unlock();

            // clear the arrays without destroying the memory of the entries
            mLock.Lock();
            mMotions.Clear();
            mLock.Unlock();
        }
    }


    // find the motion and return a pointer, nullptr if the motion is not in
    Motion* MotionManager::FindMotionByName(const char* motionName, bool isTool) const
    {
        // get the number of motions and iterate through them
        const uint32 numMotions = mMotions.GetLength();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            if (mMotions[i]->GetIsOwnedByRuntime() == isTool)
            {
                continue;
            }

            // compare the motion names
            if (mMotions[i]->GetNameString() == motionName)
            {
                return mMotions[i];
            }
        }

        return nullptr;
    }


    // find the motion and return a pointer, nullptr if the motion is not in
    Motion* MotionManager::FindMotionByFileName(const char* fileName, bool isTool) const
    {
        // get the number of motions and iterate through them
        const uint32 numMotions = mMotions.GetLength();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            if (mMotions[i]->GetIsOwnedByRuntime() == isTool)
            {
                continue;
            }

            // compare the motion names
            if (AzFramework::StringFunc::Equal(mMotions[i]->GetFileNameString().c_str(), fileName, false /* no case */))
            {
                return mMotions[i];
            }
        }

        return nullptr;
    }


    // find the motion set by filename and return a pointer, nullptr if the motion set is not in yet
    MotionSet* MotionManager::FindMotionSetByFileName(const char* fileName, bool isTool) const
    {
        // get the number of motion sets and iterate through them
        const uint32 numMotionSets = mMotionSets.GetLength();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            MotionSet* motionSet = mMotionSets[i];
            if (motionSet->GetIsOwnedByRuntime() == isTool)
            {
                continue;
            }

            // compare the motion set filenames
            if (AzFramework::StringFunc::Equal(motionSet->GetFilename(), fileName))
            {
                return motionSet;
            }
        }

        return nullptr;
    }


    // find the motion set and return a pointer, nullptr if the motion set has not been found
    MotionSet* MotionManager::FindMotionSetByName(const char* name, bool isOwnedByRuntime) const
    {
        // get the number of motion sets and iterate through them
        const uint32 numMotionSets = mMotionSets.GetLength();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            MotionSet* motionSet = mMotionSets[i];

            if (motionSet->GetIsOwnedByRuntime() == isOwnedByRuntime)
            {
                // compare the motion set names
                if (AzFramework::StringFunc::Equal(motionSet->GetName(), name))
                {
                    return motionSet;
                }
            }
        }

        return nullptr;
    }


    // find the motion index for the given motion
    uint32 MotionManager::FindMotionIndexByName(const char* motionName, bool isTool) const
    {
        // get the number of motions and iterate through them
        const uint32 numMotions = mMotions.GetLength();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            if (mMotions[i]->GetIsOwnedByRuntime() == isTool)
            {
                continue;
            }

            // compare the motion names
            if (mMotions[i]->GetNameString() == motionName)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find the motion set index for the given motion
    uint32 MotionManager::FindMotionSetIndexByName(const char* name, bool isTool) const
    {
        // get the number of motions and iterate through them
        const uint32 numMotionSets = mMotionSets.GetLength();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            MotionSet* motionSet = mMotionSets[i];

            if (motionSet->GetIsOwnedByRuntime() == isTool)
            {
                continue;
            }

            // compare the motion set names
            if (AzFramework::StringFunc::Equal(motionSet->GetName(), name))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find the motion index for the given motion
    uint32 MotionManager::FindMotionIndexByID(uint32 id) const
    {
        // get the number of motions and iterate through them
        const uint32 numMotions = mMotions.GetLength();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            if (mMotions[i]->GetID() == id)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find the motion set index
    uint32 MotionManager::FindMotionSetIndexByID(uint32 id) const
    {
        // get the number of motion sets and iterate through them
        const uint32 numMotionSets = mMotionSets.GetLength();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            // compare the motion names
            if (mMotionSets[i]->GetID() == id)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find the motion and return a pointer, nullptr if the motion is not in
    Motion* MotionManager::FindMotionByID(uint32 id) const
    {
        // get the number of motions and iterate through them
        const uint32 numMotions = mMotions.GetLength();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            if (mMotions[i]->GetID() == id)
            {
                return mMotions[i];
            }
        }

        return nullptr;
    }


    // find the motion set with the given and return it, nullptr if the motion set won't be found
    MotionSet* MotionManager::FindMotionSetByID(uint32 id) const
    {
        // get the number of motion sets and iterate through them
        const uint32 numMotionSets = mMotionSets.GetLength();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            if (mMotionSets[i]->GetID() == id)
            {
                return mMotionSets[i];
            }
        }

        return nullptr;
    }


    // find the motion set index and return it
    uint32 MotionManager::FindMotionSetIndex(MotionSet* motionSet) const
    {
        // get the number of motion sets and iterate through them
        const uint32 numMotionSets = mMotionSets.GetLength();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            if (mMotionSets[i] == motionSet)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find the motion index for the given motion
    uint32 MotionManager::FindMotionIndex(Motion* motion) const
    {
        // get the number of motions and iterate through them
        const uint32 numMotions = mMotions.GetLength();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            // compare the motions
            if (motion == mMotions[i])
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // add the motion to the manager
    void MotionManager::AddMotion(Motion* motion)
    {
        // wait with execution until we can set the lock
        mLock.Lock();
        mMotions.Add(motion);
        mLock.Unlock();
    }


    // find the motion based on the name and remove it
    bool MotionManager::RemoveMotionByName(const char* motionName, bool delFromMemory, bool isTool)
    {
        MCore::LockGuard lock(mLock);
        return RemoveMotionWithoutLock(FindMotionIndexByName(motionName, isTool), delFromMemory);
    }


    // find the motion set based on the name and remove it
    bool MotionManager::RemoveMotionSetByName(const char* motionName, bool delFromMemory, bool isTool)
    {
        MCore::LockGuard lock(mSetLock);
        return RemoveMotionSetWithoutLock(FindMotionSetIndexByName(motionName, isTool), delFromMemory);
    }


    // find the motion based on the id and remove it
    bool MotionManager::RemoveMotionByID(uint32 id, bool delFromMemory)
    {
        MCore::LockGuard lock(mLock);
        return RemoveMotionWithoutLock(FindMotionIndexByID(id), delFromMemory);
    }


    // find the motion set based on the id and remove it
    bool MotionManager::RemoveMotionSetByID(uint32 id, bool delFromMemory)
    {
        MCore::LockGuard lock(mSetLock);
        return RemoveMotionSetWithoutLock(FindMotionSetIndexByID(id), delFromMemory);
    }


    // find the index by filename
    uint32 MotionManager::FindMotionIndexByFileName(const char* fileName, bool isTool) const
    {
        // get the number of motions and iterate through them
        const uint32 numMotions = mMotions.GetLength();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            if (mMotions[i]->GetIsOwnedByRuntime() == isTool)
            {
                continue;
            }

            // compare the motions
            if (mMotions[i]->GetFileNameString() == fileName)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // remove the motion by a given filename
    bool MotionManager::RemoveMotionByFileName(const char* fileName, bool delFromMemory, bool isTool)
    {
        MCore::LockGuard lock(mLock);
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
            const uint32 numNodes = animGraph->GetNumNodes();
            for (uint32 m = 0; m < numNodes; ++m)
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
                        const MotionInstance* motionInstance = motionNodeData->mMotionInstance;
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
    bool MotionManager::RemoveMotionWithoutLock(uint32 index, bool delFromMemory)
    {
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        uint32 i;
        Motion* motion = mMotions[index];

        // stop all motion instances of the motion to delete
        const uint32 numActorInstances = GetActorManager().GetNumActorInstances();
        for (i = 0; i < numActorInstances; ++i)
        {
            ActorInstance* actorInstance = GetActorManager().GetActorInstance(i);
            MotionSystem* motionSystem = actorInstance->GetMotionSystem();
            MCORE_ASSERT(actorInstance->GetMotionSystem());

            // instances and iterate through the motion instances
            for (uint32 j = 0; j < motionSystem->GetNumMotionInstances(); )
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
        const uint32 numMotionSets = mMotionSets.GetLength();
        for (i = 0; i < numMotionSets; ++i)
        {
            MotionSet* motionSet = mMotionSets[i];

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
        const uint32 numAnimGraphs = GetAnimGraphManager().GetNumAnimGraphs();
        for (i = 0; i < numAnimGraphs; ++i)
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
            mMotions.Remove(index); // only remove the motion from the motion manager without destroying its memory
        }
        else
        {
            mMotions.Remove(index); // only remove the motion from the motion manager without destroying its memory
        }

        return true;
    }


    // add a new motion set
    void MotionManager::AddMotionSet(MotionSet* motionSet)
    {
        MCore::LockGuard lock(mLock);
        mMotionSets.Add(motionSet);
    }


    // remove the motion set with the given index from the motion manager
    bool MotionManager::RemoveMotionSetWithoutLock(uint32 index, bool delFromMemory)
    {
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        MotionSet* motionSet = mMotionSets[index];

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

        mMotionSets.Remove(index);

        return true;
    }


    // remove the motion from the motion manager
    bool MotionManager::RemoveMotion(Motion* motion, bool delFromMemory)
    {
        MCore::LockGuard lock(mLock);
        return RemoveMotionWithoutLock(FindMotionIndex(motion), delFromMemory);
    }


    // remove the motion set from the motion manager
    bool MotionManager::RemoveMotionSet(MotionSet* motionSet, bool delFromMemory)
    {
        MCore::LockGuard lock(mSetLock);
        return RemoveMotionSetWithoutLock(FindMotionSetIndex(motionSet), delFromMemory);
    }


    // calculate the number of root motion sets
    uint32 MotionManager::CalcNumRootMotionSets() const
    {
        uint32 result = 0;

        // get the number of motion sets and iterate through them
        const uint32 numMotionSets = mMotionSets.GetLength();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            // sum up the root motion sets
            if (mMotionSets[i]->GetParentSet() == nullptr)
            {
                result++;
            }
        }

        return result;
    }


    // find the given root motion set
    MotionSet* MotionManager::FindRootMotionSet(uint32 index)
    {
        uint32 currentIndex = 0;

        // get the number of motion sets and iterate through them
        const uint32 numMotionSets = mMotionSets.GetLength();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            // get the current motion set
            MotionSet* motionSet = mMotionSets[i];

            // check if we are dealing with a root motion set and skip all others
            if (mMotionSets[i]->GetParentSet())
            {
                continue;
            }

            // compare the indices and return in case we reached it, if not increase the counter
            if (currentIndex == index)
            {
                return motionSet;
            }
            currentIndex++;
        }

        return nullptr;
    }


    // wait with execution until we can set the lock
    void MotionManager::Lock()
    {
        mLock.Lock();
    }


    // release the lock again
    void MotionManager::Unlock()
    {
        mLock.Unlock();
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
