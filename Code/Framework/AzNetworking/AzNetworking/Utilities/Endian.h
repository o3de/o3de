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

#include <AzNetworking/AzNetworking_Traits_Platform.h>
#include <AzCore/PlatformIncl.h>

#include <AzNetworking/Utilities/Endian_Platform.h>

#if AZ_TRAIT_NEEDS_HTONLL
static const uint64_t htonll(uint64_t value)
{
    const uint32_t hiValue = htonl(static_cast<uint32_t>(value >> 32));
    const uint32_t loValue = htonl(static_cast<uint32_t>(value & 0x00000000FFFFFFFF));
    return static_cast<uint64_t>(hiValue) << 32 | static_cast<uint64_t>(loValue);
}

static const uint64_t ntohll(uint64_t value)
{
    return htonll(value);
}
#endif
