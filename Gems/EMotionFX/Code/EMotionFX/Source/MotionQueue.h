/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include "PlayBackInfo.h"
#include <AzCore/std/containers/vector.h>


namespace EMotionFX
{
    // forward delcaration
    class ActorInstance;
    class MotionInstance;
    class MotionSystem;


    /**
     * The motion queue is a motion stack which stores and plays the motion, one after
     * each other, if the previous one ended. This can be used for prescriped animations:
     * Imagine you want to let your character walk, and after that play some specific run animation.
     * The motion queue schedules the motions, so that the run animation will be played when the walk
     * animation has been finished. A smooth transition between both motions can be made when desired.
     */
    class EMFX_API MotionQueue
        : public MCore::RefCounted

    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * The queue-entry struct.
         * This contains information about a motion which is scheduled.
         */
        class QueueEntry
        {
        public:
            MotionInstance* m_motion;            /**< The motion instance we want to play. */
            PlayBackInfo    m_playInfo;          /**< The motion playback settings. */

            /// The default constructor
            QueueEntry()
                : m_motion(nullptr) {}

            /// The extended constructor.
            QueueEntry(MotionInstance* motion, class PlayBackInfo* info)
                : m_motion(motion)
                , m_playInfo(*info) {}
        };

        /**
         * Creation method.
         * @param actorInstance The actor to which this queue belongs to.
         * @param motionSystem A pointer to the motion system of the actor.
         */
        static MotionQueue* Create(ActorInstance* actorInstance, MotionSystem* motionSystem);

        /**
         * Remove all entries from the queue.
         * Does not delete them from the actor or from memory though.
         */
        void ClearAllEntries();

        /**
         * Add an entry to the queue.
         * @param motion The queue entry to add to the back of the queue.
         */
        void AddEntry(const QueueEntry& motion);

        /**
         * Get the number of entries currently in the queue.
         * @result The number of entries currently scheduled in the queue.
         */
        size_t GetNumEntries() const;

        /**
         * Get the first entry.
         * BEWARE: only call when GetNumEntries() returns a value above zero.
         * @result A reference to the first entry, with write access.
         */
        QueueEntry& GetFirstEntry();

        /**
         * Remove the first entry from the queueu.
         * BEWARE: only call when GetNumEntries() returns a value above zero.
         */
        void RemoveFirstEntry();

        /**
         * Get a specific queue entry.
         * @param nr The queue entry number to get.
         * @result A reference to the queue entry, with write access.
         */
        QueueEntry& GetEntry(size_t nr);

        /**
         * Remove a given entry from the queue.
         * @param nr The entry number to remove from the queue.
         */
        void RemoveEntry(size_t nr);

        /**
         * Updates the motion queue.
         * This will automatically start playing the motions on the queue when they have to be played.
         */
        void Update();

        /**
         * Find out if we should start playing the next motion or not.
         * It does this by finding the first non-mixing motion that is being played.
         * Next it checks if the motion has ended or is stopping. If so, true is returned.
         * In case the motion hasn't ended or isn't being stopped yet, true is returned at the last moment so that
         * the new motion on the queue has faded in, at the same time the current last mixing motion is going to fade out.
         * This will prevent the bind pose to be mixed in the final result.
         * So basically the next motion will be played and will be fully faded in before the old motion will fade out.
         * @result True is returned when PlayNextMotion() should be called to start the next motion. Otherwise false is returned.
         * @note The Update() method will automatically call PlayNextMotion() when this method returns true.
         */
        bool ShouldPlayNextMotion();

        /**
         * Play the next motion in the motion queue.
         * If there are no entries in the queue, nothing will happen.
         */
        void PlayNextMotion();

    private:
        AZStd::vector<QueueEntry>    m_entries;           /**< The motion queue entries. */
        MotionSystem*               m_motionSystem;      /**< Motion system access pointer. */
        ActorInstance*              m_actorInstance;     /**< The actor instance where this queue works on. */

        /**
         * Constructor.
         * @param actorInstance The actor to which this queue belongs to.
         * @param motionSystem A pointer to the motion system of the actor.
         */
        MotionQueue(ActorInstance* actorInstance, MotionSystem* motionSystem);

        /**
         * Destructor.
         */
        ~MotionQueue();
    };
} // namespace EMotionFX
