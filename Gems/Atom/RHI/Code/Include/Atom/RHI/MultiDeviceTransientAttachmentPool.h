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
#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceImage.h>
#include <Atom/RHI/MultiDeviceResourcePool.h>
#include <Atom/RHI/ObjectCache.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/TransientAttachmentPool.h>

#include <AzCore/std/optional.h>

namespace AZ
{
    namespace RHI
    {
        class BufferAttachment;
        class ImageAttachment;
        class Scope;
        class SwapChainAttachment;
        struct TransientImageDescriptor;
        struct TransientBufferDescriptor;

        //! The transient attachment pool interface is used by the frame scheduler to compile the
        //! working set of transient attachments for the frame. Each scope is iterated topologically
        //! and transient resources are allocated and de-allocated. This is all done from within the
        //! compile phase. Therefore, an allocation may create a resource, but a de-allocation does not
        //! destroy resources! All de-allocation does is inform the pool that a resource can
        //! be re-used within a subsequent scope. The final result of this process is a set of
        //! image / buffer attachments that are backed by guaranteed memory valid *only* for the scope in
        //! which they attached.
        class MultiDeviceTransientAttachmentPool : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(MultiDeviceTransientAttachmentPool, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDeviceTransientAttachmentPool, "{7CCD1108-B233-4D37-8A80-65CBB1988B22}");
            MultiDeviceTransientAttachmentPool() = default;
            virtual ~MultiDeviceTransientAttachmentPool() = default;

            //! Returns true if a Transient Attachment Pool is needed according to the supplied descriptor.
            static bool NeedsTransientAttachmentPool(const TransientAttachmentPoolDescriptor& descriptor);

            const RHI::Ptr<TransientAttachmentPool>& GetDeviceTransientAttachmentPool(int deviceIndex) const
            {
                AZ_Error(
                    "MultiDeviceTransientAttachmentPool",
                    m_deviceTransientAttachmentPools.find(deviceIndex) != m_deviceTransientAttachmentPools.end(),
                    "No DeviceTransientAttachmentPool found for device index %d\n",
                    deviceIndex);

                return m_deviceTransientAttachmentPools.at(deviceIndex);
            }

            //! Called to initialize the pool.
            ResultCode Init(MultiDevice::DeviceMask deviceMask, const TransientAttachmentPoolDescriptor& descriptor);

            //! Called to shutdown the pool.
            void Shutdown();

            //! This is called at the beginning of the compile phase for the current frame,
            //! before any allocations occur. The user should clear the backing allocator to
            //! a fresh state.
            void Begin(
                const TransientAttachmentPoolCompileFlags flags = TransientAttachmentPoolCompileFlags::None,
                const TransientAttachmentStatistics::MemoryUsage* memoryHint = nullptr);

            //! Called when a new scope is being allocated. Scopes are allocated in submission order.
            void BeginScope(Scope& scopeBase);

            //! Called when an image is being activated for the first time. This class should acquire
            //! an image from the pool, configured for the provided descriptor. This may involve aliasing
            //! from a heap, or simple object pooling.
            virtual MultiDeviceImage* ActivateImage(const TransientImageDescriptor& descriptor);

            //! Called when an buffer is being activated for the first time. This class should acquire
            //! an buffer from the pool, configured for the provided descriptor. This may involve aliasing
            //! from a heap, or simple object pooling.
            virtual MultiDeviceBuffer* ActivateBuffer(const TransientBufferDescriptor& descriptor);

            //! Called when a buffer is being de-allocated from the pool. Called during the last scope the attachment
            //! is used, after all allocations for that scope have been processed.
            virtual void DeactivateBuffer(const AttachmentId& attachmentId);

            //! Called when a image is being de-allocated from the pool. Called during the last scope the attachment
            //! is used, after all allocations for that scope have been processed.
            virtual void DeactivateImage(const AttachmentId& attachmentId);

            //! Called when all allocations for the current scope have completed.
            void EndScope();

            //! Called when the allocations / deallocations have completed for all scopes.
            void End();

            //! Get statistics for the pool (built during End).
            AZStd::unordered_map<int, TransientAttachmentStatistics> GetStatistics() const;

            //! Get pool descriptor
            const TransientAttachmentPoolDescriptor& GetDescriptor() const;

            //! Get the compile flags being used during the allocation of resources.
            TransientAttachmentPoolCompileFlags GetCompileFlags() const;

        private:
            bool ValidateInitParameters(const TransientAttachmentPoolDescriptor& descriptor) const;

            //! Remove the entry related to the provided attachmentId from the cache as it is probably stale now
            void RemoveFromCache(RHI::AttachmentId attachmentId);

            Scope* m_currentScope = nullptr;
            TransientAttachmentPoolDescriptor m_descriptor;
            TransientAttachmentPoolCompileFlags m_compileFlags = TransientAttachmentPoolCompileFlags::None;

            ObjectCache<MultiDeviceResource> m_cache;

            // This map is used to reverse look up resource hash so we can clear them out of m_cache
            // once they have been replaced with a new resource at a different place in the heap.
            AZStd::unordered_map<AttachmentId, HashValue64> m_reverseLookupHash;

            AZStd::unordered_map<int, Ptr<TransientAttachmentPool>> m_deviceTransientAttachmentPools;
        };
    } // namespace RHI
} // namespace AZ
