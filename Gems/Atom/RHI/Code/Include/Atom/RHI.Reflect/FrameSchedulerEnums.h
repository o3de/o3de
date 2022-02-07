/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * Describes the policy for threading by client code. Serial policies mean the context
         * or group in question is not thread-safe and must be executed in-order on a single thread.
         */
        enum class JobPolicy : uint32_t
        {
            /// Must execute serially (in-order) on a single thread.
            Serial = 0,

            /// Can jobify across threads.
            Parallel
        };

        /**
         * Controls verbosity of compilation result logging to the console.
         */
        enum class FrameSchedulerLogVerbosity
        {
            None,

            /// Logs a summary of the frame scheduler compilation results.
            Summary,

            /// Logs detailed info about the compilation results.
            Detail
        };

        enum class FrameSchedulerCompileFlags : uint32_t
        {
            None = 0,

            /// Disables hardware async queues on platforms that support it.
            DisableAsyncQueues = AZ_BIT(1),

            /// Disables aliasing of transient attachment memory on platforms that support it.
            DisableAttachmentAliasing = AZ_BIT(2),

            /// Disables aliasing of transient attachment memory during async queue regions.
            DisableAttachmentAliasingAsyncQueue = AZ_BIT(3)
        };
        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::FrameSchedulerCompileFlags)

        /**
         * Enables statistics gathering for the current frame. Results are made
         * available through the FrameSchedulerQueryBus interface after the frame is complete.
         * Results are considered valid until the next compilation cycle. Statistics gathering may
         * incur a non-negligible performance cost, so it is recommended to enable gathering only when
         * necessary.
         */
        enum class FrameSchedulerStatisticsFlags : uint32_t
        {
            None = 0,

            //! Enables gathering of transient attachment statistics.
            GatherTransientAttachmentStatistics = AZ_BIT(2),

            //! Enables gathering of memory statistics across pools.
            GatherMemoryStatistics = AZ_BIT(3)
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::FrameSchedulerStatisticsFlags)
    }
}
