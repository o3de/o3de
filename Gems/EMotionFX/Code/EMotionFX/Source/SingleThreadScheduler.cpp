/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "SingleThreadScheduler.h"
#include "ActorManager.h"
#include "ActorInstance.h"
#include "Attachment.h"
#include "EMotionFXManager.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SingleThreadScheduler, ActorUpdateAllocator)

    // constructor
    SingleThreadScheduler::SingleThreadScheduler()
        : ActorUpdateScheduler()
    {
    }


    // destructor
    SingleThreadScheduler::~SingleThreadScheduler()
    {
    }


    // create
    SingleThreadScheduler* SingleThreadScheduler::Create()
    {
        return aznew SingleThreadScheduler();
    }


    // execute the schedule
    void SingleThreadScheduler::Execute(float timePassedInSeconds)
    {
        const ActorManager& actorManager = GetActorManager();

        // reset stats
        m_numUpdated.SetValue(0);
        m_numVisible.SetValue(0);
        m_numSampled.SetValue(0);

        // propagate root actor instance visibility to their attachments
        const size_t numRootActorInstances = GetActorManager().GetNumRootActorInstances();
        for (size_t i = 0; i < numRootActorInstances; ++i)
        {
            ActorInstance* rootInstance = actorManager.GetRootActorInstance(i);
            if (rootInstance->GetIsEnabled() == false)
            {
                continue;
            }

            rootInstance->RecursiveSetIsVisible(rootInstance->GetIsVisible());
        }

        // process all root actor instances, and execute them and their attachments
        for (size_t i = 0; i < numRootActorInstances; ++i)
        {
            ActorInstance* rootActorInstance = actorManager.GetRootActorInstance(i);
            if (rootActorInstance->GetIsEnabled() == false)
            {
                continue;
            }

            RecursiveExecuteActorInstance(rootActorInstance, timePassedInSeconds);
        }
    }


    // execute the actor instance, and its attachments
    void SingleThreadScheduler::RecursiveExecuteActorInstance(ActorInstance* actorInstance, float timePassedInSeconds)
    {
        actorInstance->SetThreadIndex(0);

        m_numUpdated.Increment();

        const bool isVisible = actorInstance->GetIsVisible();

        // check if we want to sample motions
        bool sampleMotions = false;
        actorInstance->SetMotionSamplingTimer(actorInstance->GetMotionSamplingTimer() + timePassedInSeconds);
        if (actorInstance->GetMotionSamplingTimer() >= actorInstance->GetMotionSamplingRate())
        {
            sampleMotions = true;
            actorInstance->SetMotionSamplingTimer(0.0f);

            if (isVisible)
            {
                m_numSampled.Increment();
            }
        }

        if (isVisible)
        {
            m_numVisible.Increment();
        }

        // update the transformations
        actorInstance->UpdateTransformations(timePassedInSeconds, isVisible, sampleMotions);

        // recursively process the attachments
        const size_t numAttachments = actorInstance->GetNumAttachments();
        for (size_t i = 0; i < numAttachments; ++i)
        {
            ActorInstance* attachment = actorInstance->GetAttachment(i)->GetAttachmentActorInstance();
            if (attachment && attachment->GetIsEnabled())
            {
                RecursiveExecuteActorInstance(attachment, timePassedInSeconds);
            }
        }
    }
}   // namespace EMotionFX
