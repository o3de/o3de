/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "EMotionFXConfig.h"
#include "MotionSystem.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "EventManager.h"
#include "MotionInstancePool.h"
#include "EMotionFXManager.h"
#include "MotionInstancePool.h"
#include "MotionQueue.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionSystem, MotionAllocator, 0)


    // constructor
    MotionSystem::MotionSystem(ActorInstance* actorInstance)
        : BaseObject()
    {
        MCORE_ASSERT(actorInstance);

        mMotionInstances.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONSYSTEMS);
        mActorInstance  = actorInstance;
        mMotionQueue    = nullptr;

        // create the motion queue
        mMotionQueue = MotionQueue::Create(actorInstance, this);

        GetEventManager().OnCreateMotionSystem(this);
    }


    // destructor
    MotionSystem::~MotionSystem()
    {
        GetEventManager().OnDeleteMotionSystem(this);

        // delete the motion infos
        while (mMotionInstances.GetLength())
        {
            //delete mMotionInstances.GetLast();
            GetMotionInstancePool().Free(mMotionInstances.GetLast());
            mMotionInstances.RemoveLast();
        }

        // get rid of the motion queue
        if (mMotionQueue)
        {
            mMotionQueue->Destroy();
        }
    }


    // play a given motion on an actor, and return the instance
    MotionInstance* MotionSystem::PlayMotion(Motion* motion, PlayBackInfo* info)
    {
        // check some things
        if (motion == nullptr)
        {
            return nullptr;
        }

        PlayBackInfo tempInfo;
        if (info == nullptr)
        {
            info = &tempInfo;
        }

        /*
            // if we want to play a motion which will loop forever (so never ends) and we want to put it on the queue
            if (info->mNumLoops==FOREVER && info->mPlayNow==false)
            {
                // if there is already a motion on the queue, this means the queue would end up in some kind of deadlock
                // because it has to wait until the current motion is finished with playing, before it would start this motion
                // and since that will never happen, the queue won't be processed anymore...
                // so we may simply not allow this to happen.
                if (mMotionQueue->GetNumEntries() > 0)
                    throw Exception("Cannot schedule this LOOPING motion to be played later, because there are already motions queued. If we would put this motion on the queue, all motions added later on to the queue will never be processed because this motion is a looping (so never ending) one.", MCORE_HERE);
            }*/

        // trigger the OnPlayMotion event
        GetEventManager().OnPlayMotion(motion, info);

        // make sure we always mix when using additive blending
        if (info->mBlendMode == BLENDMODE_ADDITIVE && info->mMix == false)
        {
            MCORE_ASSERT(false); // this shouldn't happen actually, please make sure you always mix additive motions
            info->mMix = true;
        }

        // create the motion instance and add the motion info the this actor
        MotionInstance* motionInst = CreateMotionInstance(motion, info);

        // if we want to play it immediately (so if we do NOT want to schedule it for later on)
        if (info->mPlayNow)
        {
            // start the motion for real
            StartMotion(motionInst, info);
        }
        else
        {
            // schedule the motion, by adding it to the back of the motion queue
            mMotionQueue->AddEntry(MotionQueue::QueueEntry(motionInst, info));
            motionInst->Pause();
            motionInst->SetIsActive(false);
            GetEventManager().OnQueueMotionInstance(motionInst, info);
        }

        // return the pointer to the motion info
        return motionInst;
    }


    // create the motion instance and add the motion info the this actor
    MotionInstance* MotionSystem::CreateMotionInstance(Motion* motion, PlayBackInfo* info)
    {
        // create the motion instance
        MotionInstance* motionInst = GetMotionInstancePool().RequestNew(motion, mActorInstance);

        // initialize the motion instance from the playback info settings
        motionInst->InitFromPlayBackInfo(*info);

        return motionInst;
    }


    // remove a given motion instance
    bool MotionSystem::RemoveMotionInstance(MotionInstance* instance)
    {
        // remove the motion instance from the actor
        const bool isSuccess = mMotionInstances.RemoveByValue(instance);

        // delete the motion instance from memory
        if (isSuccess)
        {
            GetMotionInstancePool().Free(instance);
        }

        // return if it all worked :)
        return isSuccess;
    }


    // update motion queue and instances
    void MotionSystem::Update(float timePassed, bool updateNodes)
    {
        MCORE_UNUSED(updateNodes);

        // update the motion queue
        mMotionQueue->Update();

        // update the motions
        UpdateMotionInstances(timePassed);
    }


    // stop all the motions that are currently playing
    void MotionSystem::StopAllMotions()
    {
        const uint32 numInstances = mMotionInstances.GetLength();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            mMotionInstances[i]->Stop();
        }
    }


    // stop all motion instances of a given motion
    void MotionSystem::StopAllMotions(Motion* motion)
    {
        const uint32 numInstances = mMotionInstances.GetLength();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            if (mMotionInstances[i]->GetMotion()->GetID() == motion->GetID())
            {
                mMotionInstances[i]->Stop();
            }
        }
    }


    // remove the given motion
    void MotionSystem::RemoveMotion(uint32 nr, bool deleteMem)
    {
        MCORE_ASSERT(nr < mMotionInstances.GetLength());

        if (deleteMem)
        {
            GetEMotionFX().GetMotionInstancePool()->Free(mMotionInstances[nr]);
        }

        mMotionInstances.Remove(nr);
    }


    // remove the given motion
    void MotionSystem::RemoveMotion(MotionInstance* motion, bool delMem)
    {
        MCORE_ASSERT(motion);

        uint32 nr = mMotionInstances.Find(motion);
        MCORE_ASSERT(nr != MCORE_INVALIDINDEX32);

        if (nr == MCORE_INVALIDINDEX32)
        {
            return;
        }

        RemoveMotion(nr, delMem);
    }


    // update the motion infos
    void MotionSystem::UpdateMotionInstances(float timePassed)
    {
        // update all the motion infos
        const uint32 numInstances = mMotionInstances.GetLength();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            mMotionInstances[i]->Update(timePassed);
        }
    }


    // check if the given motion instance still exists within the actor, so if it hasn't been deleted from memory yet
    bool MotionSystem::CheckIfIsValidMotionInstance(MotionInstance* instance) const
    {
        // if it's a null pointer, just return
        if (instance == nullptr)
        {
            return false;
        }

        // for all motion instances currently playing in this actor
        const uint32 numInstances = mMotionInstances.GetLength();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            // check if this one is the one we are searching for, if so, return that it is still valid
            if (mMotionInstances[i] == instance) // if the memory object appears to be valid
            {
                if (mMotionInstances[i]->GetID() == instance->GetID()) // check if the id is the same, as a new motion theoretically could have received the same memory address
                {
                    return true;
                }
            }
        }

        // it's not found, this means it has already been deleted from memory and is not valid anymore
        return false;
    }


    // check if there is a motion instance playing, which is an instance of a specified motion
    bool MotionSystem::CheckIfIsPlayingMotion(Motion* motion, bool ignorePausedMotions) const
    {
        if (!motion)
        {
            return false;
        }

        // for all motion instances currently playing in this actor
        const uint32 numInstances = mMotionInstances.GetLength();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            const MotionInstance* motionInstance = mMotionInstances[i];

            if (ignorePausedMotions && motionInstance->GetIsPaused())
            {
                continue;
            }

            // check if the motion instance is an instance of the motion we are searching for
            if (motionInstance->GetMotion()->GetID() == motion->GetID())
            {
                return true;
            }
        }

        // it's not found, this means it has already been deleted from memory and is not valid anymore
        return false;
    }


    // return given motion instance
    MotionInstance* MotionSystem::GetMotionInstance(uint32 nr) const
    {
        MCORE_ASSERT(nr < mMotionInstances.GetLength());
        return mMotionInstances[nr];
    }


    // return number of motion instances
    uint32 MotionSystem::GetNumMotionInstances() const
    {
        return mMotionInstances.GetLength();
    }


    // set a new motion queue
    void MotionSystem::SetMotionQueue(MotionQueue* motionQueue)
    {
        if (mMotionQueue)
        {
            mMotionQueue->Destroy();
        }

        mMotionQueue = motionQueue;
    }


    // add a motion queue to the motion system
    void MotionSystem::AddMotionQueue(MotionQueue* motionQueue)
    {
        MCORE_ASSERT(motionQueue);

        // copy entries from the given queue to the motion system's one
        for (uint32 i = 0; i < motionQueue->GetNumEntries(); ++i)
        {
            mMotionQueue->AddEntry(motionQueue->GetEntry(i));
        }

        // get rid of the given motion queue
        motionQueue->Destroy();
    }


    // return motion queue pointer
    MotionQueue* MotionSystem::GetMotionQueue() const
    {
        return mMotionQueue;
    }


    // return the actor to which this motion system belongs to
    ActorInstance* MotionSystem::GetActorInstance() const
    {
        return mActorInstance;
    }


    void MotionSystem::AddMotionInstance(MotionInstance* instance)
    {
        mMotionInstances.Add(instance);
    }


    bool MotionSystem::GetIsPlaying() const
    {
        return (mMotionInstances.GetLength() > 0);
    }
} // namespace EMotionFX
