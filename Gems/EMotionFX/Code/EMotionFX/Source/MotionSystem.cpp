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

        m_actorInstance  = actorInstance;
        m_motionQueue    = nullptr;

        // create the motion queue
        m_motionQueue = MotionQueue::Create(actorInstance, this);

        GetEventManager().OnCreateMotionSystem(this);
    }


    // destructor
    MotionSystem::~MotionSystem()
    {
        GetEventManager().OnDeleteMotionSystem(this);

        // delete the motion infos
        while (!m_motionInstances.empty())
        {
            GetMotionInstancePool().Free(m_motionInstances.back());
            m_motionInstances.pop_back();
        }

        // get rid of the motion queue
        if (m_motionQueue)
        {
            m_motionQueue->Destroy();
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

        // trigger the OnPlayMotion event
        GetEventManager().OnPlayMotion(motion, info);

        // make sure we always mix when using additive blending
        if (info->m_blendMode == BLENDMODE_ADDITIVE && info->m_mix == false)
        {
            MCORE_ASSERT(false); // this shouldn't happen actually, please make sure you always mix additive motions
            info->m_mix = true;
        }

        // create the motion instance and add the motion info the this actor
        MotionInstance* motionInst = CreateMotionInstance(motion, info);

        // if we want to play it immediately (so if we do NOT want to schedule it for later on)
        if (info->m_playNow)
        {
            // start the motion for real
            StartMotion(motionInst, info);
        }
        else
        {
            // schedule the motion, by adding it to the back of the motion queue
            m_motionQueue->AddEntry(MotionQueue::QueueEntry(motionInst, info));
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
        MotionInstance* motionInst = GetMotionInstancePool().RequestNew(motion, m_actorInstance);

        // initialize the motion instance from the playback info settings
        motionInst->InitFromPlayBackInfo(*info);

        return motionInst;
    }


    // remove a given motion instance
    bool MotionSystem::RemoveMotionInstance(MotionInstance* instance)
    {
        // remove the motion instance from the actor
        const bool isSuccess = [this, instance] {
            if(const auto it = AZStd::find(begin(m_motionInstances), end(m_motionInstances), instance); it != end(m_motionInstances))
            {
                m_motionInstances.erase(it);
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
        AZ_PROFILE_SCOPE(Animation, "MotionSystem::Update");

        MCORE_UNUSED(updateNodes);

        // update the motion queue
        m_motionQueue->Update();

        // update the motions
        UpdateMotionInstances(timePassed);
    }


    // stop all the motions that are currently playing
    void MotionSystem::StopAllMotions()
    {
        for (MotionInstance* motionInstance : m_motionInstances)
        {
            motionInstance->Stop();
        }
    }


    // stop all motion instances of a given motion
    void MotionSystem::StopAllMotions(Motion* motion)
    {
        for (MotionInstance* motionInstance : m_motionInstances)
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
        MCORE_ASSERT(nr < m_motionInstances.size());

        if (deleteMem)
        {
            GetEMotionFX().GetMotionInstancePool()->Free(m_motionInstances[nr]);
        }

        m_motionInstances.erase(AZStd::next(begin(m_motionInstances), nr));
    }


    // remove the given motion
    void MotionSystem::RemoveMotion(MotionInstance* motion, bool delMem)
    {
        MCORE_ASSERT(motion);

        const auto it = AZStd::find(begin(m_motionInstances), end(m_motionInstances), motion);
        MCORE_ASSERT(it != end(m_motionInstances));

        if (it == end(m_motionInstances))
        {
            return;
        }

        RemoveMotion(AZStd::distance(begin(m_motionInstances), it), delMem);
    }


    // update the motion infos
    void MotionSystem::UpdateMotionInstances(float timePassed)
    {
        // update all the motion infos
        for (MotionInstance* motionInstance : m_motionInstances)
        {
            motionInstance->Update(timePassed);
        }
    }


    // check if the given motion instance still exists within the actor, so if it hasn't been deleted from memory yet
    bool MotionSystem::CheckIfIsValidMotionInstance(MotionInstance* instance) const
    {
        return instance && AZStd::any_of(begin(m_motionInstances), end(m_motionInstances), [instance](const MotionInstance* motionInstance)
        {
            return motionInstance->GetID() == instance->GetID();
        });
    }


    // check if there is a motion instance playing, which is an instance of a specified motion
    bool MotionSystem::CheckIfIsPlayingMotion(Motion* motion, bool ignorePausedMotions) const
    {
        return motion && AZStd::any_of(begin(m_motionInstances), end(m_motionInstances), [motion, ignorePausedMotions](const MotionInstance* motionInstance)
        {
            return !(ignorePausedMotions && motionInstance->GetIsPaused()) &&
                motionInstance->GetMotion()->GetID() == motion->GetID();
        });
    }


    // return given motion instance
    MotionInstance* MotionSystem::GetMotionInstance(size_t nr) const
    {
        MCORE_ASSERT(nr < m_motionInstances.size());
        return m_motionInstances[nr];
    }


    // return number of motion instances
    size_t MotionSystem::GetNumMotionInstances() const
    {
        return m_motionInstances.size();
    }


    // set a new motion queue
    void MotionSystem::SetMotionQueue(MotionQueue* motionQueue)
    {
        if (m_motionQueue)
        {
            m_motionQueue->Destroy();
        }

        m_motionQueue = motionQueue;
    }


    // add a motion queue to the motion system
    void MotionSystem::AddMotionQueue(MotionQueue* motionQueue)
    {
        MCORE_ASSERT(motionQueue);

        // copy entries from the given queue to the motion system's one
        for (size_t i = 0; i < motionQueue->GetNumEntries(); ++i)
        {
            m_motionQueue->AddEntry(motionQueue->GetEntry(i));
        }

        // get rid of the given motion queue
        motionQueue->Destroy();
    }


    // return motion queue pointer
    MotionQueue* MotionSystem::GetMotionQueue() const
    {
        return m_motionQueue;
    }


    // return the actor to which this motion system belongs to
    ActorInstance* MotionSystem::GetActorInstance() const
    {
        return m_actorInstance;
    }


    void MotionSystem::AddMotionInstance(MotionInstance* instance)
    {
        m_motionInstances.emplace_back(instance);
    }


    bool MotionSystem::GetIsPlaying() const
    {
        return !m_motionInstances.empty();
    }
} // namespace EMotionFX
