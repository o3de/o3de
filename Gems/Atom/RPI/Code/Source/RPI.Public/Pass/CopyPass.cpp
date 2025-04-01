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
#include <Atom/RPI.Public/GpuQuery/Query.h>
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

                m_perAspectCopyInfos.clear();
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
                if (IsTimestampQueryEnabled())
                {
                    m_queryEntries[AZStd::to_underlying(CopyIndex::SameDevice)].m_timestampResult = AZ::RPI::TimestampResult();
                }

                params.m_frameGraphBuilder->ImportScopeProducer(*m_copyScopeProducerSameDevice);
                ReadbackScopeQueryResults(CopyIndex::SameDevice);
            }
            else if (m_copyMode == CopyMode::DifferentDevicesIntermediateHost)
            {
                if (IsTimestampQueryEnabled())
                {
                    m_queryEntries[AZStd::to_underlying(CopyIndex::DeviceToHost)].m_timestampResult = AZ::RPI::TimestampResult();
                    m_queryEntries[AZStd::to_underlying(CopyIndex::HostToDevice)].m_timestampResult = AZ::RPI::TimestampResult();
                }

                params.m_frameGraphBuilder->ImportScopeProducer(*m_copyScopeProducerDeviceToHost);
                params.m_frameGraphBuilder->ImportScopeProducer(*m_copyScopeProducerHostToDevice);
                m_currentBufferIndex = (m_currentBufferIndex + 1) % MaxFrames;
                m_device1SignalFence[m_currentBufferIndex]->Reset();
                m_device2WaitFence[m_currentBufferIndex]->Reset();

                ReadbackScopeQueryResults(CopyIndex::DeviceToHost);
                ReadbackScopeQueryResults(CopyIndex::HostToDevice);
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
            AddScopeQueryToFrameGraph(frameGraph, CopyIndex::SameDevice);
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
            BeginScopeQuery(context, CopyIndex::SameDevice);
            if (m_copyItemSameDevice.m_type != RHI::CopyItemType::Invalid)
            {
                context.GetCommandList()->Submit(m_copyItemSameDevice.GetDeviceCopyItem(context.GetDeviceIndex()));
            }
            EndScopeQuery(context, CopyIndex::SameDevice);
        }

        void CopyPass::SetupFrameGraphDependenciesDeviceToHost(RHI::FrameGraphInterface frameGraph)
        {
            // We need to set the access mask to read since we only copy from the device otherwise we would get an error with InputOutput.
            // We also need the size of the output image when copying from image to image, so we need all attachments (even the output ones)
            // We also need it so the framegraph knows the two scopes depend on each other
            DeclareAttachmentsToFrameGraph(frameGraph, PassSlotType::Uninitialized, RHI::ScopeAttachmentAccess::Read);

            frameGraph.SetEstimatedItemCount(2);
            frameGraph.SignalFence(*m_device1SignalFence[m_currentBufferIndex]);
            AddScopeQueryToFrameGraph(frameGraph, CopyIndex::DeviceToHost);
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
                    m_sourceFormat = sourceImageDescriptor.m_format;
                    const uint16_t sourceMipSlice = m_data.m_imageSourceSubresource.m_mipSlice;
                    RHI::ImageSubresourceRange sourceRange(sourceMipSlice, sourceMipSlice, 0, 0);
                    RHI::ImageAspectFlags sourceImageAspectFlags = RHI::GetImageAspectFlags(m_sourceFormat);
                    uint32_t sourceImageAspect = az_ctz_u32(AZStd::to_underlying(sourceImageAspectFlags));

                    auto aspectCount{ static_cast<int>(az_popcnt_u32(AZStd::to_underlying(sourceImageAspectFlags))) };

                    AZ_Assert(
                        copyType == AZ::RHI::CopyItemType::Image || aspectCount == 1,
                        "CopyPass cannot copy %d image aspects into a buffer.",
                        aspectCount);

                    m_perAspectCopyInfos.resize(aspectCount);

                    AZStd::vector<RHI::DeviceImageSubresourceLayout> sourceImageSubResourcesLayouts;
                    sourceImageSubResourcesLayouts.resize_no_construct(sourceImageDescriptor.m_mipLevels);

                    for (auto aspectIndex{ 0 }; aspectIndex < aspectCount; ++aspectIndex)
                    {
                        auto& perAspectInfo = m_perAspectCopyInfos[aspectIndex];

                        while (!RHI::CheckBit(AZStd::to_underlying(sourceImageAspectFlags), sourceImageAspect))
                        {
                            ++sourceImageAspect;
                        }

                        sourceRange.m_aspectFlags = static_cast<RHI::ImageAspectFlags>(AZ_BIT(sourceImageAspect));

                        size_t sourceTotalSizeInBytes = 0;
                        sourceImage->GetDeviceImage(m_data.m_sourceDeviceIndex)
                            ->GetSubresourceLayouts(sourceRange, sourceImageSubResourcesLayouts.data(), &sourceTotalSizeInBytes);
                        AZ::u64 sourceByteCount = sourceTotalSizeInBytes;

                        if (perAspectInfo.m_deviceHostBufferByteCount[m_currentBufferIndex] != sourceByteCount)
                        {
                            perAspectInfo.m_deviceHostBufferByteCount[m_currentBufferIndex] = sourceByteCount;

                            RPI::CommonBufferDescriptor desc;
                            desc.m_poolType = RPI::CommonBufferPoolType::ReadBack;
                            desc.m_bufferName =
                                AZStd::string(GetPathName().GetStringView()) + "_hostbuffer_" + AZStd::to_string(aspectIndex);
                            desc.m_byteCount = sourceByteCount;
                            perAspectInfo.m_device1HostBuffer[m_currentBufferIndex] =
                                BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

                            desc.m_bufferName =
                                AZStd::string(GetPathName().GetStringView()) + "_hostbuffer2_" + AZStd::to_string(aspectIndex);
                            desc.m_poolType = RPI::CommonBufferPoolType::Staging;
                            perAspectInfo.m_device2HostBuffer[m_currentBufferIndex] =
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
                        copyImageToBufferDesc.m_destinationBuffer = perAspectInfo.m_device1HostBuffer[m_currentBufferIndex]->GetRHIBuffer();
                        copyImageToBufferDesc.m_destinationFormat =
                            FindFormatForAspect(sourceImageDescriptor.m_format, static_cast<RHI::ImageAspect>(sourceImageAspect));

                        ++sourceImageAspect;

                        perAspectInfo.m_copyItemDeviceToHost = copyImageToBufferDesc;
                        perAspectInfo.m_inputImageLayout = sourceImageSubResourcesLayouts[sourceMipSlice];
                    }
                }
                break;
            case AZ::RHI::CopyItemType::Buffer:
                [[fallthrough]];
            case AZ::RHI::CopyItemType::BufferToImage:
                {
                    const auto* buffer = context.GetBuffer(inputId);

                    m_perAspectCopyInfos.resize(1);
                    auto& perAspectCopyInfo{ m_perAspectCopyInfos[0] };

                    if (perAspectCopyInfo.m_deviceHostBufferByteCount[m_currentBufferIndex] != buffer->GetDescriptor().m_byteCount)
                    {
                        perAspectCopyInfo.m_deviceHostBufferByteCount[m_currentBufferIndex] = buffer->GetDescriptor().m_byteCount;

                        RPI::CommonBufferDescriptor desc;
                        desc.m_poolType = RPI::CommonBufferPoolType::ReadBack;
                        desc.m_bufferName = AZStd::string(GetPathName().GetStringView()) + "_hostbuffer";
                        desc.m_byteCount = perAspectCopyInfo.m_deviceHostBufferByteCount[m_currentBufferIndex];

                        perAspectCopyInfo.m_device1HostBuffer[m_currentBufferIndex] =
                            BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                        desc.m_poolType = RPI::CommonBufferPoolType::Staging;
                        desc.m_bufferName = AZStd::string(GetPathName().GetStringView()) + "_hostbuffer2";
                        perAspectCopyInfo.m_device2HostBuffer[m_currentBufferIndex] =
                            BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                    }

                    // copy buffer
                    RHI::CopyBufferDescriptor copyBuffer;
                    copyBuffer.m_sourceBuffer = buffer;
                    copyBuffer.m_destinationBuffer = perAspectCopyInfo.m_device1HostBuffer[m_currentBufferIndex]->GetRHIBuffer();
                    copyBuffer.m_size = aznumeric_cast<uint32_t>(perAspectCopyInfo.m_deviceHostBufferByteCount[m_currentBufferIndex]);

                    perAspectCopyInfo.m_copyItemDeviceToHost = copyBuffer;
                }
                break;
            default:
                break;
            }
        }

        void CopyPass::BuildCommandListInternalDeviceToHost(const RHI::FrameGraphExecuteContext& context)
        {
            AZ_Warning(
                "CopyPass", context.GetCommandListCount() == 1, "This will be wrong if the Scope is split across multiple command lists");
            BeginScopeQuery(context, CopyIndex::DeviceToHost);
            for (const auto& perAspectCopyInfo : m_perAspectCopyInfos)
            {
                const auto& copyItem{ perAspectCopyInfo.m_copyItemDeviceToHost };
                if (copyItem.m_type != RHI::CopyItemType::Invalid)
                {
                    context.GetCommandList()->Submit(copyItem.GetDeviceCopyItem(context.GetDeviceIndex()));
                }
            }
            EndScopeQuery(context, CopyIndex::DeviceToHost);

            // Once signaled on device 1, we can map the host staging buffers on device 1 and 2 and copy data from 1 -> 2 and then signal the upload on device 2
            m_device1SignalFence[m_currentBufferIndex]
                ->GetDeviceFence(context.GetDeviceIndex())
                ->WaitOnCpuAsync(
                    [this, bufferIndex = m_currentBufferIndex]()
                    {
                        for (const auto& perAspectCopyInfo : m_perAspectCopyInfos)
                        {
                            auto bufferSize = perAspectCopyInfo.m_device2HostBuffer[bufferIndex]->GetBufferSize();
                            void* data1 =
                                perAspectCopyInfo.m_device1HostBuffer[bufferIndex]->Map(bufferSize, 0)[m_data.m_sourceDeviceIndex];
                            void* data2 =
                                perAspectCopyInfo.m_device2HostBuffer[bufferIndex]->Map(bufferSize, 0)[m_data.m_destinationDeviceIndex];
                            memcpy(data2, data1, bufferSize);
                            perAspectCopyInfo.m_device1HostBuffer[bufferIndex]->Unmap();
                            perAspectCopyInfo.m_device2HostBuffer[bufferIndex]->Unmap();
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
            AddScopeQueryToFrameGraph(frameGraph, CopyIndex::HostToDevice);

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
                    m_perAspectCopyInfos.resize(1);
                    auto& perAspectCopyInfo{ m_perAspectCopyInfos[0] };

                    const auto* buffer = context.GetBuffer(outputId);
                    RHI::CopyBufferDescriptor copyBuffer;
                    copyBuffer.m_sourceBuffer = perAspectCopyInfo.m_device2HostBuffer[m_currentBufferIndex]->GetRHIBuffer();
                    copyBuffer.m_destinationBuffer = buffer;
                    copyBuffer.m_size =
                        aznumeric_cast<uint32_t>(perAspectCopyInfo.m_device2HostBuffer[m_currentBufferIndex]->GetBufferSize());

                    perAspectCopyInfo.m_copyItemHostToDevice = copyBuffer;
                }
                break;
            case AZ::RHI::CopyItemType::Image:
                [[fallthrough]];
            case AZ::RHI::CopyItemType::BufferToImage:
                {
                    RHI::CopyBufferToImageDescriptor copyDesc;
                    copyDesc.m_sourceOffset = 0;

                    RHI::ImageAspectFlags sourceImageAspectFlags{ RHI::ImageAspectFlags::Color };
                    uint32_t sourceImageAspect{};

                    if (copyType == RHI::CopyItemType::Image)
                    {
                        sourceImageAspectFlags = RHI::GetImageAspectFlags(m_sourceFormat);
                        sourceImageAspect = az_ctz_u32(AZStd::to_underlying(sourceImageAspectFlags));
                    }

                    // Destination Image
                    copyDesc.m_destinationImage = context.GetImage(copyDest.GetAttachment()->GetAttachmentId());
                    copyDesc.m_destinationOrigin = m_data.m_imageDestinationOrigin;
                    copyDesc.m_destinationSubresource = m_data.m_imageDestinationSubresource;

                    const auto& destinationImageDescriptor = copyDesc.m_destinationImage->GetDescriptor();
                    auto destImageAspectFlags = RHI::GetImageAspectFlags(destinationImageDescriptor.m_format);
                    uint32_t destImageAspect = az_ctz_u32(AZStd::to_underlying(destImageAspectFlags));

                    auto aspectCount{ static_cast<int>(AZStd::min(
                        az_popcnt_u32(AZStd::to_underlying(sourceImageAspectFlags)),
                        az_popcnt_u32(AZStd::to_underlying(destImageAspectFlags)))) };

                    m_perAspectCopyInfos.resize(aspectCount);

                    for (auto aspectIndex{ 0 }; aspectIndex < aspectCount; ++aspectIndex)
                    {
                        auto& perAspectInfo = m_perAspectCopyInfos[aspectIndex];

                        while (!RHI::CheckBit(AZStd::to_underlying(sourceImageAspectFlags), sourceImageAspect))
                        {
                            ++sourceImageAspect;
                        }

                        while (!RHI::CheckBit(AZStd::to_underlying(destImageAspectFlags), destImageAspect))
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
                            copyDesc.m_sourceBytesPerRow = perAspectInfo.m_inputImageLayout.m_bytesPerRow;
                            copyDesc.m_sourceBytesPerImage = perAspectInfo.m_inputImageLayout.m_bytesPerImage;
                            copyDesc.m_sourceSize = perAspectInfo.m_inputImageLayout.m_size;
                            copyDesc.m_sourceFormat = FindFormatForAspect(m_sourceFormat, static_cast<RHI::ImageAspect>(sourceImageAspect));
                        }

                        const auto* sourceBuffer = perAspectInfo.m_device2HostBuffer[m_currentBufferIndex]->GetRHIBuffer();
                        copyDesc.m_sourceBuffer = sourceBuffer;
                        copyDesc.m_destinationSubresource.m_aspect = static_cast<RHI::ImageAspect>(destImageAspect);

                        perAspectInfo.m_copyItemHostToDevice = copyDesc;

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
            AZ_Warning(
                "CopyPass", context.GetCommandListCount() == 1, "This will be wrong if the Scope is split across multiple command lists");
            BeginScopeQuery(context, CopyIndex::HostToDevice);
            for (const auto& perAspectCopyInfo : m_perAspectCopyInfos)
            {
                const auto& copyItem{ perAspectCopyInfo.m_copyItemHostToDevice };
                if (copyItem.m_type != RHI::CopyItemType::Invalid)
                {
                    context.GetCommandList()->Submit(copyItem.GetDeviceCopyItem(context.GetDeviceIndex()));
                }
            }
            EndScopeQuery(context, CopyIndex::HostToDevice);
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

        RHI::Ptr<Query> CopyPass::GetQuery(ScopeQueryType queryType, CopyIndex copyIndex)
        {
            uint32_t typeIndex = static_cast<uint32_t>(queryType);
            if (!m_queryEntries[AZStd::to_underlying(copyIndex)].m_scopeQuery[typeIndex])
            {
                RHI::Ptr<Query> query;
                switch (queryType)
                {
                case ScopeQueryType::Timestamp:
                    query = GpuQuerySystemInterface::Get()->CreateQuery(
                        RHI::QueryType::Timestamp, RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
                    break;
                case ScopeQueryType::PipelineStatistics:
                    query = GpuQuerySystemInterface::Get()->CreateQuery(
                        RHI::QueryType::PipelineStatistics, RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
                    break;
                }

                m_queryEntries[AZStd::to_underlying(copyIndex)].m_scopeQuery[typeIndex] = query;
            }

            return m_queryEntries[AZStd::to_underlying(copyIndex)].m_scopeQuery[typeIndex];
        }

        template<typename Func>
        inline void CopyPass::ExecuteOnTimestampQuery(Func&& func, CopyIndex copyIndex)
        {
            if (IsTimestampQueryEnabled())
            {
                auto query = GetQuery(ScopeQueryType::Timestamp, copyIndex);
                if (query)
                {
                    func(query);
                }
            }
        }

        template<typename Func>
        inline void CopyPass::ExecuteOnPipelineStatisticsQuery(Func&& func, CopyIndex copyIndex)
        {
            if (IsPipelineStatisticsQueryEnabled())
            {
                auto query = GetQuery(ScopeQueryType::PipelineStatistics, copyIndex);
                if (query)
                {
                    func(query);
                }
            }
        }

        void CopyPass::AddScopeQueryToFrameGraph(RHI::FrameGraphInterface frameGraph, CopyIndex copyIndex)
        {
            const auto addToFrameGraph = [&frameGraph](RHI::Ptr<Query> query)
            {
                query->AddToFrameGraph(frameGraph);
            };

            ExecuteOnTimestampQuery(addToFrameGraph, copyIndex);
            ExecuteOnPipelineStatisticsQuery(addToFrameGraph, copyIndex);
        }

        void CopyPass::BeginScopeQuery(const RHI::FrameGraphExecuteContext& context, CopyIndex copyIndex)
        {
            const auto beginQuery = [&context, this](RHI::Ptr<Query> query)
            {
                if (query->BeginQuery(context) == QueryResultCode::Fail)
                {
                    AZ_UNUSED(this); // Prevent unused warning in release builds
                    AZ_WarningOnce(
                        "RenderPass",
                        false,
                        "BeginScopeQuery failed. Make sure AddScopeQueryToFrameGraph was called in SetupFrameGraphDependencies"
                        " for this pass: %s",
                        this->RTTI_GetTypeName());
                }
            };

            AZ_Warning("CopyPass", context.GetCommandListIndex() == 0, "Cannot handle multiple CommandLists at the moment");

            ExecuteOnTimestampQuery(beginQuery, copyIndex);
            ExecuteOnPipelineStatisticsQuery(beginQuery, copyIndex);
        }

        void CopyPass::EndScopeQuery(const RHI::FrameGraphExecuteContext& context, CopyIndex copyIndex)
        {
            const auto endQuery = [&context](RHI::Ptr<Query> query)
            {
                query->EndQuery(context);
            };

            ExecuteOnTimestampQuery(endQuery, copyIndex);
            ExecuteOnPipelineStatisticsQuery(endQuery, copyIndex);
        }

        void CopyPass::ReadbackScopeQueryResults(CopyIndex copyIndex)
        {
            int deviceIndex = copyIndex == CopyIndex::DeviceToHost ? m_data.m_sourceDeviceIndex : m_data.m_destinationDeviceIndex;
            deviceIndex = deviceIndex != RHI::MultiDevice::InvalidDeviceIndex ? deviceIndex : RHI::MultiDevice::DefaultDeviceIndex;

            ExecuteOnTimestampQuery(
                [this, copyIndex, deviceIndex](RHI::Ptr<Query> query)
                {
                    const uint32_t TimestampResultQueryCount = 2u;
                    uint64_t timestampResult[TimestampResultQueryCount] = { 0 };
                    query->GetLatestResult(&timestampResult, sizeof(uint64_t) * TimestampResultQueryCount, deviceIndex);
                    m_queryEntries[AZStd::to_underlying(copyIndex)].m_timestampResult =
                        TimestampResult(timestampResult[0], timestampResult[1], m_hardwareQueueClass);
                },
                copyIndex);

            ExecuteOnPipelineStatisticsQuery(
                [this, copyIndex, deviceIndex](RHI::Ptr<Query> query)
                {
                    query->GetLatestResult(
                        &m_queryEntries[AZStd::to_underlying(copyIndex)].m_statisticsResult, sizeof(PipelineStatisticsResult), deviceIndex);
                },
                copyIndex);
        }

        TimestampResult CopyPass::GetTimestampResultInternal() const
        {
            // TODO: As there is currently no good solution to multi-device Timestamps
            // as discusssed here (https://github.com/o3de/o3de/pull/18268)
            // we will return the first Timestamp for now
            return m_queryEntries[AZStd::to_underlying(CopyIndex::SameDevice)].m_timestampResult;
        }

        PipelineStatisticsResult CopyPass::GetPipelineStatisticsResultInternal() const
        {
            return m_queryEntries[AZStd::to_underlying(CopyIndex::SameDevice)].m_statisticsResult;
        }
    } // namespace RPI
} // namespace AZ
