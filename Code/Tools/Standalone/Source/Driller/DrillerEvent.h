/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_DRILLER_EVENT_H
#define DRILLER_DRILLER_EVENT_H

#include <AzCore/RTTI/RTTI.h>

namespace Driller
{
    class Aggregator;

    /**
     * Base class for a driller events. All events collected in aggregators should use this class as a base.
     *
     * IMPORTANT: It's very important to note that DrillerEvents will NEVER be removed during a driller session
     * this allows you to RELY on the fact that data which the event contains will be present at all times. This is important
     * because it's heavily used to support StepBackward/StepForward in sequential driller events (almost all driller events are deltas
     * and in that sense they are sequential - we can't just jump to a state directly).
     */
    class DrillerEvent
    {
    public:
        AZ_RTTI(DrillerEvent, "{3B0B15CF-A359-47AA-B8D3-DCEFA39BD097}");
        DrillerEvent(unsigned int eventType)
            : m_eventType(eventType)
            , m_globalEventId(s_globalEventId++) {}
        virtual ~DrillerEvent() {}

        virtual void    StepForward(Aggregator* data) = 0;
        virtual void    StepBackward(Aggregator* data) = 0;

        static inline unsigned int GetNumGlobalEvents()         { return s_globalEventId; }
        static inline void         ResetGlobalEventId()         { s_globalEventId = 0; }

        unsigned int    GetGlobalEventId() const                { return m_globalEventId; }
        unsigned int    GetEventType() const                    { return m_eventType; }

    protected:
        unsigned int    m_eventType;
        unsigned int    m_globalEventId;        ///< Event unique ID, which is the global event index (in order) too.

    private:
        static unsigned int s_globalEventId;    ///< Current number of events used in all aggregators.
    };
}

#endif
