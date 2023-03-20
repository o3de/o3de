/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>

namespace AtomToolsFramework
{
    //! Data structure containing performance metrics
    struct PerformanceMetrics final
    {
        AZ_CLASS_ALLOCATOR(PerformanceMetrics, AZ::SystemAllocator);

        double m_cpuFrameTimeMs = 0;
        double m_gpuFrameTimeMs = 0;
    };
} // namespace AtomToolsFramework
