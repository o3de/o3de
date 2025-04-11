/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/sort.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/Device.h>
#include <RHI/MemoryView.h>
#include <RHI/Queue.h>
#include <RHI/RayTracingAccelerationStructure.h>
#include <RHI/ReleaseContainer.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<Buffer> Buffer::Create()
        {
            return aznew Buffer();
        }

        Buffer::~Buffer()
        {
            Invalidate();
        }

        void Buffer::Invalidate()
        {
            AZ_Assert(!m_memoryView.IsValid(), "Memory has not been deallocated");
            m_memoryView = BufferMemoryView();
            m_ownerQueue.Reset();
        }

        const BufferMemoryView* Buffer::GetBufferMemoryView() const
        {
            return &m_memoryView;
        }

        BufferMemoryView* Buffer::GetBufferMemoryView()
        {
            return &m_memoryView;
        }

        RHI::ResultCode Buffer::Init(Device& device, const RHI::BufferDescriptor& bufferDescriptor, const BufferMemoryView& memoryView)
        {
            DeviceObject::Init(device);
            m_ownerQueue.Init(bufferDescriptor);
            m_pipelineAccess.Init(bufferDescriptor);
            m_memoryView = memoryView;

            SetInitalQueueOwner();
            SetPipelineAccess({ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE });
            SetName(GetName());
            return RHI::ResultCode::Success;
        }

        void Buffer::SetInitalQueueOwner()
        {
            if (!IsInitialized())
            {
                return;
            }

            auto& device = static_cast<Device&>(GetDevice());
            const RHI::BufferDescriptor& descriptor = GetDescriptor();
            for (uint32_t i = 0; i < RHI::HardwareQueueClassCount; ++i)
            {
                if (RHI::CheckBitsAny(descriptor.m_sharedQueueMask, static_cast<RHI::HardwareQueueClassMask>(AZ_BIT(i))))
                {
                    SetOwnerQueue(device.GetCommandQueueContext().GetCommandQueue(static_cast<RHI::HardwareQueueClass>(i)).GetId());
                    break;
                }
            }
        }

        AZStd::vector<Buffer::SubresourceRangeOwner> Buffer::GetOwnerQueue(const RHI::BufferSubresourceRange* range /*= nullptr*/) const
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_ownerQueueMutex);
            return m_ownerQueue.Get(range ? *range : RHI::BufferSubresourceRange(GetDescriptor()));
        }

        AZStd::vector<Buffer::SubresourceRangeOwner> Buffer::GetOwnerQueue(const RHI::DeviceBufferView& bufferView) const
        {
            auto range = RHI::BufferSubresourceRange(bufferView.GetDescriptor());
            return GetOwnerQueue(&range);
        }

        void Buffer::SetOwnerQueue(const QueueId& queueId, const RHI::BufferSubresourceRange* range /*= nullptr*/)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_ownerQueueMutex);
            m_ownerQueue.Set(range ? *range : RHI::BufferSubresourceRange(GetDescriptor()), queueId);
        }

        void Buffer::SetOwnerQueue(const QueueId& queueId, const RHI::DeviceBufferView& bufferView)
        {
            auto range = RHI::BufferSubresourceRange(bufferView.GetDescriptor());
            return SetOwnerQueue(queueId , &range);
        }

        PipelineAccessFlags Buffer::GetPipelineAccess(const RHI::BufferSubresourceRange* range) const
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_pipelineAccessMutex);
            PipelineAccessFlags pipelineAccessFlags = {};
            for (const auto& propertyRange : m_pipelineAccess.Get(range ? *range : RHI::BufferSubresourceRange(GetDescriptor())))
            {
                pipelineAccessFlags.m_pipelineStage |= propertyRange.m_property.m_pipelineStage;
                pipelineAccessFlags.m_access |= propertyRange.m_property.m_access;
            }
            return pipelineAccessFlags;
        }

        void Buffer::SetPipelineAccess(const PipelineAccessFlags& pipelineAccess, const RHI::BufferSubresourceRange* range)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_pipelineAccessMutex);
            m_pipelineAccess.Set(range ? *range : RHI::BufferSubresourceRange(GetDescriptor()), pipelineAccess);
        }

        void Buffer::SetUploadHandle(const RHI::AsyncWorkHandle& handle)
        {
            m_uploadHandle = handle;
        }

        const RHI::AsyncWorkHandle& Buffer::GetUploadHandle() const
        {
            return m_uploadHandle;
        }

        void Buffer::SetNameInternal(const AZStd::string_view& name)
        {
            if (m_memoryView.IsValid())
            {
                m_memoryView.SetName(name);
            }
        }

        void Buffer::ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const
        {
            const RHI::BufferDescriptor& descriptor = GetDescriptor();

            RHI::MemoryStatistics::Buffer* bufferStats = builder.AddBuffer();
            bufferStats->m_name = GetName();
            bufferStats->m_bindFlags = descriptor.m_bindFlags;
            bufferStats->m_sizeInBytes = m_memoryView.GetSize();
        }

        VkAccelerationStructureKHR Buffer::GetNativeAccelerationStructure() const
        {
            AZ_Assert(RHI::CheckBitsAll(GetDescriptor().m_bindFlags, RHI::BufferBindFlags::RayTracingAccelerationStructure),
                "GetNativeAccelerationStructure() is only valid for buffers with the RayTracingAccelerationStructure bind flag");

            return m_nativeAccelerationStructure->GetNativeAccelerationStructure();
        }

        void Buffer::SetNativeAccelerationStructure(const RHI::Ptr<RayTracingAccelerationStructure>& accelerationStructure)
        {
            AZ_Assert(RHI::CheckBitsAll(GetDescriptor().m_bindFlags, RHI::BufferBindFlags::RayTracingAccelerationStructure),
                "SetNativeAccelerationStructure() is only valid for buffers with the RayTracingAccelerationStructure bind flag");

            m_nativeAccelerationStructure = accelerationStructure;
        }

        VkSharingMode Buffer::GetSharingMode() const
        {
            if (!m_memoryView.GetAllocation())
            {
                return VK_SHARING_MODE_MAX_ENUM;
            }

            return m_memoryView.GetAllocation()->GetSharingMode();
        }
    }
}
