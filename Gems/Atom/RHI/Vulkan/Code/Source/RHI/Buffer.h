/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI/AsyncWorkQueue.h>
#include <Atom/RHI/BufferProperty.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/parallel/mutex.h>
#include <RHI/BufferMemoryView.h>
#include <RHI/Queue.h>
#include <RHI/RayTracingAccelerationStructure.h>

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
            : public RHI::DeviceBuffer
        {
            using Base = RHI::DeviceBuffer;
            friend class BufferPool;
            friend class AliasedHeap;
            friend class Device;

        public:
            AZ_CLASS_ALLOCATOR(Buffer, AZ::SystemAllocator);
            AZ_RTTI(Buffer, "D3416E5D-D058-4EB1-95D9-06C9B3B1C52A", Base);

            ~Buffer();

            static RHI::Ptr<Buffer> Create();
            // Returns the memory view allocated to this buffer.
            const BufferMemoryView* GetBufferMemoryView() const;
            BufferMemoryView* GetBufferMemoryView();

            using BufferOwnerProperty = RHI::BufferProperty<QueueId>;
            using SubresourceRangeOwner = BufferOwnerProperty::PropertyRange;

            // Returns a list of queues that owns a subresource region.
            AZStd::vector<SubresourceRangeOwner> GetOwnerQueue(const RHI::DeviceBufferView& bufferView) const;
            AZStd::vector<SubresourceRangeOwner> GetOwnerQueue(const RHI::BufferSubresourceRange* range = nullptr) const;

            // Set the owner queue of a subresource region.
            void SetOwnerQueue(const QueueId& queueId, const RHI::BufferSubresourceRange* range = nullptr);
            void SetOwnerQueue(const QueueId& queueId, const RHI::DeviceBufferView& bufferView);

            using BufferPipelineAccessProperty = RHI::BufferProperty<PipelineAccessFlags>;

            PipelineAccessFlags GetPipelineAccess(const RHI::BufferSubresourceRange* range = nullptr) const;
            void SetPipelineAccess(const PipelineAccessFlags& pipelineAccess, const RHI::BufferSubresourceRange* range = nullptr);

            void SetUploadHandle(const RHI::AsyncWorkHandle& handle);
            const RHI::AsyncWorkHandle& GetUploadHandle() const;

            /// Only valid for buffers with the RayTracingAccelerationStructure bind flag
            VkAccelerationStructureKHR GetNativeAccelerationStructure() const;
            void SetNativeAccelerationStructure(const RHI::Ptr<RayTracingAccelerationStructure>& accelerationStructure);

            VkSharingMode GetSharingMode() const;

        private:
            Buffer() = default;
            RHI::ResultCode Init(Device& device, const RHI::BufferDescriptor& bufferDescriptor, const BufferMemoryView& memoryView);

            void Invalidate();

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceResource
            void ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const override;
            //////////////////////////////////////////////////////////////////////////

            void SetInitalQueueOwner();
   
            BufferMemoryView m_memoryView;

            // Family queue index that owns the buffer regions.
            BufferOwnerProperty m_ownerQueue;
            mutable AZStd::mutex m_ownerQueueMutex;

            // Last pipeline access to the image subresources
            BufferPipelineAccessProperty m_pipelineAccess;
            mutable AZStd::mutex m_pipelineAccessMutex;

            RHI::AsyncWorkHandle m_uploadHandle;

            // native raytracing acceleration structure handle, necessary to bind the descriptor for
            // this buffer if it is of type VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
            RHI::Ptr<RayTracingAccelerationStructure> m_nativeAccelerationStructure = VK_NULL_HANDLE;
        };
    }
}
