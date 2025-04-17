/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Buffer.h>
#include <RHI/BufferView.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/ReleaseContainer.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<BufferView> BufferView::Create()
        {
            return aznew BufferView();
        }

        VkBufferView BufferView::GetNativeTexelBufferView() const
        {
            AZ_Assert(m_nativeBufferView != VK_NULL_HANDLE, "Vulkan buffer view is null");
            return m_nativeBufferView;
        }

        void BufferView::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && m_nativeBufferView != VK_NULL_HANDLE && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeBufferView), name.data(), VK_OBJECT_TYPE_BUFFER_VIEW, static_cast<Device&>(GetDevice()));
            }
        }

        RHI::ResultCode BufferView::InitInternal(RHI::Device& deviceBase, const RHI::DeviceResource& resourceBase)
        {
            auto& device = static_cast<Device&>(deviceBase);
            const Buffer& buffer = static_cast<const Buffer&>(resourceBase);
            const RHI::BufferViewDescriptor& viewDescriptor = GetDescriptor();
            bool hasOverrideFlags = viewDescriptor.m_overrideBindFlags != RHI::BufferBindFlags::None;
            const RHI::BufferBindFlags bindFlags = hasOverrideFlags ? viewDescriptor.m_overrideBindFlags : buffer.GetDescriptor().m_bindFlags;
            DeviceObject::Init(deviceBase);

#if defined(AZ_RHI_ENABLE_VALIDATION)
            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::Constant))
            {
                AZ_Assert(RHI::IsAligned(viewDescriptor.m_elementSize * viewDescriptor.m_elementOffset, device.GetLimits().m_minConstantBufferViewOffset),
                    "Uniform Buffer View has to be aligned to a multiple of %d bytes.", device.GetLimits().m_minConstantBufferViewOffset);
            }
#endif

            // Vulkan BufferViews are used to enable shaders to access buffer contents interpreted as formatted data.
            bool shaderRead = RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::ShaderRead);
            bool shaderReadWrite = RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::ShaderWrite);
            if (viewDescriptor.m_elementFormat != RHI::Format::Unknown && (shaderRead || shaderReadWrite))
            {
#if defined(AZ_RHI_ENABLE_VALIDATION)
                AZ_Assert(
                    RHI::IsAligned(
                        viewDescriptor.m_elementOffset * viewDescriptor.m_elementSize,
                        device.GetLimits().m_minTexelBufferOffsetAlignment),
                    "Typed Buffer View has to be aligned to a multiple of %d bytes.",
                    device.GetLimits().m_minTexelBufferOffsetAlignment);
#endif
                auto result = BuildNativeBufferView(device, buffer, viewDescriptor);

                if (device.GetBindlessDescriptorPool().IsInitialized())
                {
                    if (shaderRead)
                    {
                        m_readIndex = device.GetBindlessDescriptorPool().AttachReadBuffer(this);
                    }

                    if (shaderReadWrite)
                    {
                        m_readWriteIndex = device.GetBindlessDescriptorPool().AttachReadWriteBuffer(this);
                    }
                }
                RETURN_RESULT_IF_UNSUCCESSFUL(result);
            }
            else if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::RayTracingAccelerationStructure))
            {
                m_nativeAccelerationStructure = buffer.GetNativeAccelerationStructure();
            }

            SetName(GetName());
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode BufferView::InvalidateInternal()
        {
            ReleaseView();
            RHI::ResultCode initResult = InitInternal(GetDevice(), GetResource());
            if (initResult != RHI::ResultCode::Success)
            {
                ReleaseBindlessIndices();
            }
            return initResult;
        }

        void BufferView::ReleaseView()
        {
            if (m_nativeBufferView != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.QueueForRelease(new ReleaseContainer<VkBufferView>(
                    device.GetNativeDevice(), m_nativeBufferView, device.GetContext().DestroyBufferView));
                m_nativeBufferView = VK_NULL_HANDLE;
            }
        }

        void BufferView::ReleaseBindlessIndices()
        {
            auto& device = static_cast<Device&>(GetDevice());
            if (device.GetBindlessDescriptorPool().IsInitialized())
            {
                if (m_readIndex != InvalidBindlessIndex)
                {
                    device.GetBindlessDescriptorPool().DetachReadBuffer(m_readIndex);
                    m_readIndex = InvalidBindlessIndex;
                }

                if (m_readWriteIndex != InvalidBindlessIndex)
                {
                    device.GetBindlessDescriptorPool().DetachReadWriteBuffer(m_readWriteIndex);
                    m_readWriteIndex = InvalidBindlessIndex;
                }
            }
        }

        void BufferView::ShutdownInternal()
        {
            ReleaseView();
            ReleaseBindlessIndices();
        }

        RHI::ResultCode BufferView::BuildNativeBufferView(Device& device, const Buffer& buffer, const RHI::BufferViewDescriptor& descriptor)
        {
            const BufferMemoryView* bufferMemoryView = buffer.GetBufferMemoryView();

            VkBufferViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.buffer = bufferMemoryView->GetNativeBuffer();
            createInfo.format = ConvertFormat(descriptor.m_elementFormat);
            createInfo.offset = bufferMemoryView->GetOffset() + descriptor.m_elementOffset * descriptor.m_elementSize;
            createInfo.range = descriptor.m_elementCount * descriptor.m_elementSize;

            const VkResult result =
                device.GetContext().CreateBufferView(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeBufferView);
            AssertSuccess(result);

            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            return RHI::ResultCode::Success;
        }

        VkAccelerationStructureKHR BufferView::GetNativeAccelerationStructure() const
        {
            bool hasOverrideFlags = GetDescriptor().m_overrideBindFlags != RHI::BufferBindFlags::None;
            [[maybe_unused]] const RHI::BufferBindFlags bindFlags = hasOverrideFlags ? GetDescriptor().m_overrideBindFlags : GetBuffer().GetDescriptor().m_bindFlags;

            AZ_Assert(RHI::CheckBitsAll(bindFlags, RHI::BufferBindFlags::RayTracingAccelerationStructure),
                "GetNativeAccelerationStructure() is only valid for buffers with the RayTracingAccelerationStructure bind flag");

            return m_nativeAccelerationStructure;
        }

        uint32_t BufferView::GetBindlessReadIndex() const
        {
            return m_readIndex;
        }

        uint32_t BufferView::GetBindlessReadWriteIndex() const
        {
            return m_readWriteIndex;
        }

    } // namespace Vulkan
} // namespace AZ
