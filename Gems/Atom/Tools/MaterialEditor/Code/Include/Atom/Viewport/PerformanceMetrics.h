/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace MaterialEditor
{
    //! Data structure containing performance metrics for Material Editor
    struct PerformanceMetrics final
    {
        AZ_CLASS_ALLOCATOR(PerformanceMetrics, AZ::SystemAllocator, 0);

        double m_cpuFrameTimeMs = 0;
        double m_gpuFrameTimeMs = 0;
    };
} // namespace MaterialEditor
