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

AZ_DECLARE_BUDGET(RHI);
inline static constexpr AZ::Crc32 rhiMetricsId = AZ_CRC_CE("RHI");

namespace UnitTest
{
    class RHITestFixture;
    class RPITestFixture;
}

namespace AZ
{
    namespace RHI
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

    }

    AZ_TYPE_INFO_SPECIALIZE(RHI::DrawListSortType, "{D43AF0B7-7314-4B57-AA98-6209235B91BB}");
}
