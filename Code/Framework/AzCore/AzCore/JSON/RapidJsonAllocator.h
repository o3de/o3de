/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/ChildAllocatorSchema.h>
#include <AzCore/Memory/OSAllocator.h>

namespace AZ::JSON
{
    class RapidJSONAllocator : public AZ::ChildAllocatorSchema<AZ::OSAllocator>
    {
    public:
        AZ_TYPE_INFO(RapidJSONAllocator, "{CCD24805-0E41-48CC-B92E-DB77F10FBEE3}");
    };
} // namespace AZ::JSON


