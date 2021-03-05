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

// include required headers
#include "EMotionFXConfig.h"
#include "MotionQueue.h"
#include "MotionInstance.h"
#include "MotionSystem.h"
#include "EMotionFXManager.h"
#include "MotionInstancePool.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionQueue, MotionAllocator, 0)


    // constructor
    MotionQueue::MotionQueue(ActorInstance* actorInstance, MotionSystem* motionSystem)
        : BaseObject()
    {
        MCORE_ASSERT(actorInstance && motionSystem);

        mEntries.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONS_MISC);
        mActorInstance  = actorInstance;
        mMotionSystem   = motionSystem;
    }


    // destructor
    MotionQueue::~MotionQueue()
    {
        ClearAllEntries();
    }


    // create
    MotionQueue* MotionQueue::Create(ActorInstance* actorInstance, MotionSystem* motionSystem)
    {
        return aznew MotionQueue(actorInstance, motionSystem);
    }


    // remove a given entry from the queue
    void MotionQueue::RemoveEntry(uint32 nr)
    {
        if (mMotionSystem->RemoveMotionInstance(mEntries[nr].mMotion) == false)
        {
            GetMotionInstancePool().Free(mEntries[nr].mMotion);
        }

        mEntries.Remove(nr);
    }


    // update the motion queue
    void MotionQueue::Update()
    {
        // get the number of entries
        uint32 numEntries = GetNumEntries();

        // if there are entries in the queue
        if (numEntries == 0)
        {
            return;
        }

        // if there is only one entry in the queue, we can start playing it immediately
        if (mMotionSystem->GetIsPlaying() == false)
        {
            // get the entry from the queue to play next
            MotionQueue::QueueEntry queueEntry = GetFirstEntry();

            // remove it from the queue
            RemoveFirstEntry();

            // start the motion on the queue
            mMotionSystem->StartMotion(queueEntry.mMotion, &queueEntry.mPlayInfo);

            // get out of this method, nothing more to do :)
            return;
        }

        // if we want to play the next motion, do it
        if (ShouldPlayNextMotion())
        {
            PlayNextMotion();
        }
    }


    // play the next motion
    void MotionQueue::PlayNextMotion()
    {
        // if there are no entries in the queue there is nothing to play
        if (GetNumEntries() == 0)
        {
            return;
        }

        // get the entry from the queue
        MotionQueue::QueueEntry queueEntry = GetFirstEntry();

        // remove it from the queue
        RemoveFirstEntry();

        // start the motion
        mMotionSystem->StartMotion(queueEntry.mMotion, &queueEntry.mPlayInfo);
    }


    // find out if we should start playing the next motion in the queue or not
    bool MotionQueue::ShouldPlayNextMotion()
    {
        // find the first non mixing motion
        MotionInstance* motionInst = mMotionSystem->FindFirstNonMixingMotionInstance();

        // if there isn't a non mixing motion
        if (motionInst == nullptr)
        {
            return false;
        }

        // the total amount of blending time
        const float timeToRemoveFromMaxTime = GetFirstEntry().mPlayInfo.mBlendInTime + motionInst->GetFadeTime();

        // if the motion has ended or is stopping, then we should start the next motion
        if (motionInst->GetIsStopping() || motionInst->GetHasEnded())
        {
            return true;
        }
        else
        {
            // if the max playback time is used
            if (motionInst->GetMaxPlayTime() > 0.0f)
            {
                // if the start of the next motion will have completely faded in, before this motion instance will fade out
                if (motionInst->GetCurrentTime() >= motionInst->GetMaxPlayTime() - timeToRemoveFromMaxTime)
                {
                    return true;
                }
            }

            // if we are not looping forever
            if (motionInst->GetMaxLoops() != EMFX_LOOPFOREVER)
            {
                // if we are in our last loop
                if (motionInst->GetMaxLoops() - 1 == motionInst->GetNumCurrentLoops())
                {
                    // if the start of the next motion will have completely faded in, before this motion instance will fade out
                    if (motionInst->GetCurrentTime() >= motionInst->GetDuration() - timeToRemoveFromMaxTime)
                    {
                        return true;
                    }
                }
            }
        }

        // shouldn't play the next motion yet
        return false;
    }


    void MotionQueue::ClearAllEntries()
    {
        while (mEntries.GetLength())
        {
            RemoveEntry(0);
        }
    }


    void MotionQueue::AddEntry(const MotionQueue::QueueEntry& motion)
    {
        mEntries.Add(motion);
    }


    uint32 MotionQueue::GetNumEntries() const
    {
        return mEntries.GetLength();
    }


    MotionQueue::QueueEntry& MotionQueue::GetFirstEntry()
    {
        MCORE_ASSERT(mEntries.GetLength() > 0);
        return mEntries[0];
    }


    void MotionQueue::RemoveFirstEntry()
    {
        mEntries.RemoveFirst();
    }


    MotionQueue::QueueEntry& MotionQueue::GetEntry(uint32 nr)
    {
        return mEntries[nr];
    }
} // namespace EMotionFX
