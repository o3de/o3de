/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>

namespace Physics
{
    namespace Edit
    {
        const static AZ::Crc32 CollisionLayerSelector = AZ_CRC("CollisionLayerSelector", 0xae1da12d);
        const static AZ::Crc32 CollisionGroupSelector = AZ_CRC("CollisionGroupSelector", 0x7d498664);
        const static AZ::Crc32 MaterialIdSelector = AZ_CRC("MaterialIdSelector", 0x494511ad);
    }
}
