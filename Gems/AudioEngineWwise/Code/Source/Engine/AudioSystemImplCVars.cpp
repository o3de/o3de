/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioSystemImplCVars.h>

#include <AudioEngineWwise_Traits_Platform.h>

namespace Audio::Wwise::Cvars
{
    AZ_CVAR(AZ::u64, s_PrimaryMemorySize, AZ_TRAIT_AUDIOENGINEWWISE_PRIMARY_POOL_SIZE,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "The size in KiB of the primary memory pool used by the Wwise audio integration.\n"
        "Usage: s_PrimaryMemorySize=" AZ_TRAIT_AUDIOENGINEWWISE_PRIMARY_POOL_SIZE_DEFAULT_TEXT "\n");

    AZ_CVAR(AZ::u64, s_SecondaryMemorySize, AZ_TRAIT_AUDIOENGINEWWISE_SECONDARY_POOL_SIZE,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "The size in KiB of the secondary memory pool.  Most platforms do not use this.\n"
        "Usage: s_SecondaryMemorySize=" AZ_TRAIT_AUDIOENGINEWWISE_PRIMARY_POOL_SIZE_DEFAULT_TEXT "\n");

    AZ_CVAR(AZ::u64, s_StreamDeviceMemorySize, AZ_TRAIT_AUDIOENGINEWWISE_STREAMER_DEVICE_MEMORY_POOL_SIZE,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "The size in KiB of the Wwise Stream Device.\n"
        "Usage: s_StreamDeviceMemorySize=" AZ_TRAIT_AUDIOENGINEWWISE_STREAMER_DEVICE_MEMORY_POOL_SIZE_DEFAULT_TEXT "\n");

    AZ_CVAR(AZ::u64, s_CommandQueueMemorySize, AZ_TRAIT_AUDIOENGINEWWISE_COMMAND_QUEUE_MEMORY_POOL_SIZE,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "The size in KiB of the Wwise Command Queue.\n"
        "Usage: s_CommandQueueMemorySize=" AZ_TRAIT_AUDIOENGINEWWISE_COMMAND_QUEUE_MEMORY_POOL_SIZE_DEFAULT_TEXT "\n");

#if !defined(WWISE_RELEASE)
    AZ_CVAR(AZ::u64, s_MonitorQueueMemorySize, AZ_TRAIT_AUDIOENGINEWWISE_MONITOR_QUEUE_MEMORY_POOL_SIZE,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "The size in KiB of the Wwise Monitor Queue.\n"
        "Not available in Release build.\n"
        "Usage: s_MonitorQueueMemorySize=" AZ_TRAIT_AUDIOENGINEWWISE_MONITOR_QUEUE_MEMORY_POOL_SIZE_DEFAULT_TEXT "\n");

    AZ_CVAR(bool, s_EnableCommSystem, false,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "Enable initialization of the Wwise Comm system, which allows for remote profiling.\n"
        "Not available in Release build.\n"
        "Usage: s_EnableCommSystem=true (false)\n");

    AZ_CVAR(bool, s_EnableOutputCapture, false,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "Capture the main audio output to a WAV file.\n"
        "Not available in Release build.\n"
        "Usage: s_EnableOutputCapture=true (false)\n");
#endif // !WWISE_RELEASE

} // namespace Audio::Wwise::Cvars
