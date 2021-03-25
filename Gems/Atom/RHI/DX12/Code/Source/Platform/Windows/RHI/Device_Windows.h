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

#include <Atom/RHI/Device.h>

namespace AZ
{
    namespace DX12
    {
        class Device_Windows
            : public RHI::Device
        {
            using Base = RHI::Device;
        public:
            void* GetAftermathGPUCrashTracker()
            {
#if defined(USE_NSIGHT_AFTERMATH)
                return static_cast<void*>(&m_gpuCrashTracker);
#else
                return nullptr;
#endif
            }

        protected:
            Device_Windows() = default;

            // [GFX TODO] ATOM-4149 - NVAPI
            // [GFX TODO] ATOM-4151 - AMD AGS

#if defined(USE_NSIGHT_AFTERMATH)
            // Nsight Aftermath instrumentation
            GpuCrashTracker m_gpuCrashTracker;
#endif
        };

        using Device_Platform = Device_Windows;
    }
}
