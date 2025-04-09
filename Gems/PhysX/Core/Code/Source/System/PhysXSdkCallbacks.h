/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PxPhysicsAPI.h>

namespace PhysX
{
    //! Implementation of the PhysX error callback interface directing errors to Open 3D Engine error output.
    class PxAzErrorCallback
        : public physx::PxErrorCallback
    {
    public:
        void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override;
    };

    //! Implementation of the PhysX profiler callback interface.
    class PxAzProfilerCallback
        : public physx::PxProfilerCallback
    {
    public:

        //! Mark the beginning of a nested profile block.
        //! @param eventName Event name. Must be a persistent const char *.
        //! @param detached True for cross thread events.
        //! @param contextId The context id of this zone. Zones with the same id belong to the same group. 0 is used for no specific group.
        //! @return Returns implementation-specific profiler data for this event.
        void* zoneStart(const char* eventName, bool detached, uint64_t contextId) override;

        //! Mark the end of a nested profile block.
        //! @param profilerData The data returned by the corresponding zoneStart call (or NULL if not available)
        //! @param eventName The name of the zone ending, must match the corresponding name passed with 'zoneStart'. Must be a persistent const char *.
        //! @param detached True for cross thread events. Should match the value passed to zoneStart.
        //! @param contextId The context of this zone. Should match the value passed to zoneStart.
        //! Note: eventName plus contextId can be used to uniquely match up start and end of a zone.
        void zoneEnd(void* profilerData, const char* eventName, bool detached, uint64_t contextId) override;
    };
}
