/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <AzCore/std/containers/vector.h>
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include "MemoryCategories.h"
#include <MCore/Source/MultiThreadManager.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <Source/Integration/Assets/ActorAsset.h>
#include <Source/Integration/System/SystemCommon.h>

namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class Actor;
    class ActorUpdateScheduler;

    //-----------------------------------------------------------------------------

    /**
     * The actor manager.
     * This class maintains a list of registered actors and actor instances that have been created.
     * Also it stores a list of root actor instances, which are roots in the chains of attachments.
     * For example if you attach a cowboy to a horse, the horse is the root actor instance.
     */
    class EMFX_API ActorManager
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class Initializer;
        friend class EMotionFXManager;

    public:
        static ActorManager* Create();

        /**
         * Register an actor.
         * @param actor The actor to register.
         */
        void RegisterActor(ActorAssetData actorAsset);

        /**
         * Unregister all actors.
         * This does not release/delete the actual actor objects, but just clears the internal array of actor instances.
         * This method is automatically called at shutdown of your application.
         */
        void UnregisterAllActors();

        /**
         * Unregister a specific actor.
         * @param actor The actor you passed to the RegisterActor function sometime before.
         */
        void UnregisterActor(AZ::Data::AssetId actorAssetID);

        /**
         * Get the number of registered actors.
         * This does not include the clones that have been optionally created.
         * @result The number of registered actors.
         */
        MCORE_INLINE size_t GetNumActors() const { return m_actorAssets.size(); }

        /**
         * Get a given actor.
         * This will return a Actor object that contains an array of Actor objects.
         * The first Actor in this array will be the actor you passed to RegisterActor.
         * The following Actor objects in the array will be the created clones, if there are any.
         * @param nr The actor number, which must be in range of [0..GetNumActors()-1].
         * @result A reference to the actor object that contains the array of Actor objects.
         */
        Actor* GetActor(size_t nr) const;
        ActorAssetData GetActorAsset(size_t nr) const;

        /**
         * Find the given actor by name.
         * @param[in] actorName The name of the actor.
         * @return A pointer to the actor with the given name. nullptr in case the actor has not been found.
         */
        Actor* FindActorByName(const char* actorName) const;

        /**
         * Find the given actor by filename.
         * @param[in] fileName The filename of the actor.
         * @return A pointer to the actor with the given filename. nullptr in case the actor has not been found.
         */
        Actor* FindActorByFileName(const char* fileName) const;

        /**
         * Find the actor number for a given Actor object.
         * This will find the actor number for the Actor object that you passed to RegisterActor before.
         * @param actor The actor object you once passed to RegisterActor.
         * @result Returns the actor number, which is in range of [0..GetNumActors()-1], or returns MCORE_INVALIDINDEX32 when not found.
         */
        size_t FindActorIndex(AZ::Data::AssetId assetId) const;
        size_t FindActorIndex(const Actor* actor) const;

        /**
         * Find the actor number for a given actor name.
         * This will find the actor number for the Actor object that you passed to RegisterActor before.
         * @param actorName The name of the actor.
         * @result Returns the actor number, which is in range of [0..GetNumActors()-1], or returns MCORE_INVALIDINDEX32 when not found.
         */
        size_t FindActorIndexByName(const char* actorName) const;

        /**
         * Find the actor number for a given actor filename.
         * This will find the actor number for the Actor object that you passed to RegisterActor before.
         * @param filename The filename of the actor.
         * @result Returns the actor number, which is in range of [0..GetNumActors()-1], or returns MCORE_INVALIDINDEX32 when not found.
         */
        size_t FindActorIndexByFileName(const char* filename) const;

        // register the actor instance
        void RegisterActorInstance(ActorInstance* actorInstance);

        /**
         * Get the number of actor instances that currently are registered.
         * @result The number of registered actor instances.
         */
        MCORE_INLINE size_t GetNumActorInstances() const                                { return m_actorInstances.size(); }

        /**
         * Get a given registered actor instance.
         * @param nr The actor instance number, which must be in range of [0..GetNumActorInstances()-1].
         * @result A pointer to the actor instance.
         */
        MCORE_INLINE ActorInstance* GetActorInstance(size_t nr) const                   { return m_actorInstances[nr]; }

        /**
         * Get a given registered actor instance owned by editor (not owned by runtime).
         * @result A pointer to the actor instance.
         */
        ActorInstance* GetFirstEditorActorInstance() const;

        /**
         * Get the array of actor instances.
         * @result The const reference to the actor instance array.
         */
        const AZStd::vector<ActorInstance*>& GetActorInstanceArray() const;

        /**
         * Find the given actor instance inside the actor manager and return its index.
         * @param actorInstance A pointer to the actor instance to be searched.
         * @result The actor instance index for the actor manager, MCORE_INVALIDINDEX32 in case the actor instance hasn't been found.
         */
        size_t FindActorInstanceIndex(ActorInstance* actorInstance) const;

        /**
         * Find an actor instance inside the actor manager by its id.
         * @param[in] id The unique identification number of the actor instance to be searched.
         * @result A pointer to the actor instance, nullptr in case the actor instance hasn't been found.
         */
        ActorInstance* FindActorInstanceByID(uint32 id) const;

        /**
         * Find an actor inside the actor manager by its id.
         * @param[in] id The unique identification number of the actor to be searched.
         * @result A pointer to the actor, nullptr in case the actor hasn't been found.
         */
        Actor* FindActorByID(uint32 id) const;

        AZ::Data::AssetId FindAssetIdByActorId(uint32 id) const;

        /**
         * Check if the given actor instance is registered.
         * @param actorInstance A pointer to the actor instance to be checked.
         * @result True if the actor instance has been found in the registered actor instances, false if not.
         */
        bool CheckIfIsActorInstanceRegistered(ActorInstance* actorInstance);

        /**
         * Unregister all actor instances.
         * Please note that this is already done automatically at application shutdown.
         * The actor instance objects will not be deleted from memory though.
         * When you delete an actor instance, it automatically will unregister itself from the manager.
         */
        void UnregisterAllActorInstances();

        /**
         * Unregister a specific actor instance.
         * This does not delete the actual actor instance from memory.
         * When you delete an actor instance, it automatically will unregister itself from the manager.
         * So you will most likely not be calling this method, unless you have a special reason for this.
         * @param instance The actor instance to unregister.
         */
        void UnregisterActorInstance(ActorInstance* instance);

        /**
         * Unregister a given actor instance.
         * This does not delete the actual actor instance from memory.
         * When you delete an actor instance, it automatically will unregister itself from the manager.
         * @param nr The actor instance number, which has to be in range of [0..GetNumActorInstances()-1].
         */
        void UnregisterActorInstance(size_t nr);

        /**
         * Get the number of root actor instances.
         * A root actor instance is an actor instance that is no attachment.
         * So if you have a gun attached to a cowboy, where the cowboy is attached to a horse, then the
         * horse is the root attachment instance.
         * @result Returns the number of root actor instances.
         */
        MCORE_INLINE size_t GetNumRootActorInstances() const                    { return m_rootActorInstances.size(); }

        /**
         * Get a given root actor instance.
         * A root actor instance is an actor instance that is no attachment.
         * So if you have a gun attached to a cowboy, where the cowboy is attached to a horse, then the
         * horse is the root attachment instance.
         * @param nr The root actor instance number, which must be in range of [0..GetNumRootActorInstances()-1].
         * @result A pointer to the actor instance that is a root.
         */
        MCORE_INLINE ActorInstance* GetRootActorInstance(size_t nr) const       { return m_rootActorInstances[nr]; }

        /**
         * Get the currently used actor update scheduler.
         * @result A pointer to the actor update scheduler that is currently used.
         */
        ActorUpdateScheduler* GetScheduler() const;

        /**
         * Set the scheduler to use.
         * EMotion FX provides two different scheduler implementations:
         * A single threaded scheduler (SingleThreadScheduler) and a multithreaded scheduler (MultiThreadScheduler, the default).
         * The current scheduler will automatically be deleted at application shutdown.
         * The schedulers are responsible for figuring out the update order.
         * @param scheduler The new scheduler to use.
         * @param delExisting When set to true, the existing scheduler, as returned by GetScheduler() will be deleted from memory.
         */
        void SetScheduler(ActorUpdateScheduler* scheduler, bool delExisting = true);

        /**
         * Update the actor instance status for a given actor instance.
         * This checks if the actor instance is still a root actor instance or not and it makes sure that it is
         * registered internally as root actor instance when needed.
         * So this method is being called when attachments change. This is handled automatically.
         * @param actorInstance The actor instance to update the status for.
         * @param lock Set to true if you want to automatically call LockActorInstances().
         */
        void UpdateActorInstanceStatus(ActorInstance* actorInstance, bool lock = true);

        /**
         * The main method that will execute the scheduler which will on its turn updates all the actor instances.
         * @param timePassedInSeconds The time, in seconds, since the last time you called the Update method.
         */
        void UpdateActorInstances(float timePassedInSeconds);

        void DestroyAllActorInstances();
        void DestroyAllActors();

        void LockActorInstances();
        void UnlockActorInstances();
        void LockActors();
        void UnlockActors();

    private:
        AZStd::vector<ActorInstance*>   m_actorInstances;        /**< The registered actor instances. */
        AZStd::vector<ActorAssetData>   m_actorAssets;
        AZStd::vector<ActorInstance*>   m_rootActorInstances;    /**< Root actor instances (roots of all attachment chains). */
        ActorUpdateScheduler*           m_scheduler;             /**< The update scheduler to use. */
        MCore::MutexRecursive           m_actorLock;             /**< The multithread lock for touching the actors array. */
        MCore::MutexRecursive           m_actorInstanceLock;     /**< The multithread lock for touching the actor instances array. */

        /**
         * The constructor, which initializes using the multi processor scheduler.
         */
        ActorManager();

        /**
         * The destructor. Automatically deletes the callback and scheduler.
         */
        ~ActorManager() override;
    };
}   // namespace EMotionFX
