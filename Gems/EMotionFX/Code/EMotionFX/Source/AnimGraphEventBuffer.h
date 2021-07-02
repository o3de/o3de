/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

#include "EMotionFXConfig.h"
#include "EventInfo.h"


namespace EMotionFX
{
    // forward declarations
    class MotionEventTrack;
    class AnimGraphNode;
    class AnimGraphInstance;


    /**
     * The anim graph event buffer class, which holds a collection of events which later have to be triggered.
     * This buffer is passed around the animgraph when processing it. The event buffer emitted by the root state machine of the animgraph will get triggered.
     */
    class EMFX_API AnimGraphEventBuffer
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphEventBuffer, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_EVENTBUFFERS);

    public:
        AnimGraphEventBuffer();
        ~AnimGraphEventBuffer() = default;
        AnimGraphEventBuffer(const AnimGraphEventBuffer&) = default;
        AnimGraphEventBuffer(AnimGraphEventBuffer&&) = default;
        AnimGraphEventBuffer& operator=(const AnimGraphEventBuffer&) = default;
        AnimGraphEventBuffer& operator=(AnimGraphEventBuffer&&) = default;

        void Reserve(uint32 numEvents);
        void Resize(uint32 numEvents);
        void AddEvent(const EventInfo& newEvent);
        void AddAllEventsFrom(const AnimGraphEventBuffer& eventBuffer);

        template<typename... Args>
        void AddEvent(Args&&... args)
        {
            m_events.emplace_back(AZStd::forward<Args>(args)...);
        }

        void SetEvent(uint32 index, const EventInfo& eventInfo);
        void Clear();

        MCORE_INLINE uint32 GetNumEvents() const                    { return static_cast<uint32>(m_events.size()); }
        MCORE_INLINE const EventInfo& GetEvent(uint32 index) const  { return m_events[index]; }

        void TriggerEvents() const;
        void UpdateWeights(AnimGraphInstance* animGraphInstance);
        void UpdateEmitters(AnimGraphNode* emitterNode);
        void Log() const;

    private:
        AZStd::vector<EventInfo> m_events;    /**< The collection of events inside this buffer. */
    };
}   // namespace EMotionFX
