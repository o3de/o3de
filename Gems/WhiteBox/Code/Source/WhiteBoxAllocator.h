/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace WhiteBox
{
    //! White Box Gem allocator for all default allocations.
    class WhiteBoxAllocator : public AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>
    {
    public:
        AZ_TYPE_INFO(WhiteBoxAllocator, "{BFEB8C64-FDB7-4A19-B9B4-DDF57A434F14}");

        using Schema = AZ::ChildAllocatorSchema<AZ::SystemAllocator>;
        using Base = AZ::SimpleSchemaAllocator<Schema>;
        using Descriptor = Base::Descriptor;

        WhiteBoxAllocator()
            : Base("White Box Allocator", "Child Allocator used to track White Box allocations")
        {
        }
    };

    //! Alias for using WhiteBoxAllocator with std container types.
    using WhiteBoxAZStdAlloc = AZ::AZStdAlloc<WhiteBoxAllocator>;
} // namespace WhiteBox
