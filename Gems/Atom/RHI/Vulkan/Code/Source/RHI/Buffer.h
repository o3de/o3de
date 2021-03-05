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

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferProperty.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/parallel/mutex.h>
#include <Atom/RHI/AsyncWorkQueue.h>
#include <RHI/BufferMemoryView.h>
#include <RHI/Queue.h>

namespace AZ
{
    namespace RHI
    {
        class BufferView;
    }
    namespace Vulkan
    {
        class BufferPool;
        class Device;
        class CommandQueue;

        class Buffer final
            : public RHI::Buffer
        {
            using Base = RHI::Buffer;
            friend class BufferPool;
            friend class AliasedHeap;
            friend class Device;

        public:
            AZ_CLASS_ALLOCATOR(Buffer, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(Buffer, "D3416E5D-D058-4EB1-95D9-06C9B3B1C52A", Base);

            ~Buffer();

            static RHI::Ptr<Buffer> Create();
            // Returns the memory view allocated to this buffer.
            const BufferMemoryView* GetBufferMemoryView() const;
            BufferMemoryView* GetBufferMemoryView();

            using BufferOwnerProperty = RHI::BufferProperty<QueueId>;
            using SubresourceRangeOwner = BufferOwnerProperty::PropertyRange;

            // Returns a list of queues that owns a subresource region.
            AZStd::vector<SubresourceRangeOwner> GetOwnerQueue(const RHI::BufferView& bufferView) const;
            AZStd::vector<SubresourceRangeOwner> GetOwnerQueue(const RHI::BufferSubresourceRange* range = nullptr) const;

            // Set the owner queue of a subresource region.
            void SetOwnerQueue(const QueueId& queueId, const RHI::BufferSubresourceRange* range = nullptr);
            void SetOwnerQueue(const QueueId& queueId, const RHI::BufferView& bufferView);

            void SetUploadHandle(const RHI::AsyncWorkHandle& handle);
            const RHI::AsyncWorkHandle& GetUploadHandle() const;

        private:
            Buffer() = default;
            RHI::ResultCode Init(Device& device, const RHI::BufferDescriptor& bufferDescriptor, BufferMemoryView& memmoryView);

            void Invalidate();

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Resource
            void ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const override;
            //////////////////////////////////////////////////////////////////////////
   
            BufferMemoryView m_memoryView;

            // Family queue index that owns the buffer regions.
            BufferOwnerProperty m_ownerQueue;
            mutable AZStd::mutex m_ownerQueueMutex;

            RHI::AsyncWorkHandle m_uploadHandle;
        };
    }
}
