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

// include the required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class ActorManager;


    /**
     * The actor update scheduler base class.
     * This class is responsible for updating the transformations of all actor instances, in the right order.
     */
    class EMFX_API ActorUpdateScheduler
        : public BaseObject
    {
    public:
        /**
         * Get the name of this class, or a description.
         * @result The string containing the name of the scheduler.
         */
        virtual const char* GetName() const = 0;

        /**
         * Get the unique type ID of the scheduler type.
         * All schedulers will have another ID, so that you can use this to identify what scheduler you are dealing with.
         * @result The unique ID of the scheduler type.
         */
        virtual uint32 GetType() const = 0;

        /**
         * The main method that will trigger all updates of the actor instances.
         * @param timePassedInSeconds The time passed, in seconds, since the last call to the update.
         */
        virtual void Execute(float timePassedInSeconds) = 0;

        /**
         * Clear the schedule.
         */
        virtual void Clear() = 0;

        /**
         * LOG the schedule using the LOG method.
         * This can for example show the update order, in which order the actor instances will be updated.
         */
        virtual void Print() {}

        /**
         * Recursively insert an actor instance into the schedule, including all its attachments.
         * @param actorInstance The actor instance to insert.
         * @param startStep An offset in the schedule where to start trying to insert the actor instances.
         */
        virtual void RecursiveInsertActorInstance(ActorInstance* actorInstance, uint32 startStep = 0) = 0;

        /**
         * Recursively remove an actor instance and its attachments from the schedule.
         * @param actorInstance The actor instance to remove.
         * @param startStep An offset in the schedule where to start trying to remove from.
         */
        virtual void RecursiveRemoveActorInstance(ActorInstance* actorInstance, uint32 startStep = 0) = 0;

        /**
         * Remove a single actor instance from the schedule. This will not remove its attachments.
         * @param actorInstance The actor instance to remove.
         * @param startStep An offset in the schedule where to start trying to remove from.
         * @result Returns the offset in the schedule where the actor instance was removed.
         */
        virtual uint32 RemoveActorInstance(ActorInstance* actorInstance, uint32 startStep = 0) = 0;

        uint32 GetNumUpdatedActorInstances() const                  { return mNumUpdated.GetValue(); }
        uint32 GetNumVisibleActorInstances() const                  { return mNumVisible.GetValue(); }
        uint32 GetNumSampledActorInstances() const                  { return mNumSampled.GetValue(); }

    protected:
        MCore::AtomicUInt32 mNumUpdated;
        MCore::AtomicUInt32 mNumVisible;
        MCore::AtomicUInt32 mNumSampled;

        /**
         * The constructor.
         */
        ActorUpdateScheduler()
            : BaseObject()   {}

        /**
         * The destructor.
         */
        virtual ~ActorUpdateScheduler() {}
    };
}   // namespace EMotionFX
