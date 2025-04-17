/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Settings/ConfigurableStack.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

#if !defined(AZ_STREAMER_ADD_EXTRA_PROFILING_INFO)
#   if defined(_RELEASE)
#       define AZ_STREAMER_ADD_EXTRA_PROFILING_INFO 0
#   else
#       define AZ_STREAMER_ADD_EXTRA_PROFILING_INFO 1
#   endif
#endif

namespace AZ
{
    class ReflectContext;
}

namespace AZ::IO
{
    class StreamStackEntry;

    struct HardwareInformation
    {
        AZStd::any m_platformData;
        AZStd::string m_profile{"Default"};
        size_t m_maxPhysicalSectorSize{ AZCORE_GLOBAL_NEW_ALIGNMENT };
        size_t m_maxLogicalSectorSize{ AZCORE_GLOBAL_NEW_ALIGNMENT };
        size_t m_maxPageSize{ AZCORE_GLOBAL_NEW_ALIGNMENT };
        size_t m_maxTransfer{ AZCORE_GLOBAL_NEW_ALIGNMENT };
    };

    class IStreamerStackConfig
    {
    public:
        AZ_RTTI(AZ::IO::IStreamerStackConfig, "{97266736-E55E-4BF4-9E4A-9D5A9FF4D230}");
        AZ_CLASS_ALLOCATOR(IStreamerStackConfig, SystemAllocator);

        virtual ~IStreamerStackConfig() = default;
        virtual AZStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
            const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent) = 0;
        static void Reflect(ReflectContext* context);
    };

    class StreamerConfig final
    {
    public:
        AZ_RTTI(AZ::IO::StreamerConfig, "{B20540CB-A75C-45F9-A891-7ABFD9192E9F}");
        AZ_CLASS_ALLOCATOR(StreamerConfig, SystemAllocator);

        ConfigurableStack<AZ::IO::IStreamerStackConfig> m_stackConfig;
        static void Reflect(ReflectContext* context);
    };

    //! Collects hardware information for all hardware that can be used for file IO.
    //! @param info The retrieved hardware information.
    //! @param includeAllHardware Includes all available hardware that can be used by AZ::IO::Streamer. If set to false
    //!         only hardware is listed that is known to be used. This may be more performant, but can result is file
    //!         requests failing if they use an previously unknown path.
    //! @param reportHardware If true, hardware information will be printed to the log if available.
    extern bool CollectIoHardwareInformation(HardwareInformation& info, bool includeAllHardware, bool reportHardware);
    extern void ReflectNative(ReflectContext* context);

    //! Constant used to denote "file not found" in StreamStackEntry processing.
    inline static constexpr size_t s_fileNotFound = std::numeric_limits<size_t>::max();

    //! The number of entries in the statistics window. Larger number will measure over a longer time and will be more
    //! accurate, but also less responsive to sudden changes.
    static const size_t s_statisticsWindowSize = 128;
} // namespace AZ::IO
