/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_PROFILER_DRILLER_BUS_H
#define AZCORE_PROFILER_DRILLER_BUS_H

#include <AzCore/Driller/DrillerBus.h>

namespace AZStd
{
    struct thread_id;
}

namespace AZ
{
    namespace Debug
    {
        class ProfilerRegister;

        /**
         * ProfilerDrillerInterface driller profiler event interface, that records events from the profiler system.
         */
        class ProfilerDrillerInterface
            : public DrillerEBusTraits
        {
        public:
            virtual ~ProfilerDrillerInterface() {}

            virtual void OnRegisterSystem(AZ::u32 id, const char* name) = 0;

            virtual void OnUnregisterSystem(AZ::u32 id) = 0;

            virtual void OnNewRegister(const ProfilerRegister& reg, const AZStd::thread_id& threadId) = 0;
        };

        typedef AZ::EBus<ProfilerDrillerInterface> ProfilerDrillerBus;
    }
}

#endif // AZCORE_PROFILER_DRILLER_BUS_H
#pragma once
