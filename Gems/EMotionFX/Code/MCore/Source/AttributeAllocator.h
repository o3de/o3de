/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzCore/Memory/PoolAllocator.h>

namespace MCore
{
    class AttributeAllocator
        : public AZ::PoolAllocator
    {
        friend class AZ::AllocatorInstance<AZ::PoolAllocator>;
    public:
        AZ_TYPE_INFO_WITH_NAME(AttributeAllocator, "EMotionFX::AttributeAllocator", "{005003CF-87D1-4DAD-A159-59217F67886B}");
        AZ_RTTI_NO_TYPE_INFO_DECL();
    };

    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(AttributeAllocator);
}
