/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/AliasedHeapEnums.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    struct TransientAttachmentPoolBudgets
    {
        AZ_TYPE_INFO(AZ::RHI::TransientAttachmentPoolBudgets, "{CE39BBEF-C9CD-4B9A-BA41-C886D9F063BC}");
        static void Reflect(AZ::ReflectContext* context);

        //! Defines the maximum amount of memory the pool is allowed to consume for transient buffers.
        //! If the budget is zero, the budget is not enforced by the RHI and reservations can grow unbounded.
        AZ::u64 m_bufferBudgetInBytes = 0;
        //! Defines the maximum amount of memory the pool is allowed to consume for transient images.
        //! If the budget is zero, the budget is not enforced by the RHI and reservations can grow unbounded.
        AZ::u64 m_imageBudgetInBytes = 0;
        //! Defines the maximum amount of memory the pool is allowed to consume for transient rendertargets.
        //! If the budget is zero, the budget is not enforced by the RHI and reservations can grow unbounded.
        AZ::u64 m_renderTargetBudgetInBytes = 0;
    };

    //! The platform default values are initially set with those hard coded in RHI Limits.
    //! They can be overridden by PlatformLimits.azasset from each platform, if a value is provided in that file.
    struct PlatformDefaultValues
    {
        AZ_TYPE_INFO(AZ::RHI::PlatformDefaultValues, "{F928CA84-C3F8-4F0B-8F34-808A24FA7C86}");
        static void Reflect(AZ::ReflectContext* context);
        AZ::u64  m_stagingBufferBudgetInBytes          = RHI::DefaultValues::Memory::StagingBufferBudgetInBytes;
        AZ::u64  m_asyncQueueStagingBufferSizeInBytes  = RHI::DefaultValues::Memory::AsyncQueueStagingBufferSizeInBytes;
        AZ::u64  m_mediumStagingBufferPageSizeInBytes  = RHI::DefaultValues::Memory::MediumStagingBufferPageSizeInBytes;
        AZ::u64  m_largestStagingBufferPageSizeInBytes = RHI::DefaultValues::Memory::LargestStagingBufferPageSizeInBytes;
        AZ::u64  m_imagePoolPageSizeInBytes            = RHI::DefaultValues::Memory::ImagePoolPageSizeInBytes;
        AZ::u64  m_bufferPoolPageSizeInBytes           = RHI::DefaultValues::Memory::BufferPoolPageSizeInBytes;
    };

    //! A descriptor used to configure limits for each backend. Can be overridden by platformlimits.azasset file
    class PlatformLimitsDescriptor
        : public AZStd::intrusive_base
    {
    public:
        AZ_RTTI(AZ::RHI::PlatformLimitsDescriptor, "{3A7B2BE4-0337-4F59-B4FC-B7E529EBE6C5}");
        AZ_CLASS_ALLOCATOR(AZ::RHI::PlatformLimitsDescriptor, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);
        static RHI::Ptr<PlatformLimitsDescriptor> Create();

        virtual ~PlatformLimitsDescriptor() = default;
        PlatformLimitsDescriptor() = default;

        TransientAttachmentPoolBudgets m_transientAttachmentPoolBudgets;
        PlatformDefaultValues m_platformDefaultValues;
            
        HeapPagingParameters m_pagingParameters;
        HeapMemoryHintParameters m_usageHintParameters;
        HeapAllocationStrategy m_heapAllocationStrategy = HeapAllocationStrategy::MemoryHint;

        virtual void LoadPlatformLimitsDescriptor(const char* rhiName);
    };

    class PlatformLimits final
    {
    public:
        AZ_RTTI(AZ::RHI::PlatformLimits, "{48158F25-5044-441C-A2B2-2D3E9255B0C3}");
        AZ_CLASS_ALLOCATOR(AZ::RHI::PlatformLimits, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        Ptr<PlatformLimitsDescriptor> m_platformLimitsDescriptor = nullptr;
    };

}
