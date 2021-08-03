/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/limits.h>

namespace AZ
{
    // Job priorities MAY be used judiciously to fine tune runtime execution, with the understanding
    // that profiling is needed to understand what the critical path per frame is. Modifying
    // job priorities is an EXPERT setting that should succeed a healthy dose of measurement.
    enum class JobPriority : uint8_t
    {
        CRITICAL = 0,
        HIGH = 1,
        MEDIUM = 2, // Default
        LOW = 3,
        PRIORITY_COUNT = 4,
    };

    // All submitted jobs are associated with a JobDescriptor which defines the priority, affinitization,
    // and tracking of the job resource utilization.
    //
    // TODO: Define various job kinds and provide a mechanism for cpuMask computation on different systems.
    struct JobDescriptor
    {
        // Unique job kind label (e.g. "frustum culling")
        // Job names *must* be provided
        const char* jobName = nullptr;

        // Associates a set of job kinds together for budget tracking (e.g. "graphics")
        const char* jobGroup = nullptr;

        // EXPERTS ONLY. Jobs of higher priority are executed ahead of any lower priority jobs
        // that were queued before it provided they had not yet started
        JobPriority priority = JobPriority::MEDIUM;

        // EXPERTS ONLY. A bitmask that restricts jobs of this kind to run only on cores
        // corresponding to a set bit. 0 is synonymous with all bits set
        uint32_t cpuMask = 0;
    };
}
