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
        m_scheduler  = nullptr;

        // setup the default scheduler
        SetScheduler(MultiThreadScheduler::Create());

        // reserve memory
        m_actorInstances.reserve(1024);
        m_rootActorInstances.reserve(1024);
    }


    // destructor
    ActorManager::~ActorManager()
    {
        // delete the scheduler
        m_scheduler->Destroy();
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
            MCORE_ASSERT(m_actorInstances[0]->GetReferenceCount() == 1);
            m_actorInstances[0]->Destroy();
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
        m_actorInstances.clear();
        m_rootActorInstances.clear();
        if (m_scheduler)
        {
            m_scheduler->Clear();
        }
        UnlockActorInstances();
    }


    // set the scheduler to use
    void ActorManager::SetScheduler(ActorUpdateScheduler* scheduler, bool delExisting)
    {
        LockActorInstances();

        // delete the existing scheduler, if wanted
        if (delExisting && m_scheduler)
        {
            m_scheduler->Destroy();
        }

        // update the scheduler pointer
        m_scheduler = scheduler;

        // adjust all visibility flags to false for all actor instances
        const size_t numActorInstances = m_actorInstances.size();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            m_actorInstances[i]->SetIsVisible(false);
        }

        UnlockActorInstances();
    }


    // register the actor
    void ActorManager::RegisterActor(ActorAssetData actorAsset)
    {
        LockActors();

        // check if we already registered
        if (FindActorIndex(actorAsset.GetId()) != InvalidIndex)
        {
            MCore::LogWarning(
                "EMotionFX::ActorManager::RegisterActor() - The actor %s has already been registered as actor, most likely "
                "already by the LoadActor of the importer.",
                actorAsset->GetActor()->GetName());
            UnlockActors();
            return;
        }

        // register it
        m_actorAssets.emplace_back(AZStd::move(actorAsset));

        UnlockActors();
    }


    // register the actor instance
    void ActorManager::RegisterActorInstance(ActorInstance* actorInstance)
    {
        LockActorInstances();

        m_actorInstances.emplace_back(actorInstance);
        UpdateActorInstanceStatus(actorInstance, false);

        UnlockActorInstances();
    }


    // find the actor for a given actor name
    Actor* ActorManager::FindActorByName(const char* actorName) const
    {
        // get the number of actors and iterate through them
        const auto found = AZStd::find_if(
            m_actorAssets.begin(), m_actorAssets.end(),
            [actorName](const ActorAssetData& a)
        {
                return a->GetActor()->GetNameString() == actorName;
        });

        return (found != m_actorAssets.end()) ? (*found)->GetActor() : nullptr;
    }


    // find the actor for a given filename
    Actor* ActorManager::FindActorByFileName(const char* fileName) const
    {
        const auto found = AZStd::find_if(
            m_actorAssets.begin(), m_actorAssets.end(),
            [fileName](const ActorAssetData& a)
        {
                return AzFramework::StringFunc::Equal(
                    a->GetActor()->GetFileNameString().c_str(), fileName, false /* no case */);
        });

        return (found != m_actorAssets.end()) ? (*found)->GetActor() : nullptr;
    }


    // find the leader actor record for a given actor
    size_t ActorManager::FindActorIndex(AZ::Data::AssetId assetId) const
    {
        const auto found = AZStd::find_if(
            m_actorAssets.begin(), m_actorAssets.end(),
            [assetId](const ActorAssetData& a)
        {
                return a.GetId() == assetId;
        });

        return (found != m_actorAssets.end()) ? AZStd::distance(m_actorAssets.begin(), found) : InvalidIndex;
    }

    size_t ActorManager::FindActorIndex(const Actor* actor) const
    {
        const auto found = AZStd::find_if(
            m_actorAssets.begin(), m_actorAssets.end(),
            [actor](const ActorAssetData& a)
            {
                return a->GetActor() == actor;
            });

        return (found != m_actorAssets.end()) ? AZStd::distance(m_actorAssets.begin(), found) : InvalidIndex;
    }

    // find the actor for a given actor name
    size_t ActorManager::FindActorIndexByName(const char* actorName) const
    {
        const auto found = AZStd::find_if(
            m_actorAssets.begin(), m_actorAssets.end(),
            [actorName](const ActorAssetData& a)
        {
                return a->GetActor()->GetNameString() == actorName;
        });

        return (found != m_actorAssets.end()) ? AZStd::distance(m_actorAssets.begin(), found) : InvalidIndex;
    }


    // find the actor for a given actor filename
    size_t ActorManager::FindActorIndexByFileName(const char* filename) const
    {
        const auto found = AZStd::find_if(
            m_actorAssets.begin(), m_actorAssets.end(),
            [filename](const ActorAssetData& a)
        {
                return a->GetActor()->GetFileNameString() == filename;
        });

        return (found != m_actorAssets.end()) ? AZStd::distance(m_actorAssets.begin(), found) : InvalidIndex;
    }


    // check if the given actor instance is registered
    bool ActorManager::CheckIfIsActorInstanceRegistered(ActorInstance* actorInstance)
    {
        LockActorInstances();

        // get the number of actor instances and iterate through them
        const bool foundActor = AZStd::find(begin(m_actorInstances), end(m_actorInstances), actorInstance) != end(m_actorInstances);
        UnlockActorInstances();
        return foundActor;
    }


    // find the given actor instance inside the actor manager and return its index
    size_t ActorManager::FindActorInstanceIndex(ActorInstance* actorInstance) const
    {
        const auto foundActorInstance = AZStd::find(begin(m_actorInstances), end(m_actorInstances), actorInstance);
        return foundActorInstance != end(m_actorInstances) ? AZStd::distance(begin(m_actorInstances), foundActorInstance) : InvalidIndex;
    }


    // find the actor instance by the identification number
    ActorInstance* ActorManager::FindActorInstanceByID(uint32 id) const
    {
        const auto foundActorInstance = AZStd::find_if(begin(m_actorInstances), end(m_actorInstances), [id](const ActorInstance* actorInstance)
        {
            return actorInstance->GetID() == id;
        });
        return foundActorInstance != end(m_actorInstances) ? *foundActorInstance : nullptr;
    }


    // find the actor by the identification number
    Actor* ActorManager::FindActorByID(uint32 id) const
    {
        const auto found = AZStd::find_if(
            m_actorAssets.begin(), m_actorAssets.end(),
            [id](const ActorAssetData& a)
        {
                return a->GetActor()->GetID() == id;
        });

        return (found != m_actorAssets.end()) ? (*found)->GetActor() : nullptr;
    }

    AZ::Data::AssetId ActorManager::FindAssetIdByActorId(uint32 id) const
    {
        const auto found = AZStd::find_if(
            m_actorAssets.begin(), m_actorAssets.end(),
            [id](const ActorAssetData& a)
        {
            return a->GetActor()->GetID() == id;
        });

        return (found != m_actorAssets.end()) ? found->GetId() : AZ::Data::AssetId();
    }


    // unregister a given actor instance
    void ActorManager::UnregisterActorInstance(size_t nr)
    {
        UnregisterActorInstance(m_actorInstances[nr]);
    }


    // unregister an actor
    void ActorManager::UnregisterActor(AZ::Data::AssetId actorAssetID)
    {
        LockActors();
        const auto found = AZStd::find_if(
        m_actorAssets.begin(), m_actorAssets.end(),
            [actorAssetID](const ActorAssetData& a)
            {
                return a.GetId() == actorAssetID;
            });
        if (found != m_actorAssets.end())
        {
            m_actorAssets.erase(found);
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
        m_scheduler->Execute(timePassedInSeconds);

        UnlockActorInstances();
        UnlockActors();
    }


    // unregister all the actors
    void ActorManager::UnregisterAllActors()
    {
        LockActors();

        // clear all actors
        m_actorAssets.clear();

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
            if (AZStd::find(begin(m_rootActorInstances), end(m_rootActorInstances), actorInstance) == end(m_rootActorInstances))
            {
                m_rootActorInstances.emplace_back(actorInstance);
            }
        }
        else // no root actor instance
        {
            // remove it from the root list
            if (const auto it = AZStd::find(begin(m_rootActorInstances), end(m_rootActorInstances), actorInstance); it != end(m_rootActorInstances))
            {
                m_rootActorInstances.erase(it);
            }
            m_scheduler->RecursiveRemoveActorInstance(actorInstance);
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
        if (const auto it = AZStd::find(begin(m_actorInstances), end(m_actorInstances), instance); it != end(m_actorInstances))
        {
            m_actorInstances.erase(it);
        }

        // remove it from the list of roots, if it is in there
        if (const auto it = AZStd::find(begin(m_rootActorInstances), end(m_rootActorInstances), instance); it != end(m_rootActorInstances))
        {
            m_rootActorInstances.erase(it);
        }

        // remove it from the schedule
        m_scheduler->RemoveActorInstance(instance);

        UnlockActorInstances();
    }


    void ActorManager::LockActorInstances()
    {
        m_actorInstanceLock.Lock();
    }


    void ActorManager::UnlockActorInstances()
    {
        m_actorInstanceLock.Unlock();
    }


    void ActorManager::LockActors()
    {
        m_actorLock.Lock();
    }


    void ActorManager::UnlockActors()
    {
        m_actorLock.Unlock();
    }


    Actor* ActorManager::GetActor(size_t nr) const
    {
        return m_actorAssets[nr]->GetActor();
    }


    ActorAssetData ActorManager::GetActorAsset(size_t nr) const
    {
        return m_actorAssets[nr];
    }


    const AZStd::vector<ActorInstance*>& ActorManager::GetActorInstanceArray() const
    {
        return m_actorInstances;
    }


    ActorUpdateScheduler* ActorManager::GetScheduler() const
    {
        return m_scheduler;
    }
}   // namespace EMotionFX
