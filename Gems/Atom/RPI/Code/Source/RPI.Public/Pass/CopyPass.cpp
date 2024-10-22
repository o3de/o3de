/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/ScopeProducerFunction.h>
#include <Atom/RPI.Public/Pass/CopyPass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace RPI
    {        
        // --- Creation & Initialization ---

        CopyPass::~CopyPass()
        {
            ResetInternal();
        }

        Ptr<CopyPass> CopyPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<CopyPass> pass = aznew CopyPass(descriptor);
            return pass;
        }

        CopyPass::CopyPass(const PassDescriptor& descriptor)
            : Pass(descriptor)
        {
            const CopyPassData* copyData = PassUtils::GetPassData<CopyPassData>(descriptor);

            if (copyData)
            {
                m_data = *copyData;

                if (copyData->m_useCopyQueue)
                {
                    m_hardwareQueueClass = RHI::HardwareQueueClass::Copy;
                }
            }
        }

        RHI::CopyItemType CopyPass::GetCopyItemType()
        {
            RHI::AttachmentType inputType =
                (m_inputOutputCopy ? GetInputOutputBinding(0) : GetInputBinding(0)).GetAttachment()->GetAttachmentType();
            RHI::AttachmentType outputType =
                (m_inputOutputCopy ? GetInputOutputBinding(0) : GetOutputBinding(0)).GetAttachment()->GetAttachmentType();

            RHI::CopyItemType copyType = RHI::CopyItemType::Invalid;

            if (inputType == RHI::AttachmentType::Buffer && outputType == RHI::AttachmentType::Buffer)
            {
                copyType = RHI::CopyItemType::Buffer;
            }
            else if (inputType == RHI::AttachmentType::Image && outputType == RHI::AttachmentType::Image)
            {
                copyType = RHI::CopyItemType::Image;
            }
            else if (inputType == RHI::AttachmentType::Buffer && outputType == RHI::AttachmentType::Image)
            {
                copyType = RHI::CopyItemType::BufferToImage;
            }
            else if (inputType == RHI::AttachmentType::Image && outputType == RHI::AttachmentType::Buffer)
            {
                copyType = RHI::CopyItemType::ImageToBuffer;
            }

            return copyType;
        }

        // --- Pass behavior overrides ---

        void CopyPass::BuildInternal()
        {
            m_inputOutputCopy = GetInputOutputCount() == 1 && m_data.m_sourceDeviceIndex != m_data.m_destinationDeviceIndex;

            AZ_Assert(
                (GetInputCount() == 1 && GetOutputCount() == 1) || m_inputOutputCopy,
                "CopyPass has %d inputs and %d outputs. It should have exactly one of each.",
                GetInputCount(),
                GetOutputCount());

            AZ_Assert(
                (m_attachmentBindings.size() == 2) || (m_inputOutputCopy && m_attachmentBindings.size() == 1),
                "CopyPass must have exactly 2 bindings: 1 input and 1 output. %s has %d bindings.",
                GetPathName().GetCStr(),
                m_attachmentBindings.size());

            bool sameDevice = (m_data.m_sourceDeviceIndex == -1 && m_data.m_destinationDeviceIndex == -1) ||
                m_data.m_sourceDeviceIndex == m_data.m_destinationDeviceIndex;
            AZ_Assert(
                sameDevice || (m_data.m_sourceDeviceIndex != -1 && m_data.m_destinationDeviceIndex != -1),
                "CopyPass: Either source and destination device indices must be invalid, or both must be valid");

            m_copyMode = sameDevice ? CopyMode::SameDevice : CopyMode::DifferentDevicesIntermediateHost;

            if (m_copyMode == CopyMode::SameDevice)
            {
                m_copyScopeProducerSameDevice = AZStd::make_shared<RHI::ScopeProducerFunctionNoData>(
                    RHI::ScopeId{ GetPathName() },
                    AZStd::bind(&CopyPass::SetupFrameGraphDependenciesSameDevice, this, AZStd::placeholders::_1),
                    AZStd::bind(&CopyPass::CompileResourcesSameDevice, this, AZStd::placeholders::_1),
                    AZStd::bind(&CopyPass::BuildCommandListInternalSameDevice, this, AZStd::placeholders::_1),
                    m_hardwareQueueClass,
                    m_data.m_sourceDeviceIndex);
            }
            else if (m_copyMode == CopyMode::DifferentDevicesIntermediateHost)
            {
                [[maybe_unused]] auto* device1 =
                    RHI::RHISystemInterface::Get()->GetDevice(m_data.m_sourceDeviceIndex != RHI::MultiDevice::InvalidDeviceIndex ? m_data.m_sourceDeviceIndex : RHI::MultiDevice::DefaultDeviceIndex);
                AZ_Assert(
                    device1->GetFeatures().m_signalFenceFromCPU,
                    "CopyPass: Device to device copy is only possible if all devices support signalling fences from the CPU");
                [[maybe_unused]] auto* device2 =
                    RHI::RHISystemInterface::Get()->GetDevice(m_data.m_destinationDeviceIndex != RHI::MultiDevice::InvalidDeviceIndex ? m_data.m_destinationDeviceIndex : RHI::MultiDevice::DefaultDeviceIndex);
                AZ_Assert(
                    device2->GetFeatures().m_signalFenceFromCPU,
                    "CopyPass: Device to device copy is only possible if all devices support signalling fences from the CPU");

                // Initialize #MaxFrames fences that are signaled on device 1 and perform the copy between the host staging buffers from device 1 to device 2
                for (auto& fence : m_device1SignalFence)
                {
                    fence = new RHI::Fence();
                    AZ_Assert(fence != nullptr, "CopyPass failed to create a fence");
                    [[maybe_unused]] RHI::ResultCode result = fence->Init(RHI::MultiDevice::AllDevices, RHI::FenceState::Signaled);
                    AZ_Assert(result == RHI::ResultCode::Success, "CopyPass failed to init fence");
                }

                // Initialize #MaxFrames fences that can be waited for on device 2 before data is uploaded to device 2
                for (auto& fence : m_device2WaitFence)
                {
                    fence = new RHI::Fence();
                    AZ_Assert(fence != nullptr, "CopyPass failed to create a fence");
                    [[maybe_unused]] auto result = fence->Init(RHI::MultiDevice::AllDevices, RHI::FenceState::Signaled, true);
                    AZ_Assert(result == RHI::ResultCode::Success, "CopyPass failed to init fence");
                }

                m_copyScopeProducerDeviceToHost = AZStd::make_shared<RHI::ScopeProducerFunctionNoData>(
                    RHI::ScopeId{ AZStd::string(GetPathName().GetStringView()) },
                    AZStd::bind(&CopyPass::SetupFrameGraphDependenciesDeviceToHost, this, AZStd::placeholders::_1),
                    AZStd::bind(&CopyPass::CompileResourcesDeviceToHost, this, AZStd::placeholders::_1),
                    AZStd::bind(&CopyPass::BuildCommandListInternalDeviceToHost, this, AZStd::placeholders::_1),
                    m_hardwareQueueClass,
                    m_data.m_sourceDeviceIndex);

                m_copyScopeProducerHostToDevice = AZStd::make_shared<RHI::ScopeProducerFunctionNoData>(
                    RHI::ScopeId{ AZStd::string(GetPathName().GetStringView()) + "_2" },
                    AZStd::bind(&CopyPass::SetupFrameGraphDependenciesHostToDevice, this, AZStd::placeholders::_1),
                    AZStd::bind(&CopyPass::CompileResourcesHostToDevice, this, AZStd::placeholders::_1),
                    AZStd::bind(&CopyPass::BuildCommandListInternalHostToDevice, this, AZStd::placeholders::_1),
                    m_hardwareQueueClass,
                    m_data.m_destinationDeviceIndex);

                m_copyItemDeviceToHost.clear();
                m_copyItemDeviceToHost.emplace_back();
                m_copyItemHostToDevice.clear();
                m_copyItemHostToDevice.emplace_back();
            }
            
            // Create transient attachment based on input if required
            if (m_data.m_cloneInput && !m_inputOutputCopy)
            {
                const Ptr<PassAttachment>& source = GetInputBinding(0).GetAttachment();
                Ptr<PassAttachment> dest = source->Clone();

                dest->m_lifetime = RHI::AttachmentLifetimeType::Transient;

                // Set bind flags to CopyWrite. Other bind flags will be auto-inferred by pass system
                if (dest->m_descriptor.m_type == RHI::AttachmentType::Image)
                {
                    dest->m_descriptor.m_image.m_bindFlags = RHI::ImageBindFlags::CopyWrite;
                }
                else if (dest->m_descriptor.m_type == RHI::AttachmentType::Buffer)
                {
                    dest->m_descriptor.m_buffer.m_bindFlags = RHI::BufferBindFlags::CopyWrite;
                    if (dest->m_descriptor.m_bufferView.m_elementCount == 0)
                    {
                        dest->m_descriptor.m_bufferView =
                            RHI::BufferViewDescriptor::CreateRaw(0, static_cast<uint32_t>(dest->m_descriptor.m_buffer.m_byteCount));
                    }
                }

                // Set path name for the new attachment and add it to our attachment list
                dest->ComputePathName(GetPathName());
                m_ownedAttachments.push_back(dest);

                // Set the output binding to the new attachment
                GetOutputBinding(0).SetAttachment(dest);
            }
        }

        void CopyPass::FrameBeginInternal(Pass::FramePrepareParams params)
        {
            if (m_copyMode == CopyMode::SameDevice)
            {
                params.m_frameGraphBuilder->ImportScopeProducer(*m_copyScopeProducerSameDevice);
            }
            else if (m_copyMode == CopyMode::DifferentDevicesIntermediateHost)
            {
                params.m_frameGraphBuilder->ImportScopeProducer(*m_copyScopeProducerDeviceToHost);
                params.m_frameGraphBuilder->ImportScopeProducer(*m_copyScopeProducerHostToDevice);
                m_currentBufferIndex = (m_currentBufferIndex + 1) % MaxFrames;
                m_device1SignalFence[m_currentBufferIndex]->Reset();
                m_device2WaitFence[m_currentBufferIndex]->Reset();
            }
        }

        void CopyPass::ResetInternal()
        {
            Pass::ResetInternal();
            if (m_copyMode == CopyMode::DifferentDevicesIntermediateHost)
            {
                for (auto& fence : m_device1SignalFence)
                {
                    fence
                        ->GetDeviceFence(
                            m_data.m_sourceDeviceIndex != RHI::MultiDevice::InvalidDeviceIndex ? m_data.m_sourceDeviceIndex
                                                                                               : RHI::MultiDevice::DefaultDeviceIndex)
                        ->WaitOnCpu();
                }
                for (auto& fence : m_device2WaitFence)
                {
                    fence
                        ->GetDeviceFence(
                            m_data.m_destinationDeviceIndex != RHI::MultiDevice::InvalidDeviceIndex ? m_data.m_destinationDeviceIndex
                                                                                                    : RHI::MultiDevice::DefaultDeviceIndex)
                        ->WaitOnCpu();
                }
            }
        }

        // --- Scope producer functions ---

        void CopyPass::SetupFrameGraphDependenciesSameDevice(RHI::FrameGraphInterface frameGraph)
        {
            DeclareAttachmentsToFrameGraph(frameGraph);
        }

        void CopyPass::CompileResourcesSameDevice(const RHI::FrameGraphCompileContext& context)
        {
            RHI::CopyItemType copyType = GetCopyItemType();
            switch (copyType)
            {
            case AZ::RHI::CopyItemType::Buffer:
                CopyBuffer(context);
                break;
            case AZ::RHI::CopyItemType::Image:
                CopyImage(context);
                break;
            case AZ::RHI::CopyItemType::BufferToImage:
                CopyBufferToImage(context);
                break;
            case AZ::RHI::CopyItemType::ImageToBuffer:
                CopyImageToBuffer(context);
                break;
            default:
                break;
            }
        }

        void CopyPass::BuildCommandListInternalSameDevice(const RHI::FrameGraphExecuteContext& context)
        {
            if (m_copyItemSameDevice.m_type != RHI::CopyItemType::Invalid)
            {
                context.GetCommandList()->Submit(m_copyItemSameDevice.GetDeviceCopyItem(context.GetDeviceIndex()));
            }
        }

        void CopyPass::SetupFrameGraphDependenciesDeviceToHost(RHI::FrameGraphInterface frameGraph)
        {
            if (m_inputOutputCopy)
            {
                // We need to manually do what DeclareAttachmentsToFrameGraph in order to correctly set the ScopeAttachmentAccess to Read
                // since we only read within this scope.
                const auto& attachmentBinding = m_attachmentBindings[0];
                if (attachmentBinding.GetAttachment() != nullptr &&
                    frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentBinding.GetAttachment()->GetAttachmentId()))
                {
                    switch (attachmentBinding.m_unifiedScopeDesc.GetType())
                    {
                    case RHI::AttachmentType::Image:
                        {
                            frameGraph.UseAttachment(
                                attachmentBinding.m_unifiedScopeDesc.GetAsImage(),
                                RHI::ScopeAttachmentAccess::Read,
                                attachmentBinding.m_scopeAttachmentUsage,
                                attachmentBinding.m_scopeAttachmentStage);
                            break;
                        }
                    case RHI::AttachmentType::Buffer:
                        {
                            frameGraph.UseAttachment(
                                attachmentBinding.m_unifiedScopeDesc.GetAsBuffer(),
                                RHI::ScopeAttachmentAccess::Read,
                                attachmentBinding.m_scopeAttachmentUsage,
                                attachmentBinding.m_scopeAttachmentStage);
                            break;
                        }
                    default:
                        AZ_Assert(false, "Error, trying to bind an attachment that is neither an image nor a buffer!");
                        break;
                    }
                }
            }
            else
            {
                // We need the size of the output image when copying from image to image, so we need all attachments (even the output ones)
                // We also need it so the framegraph knows the two scopes depend on each other
                DeclareAttachmentsToFrameGraph(frameGraph);
            }

            frameGraph.SetEstimatedItemCount(2);
            frameGraph.SignalFence(*m_device1SignalFence[m_currentBufferIndex]);
        }

        void CopyPass::CompileResourcesDeviceToHost(const RHI::FrameGraphCompileContext& context)
        {
            RHI::CopyItemType copyType = GetCopyItemType();
            auto inputId = (m_inputOutputCopy ? GetInputOutputBinding(0) : GetInputBinding(0)).GetAttachment()->GetAttachmentId();
            switch (copyType)
            {
            case AZ::RHI::CopyItemType::Image:
                [[fallthrough]];
            case AZ::RHI::CopyItemType::ImageToBuffer:
                {
                    const auto* sourceImage = context.GetImage(inputId);
                    if (!sourceImage)
                    {
                        AZ_Warning("CopyPass", false, "Failed to find attachment image %s for copy to buffer", inputId.GetCStr());
                        return;
                    }
                    const auto& sourceImageDescriptor = sourceImage->GetDescriptor();
                    const uint16_t sourceMipSlice = m_data.m_imageSourceSubresource.m_mipSlice;
                    RHI::ImageSubresourceRange sourceRange(sourceMipSlice, sourceMipSlice, 0, 0);
                    RHI::ImageAspectFlags sourceImageAspectFlags = RHI::GetImageAspectFlags(sourceImageDescriptor.m_format);
                    uint32_t sourceImageAspect = az_ctz_u32(static_cast<uint32_t>(sourceImageAspectFlags));

                    auto aspectCount{ static_cast<int>(az_popcnt_u32(static_cast<uint32_t>(sourceImageAspectFlags))) };

                    AZ_Assert(
                        copyType == AZ::RHI::CopyItemType::Image || aspectCount == 1,
                        "CopyPass cannot copy %d image aspects into a buffer.",
                        aspectCount);

                    AZ_Assert(aspectCount <= 2, "CopyPass cannot copy more than two aspects at the moment.");

                    AZStd::vector<RHI::DeviceImageSubresourceLayout> sourceImageSubResourcesLayouts;
                    sourceImageSubResourcesLayouts.resize_no_construct(sourceImageDescriptor.m_mipLevels);

                    for (auto aspectIndex{ 0 }; aspectIndex < aspectCount; ++aspectIndex)
                    {
                        while ((static_cast<uint32_t>(sourceImageAspectFlags) & AZ_BIT(sourceImageAspect)) == 0)
                        {
                            ++sourceImageAspect;
                        }

                        sourceRange.m_aspectFlags = static_cast<RHI::ImageAspectFlags>(AZ_BIT(sourceImageAspect));

                        size_t sourceTotalSizeInBytes = 0;
                        sourceImage->GetDeviceImage(m_data.m_sourceDeviceIndex)
                            ->GetSubresourceLayouts(sourceRange, sourceImageSubResourcesLayouts.data(), &sourceTotalSizeInBytes);
                        AZ::u64 sourceByteCount = sourceTotalSizeInBytes;

                        if (m_deviceHostBufferByteCount[aspectIndex][m_currentBufferIndex] != sourceByteCount)
                        {
                            m_deviceHostBufferByteCount[aspectIndex][m_currentBufferIndex] = sourceByteCount;

                            RPI::CommonBufferDescriptor desc;
                            desc.m_poolType = RPI::CommonBufferPoolType::ReadBack;
                            desc.m_bufferName =
                                AZStd::string(GetPathName().GetStringView()) + "_hostbuffer_" + AZStd::to_string(aspectIndex);
                            desc.m_byteCount = m_deviceHostBufferByteCount[aspectIndex][m_currentBufferIndex];
                            m_device1HostBuffer[aspectIndex][m_currentBufferIndex] =
                                BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

                            desc.m_bufferName =
                                AZStd::string(GetPathName().GetStringView()) + "_hostbuffer2_" + AZStd::to_string(aspectIndex);
                            desc.m_poolType = RPI::CommonBufferPoolType::Staging;
                            m_device2HostBuffer[aspectIndex][m_currentBufferIndex] =
                                BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                        }

                        // copy descriptor for copying image to buffer
                        RHI::CopyImageToBufferDescriptor copyImageToBufferDesc;
                        copyImageToBufferDesc.m_sourceImage = sourceImage;
                        copyImageToBufferDesc.m_sourceSize = sourceImageSubResourcesLayouts[sourceMipSlice].m_size;
                        copyImageToBufferDesc.m_sourceSubresource =
                            RHI::ImageSubresource(sourceMipSlice, 0 /*arraySlice*/, static_cast<RHI::ImageAspect>(sourceImageAspect));
                        copyImageToBufferDesc.m_destinationOffset = 0;

                        copyImageToBufferDesc.m_destinationBytesPerRow = sourceImageSubResourcesLayouts[sourceMipSlice].m_bytesPerRow;
                        copyImageToBufferDesc.m_destinationBytesPerImage = sourceImageSubResourcesLayouts[sourceMipSlice].m_bytesPerImage;
                        copyImageToBufferDesc.m_destinationBuffer = m_device1HostBuffer[aspectIndex][m_currentBufferIndex]->GetRHIBuffer();
                        copyImageToBufferDesc.m_destinationFormat =
                            FindFormatForAspect(sourceImageDescriptor.m_format, static_cast<RHI::ImageAspect>(sourceImageAspect));

                        ++sourceImageAspect;

                        if (m_copyItemDeviceToHost.size() <= aspectIndex)
                        {
                            m_copyItemDeviceToHost.emplace_back();
                        }
                        m_copyItemDeviceToHost[aspectIndex] = copyImageToBufferDesc;
                        m_inputImageLayouts[aspectIndex] = sourceImageSubResourcesLayouts[sourceMipSlice];
                    }
                }
                break;
            case AZ::RHI::CopyItemType::Buffer:
                [[fallthrough]];
            case AZ::RHI::CopyItemType::BufferToImage:
                {
                    const auto* buffer = context.GetBuffer(inputId);

                    if (m_deviceHostBufferByteCount[0][m_currentBufferIndex] != buffer->GetDescriptor().m_byteCount)
                    {
                        m_deviceHostBufferByteCount[0][m_currentBufferIndex] = buffer->GetDescriptor().m_byteCount;

                        RPI::CommonBufferDescriptor desc;
                        desc.m_poolType = RPI::CommonBufferPoolType::ReadBack;
                        desc.m_bufferName = AZStd::string(GetPathName().GetStringView()) + "_hostbuffer";
                        desc.m_byteCount = m_deviceHostBufferByteCount[0][m_currentBufferIndex];

                        m_device1HostBuffer[0][m_currentBufferIndex] = BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                        desc.m_poolType = RPI::CommonBufferPoolType::Staging;
                        desc.m_bufferName = AZStd::string(GetPathName().GetStringView()) + "_hostbuffer2";
                        m_device2HostBuffer[0][m_currentBufferIndex] = BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                    }

                    // copy buffer
                    RHI::CopyBufferDescriptor copyBuffer;
                    copyBuffer.m_sourceBuffer = buffer;
                    copyBuffer.m_destinationBuffer = m_device1HostBuffer[0][m_currentBufferIndex]->GetRHIBuffer();
                    copyBuffer.m_size = aznumeric_cast<uint32_t>(m_deviceHostBufferByteCount[0][m_currentBufferIndex]);

                    m_copyItemDeviceToHost[0] = copyBuffer;
                }
                break;
            default:
                break;
            }
        }

        void CopyPass::BuildCommandListInternalDeviceToHost(const RHI::FrameGraphExecuteContext& context)
        {
            for (const auto& copyItem : m_copyItemDeviceToHost)
            {
                if (copyItem.m_type != RHI::CopyItemType::Invalid)
                {
                    context.GetCommandList()->Submit(copyItem.GetDeviceCopyItem(context.GetDeviceIndex()));
                }
            }

            // Once signaled on device 1, we can map the host staging buffers on device 1 and 2 and copy data from 1 -> 2 and then signal the upload on device 2
            m_device1SignalFence[m_currentBufferIndex]
                ->GetDeviceFence(context.GetDeviceIndex())
                ->WaitOnCpuAsync(
                    [this, bufferIndex = m_currentBufferIndex]()
                    {
                        for (int copyIndex{ 0 }; copyIndex < m_copyItemDeviceToHost.size(); ++copyIndex)
                        {
                            auto bufferSize = m_device2HostBuffer[copyIndex][bufferIndex]->GetBufferSize();
                            void* data1 = m_device1HostBuffer[copyIndex][bufferIndex]->Map(bufferSize, 0)[m_data.m_sourceDeviceIndex];
                            void* data2 = m_device2HostBuffer[copyIndex][bufferIndex]->Map(bufferSize, 0)[m_data.m_destinationDeviceIndex];
                            memcpy(data2, data1, bufferSize);
                            m_device1HostBuffer[copyIndex][bufferIndex]->Unmap();
                            m_device2HostBuffer[copyIndex][bufferIndex]->Unmap();
                        }

                        m_device2WaitFence[bufferIndex]->GetDeviceFence(m_data.m_destinationDeviceIndex)->SignalOnCpu();
                    });
        }

        void CopyPass::SetupFrameGraphDependenciesHostToDevice(RHI::FrameGraphInterface frameGraph)
        {
            DeclareAttachmentsToFrameGraph(frameGraph, m_inputOutputCopy ? PassSlotType::InputOutput : PassSlotType::Output);
            frameGraph.ExecuteAfter(m_copyScopeProducerDeviceToHost->GetScopeId());
            for (Pass* pass : m_executeBeforePasses)
            {
                RenderPass* renderPass = azrtti_cast<RenderPass*>(pass);
                if (renderPass)
                {
                    frameGraph.ExecuteBefore(renderPass->GetScopeId());
                }
            }

            frameGraph.SetEstimatedItemCount(2);

            frameGraph.WaitFence(*m_device2WaitFence[m_currentBufferIndex]);
        }

        void CopyPass::CompileResourcesHostToDevice(const RHI::FrameGraphCompileContext& context)
        {
            PassAttachmentBinding& copyDest = m_inputOutputCopy ? GetInputOutputBinding(0) : GetOutputBinding(0);
            auto outputId = copyDest.GetAttachment()->GetAttachmentId();
            RHI::CopyItemType copyType = GetCopyItemType();
            switch (copyType)
            {
            case AZ::RHI::CopyItemType::Buffer:
                [[fallthrough]];
            case AZ::RHI::CopyItemType::ImageToBuffer:
                {
                    const auto* buffer = context.GetBuffer(outputId);
                    RHI::CopyBufferDescriptor copyBuffer;
                    copyBuffer.m_sourceBuffer = m_device2HostBuffer[0][m_currentBufferIndex]->GetRHIBuffer();
                    copyBuffer.m_destinationBuffer = buffer;
                    copyBuffer.m_size = aznumeric_cast<uint32_t>(m_device2HostBuffer[0][m_currentBufferIndex]->GetBufferSize());

                    m_copyItemHostToDevice[0] = copyBuffer;
                }
                break;
            case AZ::RHI::CopyItemType::Image:
                [[fallthrough]];
            case AZ::RHI::CopyItemType::BufferToImage:
                {
                    RHI::CopyBufferToImageDescriptor copyDesc;
                    copyDesc.m_sourceOffset = 0;

                    RHI::ImageAspectFlags sourceImageAspectFlags = RHI::ImageAspectFlags::Color;
                    uint32_t sourceImageAspect;
                    RHI::Format sourceFormat;

                    if (copyType == RHI::CopyItemType::Image)
                    {
                        auto inputId =
                            (m_inputOutputCopy ? GetInputOutputBinding(0) : GetInputBinding(0)).GetAttachment()->GetAttachmentId();

                        const auto* sourceImage = context.GetImage(inputId);
                        if (!sourceImage)
                        {
                            AZ_Warning("CopyPass", false, "Failed to find attachment image %s for copy to buffer", inputId.GetCStr());
                            return;
                        }
                        const auto& sourceImageDescriptor = sourceImage->GetDescriptor();

                        sourceFormat = sourceImageDescriptor.m_format;
                        sourceImageAspectFlags = RHI::GetImageAspectFlags(sourceFormat);
                        sourceImageAspect = az_ctz_u32(static_cast<uint32_t>(sourceImageAspectFlags));
                    }

                    // Destination Image
                    copyDesc.m_destinationImage = context.GetImage(copyDest.GetAttachment()->GetAttachmentId());
                    copyDesc.m_destinationOrigin = m_data.m_imageDestinationOrigin;
                    copyDesc.m_destinationSubresource = m_data.m_imageDestinationSubresource;

                    const auto& destinationImageDescriptor = copyDesc.m_destinationImage->GetDescriptor();
                    auto destImageAspectFlags = RHI::GetImageAspectFlags(destinationImageDescriptor.m_format);
                    uint32_t destImageAspect = az_ctz_u32(static_cast<uint32_t>(destImageAspectFlags));

                    auto aspectCount{ static_cast<int>(AZStd::min(
                        az_popcnt_u32(static_cast<uint32_t>(sourceImageAspectFlags)),
                        az_popcnt_u32(static_cast<uint32_t>(destImageAspectFlags)))) };

                    AZ_Assert(aspectCount <= 2, "CopyPass cannot copy more than two aspects at the moment.");

                    for (auto aspectIndex{ 0 }; aspectIndex < aspectCount; ++aspectIndex)
                    {
                        while ((static_cast<uint32_t>(sourceImageAspectFlags) & AZ_BIT(sourceImageAspect)) == 0)
                        {
                            ++sourceImageAspect;
                        }

                        while ((static_cast<uint32_t>(destImageAspectFlags) & AZ_BIT(destImageAspect)) == 0)
                        {
                            ++destImageAspect;
                        }

                        if (copyType == RHI::CopyItemType::BufferToImage)
                        {
                            copyDesc.m_sourceBytesPerRow = m_data.m_bufferSourceBytesPerRow;
                            copyDesc.m_sourceBytesPerImage = m_data.m_bufferSourceBytesPerImage;
                            copyDesc.m_sourceSize = m_data.m_sourceSize;
                            copyDesc.m_sourceFormat =
                                FindFormatForAspect(destinationImageDescriptor.m_format, static_cast<RHI::ImageAspect>(destImageAspect));
                        }
                        else
                        {
                            copyDesc.m_sourceBytesPerRow = m_inputImageLayouts[aspectIndex].m_bytesPerRow;
                            copyDesc.m_sourceBytesPerImage = m_inputImageLayouts[aspectIndex].m_bytesPerImage;
                            copyDesc.m_sourceSize = m_inputImageLayouts[aspectIndex].m_size;
                            copyDesc.m_sourceFormat = FindFormatForAspect(sourceFormat, static_cast<RHI::ImageAspect>(sourceImageAspect));
                        }

                        const auto* sourceBuffer = m_device2HostBuffer[aspectIndex][m_currentBufferIndex]->GetRHIBuffer();
                        copyDesc.m_sourceBuffer = sourceBuffer;
                        copyDesc.m_destinationSubresource.m_aspect = static_cast<RHI::ImageAspect>(destImageAspect);

                        if (m_copyItemHostToDevice.size() <= aspectIndex)
                        {
                            m_copyItemHostToDevice.emplace_back();
                        }

                        m_copyItemHostToDevice[aspectIndex] = copyDesc;

                        ++sourceImageAspect;
                        ++destImageAspect;
                    }
                }
                break;
            default:
                break;
            }
        }

        void CopyPass::BuildCommandListInternalHostToDevice(const RHI::FrameGraphExecuteContext& context)
        {
            for (const auto& copyItem : m_copyItemHostToDevice)
            {
                if (copyItem.m_type != RHI::CopyItemType::Invalid)
                {
                    context.GetCommandList()->Submit(copyItem.GetDeviceCopyItem(context.GetDeviceIndex()));
                }
            }
        }

        // --- Copy setup functions ---

        void CopyPass::CopyBuffer(const RHI::FrameGraphCompileContext& context)
        {
            RHI::CopyBufferDescriptor copyDesc;

            // Source Buffer
            PassAttachmentBinding& copySource = GetInputBinding(0);
            const auto* sourceBuffer = context.GetBuffer(copySource.GetAttachment()->GetAttachmentId());
            copyDesc.m_sourceBuffer = sourceBuffer;
            copyDesc.m_size = static_cast<uint32_t>(sourceBuffer->GetDescriptor().m_byteCount);
            copyDesc.m_sourceOffset = m_data.m_bufferSourceOffset;

            // Destination Buffer
            PassAttachmentBinding& copyDest = GetOutputBinding(0);
            copyDesc.m_destinationBuffer = context.GetBuffer(copyDest.GetAttachment()->GetAttachmentId());
            copyDesc.m_destinationBuffer = context.GetBuffer(copyDest.GetAttachment()->GetAttachmentId());
            copyDesc.m_destinationOffset = m_data.m_bufferDestinationOffset;

            m_copyItemSameDevice = copyDesc;
        }

        void CopyPass::CopyImage(const RHI::FrameGraphCompileContext& context)
        {
            RHI::CopyImageDescriptor copyDesc;

            // Source Image
            PassAttachmentBinding& copySource = GetInputBinding(0);
            const auto* sourceImage = context.GetImage(copySource.GetAttachment()->GetAttachmentId());
            copyDesc.m_sourceImage = sourceImage;
            copyDesc.m_sourceSize = sourceImage->GetDescriptor().m_size;
            copyDesc.m_sourceOrigin = m_data.m_imageSourceOrigin;
            copyDesc.m_sourceSubresource = m_data.m_imageSourceSubresource;

            // Destination Image
            PassAttachmentBinding& copyDest = GetOutputBinding(0);
            copyDesc.m_destinationImage = context.GetImage(copyDest.GetAttachment()->GetAttachmentId());
            copyDesc.m_destinationOrigin = m_data.m_imageDestinationOrigin;
            copyDesc.m_destinationSubresource = m_data.m_imageDestinationSubresource;

            m_copyItemSameDevice = copyDesc;
        }

        void CopyPass::CopyBufferToImage(const RHI::FrameGraphCompileContext& context)
        {
            RHI::CopyBufferToImageDescriptor copyDesc;

            // Source Buffer
            PassAttachmentBinding& copySource = GetInputBinding(0);
            const auto* sourceBuffer = context.GetBuffer(copySource.GetAttachment()->GetAttachmentId());
            copyDesc.m_sourceBuffer = sourceBuffer;
            copyDesc.m_sourceSize = m_data.m_sourceSize;
            copyDesc.m_sourceOffset = m_data.m_bufferSourceOffset;
            copyDesc.m_sourceBytesPerRow = m_data.m_bufferSourceBytesPerRow;
            copyDesc.m_sourceBytesPerImage = m_data.m_bufferSourceBytesPerImage;

            // Destination Image
            PassAttachmentBinding& copyDest = GetOutputBinding(0);
            copyDesc.m_destinationImage = context.GetImage(copyDest.GetAttachment()->GetAttachmentId());
            copyDesc.m_destinationOrigin = m_data.m_imageDestinationOrigin;
            copyDesc.m_destinationSubresource = m_data.m_imageDestinationSubresource;
            copyDesc.m_sourceFormat = copyDesc.m_destinationImage->GetDescriptor().m_format;

            m_copyItemSameDevice = copyDesc;
        }

        void CopyPass::CopyImageToBuffer(const RHI::FrameGraphCompileContext& context)
        {
            RHI::CopyImageToBufferDescriptor copyDesc;

            // Source Image
            PassAttachmentBinding& copySource = GetInputBinding(0);
            const auto* sourceImage = context.GetImage(copySource.GetAttachment()->GetAttachmentId());
            copyDesc.m_sourceImage = sourceImage;
            copyDesc.m_sourceSize = sourceImage->GetDescriptor().m_size;
            copyDesc.m_sourceOrigin = m_data.m_imageSourceOrigin;
            copyDesc.m_sourceSubresource = m_data.m_imageSourceSubresource;

            // Destination Buffer
            PassAttachmentBinding& copyDest = GetOutputBinding(0);
            copyDesc.m_destinationBuffer = context.GetBuffer(copyDest.GetAttachment()->GetAttachmentId());
            copyDesc.m_destinationOffset = m_data.m_bufferDestinationOffset;
            copyDesc.m_destinationBytesPerRow = m_data.m_bufferDestinationBytesPerRow;
            copyDesc.m_destinationBytesPerImage = m_data.m_bufferDestinationBytesPerImage;
            copyDesc.m_destinationFormat = sourceImage->GetDescriptor().m_format;

            m_copyItemSameDevice = copyDesc;
        }

    } // namespace RPI
} // namespace AZ
