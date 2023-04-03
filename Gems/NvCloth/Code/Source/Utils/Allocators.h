/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>

namespace NvCloth
{
    //! System allocator to be used for all nvcloth library allocations.
    class AzClothAllocator
        : public AZ::SystemAllocator
    {
        friend class AZ::AllocatorInstance<AzClothAllocator>;

    public:
        AZ_RTTI(AzClothAllocator, "{F2C6C61F-587E-4EBB-A377-A5E57BB6B849}", AZ::SystemAllocator);
    };
} // namespace NvCloth
