/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/IndirectBufferLayout.h>
#include <Atom/RHI/DeviceDispatchRaysItem.h>
#include <Atom/RHI/DeviceIndirectBufferSignature.h>
#include <Atom/RHI/DeviceRayTracingCompactionQueryPool.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/lock.h>
#include <RHI/Buffer.h>
#include <RHI/BufferView.h>
#include <RHI/CommandList.h>
#include <RHI/CommandPool.h>
#include <RHI/Conversion.h>
#include <RHI/DescriptorSet.h>
#include <RHI/Device.h>
#include <RHI/Fence.h>
#include <RHI/Framebuffer.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/MergedShaderResourceGroup.h>
#include <RHI/MergedShaderResourceGroupPool.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineLibrary.h>
#include <RHI/PipelineState.h>
#include <RHI/Query.h>
#include <RHI/QueryPool.h>
#include <RHI/RayTracingAccelerationStructure.h>
#include <RHI/RayTracingBlas.h>
#include <RHI/RayTracingCompactionQueryPool.h>
#include <RHI/RayTracingPipelineState.h>
#include <RHI/RayTracingShaderTable.h>
#include <RHI/RayTracingTlas.h>
#include <RHI/RenderPass.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/SwapChain.h>


namespace AZ
{
    namespace Vulkan
    {
        static const RHI::Interval InvalidInterval = RHI::Interval(std::numeric_limits<uint32_t>::max(), 0);

        RHI::Ptr<CommandList> CommandList::Create()
        {
            return aznew CommandList();
        }

        RHI::ResultCode CommandList::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
            AZ_Assert(descriptor.m_device, "Device is null.");
            Device& device = *descriptor.m_device;
            DeviceObject::Init(device);

            const RHI::ResultCode result = BuildNativeCommandBuffer();
            Reset();
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            m_supportsPredication = physicalDevice.IsFeatureSupported(DeviceFeature::Predication);
            m_supportsDrawIndirectCount = physicalDevice.IsFeatureSupported(DeviceFeature::DrawIndirectCount);
            return result;
        }

        void CommandList::Reset()
        {
            // We don't reset the VkCommandBuffer because we reset the complete Command pool
            m_state = State();
            m_isUpdating = false;
        }

        VkCommandBuffer CommandList::GetNativeCommandBuffer() const
        {
            return m_nativeCommandBuffer;
        }

        void CommandList::SetViewports(const RHI::Viewport* rhiViewports, uint32_t count) 
        {
            m_state.m_viewportState.Set(AZStd::span<const RHI::Viewport>(rhiViewports, count));
        }

        void CommandList::SetScissors(const RHI::Scissor* rhiScissors, uint32_t count) 
        {
            m_state.m_scissorState.Set(AZStd::span<const RHI::Scissor>(rhiScissors, count));
        }

        void CommandList::SetShaderResourceGroupForDraw(const RHI::DeviceShaderResourceGroup& shaderResourceGroup)
        {
            SetShaderResourceGroup(shaderResourceGroup, RHI::PipelineStateType::Draw);
        }

        void CommandList::SetShaderResourceGroupForDispatch(const RHI::DeviceShaderResourceGroup& shaderResourceGroup) 
        {
            SetShaderResourceGroup(shaderResourceGroup, RHI::PipelineStateType::Dispatch);
        }

