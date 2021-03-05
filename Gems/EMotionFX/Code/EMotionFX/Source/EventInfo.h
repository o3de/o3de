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

        float           mTimeValue;         /**< The time value of the event, in seconds. */
        ActorInstance*  mActorInstance;     /**< The actor instance that triggered this event. */
        const MotionInstance* mMotionInstance;    /**< The motion instance which triggered this event, can be nullptr. */
        AnimGraphNode*  mEmitter;           /**< The animgraph node which originally did emit this event. This parameter can be nullptr. */
        const MotionEvent*    mEvent;       /**< The event itself. */
        float           mGlobalWeight;      /**< The global weight of the event. */
        float           mLocalWeight;       /**< The local weight of the event. */
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
            : mTimeValue(timeValue)
            , mActorInstance(actorInstance)
            , mMotionInstance(motionInstance)
            , mEmitter(nullptr)
            , mEvent(event)
            , mGlobalWeight(1.0f)
            , mLocalWeight(1.0f)
            , m_eventState(eventState)
        {
        }
    };
}   // namespace EMotionFX

