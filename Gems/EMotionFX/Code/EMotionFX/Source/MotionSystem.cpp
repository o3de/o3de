/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        while (!mMotionInstances.empty())
        {
            //delete mMotionInstances.GetLast();
            GetMotionInstancePool().Free(mMotionInstances.back());
            mMotionInstances.pop_back();
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
        const bool isSuccess = [this, instance] {
            if(const auto it = AZStd::find(begin(mMotionInstances), end(mMotionInstances), instance); it != end(mMotionInstances))
            {
                mMotionInstances.erase(it);
                return true;
            }
            return false;
        }();

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
        for (MotionInstance* motionInstance : mMotionInstances)
        {
            motionInstance->Stop();
        }
    }


    // stop all motion instances of a given motion
    void MotionSystem::StopAllMotions(Motion* motion)
    {
        for (MotionInstance* motionInstance : mMotionInstances)
        {
            if (motionInstance->GetMotion()->GetID() == motion->GetID())
            {
                motionInstance->Stop();
            }
        }
    }


    // remove the given motion
    void MotionSystem::RemoveMotion(size_t nr, bool deleteMem)
    {
        MCORE_ASSERT(nr < mMotionInstances.size());

        if (deleteMem)
        {
            GetEMotionFX().GetMotionInstancePool()->Free(mMotionInstances[nr]);
        }

        mMotionInstances.erase(AZStd::next(begin(mMotionInstances), nr));
    }


    // remove the given motion
    void MotionSystem::RemoveMotion(MotionInstance* motion, bool delMem)
    {
        MCORE_ASSERT(motion);

        const auto it = AZStd::find(begin(mMotionInstances), end(mMotionInstances), motion);
        MCORE_ASSERT(it != end(mMotionInstances));

        if (it == end(mMotionInstances))
        {
            return;
        }

        RemoveMotion(AZStd::distance(begin(mMotionInstances), it), delMem);
    }


    // update the motion infos
    void MotionSystem::UpdateMotionInstances(float timePassed)
    {
        // update all the motion infos
        for (MotionInstance* motionInstance : mMotionInstances)
        {
            motionInstance->Update(timePassed);
        }
    }


    // check if the given motion instance still exists within the actor, so if it hasn't been deleted from memory yet
    bool MotionSystem::CheckIfIsValidMotionInstance(MotionInstance* instance) const
    {
        return instance && AZStd::any_of(begin(mMotionInstances), end(mMotionInstances), [instance](const MotionInstance* motionInstance)
        {
            return motionInstance->GetID() == instance->GetID();
        });
    }


    // check if there is a motion instance playing, which is an instance of a specified motion
    bool MotionSystem::CheckIfIsPlayingMotion(Motion* motion, bool ignorePausedMotions) const
    {
        return motion && AZStd::any_of(begin(mMotionInstances), end(mMotionInstances), [motion, ignorePausedMotions](const MotionInstance* motionInstance)
        {
            return !(ignorePausedMotions && motionInstance->GetIsPaused()) &&
                motionInstance->GetMotion()->GetID() == motion->GetID();
        });
    }


    // return given motion instance
    MotionInstance* MotionSystem::GetMotionInstance(size_t nr) const
    {
        MCORE_ASSERT(nr < mMotionInstances.size());
        return mMotionInstances[nr];
    }


    // return number of motion instances
    size_t MotionSystem::GetNumMotionInstances() const
    {
        return mMotionInstances.size();
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
        for (size_t i = 0; i < motionQueue->GetNumEntries(); ++i)
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
        mMotionInstances.emplace_back(instance);
    }


    bool MotionSystem::GetIsPlaying() const
    {
        return !mMotionInstances.empty();
    }
} // namespace EMotionFX
