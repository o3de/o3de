/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Math/MathIntrinsics.h>

namespace AZ::IO::IStreamerTypes
{
    constexpr bool IsPowerOf2(AZ::u64 value)
    {
        return (value != 0) && ((value & (value - 1)) == 0);
    }

    constexpr bool IsAlignedTo(AZ::u64 value, AZ::u64 alignment)
    {
        AZ_Assert(IsPowerOf2(alignment), "Provided alignment is not a power of 2.");
        return (value & (alignment - 1)) == 0;
    }

    bool IsAlignedTo(void* address, AZ::u64 alignment)
    {
        return IsAlignedTo(reinterpret_cast<uintptr_t>(address), alignment);
    }

    AZ::u64 GetAlignment(AZ::u64 value)
    {
        return u64{ 1 } << az_ctz_u64(value);
    }

    AZ::u64 GetAlignment(void* address)
    {
        return GetAlignment(reinterpret_cast<uintptr_t>(address));
    }
}

constexpr AZ::u64 operator"" _kib(AZ::u64 value)
{
    return value * 1024;
}

constexpr AZ::u64 operator"" _mib(AZ::u64 value)
{
    return value * (1024 * 1024);
}

constexpr AZ::u64 operator"" _gib(AZ::u64 value)
{
    return value * (1024 * 1024 * 1024);
}
