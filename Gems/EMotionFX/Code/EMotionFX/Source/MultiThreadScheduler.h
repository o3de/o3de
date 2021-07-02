/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include "ActorUpdateScheduler.h"
#include "Actor.h"
#include <MCore/Source/MultiThreadManager.h>

namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class ActorManager;


    /**
     * The multi processor scheduler.
     * This class can manage the actor instances in such a way that multiple actor instances can be processed at the same time
     * without getting any conflicts with shared memory.
     * If however you wish to let EMotion FX only use one single CPU, or if the target system ahs only one CPU, it is recommended
     * to use the SingleThreadScheduler class instead, as that will be faster in that specific case.
     * Significant performance gains can be achieved by using this scheduler on multi-processor or multi-core systems though.
     */
    class EMFX_API MultiThreadScheduler
        : public ActorUpdateScheduler
    {
        AZ_CLASS_ALLOCATOR_DECL
    public:
        /**
         * The unique type ID of this scheduler, as returned by the GetType() method.
         */
        enum
        {
            TYPE_ID = 0x00000002
        };

        /**
         * A scheduler step.
         * This contains an array of actor instances.
         * Each actor instance will be executed in another thread.
         * The step will never contain more actor instances than the number of CPUs it is setup to use.
         */
        struct EMFX_API ScheduleStep
        {
            MCore::Array<Actor::Dependency>     mDependencies;      /**< The dependencies of this scheduler step. No actor instances with the same dependencies are allowed to be added to this step. */
            AZStd::vector<ActorInstance*>       mActorInstances;    /**< The actor instances used inside this step. Each array entry will execute in another thread. */

            /**
             * The constructor.
             */
            ScheduleStep()
            {
                mDependencies.SetMemoryCategory(EMFX_MEMCATEGORY_UPDATESCHEDULERS);
            }
        };

        /**
         * The constructor.
         */
        static MultiThreadScheduler* Create();

        /**
         * Get the name of this class, or a description.
         * @result The string containing the name of the scheduler.
         */
        const char* GetName() const override        { return "MultiThreadScheduler"; }

        /**
         * Get the unique type ID of the scheduler type.
         * All schedulers will have another ID, so that you can use this to identify what scheduler you are dealing with.
         * @result The unique ID of the scheduler type.
         */
        uint32 GetType() const override             { return TYPE_ID; }

        /**
         * The main method which will execute all callbacks, which on their turn will check for visibilty, perform updates and render.
         * @param timePassedInSeconds The time passed, in seconds, since the last call to the update.
         */
        void Execute(float timePassedInSeconds) override;

        /**
         * LOG the schedule using the LOG method.
         * This can for example show the update order, in which order the actor instances will be updated.
         */
        void Print() override;

        /**
         * Clear the schedule.
         */
        void Clear() override;

        /**
         * Remove all empty scheduler steps.
         */
        void RemoveEmptySteps();

        /**
         * Recursively insert an actor instance into the schedule, including all its attachments.
         * @param actorInstance The actor instance to insert.
         * @param startStep An offset in the schedule where to start trying to insert the actor instances.
         */
        void RecursiveInsertActorInstance(ActorInstance* actorInstance, uint32 startStep = 0) override;

        /**
         * Recursively remove an actor instance and its attachments from the schedule.
         * @param actorInstance The actor instance to remove.
         * @param startStep An offset in the schedule where to start trying to remove from.
         */
        void RecursiveRemoveActorInstance(ActorInstance* actorInstance, uint32 startStep = 0) override;

        /**
         * Remove a single actor instance from the schedule. This will not remove its attachments.
         * @param actorInstance The actor instance to remove.
         * @param startStep An offset in the schedule where to start trying to remove from.
         * @result Returns the offset in the schedule where the actor instance was removed.
         */
        uint32 RemoveActorInstance(ActorInstance* actorInstance, uint32 startStep = 0) override;

        void Lock();
        void Unlock();

        const ScheduleStep& GetScheduleStep(uint32 index) const { return mSteps[index]; }
        uint32 GetNumScheduleSteps() const { return mSteps.GetLength(); }

    protected:
        MCore::Array< ScheduleStep >    mSteps;         /**< An array of update steps, that together form the schedule. */
        float                           mCleanTimer;    /**< The time passed since the last automatic call to the Optimize method. */
        MCore::MutexRecursive           mMutex;

        bool HasActorInstanceInSteps(const ActorInstance* actorInstance) const;

        /**
         * The constructor.
         */
        MultiThreadScheduler();

        /**
         * The destructor.
         */
        virtual ~MultiThreadScheduler();

        /**
         * Check if a given update step has any similar dependencies than the specified actor instance.
         * If this is the case, the specified actor instance can't be inserted inside the same scheduler step, as
         * it would conflict with the other actor instances inside it.
         * @param instance The acotr instance to test.
         * @param step The scheduler step to test.
         * @result Returns true when there are matching dependencies, otherwise false is returned.
         */
        bool CheckIfHasMatchingDependency(ActorInstance* instance, ScheduleStep* step) const;

        /**
         * Find the next free spot in the schedule where we can insert a given actor instance.
         * @param actorInstance The actor instance we are trying to insert.
         * @param startStep The step offset in the scheduler, to start searching from.
         * @param outStepNr This will contain the step number in which we can insert the actor instance.
         * @result Returns false when there is no step where we can insert in. A new step will have to be added.
         */
        bool FindNextFreeItem(ActorInstance* actorInstance, uint32 startStep, uint32* outStepNr);

        /**
         * Add the dependencies of a given actor instance to a specified scheduler step.
         * This will also prevent any duplicated dependencies. So dependencies that are already inside the step
         * will not be added again.
         * @param instance The actor instance to add the dependencies from.
         * @param outStep The scheduler step to add the dependencies to.
         */
        void AddDependenciesToStep(ActorInstance* instance, ScheduleStep* outStep);
    };
}   // namespace EMotionFX
