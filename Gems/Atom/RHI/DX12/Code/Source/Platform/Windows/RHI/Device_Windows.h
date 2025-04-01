/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/DX12.h>

#include <Atom/RHI/Device.h>

#if defined(USE_NSIGHT_AFTERMATH)
    #include <RHI/NsightAftermathGpuCrashTracker_Windows.h>
#endif

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
