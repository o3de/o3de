/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Preprocessor/Sequences.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>

namespace AZ::RHI
{
    //! Base allocator that is used by all Atom allocators.
    //! This allocator is used for tracking purpouse and it just forward the
    //! allocations to the final allocator.
    class PassThroughAllocatorBase
        : public AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>
    {
    public:
        using Base = AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>;
        AZ_RTTI(PassThroughAllocatorBase, "{5A2780C1-3660-4F47-A529-8E4F7B2B2F84}", Base)

        PassThroughAllocatorBase();
        ~PassThroughAllocatorBase() override;
    };

// Sequence with all the allocators for the RHI. For new allocators, just 
// add a new row with the proper information
// 
//              Allocator name,               Display Name,                     Allocator type,         UUID
#define RHI_ALLOCATORS \
    ((ShaderStageFunctionAllocator) ("RHI::ShaderStageFunctionAllocator")   (PassThroughAllocatorBase)   ("{15F285F1-74D5-4FAE-8CE4-B7D235A92F23}")) \

    // if it exceeds 50, needs adjustments in AzCore/Preprocessor/Sequences.h

// Strips from the HEAD
#define RHI_SEQ_TAIL_STRIP_1(X) AZ_SEQ_TAIL(X)
#define RHI_SEQ_TAIL_STRIP_2(X) RHI_SEQ_TAIL_STRIP_1(AZ_SEQ_TAIL(X))
#define RHI_SEQ_TAIL_STRIP_3(X) RHI_SEQ_TAIL_STRIP_2(AZ_SEQ_TAIL(X))

// Gets the N element from the head
#define RHI_SEQ_HEAD_1(X) AZ_SEQ_HEAD(X)
#define RHI_SEQ_HEAD_2(X) RHI_SEQ_HEAD_1(RHI_SEQ_TAIL_STRIP_1(X))
#define RHI_SEQ_HEAD_3(X) RHI_SEQ_HEAD_1(RHI_SEQ_TAIL_STRIP_2(X))
#define RHI_SEQ_HEAD_4(X) RHI_SEQ_HEAD_1(RHI_SEQ_TAIL_STRIP_3(X))

// Gets specific arguments so the macros are more readable
#define RHI_ALLOCATOR_SEQ_GET_NAME(X) RHI_SEQ_HEAD_1(X)
#define RHI_ALLOCATOR_SEQ_GET_DISPLAY(X) RHI_SEQ_HEAD_2(X)
#define RHI_ALLOCATOR_SEQ_GET_TYPE(X) RHI_SEQ_HEAD_3(X)
#define RHI_ALLOCATOR_SEQ_GET_UUID(X) RHI_SEQ_HEAD_4(X)

// Declare the allocator using the macro arguments
#define RHI_ALLOCATOR_DECL(ALLOCATOR_SEQUENCE)                                                                                       \
    class RHI_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE)                                                                             \
        : public RHI_ALLOCATOR_SEQ_GET_TYPE(ALLOCATOR_SEQUENCE)                                                                      \
    {                                                                                                                                      \
        friend class AZ::AllocatorInstance<RHI_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE)>;                                          \
                                                                                                                                           \
    public:                                                                                                                                \
        AZ_TYPE_INFO_WITH_NAME(                                                                                                            \
            RHI_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE),                                                                                \
            RHI_ALLOCATOR_SEQ_GET_DISPLAY(ALLOCATOR_SEQUENCE),                                                                             \
            RHI_ALLOCATOR_SEQ_GET_UUID(ALLOCATOR_SEQUENCE));                                                                                \
        using Base = RHI_ALLOCATOR_SEQ_GET_TYPE(ALLOCATOR_SEQUENCE);                                                                 \
        AZ_RTTI_NO_TYPE_INFO_DECL();                                                                                                       \
    };                                                                                                                                     \
                                                                                                                                           \
    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(                                                                                                      \
        RHI_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE), RHI_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE)::Base);

    // Here we create all the classes for all the items in the above table
    AZ_SEQ_FOR_EACH(RHI_ALLOCATOR_DECL, RHI_ALLOCATORS)

} // namespace RHI