        void CommandList::Submit(const RHI::DeviceCopyItem& copyItem, uint32_t submitIndex)
        {
            ValidateSubmitIndex(submitIndex);

            const auto& context = static_cast<Device&>(GetDevice()).GetContext();

            switch (copyItem.m_type)
            {
            case RHI::CopyItemType::Buffer:
            {
                const RHI::DeviceCopyBufferDescriptor& descriptor = copyItem.m_buffer;
                const auto* sourceBufferMemoryView = static_cast<const Buffer*>(descriptor.m_sourceBuffer)->GetBufferMemoryView();
                const auto* destinationBufferMemoryView = static_cast<const Buffer*>(descriptor.m_destinationBuffer)->GetBufferMemoryView();

                VkBufferCopy copy{};
                copy.srcOffset = sourceBufferMemoryView->GetOffset() + descriptor.m_sourceOffset;
                copy.dstOffset = destinationBufferMemoryView->GetOffset() + descriptor.m_destinationOffset;
                copy.size = descriptor.m_size;

                context.CmdCopyBuffer(
                    m_nativeCommandBuffer,
                    sourceBufferMemoryView->GetNativeBuffer(),
                    destinationBufferMemoryView->GetNativeBuffer(),
                    1,
                    &copy);
                break;
            }
            case RHI::CopyItemType::BufferToImage:
            {
                const RHI::DeviceCopyBufferToImageDescriptor& descriptor = copyItem.m_bufferToImage;
                const auto* sourceBufferMemoryView = static_cast<const Buffer*>(descriptor.m_sourceBuffer)->GetBufferMemoryView();
                const auto* destinationImage = static_cast<const Image*>(descriptor.m_destinationImage);
                const RHI::Format format = descriptor.m_sourceFormat;
                RHI::Size formatDimensionAlignment = GetFormatDimensionAlignment(format);

                // VkBufferImageCopy::bufferRowLength is specified in texels not in bytes. 
                // Because of this we need to convert m_sourceBytesPerRow from bytes to pixels to account 
                // for any padding at the end of row. 
                // This only works if the padding is a multiple of the size of a texel. 
                // This appears to be an imposition from Vulkan (maybe this help the driver copy the data more efficiently).
                AZ_Assert(descriptor.m_sourceBytesPerRow % GetFormatSize(format) == 0, "Source byte-size per row has to be multiplication of the byte-size of a pixel.");

                VkBufferImageCopy copy{};
                copy.bufferOffset = sourceBufferMemoryView->GetOffset() + descriptor.m_sourceOffset;
                copy.bufferRowLength = descriptor.m_sourceBytesPerRow / GetFormatSize(format) * formatDimensionAlignment.m_width;
                copy.bufferImageHeight = RHI::AlignUp(descriptor.m_sourceSize.m_height, formatDimensionAlignment.m_height);
                copy.imageSubresource.aspectMask = ConvertImageAspect(descriptor.m_destinationSubresource.m_aspect);

                copy.imageSubresource.mipLevel = descriptor.m_destinationSubresource.m_mipSlice;
                copy.imageSubresource.baseArrayLayer = descriptor.m_destinationSubresource.m_arraySlice;
                copy.imageSubresource.layerCount = 1;
                copy.imageOffset.x = descriptor.m_destinationOrigin.m_left;
                copy.imageOffset.y = descriptor.m_destinationOrigin.m_top;
                copy.imageOffset.z = descriptor.m_destinationOrigin.m_front;
                copy.imageExtent.width = descriptor.m_sourceSize.m_width;
                copy.imageExtent.height = descriptor.m_sourceSize.m_height;
                copy.imageExtent.depth = descriptor.m_sourceSize.m_depth;

                context.CmdCopyBufferToImage(
                    m_nativeCommandBuffer,
                    sourceBufferMemoryView->GetNativeBuffer(),
                    destinationImage->GetNativeImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &copy);
                break;
            }
            case RHI::CopyItemType::Image:
            {
                const RHI::DeviceCopyImageDescriptor& descriptor = copyItem.m_image;
                const auto* sourceImage = static_cast<const Image*>(descriptor.m_sourceImage);
                const auto* destinationImage = static_cast<const Image*>(descriptor.m_destinationImage);

                VkImageCopy copy{};
                copy.srcSubresource.aspectMask = ConvertImageAspect(descriptor.m_sourceSubresource.m_aspect);
                copy.srcSubresource.mipLevel = descriptor.m_sourceSubresource.m_mipSlice;
                copy.srcSubresource.baseArrayLayer = descriptor.m_sourceSubresource.m_arraySlice;
                copy.srcSubresource.layerCount = 1;
                copy.srcOffset.x = descriptor.m_sourceOrigin.m_left;
                copy.srcOffset.y = descriptor.m_sourceOrigin.m_top;
                copy.srcOffset.z = descriptor.m_sourceOrigin.m_front;
                copy.dstSubresource.aspectMask = ConvertImageAspect(descriptor.m_destinationSubresource.m_aspect);
                copy.dstSubresource.mipLevel = descriptor.m_destinationSubresource.m_mipSlice;
                copy.dstSubresource.baseArrayLayer = descriptor.m_destinationSubresource.m_arraySlice;
                copy.dstSubresource.layerCount = 1;
                copy.dstOffset.x = descriptor.m_destinationOrigin.m_left;
                copy.dstOffset.y = descriptor.m_destinationOrigin.m_top;
                copy.dstOffset.z = descriptor.m_destinationOrigin.m_front;
                copy.extent.width = descriptor.m_sourceSize.m_width;
                copy.extent.height = descriptor.m_sourceSize.m_height;
                copy.extent.depth = descriptor.m_sourceSize.m_depth;

                context.CmdCopyImage(
                    m_nativeCommandBuffer,
                    sourceImage->GetNativeImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    destinationImage->GetNativeImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &copy);
                break;
            }
            case RHI::CopyItemType::ImageToBuffer:
            {
                const RHI::DeviceCopyImageToBufferDescriptor& descriptor = copyItem.m_imageToBuffer;
                const auto* sourceImage = static_cast<const Image*>(descriptor.m_sourceImage);
                const auto* destinationBufferMemoryView = static_cast<const Buffer*>(descriptor.m_destinationBuffer)->GetBufferMemoryView();
                const RHI::Format format = descriptor.m_destinationFormat;
                RHI::Size formatDimensionAlignment = GetFormatDimensionAlignment(format);

                // VkBufferImageCopy::bufferRowLength is specified in texels not in bytes. 
                // Because of this we need to convert m_sourceBytesPerRow from bytes to pixels to account 
                // for any padding at the end of row. 
                // This only works if the padding is a multiple of the size of a texel. 
                // This appears to be an imposition from Vulkan (maybe this help the driver copy the data more efficiently).
                AZ_Assert(descriptor.m_destinationBytesPerRow % GetFormatSize(format) == 0, "Destination byte-size per row has to be mutliplication of the byte-size of a pixel.");

                VkBufferImageCopy copy{};
                copy.bufferOffset = destinationBufferMemoryView->GetOffset() + descriptor.m_destinationOffset;
                copy.bufferRowLength = descriptor.m_destinationBytesPerRow / GetFormatSize(format) * formatDimensionAlignment.m_width;
                copy.bufferImageHeight = RHI::AlignUp(descriptor.m_sourceSize.m_height, formatDimensionAlignment.m_height);
                copy.imageSubresource.aspectMask = ConvertImageAspect(descriptor.m_sourceSubresource.m_aspect);
                copy.imageSubresource.mipLevel = descriptor.m_sourceSubresource.m_mipSlice;
                copy.imageSubresource.baseArrayLayer = descriptor.m_sourceSubresource.m_arraySlice;
                copy.imageSubresource.layerCount = 1;
                copy.imageOffset.x = descriptor.m_sourceOrigin.m_left;
                copy.imageOffset.y = descriptor.m_sourceOrigin.m_top;
                copy.imageOffset.z = descriptor.m_sourceOrigin.m_front;
                copy.imageExtent.width = descriptor.m_sourceSize.m_width;
                copy.imageExtent.height = descriptor.m_sourceSize.m_height;
                copy.imageExtent.depth = descriptor.m_sourceSize.m_depth;

                // [GFX TODO] https://github.com/o3de/o3de/issues/16444
                // It was found that after submitting the copy command, there could occur
                // a Vulkan validation error if the Source Attachment image is later used as an SRV
                // because this CmdCopyImageToBuffer command will change and leave the layout as VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
                // The solution would be to add another MemoryBarrier to change the layout back to its original
                // state. 
                // [ UNASSIGNED-CoreValidation-DrawState-InvalidImageLayout ]
                //     (subresource: aspectMask 0x1 array layer 0, mip level 0) to be in layout VK_IMAGE_LAYOUT_GENERAL
                //     --instead, current layout is VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
                context.CmdCopyImageToBuffer(
                    m_nativeCommandBuffer,
                    sourceImage->GetNativeImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    destinationBufferMemoryView->GetNativeBuffer(),
                    1,
                    &copy);
                break;
            }
            case RHI::CopyItemType::QueryToBuffer:
            {
                const RHI::DeviceCopyQueryToBufferDescriptor& descriptor = copyItem.m_queryToBuffer;
                const auto* sourceQueryPool = static_cast<const QueryPool*>(descriptor.m_sourceQueryPool);
                const auto* destinationBufferMemoryView = static_cast<const Buffer*>(descriptor.m_destinationBuffer)->GetBufferMemoryView();

                context.CmdCopyQueryPoolResults(
                    m_nativeCommandBuffer,
                    sourceQueryPool->GetNativeQueryPool(),
                    descriptor.m_firstQuery.GetIndex(),
                    descriptor.m_queryCount,
                    destinationBufferMemoryView->GetNativeBuffer(),
                    destinationBufferMemoryView->GetOffset() + descriptor.m_destinationOffset,
                    descriptor.m_destinationStride,
                    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
                break;
            }
            default:
                AZ_Assert(false, "Invalid copy-item type.");
            }
        }

        void CommandList::Submit(const RHI::DeviceDrawItem& drawItem, uint32_t submitIndex)
        {
            ValidateSubmitIndex(submitIndex);

            if (drawItem.m_geometryView == nullptr)
            {
                AZ_Assert(false, "DrawItem being submitted without GeometryView, i.e. without draw arguments, index buffer or stream buffers!");
                return;
            }

            if (!CommitShaderResource(drawItem))
            {
                AZ_Warning("CommandList", false, "Failed to bind shader resources for draw item. Skipping.");
                return;
            }

            SetStencilRef(drawItem.m_stencilRef);
            SetStreamBuffers(*drawItem.m_geometryView, drawItem.m_streamIndices);

            RHI::CommandListScissorState scissorState;
            if (drawItem.m_scissorsCount)
            {
                scissorState = m_state.m_scissorState;
                SetScissors(drawItem.m_scissors, drawItem.m_scissorsCount);
            }

            RHI::CommandListViewportState viewportState;
            if (drawItem.m_viewportsCount)
            {
                viewportState = m_state.m_viewportState;
                SetViewports(drawItem.m_viewports, drawItem.m_viewportsCount);
            }

            CommitScissorState();
            CommitViewportState();
            CommitShadingRateState();

            const auto& context = static_cast<Device&>(GetDevice()).GetContext();

            switch (drawItem.m_geometryView->GetDrawArguments().m_type)
            {
            case RHI::DrawType::Indexed:
            {
                AZ_Assert(drawItem.m_geometryView->GetIndexBufferView().GetBuffer(), "IndexBufferView is null.");

                const RHI::DrawIndexed& indexed = drawItem.m_geometryView->GetDrawArguments().m_indexed;
                SetIndexBuffer(drawItem.m_geometryView->GetIndexBufferView());

                context.CmdDrawIndexed(
                    m_nativeCommandBuffer,
                    indexed.m_indexCount,
                    drawItem.m_drawInstanceArgs.m_instanceCount,
                    indexed.m_indexOffset,
                    indexed.m_vertexOffset,
                    drawItem.m_drawInstanceArgs.m_instanceOffset);
                break;
            }
            case RHI::DrawType::Linear:
            {
                const RHI::DrawLinear& linear = drawItem.m_geometryView->GetDrawArguments().m_linear;

                context.CmdDraw(
                    m_nativeCommandBuffer,
                    linear.m_vertexCount,
                    drawItem.m_drawInstanceArgs.m_instanceCount,
                    linear.m_vertexOffset,
                    drawItem.m_drawInstanceArgs.m_instanceOffset);
                break;
            }
            case RHI::DrawType::Indirect:
            {
                const RHI::DeviceDrawIndirect& indirect = drawItem.m_geometryView->GetDrawArguments().m_indirect;
                const RHI::IndirectBufferLayout& layout = indirect.m_indirectBufferView->GetSignature()->GetDescriptor().m_layout;
                decltype(context.CmdDrawIndexedIndirectCountKHR) drawIndirectCountFunctionPtr = nullptr;
                decltype(context.CmdDrawIndexedIndirect) drawIndirectfunctionPtr = nullptr;
                switch (layout.GetType())
                {
                case RHI::IndirectBufferLayoutType::LinearDraw:
                    drawIndirectCountFunctionPtr = context.CmdDrawIndirectCountKHR;
                    drawIndirectfunctionPtr = context.CmdDrawIndirect;
                    break;
                case RHI::IndirectBufferLayoutType::IndexedDraw:
                    SetIndexBuffer(drawItem.m_geometryView->GetIndexBufferView());
                    drawIndirectCountFunctionPtr = context.CmdDrawIndexedIndirectCountKHR;
                    drawIndirectfunctionPtr = context.CmdDrawIndexedIndirect;
                    break;
                default:
                    AZ_Assert(false, "Invalid indirect layout type %d", layout.GetType());
                    return;
                }

                const auto* indirectBufferMemoryView = static_cast<const Buffer*>(indirect.m_indirectBufferView->GetBuffer())->GetBufferMemoryView();
                VkBuffer vkIndirectBuffer = indirectBufferMemoryView->GetNativeBuffer();
                // Check if we need to support the count buffer version of the function.
                if (m_supportsDrawIndirectCount && indirect.m_countBuffer)
                {
                    const auto* counterBufferMemoryView = static_cast<const Buffer*>(indirect.m_countBuffer)->GetBufferMemoryView();
                    drawIndirectCountFunctionPtr(
                        m_nativeCommandBuffer,
                        vkIndirectBuffer,
                        indirectBufferMemoryView->GetOffset() + indirect.m_indirectBufferView->GetByteOffset() + indirect.m_indirectBufferByteOffset,
                        counterBufferMemoryView->GetNativeBuffer(),
                        counterBufferMemoryView->GetOffset() + indirect.m_countBufferByteOffset,
                        indirect.m_maxSequenceCount,
                        indirect.m_indirectBufferView->GetByteStride());
                }
                else
                {
                    AZ_Assert(indirect.m_countBuffer == nullptr, "Count buffer for indirect draw is not supported on this platform. Ignoring it.");
                    drawIndirectfunctionPtr(
                        m_nativeCommandBuffer,
                        vkIndirectBuffer,
                        indirectBufferMemoryView->GetOffset() + indirect.m_indirectBufferView->GetByteOffset() + indirect.m_indirectBufferByteOffset,
                        indirect.m_maxSequenceCount,
                        indirect.m_indirectBufferView->GetByteStride());
                }

                break;
            }
            default:
                AZ_Assert(false, "DrawType is invalid.");
                break;
            }

            // Restore the scissors if needed.
            if (scissorState.IsValid())
            {
                SetScissors(scissorState.m_states.data(), aznumeric_caster(scissorState.m_states.size()));
            }

            // Restore the viewports if needed.
            if (viewportState.IsValid())
            {
                SetViewports(viewportState.m_states.data(), aznumeric_caster(viewportState.m_states.size()));
            }
        }

        void CommandList::Submit(const RHI::DeviceDispatchItem& dispatchItem, uint32_t submitIndex)
        {
            ValidateSubmitIndex(submitIndex);

            if (!CommitShaderResource(dispatchItem))
            {
                AZ_Warning("CommandList", false, "Failed to bind shader resources for dispatch item. Skipping.");
                return;
            }

            const auto& context = static_cast<Device&>(GetDevice()).GetContext();

            switch (dispatchItem.m_arguments.m_type)
            {
            case RHI::DispatchType::Direct:
            {
                const auto& arguments = dispatchItem.m_arguments.m_direct;
                context.CmdDispatch(
                    m_nativeCommandBuffer, arguments.GetNumberOfGroupsX(), arguments.GetNumberOfGroupsY(), arguments.GetNumberOfGroupsZ());
                break;
            }
            case RHI::DispatchType::Indirect:
            {
                const auto& arguments = dispatchItem.m_arguments.m_indirect;
                AZ_Assert(arguments.m_countBuffer == nullptr, "Count buffer is not supported for indirect dispatch on this platform.");
                const auto* indirectBufferMemoryView = static_cast<const Buffer*>(arguments.m_indirectBufferView->GetBuffer())->GetBufferMemoryView();
                context.CmdDispatchIndirect(
                    m_nativeCommandBuffer, indirectBufferMemoryView->GetNativeBuffer(),
                    indirectBufferMemoryView->GetOffset() + arguments.m_indirectBufferView->GetByteOffset() +
                        arguments.m_indirectBufferByteOffset);
                break;
            }
            default:
                AZ_Assert(false, "Invalid dispatch type");
                break;
            }            
        }       

        void CommandList::Submit([[maybe_unused]] const RHI::DeviceDispatchRaysItem& dispatchRaysItem, uint32_t submitIndex)
        {
            ValidateSubmitIndex(submitIndex);

            // manually clear the Dispatch bindings
            ShaderResourceBindings& bindings = GetShaderResourceBindingsByPipelineType(RHI::PipelineStateType::Dispatch);
            for (size_t i = 0; i < bindings.m_descriptorSets.size(); ++i)
            {
                bindings.m_descriptorSets[i] = nullptr;
            }

            const auto& context = static_cast<Device&>(GetDevice()).GetContext();

            const RayTracingPipelineState* rayTracingPipelineState =
                static_cast<const RayTracingPipelineState*>(dispatchRaysItem.m_rayTracingPipelineState);
            context.CmdBindPipeline(
                m_nativeCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipelineState->GetNativePipeline());

            // bind Srgs
            AZStd::vector<VkDescriptorSet> descriptorSets;
            descriptorSets.reserve(dispatchRaysItem.m_shaderResourceGroupCount);

            AZStd::array<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> srgByAzslBindingSlot = { {} };
            for (uint32_t srgIndex = 0; srgIndex < dispatchRaysItem.m_shaderResourceGroupCount; ++srgIndex)
            {
                const ShaderResourceGroup* srg = static_cast<const ShaderResourceGroup*>(dispatchRaysItem.m_shaderResourceGroups[srgIndex]);
                srgByAzslBindingSlot[srg->GetBindingSlot()] = srg;
            }

            const PipelineState& globalPipelineState = static_cast<const PipelineState&>(*dispatchRaysItem.m_globalPipelineState);
            const PipelineLayout& globalPipelineLayout = static_cast<const PipelineLayout&>(*globalPipelineState.GetPipelineLayout());
            for (uint32_t descriptorSetIndex = 0; descriptorSetIndex < globalPipelineLayout.GetDescriptorSetLayoutCount(); ++descriptorSetIndex)
            {
                RHI::ConstPtr<ShaderResourceGroup> shaderResourceGroup;
                AZStd::fixed_vector<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> shaderResourceGroupList;
                const auto& srgBitset = globalPipelineLayout.GetAZSLBindingSlotsOfIndex(descriptorSetIndex);
                for (uint32_t bindingSlot = 0; bindingSlot < srgBitset.size(); ++bindingSlot)
                {
                    if (srgBitset[bindingSlot])
                    {
                        shaderResourceGroupList.push_back(srgByAzslBindingSlot[bindingSlot]);
                    }
                }

                // handle merged descriptor set
                if (globalPipelineLayout.IsMergedDescriptorSetLayout(descriptorSetIndex))
                {
                    MergedShaderResourceGroupPool* mergedSRGPool = globalPipelineLayout.GetMergedShaderResourceGroupPool(descriptorSetIndex);
                    AZ_Assert(mergedSRGPool, "Null MergedShaderResourceGroupPool");

                    RHI::Ptr<MergedShaderResourceGroup> mergedSRG = mergedSRGPool->FindOrCreate(shaderResourceGroupList);
                    AZ_Assert(mergedSRG, "Null MergedShaderResourceGroup");
                    if (mergedSRG->NeedsCompile())
                    {
                        mergedSRG->Compile();
                    }

                    shaderResourceGroup = mergedSRG;
                }
                else
                {
                    shaderResourceGroup = shaderResourceGroupList.front();
                }

                if (shaderResourceGroup == nullptr)
                {
                    AZ_Assert(
                        srgBitset[m_descriptor.m_device->GetBindlessDescriptorPool().GetBindlessSrgBindingSlot()],
                        "Bindless SRG slot needs to match the one described in the shader.");
                    descriptorSets.push_back(m_descriptor.m_device->GetBindlessDescriptorPool().GetNativeDescriptorSet());
                }
                else
                {
                    descriptorSets.push_back(shaderResourceGroup->GetCompiledData().GetNativeDescriptorSet());
                }
            }

            context.CmdBindDescriptorSets(
                m_nativeCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipelineState->GetNativePipelineLayout(), 0,
                aznumeric_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

            const RayTracingShaderTable* shaderTable = static_cast<const RayTracingShaderTable*>(dispatchRaysItem.m_rayTracingShaderTable);
            const RayTracingShaderTable::ShaderTableBuffers& shaderTableBuffers = shaderTable->GetBuffers();

            // ray generation table
            VkBufferDeviceAddressInfo addressInfo = {};
            addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            addressInfo.pNext = nullptr;
            addressInfo.buffer = static_cast<Buffer*>(shaderTableBuffers.m_rayGenerationTable.get())->GetBufferMemoryView()->GetNativeBuffer();
            VkDeviceAddress rayGenerationTableAddress =
                context.GetBufferDeviceAddress(m_descriptor.m_device->GetNativeDevice(), &addressInfo);

            VkStridedDeviceAddressRegionKHR rayGenerationTable = {};
            rayGenerationTable.deviceAddress = rayGenerationTableAddress;
            rayGenerationTable.stride = shaderTableBuffers.m_rayGenerationTableStride;
            rayGenerationTable.size = shaderTableBuffers.m_rayGenerationTableSize;

            // miss table
            VkDeviceAddress missTableAddress = 0;
            if (shaderTableBuffers.m_missTable)
            {
                addressInfo.buffer = static_cast<Buffer*>(shaderTableBuffers.m_missTable.get())->GetBufferMemoryView()->GetNativeBuffer();
                missTableAddress = context.GetBufferDeviceAddress(m_descriptor.m_device->GetNativeDevice(), &addressInfo);
            }

            VkStridedDeviceAddressRegionKHR missTable = {};
            missTable.deviceAddress = missTableAddress;
            missTable.stride = shaderTableBuffers.m_missTableStride;
            missTable.size = shaderTableBuffers.m_missTableSize;

            // callable table
            VkDeviceAddress callableTableAddress = 0;
            if (shaderTableBuffers.m_callableTable)
            {
                addressInfo.buffer = static_cast<Buffer*>(shaderTableBuffers.m_callableTable.get())->GetBufferMemoryView()->GetNativeBuffer();
                callableTableAddress = context.GetBufferDeviceAddress(m_descriptor.m_device->GetNativeDevice(), &addressInfo);
            }

            VkStridedDeviceAddressRegionKHR callableTable = {};
            callableTable.deviceAddress = callableTableAddress;
            callableTable.stride = shaderTableBuffers.m_callableTableStride;
            callableTable.size = shaderTableBuffers.m_callableTableSize;

            // hit group table
            addressInfo.buffer = static_cast<Buffer*>(shaderTableBuffers.m_hitGroupTable.get())->GetBufferMemoryView()->GetNativeBuffer();
            VkDeviceAddress hitGroupTableAddress = context.GetBufferDeviceAddress(m_descriptor.m_device->GetNativeDevice(), &addressInfo);

            VkStridedDeviceAddressRegionKHR hitGroupTable = {};
            hitGroupTable.deviceAddress = hitGroupTableAddress;
            hitGroupTable.stride = shaderTableBuffers.m_hitGroupTableStride;
            hitGroupTable.size = shaderTableBuffers.m_hitGroupTableSize;

            switch (dispatchRaysItem.m_arguments.m_type)
            {
            case RHI::DispatchRaysType::Direct:
                {
                    const auto& arguments = dispatchRaysItem.m_arguments.m_direct;
                    context.CmdTraceRaysKHR(
                        m_nativeCommandBuffer,
                        &rayGenerationTable,
                        &missTable,
                        &hitGroupTable,
                        &callableTable,
                        arguments.m_width,
                        arguments.m_height,
                        arguments.m_depth);
                    break;
                }
            case RHI::DispatchRaysType::Indirect:
                {
                    const auto& arguments = dispatchRaysItem.m_arguments.m_indirect;
                    AZ_Assert(arguments.m_countBuffer == nullptr, "Count buffer is not supported for indirect dispatch on this platform.");
                    const auto* indirectBufferMemoryView =
                        static_cast<const Buffer*>(arguments.m_indirectBufferView->GetBuffer())->GetBufferMemoryView();
                    addressInfo.buffer = indirectBufferMemoryView->GetNativeBuffer();
                    VkDeviceAddress indirectDeviceAddress =
                        context.GetBufferDeviceAddress(m_descriptor.m_device->GetNativeDevice(), &addressInfo) +
                        indirectBufferMemoryView->GetOffset() + arguments.m_indirectBufferView->GetByteOffset() +
                        arguments.m_indirectBufferByteOffset;
                    context.CmdTraceRaysIndirectKHR(
                        m_nativeCommandBuffer, &rayGenerationTable, &missTable, &hitGroupTable, &callableTable, indirectDeviceAddress);
                    break;
                }
            default:
                AZ_Assert(false, "Invalid dispatch type");
                break;
            }
        }

        void CommandList::BeginPredication(const RHI::DeviceBuffer& buffer, uint64_t offset, RHI::PredicationOp operation)
        {
            if (!m_supportsPredication)
            {
                AZ_Error("Vulkan", false, "Predication is not supported on this device");
                return;
            }

            const auto* bufferMemoryView = static_cast<const Buffer&>(buffer).GetBufferMemoryView();

            VkConditionalRenderingBeginInfoEXT beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
            beginInfo.buffer = bufferMemoryView->GetNativeBuffer();
            beginInfo.offset = bufferMemoryView->GetOffset() + offset;
            beginInfo.flags = operation == RHI::PredicationOp::NotEqualZero ? VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT : 0;

            static_cast<Device&>(GetDevice()).GetContext().CmdBeginConditionalRenderingEXT(m_nativeCommandBuffer, &beginInfo);
        }

        void CommandList::EndPredication()
        {
            if (!m_supportsPredication)
            {
                AZ_Error("Vulkan", false, "Predication is not supported on this device");
                return;
            }

            static_cast<Device&>(GetDevice()).GetContext().CmdEndConditionalRenderingEXT(m_nativeCommandBuffer);
        }

        void CommandList::Shutdown()
        {
            m_nativeCommandBuffer = VK_NULL_HANDLE; // do not call vkFreeCommanBuffers().
            DeviceObject::Shutdown();
        }

        void CommandList::BeginCommandBuffer(InheritanceInfo* inheritance)
        {
            Reset();
            AZ_Assert(m_descriptor.m_level == VK_COMMAND_BUFFER_LEVEL_PRIMARY || inheritance, "InheritanceInfo needed for secondary command list");
            AZ_Assert(!m_isUpdating, "Already in updating state.");
            m_isUpdating = true;

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.pNext = nullptr;
            beginInfo.flags = 0;

            VkCommandBufferInheritanceInfo inheritanceInfo = {};
            if (m_descriptor.m_level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
            {
                AZ_Assert(inheritance, "Null inheritance info");
                const RenderPass* renderPass = inheritance->m_frameBuffer ? inheritance->m_frameBuffer->GetRenderPass() : nullptr;
                inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
                inheritanceInfo.renderPass = renderPass ? renderPass->GetNativeRenderPass() : VK_NULL_HANDLE;
                inheritanceInfo.subpass = inheritance->m_subpass;
                inheritanceInfo.framebuffer =
                    inheritance->m_frameBuffer ? inheritance->m_frameBuffer->GetNativeFramebuffer() : VK_NULL_HANDLE;
                beginInfo.pInheritanceInfo = &inheritanceInfo;
                beginInfo.flags |= renderPass ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0;
                m_state.m_framebuffer = inheritance->m_frameBuffer;
                m_state.m_subpassIndex = inheritance->m_subpass;
            }
            else
            {
                beginInfo.pInheritanceInfo = nullptr;
            }

            VkResult vkResult = static_cast<Device&>(GetDevice()).GetContext().BeginCommandBuffer(m_nativeCommandBuffer, &beginInfo);
            AssertSuccess(vkResult);
        }

        void CommandList::EndCommandBuffer()
        {
            AZ_Assert(m_isUpdating, "Not in updating state");

            m_state.m_framebuffer = nullptr;
            m_state.m_subpassIndex = 0;
            AssertSuccess(static_cast<Device&>(GetDevice()).GetContext().EndCommandBuffer(m_nativeCommandBuffer));
            m_isUpdating = false;
        }

        void CommandList::BeginRenderPass(const BeginRenderPassInfo& beginInfo)
        {
            AZ_Assert(m_descriptor.m_level == VK_COMMAND_BUFFER_LEVEL_PRIMARY, "Only primary command buffer can begin a render pass");

            AZStd::vector<VkClearValue> vClearValues(beginInfo.m_clearValues.size());
            for (size_t idx = 0; idx < beginInfo.m_clearValues.size(); ++idx)
            {
                FillClearValue(beginInfo.m_clearValues[idx], vClearValues[idx]);
            }

            const RenderPass* renderpass = beginInfo.m_frameBuffer->GetRenderPass();
            VkRenderPassBeginInfo info{};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.pNext = nullptr;
            info.renderPass = renderpass->GetNativeRenderPass();
            info.framebuffer = beginInfo.m_frameBuffer->GetNativeFramebuffer();
            info.renderArea.offset.x = 0;
            info.renderArea.offset.y = 0;
            info.renderArea.extent.width = beginInfo.m_frameBuffer->GetSize().m_width;
            info.renderArea.extent.height = beginInfo.m_frameBuffer->GetSize().m_height;
            info.clearValueCount = static_cast<uint32_t>(vClearValues.size());
            info.pClearValues = vClearValues.empty() ? nullptr : vClearValues.data();

            static_cast<Device&>(GetDevice()).GetContext().CmdBeginRenderPass(m_nativeCommandBuffer, &info, beginInfo.m_subpassContentType);

            m_state.m_subpassIndex = 0;
            m_state.m_framebuffer = beginInfo.m_frameBuffer;

            // If a shading rate image is being used, we change the combinators to (Passthrough, Override) so the
            // image is actually being used (if not the default of "Passthrough, Passthrough" would just
            // ignore the shading rate attachment). If a "Per Draw" rate is used, it would need to specify the combinators.
            auto& device = static_cast<Device&>(GetDevice());
            if (RHI::CheckBitsAll(
                    device.GetFeatures().m_shadingRateTypeMask, RHI::ShadingRateTypeFlags::PerDraw | RHI::ShadingRateTypeFlags::PerRegion))
            {
                const auto& subpasses = renderpass->GetDescriptor().m_subpassDescriptors;
                auto findIt = AZStd::find_if(
                    subpasses.begin(),
                    subpasses.begin() + renderpass->GetDescriptor().m_subpassCount,
                    [](const auto& subpassDesc)
                    {
                        return subpassDesc.m_fragmentShadingRateAttachment.IsValid();
                    });

                if (findIt != subpasses.end())
                {
                    SetFragmentShadingRate(
                        RHI::ShadingRate::Rate1x1,
                        RHI::ShadingRateCombinators{ RHI::ShadingRateCombinerOp::Passthrough, RHI::ShadingRateCombinerOp::Override });
                }
                else
                {
                    SetFragmentShadingRate(
                        RHI::ShadingRate::Rate1x1,
                        RHI::ShadingRateCombinators{ RHI::ShadingRateCombinerOp::Override, RHI::ShadingRateCombinerOp::Passthrough });
                }
            }
        }

        void CommandList::NextSubpass(VkSubpassContents contents)
        {
            if (m_state.m_subpassIndex + 1 < m_state.m_framebuffer->GetRenderPass()->GetDescriptor().m_subpassCount)
            {
                static_cast<Device&>(GetDevice()).GetContext().CmdNextSubpass(m_nativeCommandBuffer, contents);
                m_state.m_subpassIndex++;
            }
        }

        void CommandList::EndRenderPass()
        {
            static_cast<Device&>(GetDevice()).GetContext().CmdEndRenderPass(m_nativeCommandBuffer);
            m_state.m_framebuffer = nullptr;
            m_state.m_subpassIndex = 0;
        }

        bool CommandList::IsInsideRenderPass() const
        {
            return !!m_state.m_framebuffer;
        }
        
        RHI::ResultCode CommandList::BuildNativeCommandBuffer()
        {
            VkCommandBufferAllocateInfo allocInfo;
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;
            allocInfo.commandPool = m_descriptor.m_commandPool->GetNativeCommandPool();
            allocInfo.level = m_descriptor.m_level;;
            allocInfo.commandBufferCount = 1;

            VkResult vkResult = static_cast<Device&>(GetDevice())
                                    .GetContext()
                                    .AllocateCommandBuffers(m_descriptor.m_device->GetNativeDevice(), &allocInfo, &m_nativeCommandBuffer);
            AssertSuccess(vkResult);
            return ConvertResult(vkResult);
        }

        void CommandList::ExecuteSecondaryCommandLists(const AZStd::span<const RHI::Ptr<CommandList>>& commands)
        {
            AZ_Assert(m_isUpdating, "Secondary command buffers must be executed between BeginCommandBuffer() and EndCommandBuffer().");
            AZ_Assert(m_descriptor.m_level == VK_COMMAND_BUFFER_LEVEL_PRIMARY, "Trying to execute commands from a secondary command list");

            AZStd::vector<VkCommandBuffer> commandBuffers;
            commandBuffers.reserve(commands.size());
            AZStd::for_each(commands.begin(), commands.end(), [&commandBuffers](const RHI::Ptr<CommandList>& cmdList)
            {
                AZ_Assert(cmdList->m_descriptor.m_level == VK_COMMAND_BUFFER_LEVEL_SECONDARY, "Trying to execute a primary command list");
                commandBuffers.push_back(cmdList->GetNativeCommandBuffer());
            });

            static_cast<Device&>(GetDevice())
                .GetContext()
                .CmdExecuteCommands(m_nativeCommandBuffer, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        }

        uint32_t CommandList::GetQueueFamilyIndex() const
        {
            return m_descriptor.m_commandPool->GetDescriptor().m_queueFamilyIndex;
        }

        void CommandList::BeginDebugLabel(const char* label, const AZ::Color color)
        {
            Debug::BeginCmdDebugLabel(m_descriptor.m_device->GetContext(), m_nativeCommandBuffer, label, color);
        }

        void CommandList::EndDebugLabel()
        {
            Debug::EndCmdDebugLabel(m_descriptor.m_device->GetContext(), m_nativeCommandBuffer);
        }

        RHI::CommandListValidator& CommandList::GetValidator()
        {
            return m_validator;
        }

        void CommandList::SetShaderResourceGroup(const RHI::DeviceShaderResourceGroup& shaderResourceGroupBase, RHI::PipelineStateType type)
        {
            const uint32_t bindingSlot = shaderResourceGroupBase.GetBindingSlot();
            const auto& shaderResourceGroup = static_cast<const ShaderResourceGroup&>(shaderResourceGroupBase);
            auto& bindings = m_state.m_bindingsByPipe[static_cast<uint32_t>(type)];
            if (bindings.m_SRGByAzslBindingSlot[bindingSlot] != &shaderResourceGroup)
            {
                bindings.m_SRGByAzslBindingSlot[bindingSlot] = &shaderResourceGroup;
                bindings.m_dirtyShaderResourceGroupFlags.set(bindingSlot);
            }
        }

        void CommandList::SetStreamBuffers(const RHI::DeviceGeometryView& geometryView, const RHI::StreamBufferIndices& streamIndices)
        {
            auto streamIter = geometryView.CreateStreamIterator(streamIndices);
            RHI::Interval interval = InvalidInterval;

            for (u8 index = 0; !streamIter.HasEnded(); ++streamIter, ++index)
            {
                if (m_state.m_streamBufferHashes[index] != static_cast<uint64_t>(streamIter->GetHash()))
                {
                    m_state.m_streamBufferHashes[index] = static_cast<uint64_t>(streamIter->GetHash());
                    interval.m_min = AZStd::min<uint32_t>(interval.m_min, index);
                    interval.m_max = AZStd::max<uint32_t>(interval.m_max, index);
                }
            }

            if (interval != InvalidInterval)
            {
                uint32_t numBuffers = interval.m_max - interval.m_min + 1;
                AZStd::fixed_vector<VkBuffer, RHI::Limits::Pipeline::StreamCountMax> nativeBuffers;
                AZStd::fixed_vector<VkDeviceSize, RHI::Limits::Pipeline::StreamCountMax> offsets;

                for (u8 i = (u8)interval.m_min; i <= (u8)interval.m_max; ++i)
                {
                    if (streamIter[i].GetBuffer())
                    {
                        const auto* bufferMemoryView = static_cast<const Buffer*>(streamIter[i].GetBuffer())->GetBufferMemoryView();
                        nativeBuffers.push_back(bufferMemoryView->GetNativeBuffer());
                        offsets.push_back(bufferMemoryView->GetOffset() + streamIter[i].GetByteOffset());
                    }
                    else
                    {
                        nativeBuffers.push_back(VK_NULL_HANDLE);
                        offsets.push_back(0);
                    }
                }

                static_cast<Device&>(GetDevice())
                    .GetContext()
                    .CmdBindVertexBuffers(m_nativeCommandBuffer, interval.m_min, numBuffers, nativeBuffers.data(), offsets.data());
            }
        }

        void CommandList::SetIndexBuffer(const RHI::DeviceIndexBufferView& indexBufferView)
        {
            uint64_t indexBufferHash = static_cast<uint64_t>(indexBufferView.GetHash());
            if (indexBufferHash != m_state.m_indexBufferHash)
            {
                m_state.m_indexBufferHash = indexBufferHash;
                const BufferMemoryView* indexBufferMemoryView = static_cast<const Buffer*>(indexBufferView.GetBuffer())->GetBufferMemoryView();
                AZ_Assert(indexBufferMemoryView, "IndexBufferMemoryView is null.");

                static_cast<Device&>(GetDevice())
                    .GetContext()
                    .CmdBindIndexBuffer(
                        m_nativeCommandBuffer,
                        indexBufferMemoryView->GetNativeBuffer(),
                        indexBufferMemoryView->GetOffset() + indexBufferView.GetByteOffset(),
                        ConvertIndexBufferFormat(indexBufferView.GetIndexFormat()));
            }
        }

        void CommandList::SetStencilRef(uint8_t stencilRef)
        {
            static_cast<Device&>(GetDevice())
                .GetContext()
                .CmdSetStencilReference(m_nativeCommandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, aznumeric_cast<uint32_t>(stencilRef));
        }

        void CommandList::BindPipeline(const PipelineState* pipelineState)
        {
            ShaderResourceBindings& bindings = GetShaderResourceBindingsByPipelineType(pipelineState->GetType());
            if (bindings.m_pipelineState != pipelineState)
            {
                if (bindings.m_pipelineState && pipelineState)
                {
                    auto newPipelineLayoutHash = pipelineState->GetPipelineLayout()->GetPipelineLayoutDescriptor().GetHash();
                    auto oldPipelineLayoutHash = bindings.m_pipelineState->GetPipelineLayout()->GetPipelineLayoutDescriptor().GetHash();
                    // [ATOM-4879] If the PipelineLayout is different, we reset all descriptor sets to force that
                    // they are bind again. We could improve this by only binding again the necessary descriptor sets (see Pipeline Layout Compatibility in the standard).
                    if (newPipelineLayoutHash != oldPipelineLayoutHash)
                    {
                        bindings.m_descriptorSets.fill(VK_NULL_HANDLE);
                    }
                }

                bindings.m_pipelineState = pipelineState;

                Pipeline* pipeline = pipelineState->GetPipeline();
                static_cast<Device&>(GetDevice())
                    .GetContext()
                    .CmdBindPipeline(m_nativeCommandBuffer, GetPipelineBindPoint(*pipelineState), pipeline->GetNativePipeline());

                // Dirty all shader resource groups so they can be validate with the new pipeline.
                bindings.m_dirtyShaderResourceGroupFlags.set();
            }
        }

        void CommandList::CommitViewportState()
        {
            if (!m_state.m_viewportState.m_isDirty)
            {
                return;
            }

            const auto& rhiViewports = m_state.m_viewportState.m_states;
            AZStd::vector<VkViewport> vulkanViewports(rhiViewports.size());
            for (uint32_t index = 0; index < vulkanViewports.size(); ++index)
            {
                const RHI::Viewport& rvp = rhiViewports[index];
                VkViewport& vvp = vulkanViewports[index];
                vvp.x = rvp.m_minX;
                vvp.y = rvp.m_minY;
                vvp.width = rvp.m_maxX - rvp.m_minX;
                vvp.height = rvp.m_maxY - rvp.m_minY;
                vvp.minDepth = rvp.m_minZ;
                vvp.maxDepth = rvp.m_maxZ;
            }

            static_cast<Device&>(GetDevice())
                .GetContext()
                .CmdSetViewport(m_nativeCommandBuffer, 0, aznumeric_caster(vulkanViewports.size()), &vulkanViewports[0]);
            m_state.m_viewportState.m_isDirty = false;
        }

        void CommandList::CommitScissorState()
        {
            if (!m_state.m_scissorState.m_isDirty)
            {
                return;
            }

            const auto& rhiScissors = m_state.m_scissorState.m_states;

            AZStd::vector<VkRect2D> vulkanScissors(rhiScissors.size());
            for (uint32_t index = 0; index < vulkanScissors.size(); ++index)
            {
                const RHI::Scissor& rsc = rhiScissors[index];
                VkRect2D& vsc = vulkanScissors[index];
                vsc.offset.x = rsc.m_minX;
                vsc.offset.y = rsc.m_minY;
                vsc.extent.width = rsc.m_maxX - rsc.m_minX;
                vsc.extent.height = rsc.m_maxY - rsc.m_minY;
            }

            static_cast<Device&>(GetDevice())
                .GetContext()
                .CmdSetScissor(m_nativeCommandBuffer, 0, aznumeric_caster(vulkanScissors.size()), &vulkanScissors[0]);
            m_state.m_scissorState.m_isDirty = false;
        }

        void CommandList::CommitShadingRateState()
        {
            if (!m_state.m_shadingRateState.m_isDirty)
            {
                return;
            }

            auto& device = static_cast<Device&>(GetDevice());
            AZ_Assert(
                RHI::CheckBitsAll(device.GetFeatures().m_shadingRateTypeMask, RHI::ShadingRateTypeFlags::PerDraw),
                "PerDraw shading rate is not supported on this platform");

            VkExtent2D vkFragmentSize = ConvertFragmentShadingRate(m_state.m_shadingRateState.m_shadingRate);
            AZStd::array<VkFragmentShadingRateCombinerOpKHR, RHI::ShadingRateCombinators::array_size> vkCombinators;
            for (int i = 0; i < m_state.m_shadingRateState.m_shadingRateCombinators.size(); ++i)
            {
                vkCombinators[i] = ConvertShadingRateCombiner(m_state.m_shadingRateState.m_shadingRateCombinators[i]);
            }

            device
                .GetContext()
                .CmdSetFragmentShadingRateKHR(m_nativeCommandBuffer, &vkFragmentSize, vkCombinators.data());
            m_state.m_shadingRateState.m_isDirty = false;
        }

        void CommandList::CommitShaderResourcePushConstants(VkPipelineLayout pipelineLayout, uint8_t rootConstantSize, const uint8_t *rootConstants)
        {
            static_cast<Device&>(GetDevice())
                .GetContext()
                .CmdPushConstants(
                    m_nativeCommandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, rootConstantSize, rootConstants);
        }

        void CommandList::CommitDescriptorSets(RHI::PipelineStateType type)
        {
            ShaderResourceBindings& bindings = GetShaderResourceBindingsByPipelineType(type);
            const PipelineState& pipelineState = *bindings.m_pipelineState;
            const PipelineLayout& pipelineLayout = *pipelineState.GetPipelineLayout();
            RHI::Interval interval = InvalidInterval;
            for (uint32_t index = 0; index < pipelineLayout.GetDescriptorSetLayoutCount(); ++index)
            {
                RHI::ConstPtr<ShaderResourceGroup> shaderResourceGroup;
                const auto& srgBitset = pipelineLayout.GetAZSLBindingSlotsOfIndex(index);
                AZStd::fixed_vector<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> shaderResourceGroupList;
                // Collect all the SRGs that are part of this descriptor set. They could be more than
                // 1, so we would need to merge their values before committing the descriptor set.
                for (uint32_t bindingSlot = 0; bindingSlot < srgBitset.size(); ++bindingSlot)
                {
                    if (srgBitset[bindingSlot])
                    {
                        shaderResourceGroupList.push_back(bindings.m_SRGByAzslBindingSlot[bindingSlot]);
                    }
                }

                // Check if this is a merged descriptor set.
                if (pipelineLayout.IsMergedDescriptorSetLayout(index))
                {
                    // Get the MergedShaderResourceGroup
                    MergedShaderResourceGroupPool* mergedSRGPool = pipelineLayout.GetMergedShaderResourceGroupPool(index);
                    AZ_Assert(mergedSRGPool, "Null MergedShaderResourceGroupPool");                    

                    RHI::Ptr<MergedShaderResourceGroup> mergedSRG = mergedSRGPool->FindOrCreate(shaderResourceGroupList);
                    AZ_Assert(mergedSRG, "Null MergedShaderResourceGroup");
                    if (mergedSRG->NeedsCompile())
                    {
                        mergedSRG->Compile();
                    }
                    shaderResourceGroup = mergedSRG;
                }
                else
                {
                    shaderResourceGroup = shaderResourceGroupList.front();
                }

                bindings.m_SRGByVulkanBindingIndex[index] = shaderResourceGroup.get();

                VkDescriptorSet vkDescriptorSet = nullptr;

                if (shaderResourceGroup == nullptr)
                {
                    AZ_Assert(
                        srgBitset[m_descriptor.m_device->GetBindlessDescriptorPool().GetBindlessSrgBindingSlot()],
                        "Bindless SRG slot needs to match the one described in the shader.");
                    vkDescriptorSet = m_descriptor.m_device->GetBindlessDescriptorPool().GetNativeDescriptorSet();
                }
                else
                {
                    auto& compiledData = shaderResourceGroup->GetCompiledData();
                    vkDescriptorSet = compiledData.GetNativeDescriptorSet();
                }

                if (bindings.m_descriptorSets[index] != vkDescriptorSet)
                {
                    bindings.m_descriptorSets[index] = vkDescriptorSet;
                    interval.m_max = AZStd::max<uint32_t>(interval.m_max, index);
                    interval.m_min = AZStd::min<uint32_t>(interval.m_min, index);
                }
            }

            if (interval != InvalidInterval)
            {
                AZ_Assert(!bindings.m_descriptorSets.empty(), "No DescriptorSet.");
                static_cast<Device&>(GetDevice())
                    .GetContext()
                    .CmdBindDescriptorSets(
                        m_nativeCommandBuffer, GetPipelineBindPoint(pipelineState), pipelineLayout.GetNativePipelineLayout(),
                        interval.m_min, interval.m_max - interval.m_min + 1, bindings.m_descriptorSets.data() + interval.m_min, 0, nullptr);
            }
        }

        CommandList::ShaderResourceBindings& CommandList::GetShaderResourceBindingsByPipelineType(RHI::PipelineStateType type)
        {
            return m_state.m_bindingsByPipe[static_cast<uint32_t>(type)];
        }

        VkPipelineBindPoint CommandList::GetPipelineBindPoint(const PipelineState& pipelineState) const
        {
            switch (pipelineState.GetType())
            {
            case RHI::PipelineStateType::Draw:
                return VK_PIPELINE_BIND_POINT_GRAPHICS;
            case RHI::PipelineStateType::Dispatch:
                return VK_PIPELINE_BIND_POINT_COMPUTE;
            default:
                AZ_Assert(false, "Invalid Pipeline State Type");
                return VkPipelineBindPoint();
            }
        }

        const Framebuffer* CommandList::GetActiveFramebuffer() const
        {
            return m_state.m_framebuffer;
        }

        const RenderPass* CommandList::GetActiveRenderpass() const
        {
            if (m_state.m_framebuffer)
            {
                return m_state.m_framebuffer->GetRenderPass();
            }

            return nullptr;
        }
        
        void CommandList::ValidateShaderResourceGroups([[maybe_unused]] RHI::PipelineStateType type) const
        {
#if defined (AZ_RHI_ENABLE_VALIDATION)
            auto& bindings = m_state.m_bindingsByPipe[static_cast<uint32_t>(type)];
            const PipelineLayout* pipelineLayout = bindings.m_pipelineState->GetPipelineLayout();
            const RHI::PipelineLayoutDescriptor& pipelineLayoutDescriptor = pipelineLayout->GetPipelineLayoutDescriptor();
            for (uint32_t i = 0; i < pipelineLayout->GetDescriptorSetLayoutCount(); ++i)
            {
                const auto& srgBitset = pipelineLayout->GetAZSLBindingSlotsOfIndex(i);
                for (uint32_t bindingSlot = 0; bindingSlot < srgBitset.size(); ++bindingSlot)
                {
                    if (srgBitset[bindingSlot] && bindingSlot != m_descriptor.m_device->GetBindlessDescriptorPool().GetBindlessSrgBindingSlot())
                    {
                        const ShaderResourceGroup* shaderResourceGroup = bindings.m_SRGByAzslBindingSlot[bindingSlot];
                        AZ_Assert(shaderResourceGroup != nullptr, "NULL srg bound");
                        m_validator.ValidateShaderResourceGroup(*shaderResourceGroup, pipelineLayoutDescriptor.GetShaderResourceGroupBindingInfo(i));
                    }
                }
            }
#endif
        }

        void CommandList::BuildBottomLevelAccelerationStructure([[maybe_unused]] const RHI::DeviceRayTracingBlas& rayTracingBlas)
        {
            const RayTracingBlas& vulkanRayTracingBlas = static_cast<const RayTracingBlas&>(rayTracingBlas);
            const RayTracingBlas::BlasBuffers& blasBuffers = vulkanRayTracingBlas.GetBuffers();

            const auto& context = static_cast<Device&>(GetDevice()).GetContext();

            // submit the command to build the BLAS
            const VkAccelerationStructureBuildRangeInfoKHR* rangeInfos = blasBuffers.m_rangeInfos.data();
            context.CmdBuildAccelerationStructuresKHR(GetNativeCommandBuffer(), 1, &blasBuffers.m_buildInfo, &rangeInfos);
        }

        void CommandList::UpdateBottomLevelAccelerationStructure([[maybe_unused]] const RHI::DeviceRayTracingBlas& rayTracingBlas)
        {
            const RayTracingBlas& vulkanRayTracingBlas = dynamic_cast<const RayTracingBlas&>(rayTracingBlas);
            const RayTracingBlas::BlasBuffers& blasBuffers = vulkanRayTracingBlas.GetBuffers();

            // Set the build mode to update the acceleration structure
            VkAccelerationStructureBuildGeometryInfoKHR tempBuildInfo = blasBuffers.m_buildInfo;
            tempBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
            tempBuildInfo.srcAccelerationStructure = blasBuffers.m_accelerationStructure->GetNativeAccelerationStructure();

            const auto& context = dynamic_cast<Device&>(GetDevice()).GetContext();

            // submit the command to build the BLAS
            const VkAccelerationStructureBuildRangeInfoKHR* rangeInfos = blasBuffers.m_rangeInfos.data();
            context.CmdBuildAccelerationStructuresKHR(GetNativeCommandBuffer(), 1, &tempBuildInfo, &rangeInfos);
        }

        void CommandList::QueryBlasCompactionSizes(
            const AZStd::vector<AZStd::pair<RHI::DeviceRayTracingBlas*, RHI::DeviceRayTracingCompactionQuery*>>& blasToQuery)
        {
            const auto& context = static_cast<Device&>(GetDevice()).GetContext();

            AZStd::unordered_set<RayTracingCompactionQueryPool*> usedPools;
            for (auto& [blas, compactionQuery] : blasToQuery)
            {
                usedPools.insert(static_cast<RayTracingCompactionQueryPool*>(compactionQuery->GetPool()));
            }
            for (auto pool : usedPools)
            {
                pool->ResetFreedQueries(this);
            }

            VkMemoryBarrier memoryBarrier = {};
            memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            memoryBarrier.pNext = nullptr;
            memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            context.CmdPipelineBarrier(
                GetNativeCommandBuffer(),
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                0,
                1,
                &memoryBarrier,
                0,
                nullptr,
                0,
                nullptr);

            for (auto& [blas, compactionQuery] : blasToQuery)
            {
                auto vulkanRayTracingBlas = static_cast<const RayTracingBlas*>(blas);
                auto vulkanCompactionQuery = static_cast<RayTracingCompactionQuery*>(compactionQuery);
                auto vulkanCompactionQueryPool = static_cast<RayTracingCompactionQueryPool*>(compactionQuery->GetPool());
                auto acc = vulkanRayTracingBlas->GetBuffers().m_accelerationStructure->GetNativeAccelerationStructure();

                vulkanCompactionQuery->Allocate();
                context.CmdWriteAccelerationStructuresPropertiesKHR(
                    GetNativeCommandBuffer(),
                    1,
                    &acc,
                    VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
                    vulkanCompactionQueryPool->GetNativeQueryPool(),
                    vulkanCompactionQuery->GetIndexInPool());
            }
        }

        void CommandList::CompactBottomLevelAccelerationStructure(
            const RHI::DeviceRayTracingBlas& sourceBlas, const RHI::DeviceRayTracingBlas& compactBlas)
        {
            const RayTracingBlas& vulkanSourceBlas = static_cast<const RayTracingBlas&>(sourceBlas);
            const RayTracingBlas& vulkanCompactBlas = static_cast<const RayTracingBlas&>(compactBlas);

            VkCopyAccelerationStructureInfoKHR copyInfo;
            copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
            copyInfo.pNext = nullptr;
            copyInfo.src = vulkanSourceBlas.GetBuffers().m_accelerationStructure->GetNativeAccelerationStructure();
            copyInfo.dst = vulkanCompactBlas.GetBuffers().m_accelerationStructure->GetNativeAccelerationStructure();
            copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;

            const auto& context = static_cast<Device&>(GetDevice()).GetContext();
            context.CmdCopyAccelerationStructureKHR(GetNativeCommandBuffer(), &copyInfo);
        }

        void CommandList::BuildTopLevelAccelerationStructure(
            const RHI::DeviceRayTracingTlas& rayTracingTlas, const AZStd::vector<const RHI::DeviceRayTracingBlas*>& changedBlasList)
        {
            const auto& context = static_cast<Device&>(GetDevice()).GetContext();

            VkMemoryBarrier memoryBarrier = {};
            memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            memoryBarrier.pNext = nullptr;
            memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

            if (!changedBlasList.empty())
            {
                // we need to have a barrier on VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR to ensure that the BLAS objects
                // are built prior to building the TLAS
                context.CmdPipelineBarrier(
                    GetNativeCommandBuffer(),
                    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                    0,
                    1,
                    &memoryBarrier,
                    0,
                    nullptr,
                    0,
                    nullptr);
            }

            const RayTracingTlas& vulkanRayTracingTlas = static_cast<const RayTracingTlas&>(rayTracingTlas);
            const RayTracingTlas::TlasBuffers& tlasBuffers = vulkanRayTracingTlas.GetBuffers();

            // submit the command to build the TLAS
            const VkAccelerationStructureBuildRangeInfoKHR& offsetInfo = tlasBuffers.m_offsetInfo;
            const VkAccelerationStructureBuildRangeInfoKHR* pOffsetInfo = &offsetInfo;
            context.CmdBuildAccelerationStructuresKHR(GetNativeCommandBuffer(), 1, &tlasBuffers.m_buildInfo, &pOffsetInfo);

            // we need a pipeline barrier on VK_ACCESS_ACCELERATION_STRUCTURE (both read and write) in case we are building
            // multiple TLAS objects in a command list
            context.CmdPipelineBarrier(
                GetNativeCommandBuffer(),
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                0,
                1,
                &memoryBarrier,
                0,
                nullptr,
                0,
                nullptr);
        }

        void CommandList::SetFragmentShadingRate(RHI::ShadingRate rate, const RHI::ShadingRateCombinators& combinators)
        {
            auto& device = static_cast<Device&>(GetDevice());
            if (!RHI::CheckBitsAll(device.GetFeatures().m_shadingRateTypeMask, RHI::ShadingRateTypeFlags::PerDraw))
            {
                AZ_Assert(false, "Per Draw shading rate is not supported on this platform");
                return;
            }

            AZ_Assert(
                static_cast<const PhysicalDevice&>(device.GetPhysicalDevice())
                    .IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::FragmentShadingRate),
                "VK_KHR_fragment_shading_rate is not supported on this platform");

            m_state.m_shadingRateState.Set(rate, combinators);
        }

        void CommandList::ClearImage(const ResourceClearRequest& request)
        {
            const ImageView& imageView = static_cast<const ImageView&>(*request.m_resourceView->GetDeviceResourceView(GetDevice().GetDeviceIndex()));
            const Image& image = static_cast<const Image&>(imageView.GetImage());
            const VkImageSubresourceRange& range = imageView.GetVkImageSubresourceRange();

            const auto& context = static_cast<Device&>(GetDevice()).GetContext();

            VkClearValue vkClearValue;
            FillClearValue(request.m_clearValue, vkClearValue);

            switch (request.m_clearValue.m_type)
            {
            case RHI::ClearValueType::Vector4Float:
            case RHI::ClearValueType::Vector4Uint:
                context.CmdClearColorImage(
                    m_nativeCommandBuffer, image.GetNativeImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &vkClearValue.color, 1, &range);
                break;
            case RHI::ClearValueType::DepthStencil:
                context.CmdClearDepthStencilImage(
                    m_nativeCommandBuffer,
                    image.GetNativeImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    &vkClearValue.depthStencil,
                    1,
                    &range);
                break;
            default:
                AZ_Assert(false, "Invalid clear type %d", request.m_clearValue.m_type);
                break;
            }
        }

        void CommandList::ClearBuffer(const ResourceClearRequest& request)
        {
            const BufferView& bufferView = static_cast<const BufferView&>(*request.m_resourceView->GetDeviceResourceView(GetDevice().GetDeviceIndex()));
            const RHI::BufferViewDescriptor& bufferViewDesc = bufferView.GetDescriptor();
            const Buffer& buffer = static_cast<const Buffer&>(bufferView.GetBuffer());
            union
            {
                float f;
                uint32_t i;
            } vkClearValue;

            const RHI::ClearValue& clearValue = request.m_clearValue;
            switch (clearValue.m_type)
            {
            case RHI::ClearValueType::Vector4Float:
                AZ_Warning(
                    "Vulkan",
                    AZStd::equal(clearValue.m_vector4Float.begin() + 1, clearValue.m_vector4Float.end(), clearValue.m_vector4Float.begin()),
                    "Vulkan only supports buffer clear operation using 1 float value. Using first value and ignoring the rest. Buffer %s is trying to clear with value (%.2f, %.2f, %.2f, %.2f)",
                    buffer.GetName().GetCStr(), clearValue.m_vector4Float[0], clearValue.m_vector4Float[1], clearValue.m_vector4Float[2], clearValue.m_vector4Float[3]);
                vkClearValue.f = clearValue.m_vector4Float.front();
                break;
            case RHI::ClearValueType::Vector4Uint:
                AZ_Warning(
                    "Vulkan",
                    AZStd::equal(clearValue.m_vector4Uint.begin() + 1, clearValue.m_vector4Uint.end(), clearValue.m_vector4Uint.begin()),
                    "Vulkan only supports buffer clear operation using 1 Uint value. Using first value and ignoring the rest. Buffer %s is trying to clear with value (%u, %u, %u, %u)",
                    buffer.GetName().GetCStr(), clearValue.m_vector4Uint[0], clearValue.m_vector4Uint[1], clearValue.m_vector4Uint[2], clearValue.m_vector4Uint[3]);
                vkClearValue.i = clearValue.m_vector4Uint.front();
                break;
            default:
                AZ_Assert(false, "Invalid clear type %d when calling ClearBuffer", request.m_clearValue.m_type);
                return;
            }

            // Maybe use a compute shader to support all clear value types.
            static_cast<Device&>(GetDevice())
                .GetContext()
                .CmdFillBuffer(
                    m_nativeCommandBuffer,
                    buffer.GetBufferMemoryView()->GetNativeBuffer(),
                    buffer.GetBufferMemoryView()->GetOffset() +
                        static_cast<size_t>(bufferViewDesc.m_elementOffset) * bufferViewDesc.m_elementSize,
                    buffer.GetBufferMemoryView()->GetSize(),
                    vkClearValue.i);
        }

    }
}
