/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_MEMORY_H
#define GM_MEMORY_H

#include <AzCore/base.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace GridMate
{
    /**
    * GridMateAllocator is used by non-MP portions of GridMate
    */
    class GridMateAllocator
        : public AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>
    {
    public:
        AZ_TYPE_INFO(GridMateAllocator, "{BB127E7A-E4EF-4480-8F17-0C10146D79E0}")

        using Base = AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>;
        using Descriptor = Base::Descriptor;

        GridMateAllocator()
            : GridMateAllocator::Base("GridMate Allocator", "GridMate fundamental generic memory allocator")
        {}
    };

    /**
    * GridMateAllocatorMP is used by MP portions of GridMate
    */
    class GridMateAllocatorMP
        : public AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>
    {
        friend class AZ::AllocatorInstance<GridMateAllocatorMP>;
    public:
        AZ_TYPE_INFO(GridMateAllocatorMP, "{FABCBC6E-B3E5-4200-861E-A3EC22592678}")

        using Base = AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>;
        using Descriptor = Base::Descriptor;

        GridMateAllocatorMP()
            : GridMateAllocatorMP::Base("GridMate Multiplayer Allocator", "GridMate Multiplayer data allocations (Session,Replica,Carrier)")
        {}

        // TODO: We have an aggressive memory policy in the Carrier. We have 2 ways to fix it.
        // Either keep a cap and sacrifice performance or create a carrier->GarbageCollection and call it from here
        //virtual void          GarbageCollect()                 { EBUS_EVENT(CarrierBus,GarbageCollect); m_allocator->GarbageCollect(); }
    };

    //! GridMate system container allocator.
    typedef AZ::AZStdAlloc<GridMateAllocator> GridMateStdAlloc;

    //! GridMate system container allocator.
    typedef AZ::AZStdAlloc<GridMateAllocatorMP> SysContAlloc;
}   // namespace GridMate


#define GM_CLASS_ALLOCATOR(_type)       AZ_CLASS_ALLOCATOR(_type, GridMate::GridMateAllocatorMP, 0)
#define GM_CLASS_ALLOCATOR_DECL         AZ_CLASS_ALLOCATOR_DECL
#define GM_CLASS_ALLOCATOR_IMPL(_type)  AZ_CLASS_ALLOCATOR_IMPL(_type, GridMate::GridMateAllocatorMP, 0)

#endif // GM_MEMORY_H
