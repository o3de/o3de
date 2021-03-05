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
