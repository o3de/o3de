/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "EMotionFXConfig.h"
#include "ActorManager.h"
#include "ActorInstance.h"
#include "MultiThreadScheduler.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/EMotionFXManager.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorManager, ActorManagerAllocator, 0)

    // constructor
    ActorManager::ActorManager()
        : BaseObject()
    {
        mScheduler  = nullptr;

        // setup the default scheduler
        SetScheduler(MultiThreadScheduler::Create());

        // reserve memory
        m_actors.reserve(512);
        mActorInstances.reserve(1024);
        mRootActorInstances.reserve(1024);
    }


    // destructor
    ActorManager::~ActorManager()
    {
        // delete the scheduler
        mScheduler->Destroy();
    }


    // create
    ActorManager* ActorManager::Create()
    {
        return aznew ActorManager();
    }


    void ActorManager::DestroyAllActorInstances()
    {
        // destroy all actor instances
        while (GetNumActorInstances() > 0)
        {
            MCORE_ASSERT(mActorInstances[0]->GetReferenceCount() == 1);
            mActorInstances[0]->Destroy();
        }

        UnregisterAllActorInstances();
    }


    void ActorManager::DestroyAllActors()
    {
        UnregisterAllActors();
    }


    // unregister all the previously registered actor instances
    void ActorManager::UnregisterAllActorInstances()
    {
        LockActorInstances();
        mActorInstances.clear();
        mRootActorInstances.clear();
        if (mScheduler)
        {
            mScheduler->Clear();
        }
        UnlockActorInstances();
    }


    // set the scheduler to use
    void ActorManager::SetScheduler(ActorUpdateScheduler* scheduler, bool delExisting)
    {
        LockActorInstances();

        // delete the existing scheduler, if wanted
        if (delExisting && mScheduler)
        {
            mScheduler->Destroy();
        }

        // update the scheduler pointer
        mScheduler = scheduler;

        // adjust all visibility flags to false for all actor instances
        const size_t numActorInstances = mActorInstances.size();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            mActorInstances[i]->SetIsVisible(false);
        }

        UnlockActorInstances();
    }


    // register the actor
    void ActorManager::RegisterActor(AZStd::shared_ptr<Actor> actor)
    {
        LockActors();

        // check if we already registered
        if (FindActorIndex(actor.get()) != InvalidIndex)
        {
            MCore::LogWarning("EMotionFX::ActorManager::RegisterActor() - The actor at location 0x%x has already been registered as actor, most likely already by the LoadActor of the importer.", actor.get());
            UnlockActors();
            return;
        }

        // register it
        m_actors.emplace_back(AZStd::move(actor));

        UnlockActors();
    }


    // register the actor instance
    void ActorManager::RegisterActorInstance(ActorInstance* actorInstance)
    {
        LockActorInstances();

        mActorInstances.emplace_back(actorInstance);
        UpdateActorInstanceStatus(actorInstance, false);

        UnlockActorInstances();
    }


    // find the actor for a given actor name
    Actor* ActorManager::FindActorByName(const char* actorName) const
    {
        // get the number of actors and iterate through them
        const auto found = AZStd::find_if(m_actors.begin(), m_actors.end(), [actorName](const AZStd::shared_ptr<Actor>& a)
        {
            return a->GetNameString() == actorName;
        });

        return (found != m_actors.end()) ? found->get() : nullptr;
    }


    // find the actor for a given filename
    Actor* ActorManager::FindActorByFileName(const char* fileName) const
    {
        const auto found = AZStd::find_if(m_actors.begin(), m_actors.end(), [fileName](const AZStd::shared_ptr<Actor>& a)
        {
            return AzFramework::StringFunc::Equal(a->GetFileNameString().c_str(), fileName, false /* no case */);
        });

        return (found != m_actors.end()) ? found->get() : nullptr;
    }


    // find the leader actor record for a given actor
    size_t ActorManager::FindActorIndex(Actor* actor) const
    {
        const auto found = AZStd::find_if(m_actors.begin(), m_actors.end(), [actor](const AZStd::shared_ptr<Actor>& a)
        {
            return a.get() == actor;
        });

        return (found != m_actors.end()) ? AZStd::distance(m_actors.begin(), found) : InvalidIndex;
    }


    // find the actor for a given actor name
    size_t ActorManager::FindActorIndexByName(const char* actorName) const
    {
        const auto found = AZStd::find_if(m_actors.begin(), m_actors.end(), [actorName](const AZStd::shared_ptr<Actor>& a)
        {
            return a->GetNameString() == actorName;
        });

        return (found != m_actors.end()) ? AZStd::distance(m_actors.begin(), found) : InvalidIndex;
    }


    // find the actor for a given actor filename
    size_t ActorManager::FindActorIndexByFileName(const char* filename) const
    {
        const auto found = AZStd::find_if(m_actors.begin(), m_actors.end(), [filename](const AZStd::shared_ptr<Actor>& a)
        {
            return a->GetFileNameString() == filename;
        });

        return (found != m_actors.end()) ? AZStd::distance(m_actors.begin(), found) : InvalidIndex;
    }


    // check if the given actor instance is registered
    bool ActorManager::CheckIfIsActorInstanceRegistered(ActorInstance* actorInstance)
    {
        LockActorInstances();

        // get the number of actor instances and iterate through them
        const bool foundActor = AZStd::find(begin(mActorInstances), end(mActorInstances), actorInstance) != end(mActorInstances);
        UnlockActorInstances();
        return foundActor;
    }


    // find the given actor instance inside the actor manager and return its index
    size_t ActorManager::FindActorInstanceIndex(ActorInstance* actorInstance) const
    {
        const auto foundActorInstance = AZStd::find(begin(mActorInstances), end(mActorInstances), actorInstance);
        return foundActorInstance != end(mActorInstances) ? AZStd::distance(begin(mActorInstances), foundActorInstance) : InvalidIndex;
    }


    // find the actor instance by the identification number
    ActorInstance* ActorManager::FindActorInstanceByID(uint32 id) const
    {
        const auto foundActorInstance = AZStd::find_if(begin(mActorInstances), end(mActorInstances), [id](const ActorInstance* actorInstance)
        {
            return actorInstance->GetID() == id;
        });
        return foundActorInstance != end(mActorInstances) ? *foundActorInstance : nullptr;
    }


    // find the actor by the identification number
    Actor* ActorManager::FindActorByID(uint32 id) const
    {
        const auto found = AZStd::find_if(m_actors.begin(), m_actors.end(), [id](const AZStd::shared_ptr<Actor>& a)
        {
            return a->GetID() == id;
        });

        return (found != m_actors.end()) ? found->get() : nullptr;
    }

    AZStd::shared_ptr<Actor> ActorManager::FindSharedActorByID(uint32 id) const
    {
        const auto found = AZStd::find_if(m_actors.begin(), m_actors.end(), [id](const AZStd::shared_ptr<Actor>& a)
        {
            return a->GetID() == id;
        });

        return (found != m_actors.end()) ? *found : nullptr;
    }


    // unregister a given actor instance
    void ActorManager::UnregisterActorInstance(size_t nr)
    {
        UnregisterActorInstance(mActorInstances[nr]);
    }


    // unregister an actor
    void ActorManager::UnregisterActor(const AZStd::shared_ptr<Actor>& actor)
    {
        LockActors();
        auto result = AZStd::find(m_actors.begin(), m_actors.end(), actor);
        if (result != m_actors.end())
        {
            m_actors.erase(result);
        }
        UnlockActors();
    }


    // update all actor instances etc
    void ActorManager::UpdateActorInstances(float timePassedInSeconds)
    {
        LockActors();
        LockActorInstances();

        // execute the schedule
        // this makes all the callback OnUpdate calls etc
        mScheduler->Execute(timePassedInSeconds);

        UnlockActorInstances();
        UnlockActors();
    }


    // unregister all the actors
    void ActorManager::UnregisterAllActors()
    {
        LockActors();

        // clear all actors
        m_actors.clear();

        // TODO: what if there are still references to the actors inside the list of registered actor instances?
        UnlockActors();
    }



    // update the actor instance status, which basically checks if the actor instance is still a root actor instance or not
    // with root actor instance we mean that it isn't attached to something
    void ActorManager::UpdateActorInstanceStatus(ActorInstance* actorInstance, bool lock)
    {
        if (lock)
        {
            LockActorInstances();
        }

        // if it is a root actor instance
        if (actorInstance->GetAttachedTo() == nullptr)
        {
            // make sure it's in the root list
            if (AZStd::find(begin(mRootActorInstances), end(mRootActorInstances), actorInstance) == end(mRootActorInstances))
            {
                mRootActorInstances.emplace_back(actorInstance);
            }
        }
        else // no root actor instance
        {
            // remove it from the root list
            if (const auto it = AZStd::find(begin(mRootActorInstances), end(mRootActorInstances), actorInstance); it != end(mRootActorInstances))
            {
                mRootActorInstances.erase(it);
            }
            mScheduler->RecursiveRemoveActorInstance(actorInstance);
        }

        if (lock)
        {
            UnlockActorInstances();
        }
    }


    // unregister an actor instance
    void ActorManager::UnregisterActorInstance(ActorInstance* instance)
    {
        LockActorInstances();

        // remove the actor instance from the list
        if (const auto it = AZStd::find(begin(mActorInstances), end(mActorInstances), instance); it != end(mActorInstances))
        {
            mActorInstances.erase(it);
        }

        // remove it from the list of roots, if it is in there
        if (const auto it = AZStd::find(begin(mRootActorInstances), end(mRootActorInstances), instance); it != end(mRootActorInstances))
        {
            mRootActorInstances.erase(it);
        }

        // remove it from the schedule
        mScheduler->RemoveActorInstance(instance);

        UnlockActorInstances();
    }


    void ActorManager::LockActorInstances()
    {
        mActorInstanceLock.Lock();
    }


    void ActorManager::UnlockActorInstances()
    {
        mActorInstanceLock.Unlock();
    }


    void ActorManager::LockActors()
    {
        mActorLock.Lock();
    }


    void ActorManager::UnlockActors()
    {
        mActorLock.Unlock();
    }


    Actor* ActorManager::GetActor(size_t nr) const
    {
        return m_actors[nr].get();
    }


    const AZStd::vector<ActorInstance*>& ActorManager::GetActorInstanceArray() const
    {
        return mActorInstances;
    }


    ActorUpdateScheduler* ActorManager::GetScheduler() const
    {
        return mScheduler;
    }
}   // namespace EMotionFX
