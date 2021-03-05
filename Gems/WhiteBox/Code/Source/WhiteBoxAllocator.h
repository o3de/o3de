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

#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Memory/Memory.h>

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
