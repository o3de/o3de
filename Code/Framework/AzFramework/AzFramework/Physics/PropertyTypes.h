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
