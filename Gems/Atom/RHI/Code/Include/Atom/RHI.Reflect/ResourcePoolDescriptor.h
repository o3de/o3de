/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    class ResourcePoolDescriptor
    {
    public:
        AZ_RTTI(ResourcePoolDescriptor, "{C4B9BF83-B171-4DB9-93D6-0879C7CEF5C2}");
        AZ_CLASS_ALLOCATOR(ResourcePoolDescriptor, SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        ResourcePoolDescriptor() = default;
        virtual ~ResourcePoolDescriptor() = default;

        //! The budget defines the maximum amount of memory the pool is allowed to consume on its heap level.
        //! If the budget is zero, the budget is not enforced by the RHI and reservations can grow unbounded. However,
        //! the platform itself may still report out of memory errors. Therefore, it is strongly recommended to assign
        //! a budget to Device pools where virtual memory is not present on most platforms.
        AZ::u64 m_budgetInBytes = 0;

        /// The device mask used to create resources from this pool.
        MultiDevice::DeviceMask m_deviceMask = MultiDevice::AllDevices;
    };
}
