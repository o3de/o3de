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
    // Task priorities MAY be used judiciously to fine tune runtime execution, with the understanding
    // that profiling is needed to understand what the critical path per frame is. Modifying
    // task priorities is an EXPERT setting that should succeed a healthy dose of measurement.
    enum class TaskPriority : uint8_t
    {
        CRITICAL = 0,
        HIGH = 1,
        MEDIUM = 2, // Default
        LOW = 3,
        PRIORITY_COUNT = 4,
    };

    // All submitted tasks are associated with a TaskDescriptor which defines the priority, affinitization,
    // and tracking of the task resource utilization.
    //
    // TODO: Define various task kinds and provide a mechanism for cpuMask computation on different systems.
    struct TaskDescriptor
    {
        // Unique task kind label (e.g. "frustum culling")
        // Task names *must* be provided
        const char* taskName = nullptr;

        // Associates a set of task kinds together for budget tracking (e.g. "graphics")
        const char* taskGroup = nullptr;

        // EXPERTS ONLY. Tasks of higher priority are executed ahead of any lower priority tasks
        // that were queued before it provided they had not yet started
        TaskPriority priority = TaskPriority::MEDIUM;

        // EXPERTS ONLY. A bitmask that restricts tasks of this kind to run only on cores
        // corresponding to a set bit. 0 is synonymous with all bits set
        uint32_t cpuMask = 0;
    };
}
