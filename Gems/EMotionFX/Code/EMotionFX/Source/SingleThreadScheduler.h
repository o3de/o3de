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

#pragma once

// include the core system
#include "EMotionFXConfig.h"
#include "ActorUpdateScheduler.h"


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class ActorManager;


    /**
     * The single processor scheduler.
     * This scheduler is optimized for systems with just one CPU, or if you want EMotion FX to only use one CPU.
     * Using this class will be faster than using the MultiThreadScheduler that has been setup to use only one CPU.
     * The reason for this is that this scheduler will not have the associated multithread management overhead.
     */
    class EMFX_API SingleThreadScheduler
        : public ActorUpdateScheduler
    {
        AZ_CLASS_ALLOCATOR_DECL
    public:
        /**
         * The unique type ID of this scheduler, as returned by the GetType() method.
         */
        enum
        {
            TYPE_ID = 0x00000001
        };

        /**
         * The creation method.
         */
        static SingleThreadScheduler* Create();

        /**
         * Get the name of this class, or a description.
         * @result The string containing the name of the scheduler.
         */
        const char* GetName() const override            { return "SingleThreadScheduler"; }

        /**
         * Get the unique type ID of the scheduler type.
         * All schedulers will have another ID, so that you can use this to identify what scheduler you are dealing with.
         * @result The unique ID of the scheduler type.
         */
        uint32 GetType() const override                 { return TYPE_ID; }

        /**
         * Clear the schedule.
         */
        void Clear() override {}

        /**
         * The main method which will execute all callbacks, which on their turn will check for visibilty, perform updates and render.
         * @param timePassedInSeconds The time passed, in seconds, since the last call to the update.
         */
        void Execute(float timePassedInSeconds) override;

        /**
         * Recursively insert an actor instance into the schedule, including all its attachments.
         * @param actorInstance The actor instance to insert.
         * @param startStep An offset in the schedule where to start trying to insert the actor instances.
         */
        void RecursiveInsertActorInstance(ActorInstance* actorInstance, uint32 startStep = 0) override    { MCORE_UNUSED(actorInstance); MCORE_UNUSED(startStep); }

        /**
         * Recursively remove an actor instance and its attachments from the schedule.
         * @param actorInstance The actor instance to remove.
         * @param startStep An offset in the schedule where to start trying to remove from.
         */
        void RecursiveRemoveActorInstance(ActorInstance* actorInstance, uint32 startStep = 0) override    { MCORE_UNUSED(actorInstance); MCORE_UNUSED(startStep); }

        /**
         * Remove a single actor instance from the schedule. This will not remove its attachments.
         * @param actorInstance The actor instance to remove.
         * @param startStep An offset in the schedule where to start trying to remove from.
         * @result Returns the offset in the schedule where the actor instance was removed.
         */
        uint32 RemoveActorInstance(ActorInstance* actorInstance, uint32 startStep = 0) override           { MCORE_UNUSED(actorInstance); MCORE_UNUSED(startStep); return 0; }

    protected:
        /**
         * Recursively execute an actor instance and its attachments.
         * @param actorInstance The actor instance to execute the callbacks for.
         * @param timePassedInSeconds The time passed, in seconds, since the last update.
         */
        void RecursiveExecuteActorInstance(ActorInstance* actorInstance, float timePassedInSeconds);

        /**
         * The constructor.
         *
         */
        SingleThreadScheduler();

        /**
         * The destructor.
         */
        virtual ~SingleThreadScheduler();
    };
}   // namespace EMotionFX
