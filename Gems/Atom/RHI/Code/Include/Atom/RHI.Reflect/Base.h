/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Bits.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

// Enable this macro to force the cpu to run in lockstep with gpu. It will force
// every RHI::Scope (i.e pass) into its own command list that is explicitly flushed and
// waited on the cpu after an execute is called. This ensures that an execution for gpu
// work related to a specific scope (i.e pass) finished successfully on the gpu
// before the next scope is processed on the cpu. As long as you can repro this crash
// in this mode use it to debug GPU device removals/TDR's when
// you need to know which scope was executing right before the crash.
//#define AZ_FORCE_CPU_GPU_INSYNC

AZ_DECLARE_BUDGET(RHI);

//#define ENABLE_RHI_PROFILE_VERBOSE
#ifdef ENABLE_RHI_PROFILE_VERBOSE
// Add verbose profile markers
#define RHI_PROFILE_SCOPE_VERBOSE(...) AZ_PROFILE_SCOPE(RHI, __VA_ARGS__);
#define RHI_PROFILE_FUNCTION_VERBOSE AZ_PROFILE_FUNCTION(RHI);
#else
// Define ENABLE_RHI_PROFILE_VERBOSE to get verbose profile markers
#define RHI_PROFILE_SCOPE_VERBOSE(...)
#define RHI_PROFILE_FUNCTION_VERBOSE
#endif

inline static constexpr AZ::Crc32 rhiMetricsId = AZ_CRC_CE("RHI");

namespace UnitTest
{
    class RHITestFixture;
    class RPITestFixture;
} // namespace UnitTest

namespace AZ::RHI
{
    class BuildOptions
    {
    public:
#ifdef AZ_DEBUG_BUILD
        static constexpr bool IsDebugBuild = true;
#else
        static constexpr bool IsDebugBuild = false;
#endif

#ifdef AZ_PROFILE_BUILD
        static constexpr bool IsProfileBuild = true;
#else
        static constexpr bool IsProfileBuild = false;
#endif
    };

    class Validation
    {
        friend class UnitTest::RHITestFixture;
        friend class UnitTest::RPITestFixture;
    public:
        static bool IsEnabled()
        {
            return s_isEnabled;
        }
    private:
        static bool s_isEnabled;
    };

    template <typename T>
    using Ptr = AZStd::intrusive_ptr<T>;

    template <typename T>
    using ConstPtr = AZStd::intrusive_ptr<const T>;

    /**
        * A set of general result codes used by methods which may fail.
        */
    enum class ResultCode : uint32_t
    {
        // The operation succeeded.
        Success = 0,

        // The operation failed with an unknown error.
        Fail,

        // The operation failed due being out of memory.
        OutOfMemory,

        // The operation failed because the feature is unimplemented on the particular platform.
        Unimplemented,

        // The operation failed because the API object is not in a state to accept the call.
        InvalidOperation,

        // The operation failed due to invalid arguments.
        InvalidArgument,

        // The operation is not ready
        NotReady
    };

    using MessageOutcome = AZ::Outcome<void, AZStd::string>;

    enum class APIIndex : uint32_t
    {
        Null = 0,
        DX12,
        Vulkan,
        Metal,
    };

    using APIType = Crc32;

    enum class DrawListSortType : uint8_t
    {
        KeyThenDepth = 0,
        KeyThenReverseDepth,
        DepthThenKey,
        ReverseDepthThenKey
    };
        
    enum class Scaling : uint32_t
    {
        None = 0,               // No scaling
        Stretch,                // Scale the source to fit the target
        AspectRatioStretch,     // Scale the source to fit the target while preserving the aspect ratio of the source
    };

    //! Flags for specifying supported Scaling modes
    enum class ScalingFlags : uint32_t
    {
        None = 0,
        Stretch = AZ_BIT(static_cast<uint32_t>(Scaling ::Stretch)),
        AspectRatioStretch = AZ_BIT(static_cast<uint32_t>(Scaling ::AspectRatioStretch)),
        All = Stretch | AspectRatioStretch
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::ScalingFlags);
    AZ_TYPE_INFO_SPECIALIZE(DrawListSortType, "{D43AF0B7-7314-4B57-AA98-6209235B91BB}");
}
