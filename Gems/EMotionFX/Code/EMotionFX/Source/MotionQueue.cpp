/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    AZ_CLASS_ALLOCATOR_IMPL(MotionQueue, MotionAllocator)


    // constructor
    MotionQueue::MotionQueue(ActorInstance* actorInstance, MotionSystem* motionSystem)
        : MCore::RefCounted()
    {
        MCORE_ASSERT(actorInstance && motionSystem);

        m_actorInstance  = actorInstance;
        m_motionSystem   = motionSystem;
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
    void MotionQueue::RemoveEntry(size_t nr)
    {
        if (m_motionSystem->RemoveMotionInstance(m_entries[nr].m_motion) == false)
        {
            GetMotionInstancePool().Free(m_entries[nr].m_motion);
        }

        m_entries.erase(AZStd::next(begin(m_entries), nr));
    }


    // update the motion queue
    void MotionQueue::Update()
    {
        // get the number of entries
        size_t numEntries = GetNumEntries();

        // if there are entries in the queue
        if (numEntries == 0)
        {
            return;
        }

        // if there is only one entry in the queue, we can start playing it immediately
        if (m_motionSystem->GetIsPlaying() == false)
        {
            // get the entry from the queue to play next
            MotionQueue::QueueEntry queueEntry = GetFirstEntry();

            // remove it from the queue
            RemoveFirstEntry();

            // start the motion on the queue
            m_motionSystem->StartMotion(queueEntry.m_motion, &queueEntry.m_playInfo);

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
        m_motionSystem->StartMotion(queueEntry.m_motion, &queueEntry.m_playInfo);
    }


    // find out if we should start playing the next motion in the queue or not
    bool MotionQueue::ShouldPlayNextMotion()
    {
        // find the first non mixing motion
        MotionInstance* motionInst = m_motionSystem->FindFirstNonMixingMotionInstance();

        // if there isn't a non mixing motion
        if (motionInst == nullptr)
        {
            return false;
        }

        // the total amount of blending time
        const float timeToRemoveFromMaxTime = GetFirstEntry().m_playInfo.m_blendInTime + motionInst->GetFadeTime();

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
        while (m_entries.size())
        {
            RemoveEntry(0);
        }
    }


    void MotionQueue::AddEntry(const MotionQueue::QueueEntry& motion)
    {
        m_entries.emplace_back(motion);
    }


    size_t MotionQueue::GetNumEntries() const
    {
        return m_entries.size();
    }


    MotionQueue::QueueEntry& MotionQueue::GetFirstEntry()
    {
        MCORE_ASSERT(m_entries.size() > 0);
        return m_entries[0];
    }


    void MotionQueue::RemoveFirstEntry()
    {
        m_entries.erase(m_entries.begin());
    }


    MotionQueue::QueueEntry& MotionQueue::GetEntry(size_t nr)
    {
        return m_entries[nr];
    }
} // namespace EMotionFX
