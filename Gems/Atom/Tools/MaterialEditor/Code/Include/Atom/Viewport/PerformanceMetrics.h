/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
