/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/AzNetworking_Traits_Platform.h>
#include <AzCore/PlatformIncl.h>

#include <AzNetworking/Utilities/Endian_Platform.h>

#if AZ_TRAIT_NEEDS_HTONLL
inline const uint64_t htonll(uint64_t value)
{
    const uint32_t hiValue = htonl(static_cast<uint32_t>(value >> 32));
    const uint32_t loValue = htonl(static_cast<uint32_t>(value & 0x00000000FFFFFFFF));
    return static_cast<uint64_t>(loValue) << 32 | static_cast<uint64_t>(hiValue);
}

inline const uint64_t ntohll(uint64_t value)
{
    return htonll(value);
}
#endif
