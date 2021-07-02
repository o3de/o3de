/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_FRAME_PROFILER_BUS_H
#define AZCORE_FRAME_PROFILER_BUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Debug/FrameProfiler.h>

namespace AZ
{
    namespace Debug
    {
        class FrameProfilerComponent;

        /**
         * Interface class for frame profiler events.
         */
        class FrameProfilerEvents
            : public AZ::EBusTraits
        {
        public:
            virtual ~FrameProfilerEvents() {}

            /// Called when the frame profiler has computed a new frame (even is there is no new data).
            virtual void OnFrameProfilerData(const FrameProfiler::ThreadDataArray& data) = 0;
        };

        typedef AZ::EBus<FrameProfilerEvents> FrameProfilerBus;
    } // namespace Debug
} // namespace AZ

#endif // AZCORE_FRAME_PROFILER_BUS_H
#pragma once
