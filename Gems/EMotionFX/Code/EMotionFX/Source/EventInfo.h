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
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class MotionInstance;
    class AnimGraphNode;
    class MotionEvent;


    /**
     * Triggered event info.
     * This class holds the information for each event that gets triggered.
     */
    class EMFX_API EventInfo
    {
        MCORE_MEMORYOBJECTCATEGORY(EventInfo, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_EVENTS);

    public:
        enum EventState
        {
            START,
            ACTIVE,
            END
        };

        float           m_timeValue;         /**< The time value of the event, in seconds. */
        ActorInstance*  m_actorInstance;     /**< The actor instance that triggered this event. */
        const MotionInstance* m_motionInstance;    /**< The motion instance which triggered this event, can be nullptr. */
        AnimGraphNode*  m_emitter;           /**< The animgraph node which originally did emit this event. This parameter can be nullptr. */
        const MotionEvent*    m_event;       /**< The event itself. */
        float           m_globalWeight;      /**< The global weight of the event. */
        float           m_localWeight;       /**< The local weight of the event. */
        EventState      m_eventState;      /**< Is this the start of a ranged event? Ticked events will always have this set to true. */

        bool IsEventStart() const
        {
            return m_eventState == EventState::START;
        }

        explicit EventInfo(
                float timeValue = 0.0f,
                ActorInstance* actorInstance = nullptr,
                const MotionInstance* motionInstance = nullptr,
                MotionEvent* event = nullptr,
                EventState eventState = START
        )
            : m_timeValue(timeValue)
            , m_actorInstance(actorInstance)
            , m_motionInstance(motionInstance)
            , m_emitter(nullptr)
            , m_event(event)
            , m_globalWeight(1.0f)
            , m_localWeight(1.0f)
            , m_eventState(eventState)
        {
        }
    };
}   // namespace EMotionFX

