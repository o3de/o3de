/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AliasedHeapEnums.h>
#include <Atom/RHI.Reflect/TransientAttachmentStatistics.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/DeviceBuffer.h>

#include <AzCore/std/optional.h>

namespace AZ::RHI
{
    class BufferAttachment;
    class ImageAttachment;
    class Scope;
    class SwapChainAttachment;
    struct TransientImageDescriptor;
    struct TransientBufferDescriptor;

    //! Describes the properties of a DeviceTransientAttachmentPool.
    struct TransientAttachmentPoolDescriptor
    {
        //! Defines the maximum amount of memory the pool is allowed to consume for transient buffers.
        //! If the budget is zero, the budget is not enforced by the RHI and reservations can grow unbounded.
        size_t m_bufferBudgetInBytes = 0;
        //! Defines the maximum amount of memory the pool is allowed to consume for transient images.
        //! If the budget is zero, the budget is not enforced by the RHI and reservations can grow unbounded.
        size_t m_imageBudgetInBytes = 0;
        //! Defines the maximum amount of memory the pool is allowed to consume for transient rendertargets.
        //! If the budget is zero, the budget is not enforced by the RHI and reservations can grow unbounded.
        size_t m_renderTargetBudgetInBytes = 0;

        //! Allocation parameters when using heaps for allocating transient attachments.
        HeapAllocationParameters m_heapParameters;
    };

    //! Flags to be used when compiling transient attachment resources.
    enum class TransientAttachmentPoolCompileFlags
    {
        None = 0,
        //! Gathers memory statistics for this heap during its next Begin / End cycle.
        GatherStatistics = AZ_BIT(1),
        //! Doesn't allocate any resources. Used when doing a pass to calculate how much memory will be used.
        DontAllocateResources = AZ_BIT(2)
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::TransientAttachmentPoolCompileFlags)

    //! The transient attachment pool interface is used by the frame scheduler to compile the
    //! working set of transient attachments for the frame. Each scope is iterated topologically
    //! and transient resources are allocated and de-allocated. This is all done from within the
    //! compile phase. Therefore, an allocation may create a resource, but a de-allocation does not
    //! destroy resources! All de-allocation does is inform the pool that a resource can
    //! be re-used within a subsequent scope. The final result of this process is a set of
    //! image / buffer attachments that are backed by guaranteed memory valid *only* for the scope in
    //! which they attached.
    class DeviceTransientAttachmentPool
        : public DeviceObject
    {
    public:
        AZ_RTTI(DeviceTransientAttachmentPool, "{D4A544E9-AE4A-4BD7-9E03-646DA8D86388}");

        virtual ~DeviceTransientAttachmentPool() = default;

        //! Returns true if a Transient Attachment Pool is needed according to the supplied descriptor.
        static bool NeedsTransientAttachmentPool(const TransientAttachmentPoolDescriptor& descriptor);

        //! Called to initialize the pool.
        ResultCode Init(Device& device, const TransientAttachmentPoolDescriptor& descriptor);

        //! Called to shutdown the pool.
        void Shutdown();

        //! This is called at the beginning of the compile phase for the current frame,
        //! before any allocations occur. The user should clear the backing allocator to
        //! a fresh state.
        void Begin(const TransientAttachmentPoolCompileFlags flags = TransientAttachmentPoolCompileFlags::None, const TransientAttachmentStatistics::MemoryUsage* memoryHint = nullptr);

        //! Called when a new scope is being allocated. Scopes are allocated in submission order.
        void BeginScope(Scope& scopeBase);

        //! Called when an image is being activated for the first time. This class should acquire
        //! an image from the pool, configured for the provided descriptor. This may involve aliasing
        //! from a heap, or simple object pooling.
        virtual DeviceImage* ActivateImage(const TransientImageDescriptor& descriptor) = 0;

        //! Called when an buffer is being activated for the first time. This class should acquire
        //! an buffer from the pool, configured for the provided descriptor. This may involve aliasing
        //! from a heap, or simple object pooling.
        virtual DeviceBuffer* ActivateBuffer(const TransientBufferDescriptor& descriptor) = 0;

        //! Called when a buffer is being de-allocated from the pool. Called during the last scope the attachment
        //! is used, after all allocations for that scope have been processed.
        virtual void DeactivateBuffer(const AttachmentId& attachmentId) = 0;

        //! Called when a image is being de-allocated from the pool. Called during the last scope the attachment
        //! is used, after all allocations for that scope have been processed.
        virtual void DeactivateImage(const AttachmentId& attachmentId) = 0;

        //! Called when all allocations for the current scope have completed.
        void EndScope();

        //! Called when the allocations / deallocations have completed for all scopes.
        void End();

        //! Get statistics for the pool (built during End).
        const TransientAttachmentStatistics& GetStatistics() const;

        //! Get pool descriptor
        const TransientAttachmentPoolDescriptor& GetDescriptor() const;

        //! Get the compile flags being used during the allocation of resources.
        TransientAttachmentPoolCompileFlags GetCompileFlags() const;

        static bool ValidateInitParameters(const TransientAttachmentPoolDescriptor& descriptor);

    protected:
        // Adds the stats of a list of heaps into the Pool's TransientAttachmentStatistics.
        void CollectHeapStats(AliasedResourceTypeFlags typeMask, AZStd::span<const TransientAttachmentStatistics::Heap> heapStats);

        Scope* m_currentScope = nullptr;
        RHI::TransientAttachmentStatistics m_statistics;

    private:
        //////////////////////////////////////////////////////////////////////////
        // Platform API

        /// Called when the pool is being initialized.
        virtual ResultCode InitInternal(Device& device, const TransientAttachmentPoolDescriptor& descriptor) = 0;

        virtual void BeginInternal(const TransientAttachmentPoolCompileFlags flags, const TransientAttachmentStatistics::MemoryUsage* memoryHint) = 0;
        virtual void EndInternal() = 0;

        /// Called when the pool is shutting down.
        virtual void ShutdownInternal() = 0;

        //////////////////////////////////////////////////////////////////////////

        TransientAttachmentPoolDescriptor m_descriptor;
        TransientAttachmentPoolCompileFlags m_compileFlags = TransientAttachmentPoolCompileFlags::None;
    };
}
