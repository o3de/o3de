/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <Atom/RHI/Device.h>
#include <Atom/RHI/FrameGraphCompiler.h>
#include <Atom/RHI/FrameGraphExecuter.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/SingleDeviceBufferView.h>
#include <Atom/RHI/SingleDeviceBuffer.h>
#include <Atom/RHI/SingleDeviceBufferPool.h>
#include <Atom/RHI/SingleDeviceFence.h>
#include <Atom/RHI/SingleDeviceImage.h>
#include <Atom/RHI/SingleDeviceImagePool.h>
#include <Atom/RHI/SingleDeviceIndirectBufferSignature.h>
#include <Atom/RHI/SingleDeviceIndirectBufferWriter.h>
#include <Atom/RHI/SingleDevicePipelineLibrary.h>
#include <Atom/RHI/SingleDevicePipelineState.h>
#include <Atom/RHI/SingleDeviceQuery.h>
#include <Atom/RHI/SingleDeviceQueryPool.h>
#include <Atom/RHI/SingleDeviceShaderResourceGroup.h>
#include <Atom/RHI/SingleDeviceShaderResourceGroupPool.h>
#include <Atom/RHI/SingleDeviceStreamingImagePool.h>
#include <Atom/RHI/SingleDeviceSwapChain.h>
#include <Atom/RHI/SingleDeviceTransientAttachmentPool.h>

namespace UnitTest
{
    namespace StubRHI
    {
        class PhysicalDevice
            : public AZ::RHI::PhysicalDevice
        {
        public:
            AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator);
            static AZ::RHI::PhysicalDeviceList Enumerate();

        private:
            PhysicalDevice();
        };

        class Device
            : public AZ::RHI::Device
        {
        public:
            AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator);
            Device();

        private:
            AZ::RHI::ResultCode InitInternal([[maybe_unused]] AZ::RHI::PhysicalDevice&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitInternalBindlessSrg([[maybe_unused]] const AZ::RHI::BindlessSrgDescriptor& bindlessSrgDesc) override { return AZ::RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            AZ::RHI::ResultCode BeginFrameInternal() override { return AZ::RHI::ResultCode::Success;}
            void EndFrameInternal() override {}
            void WaitForIdleInternal() override {}
            void CompileMemoryStatisticsInternal(AZ::RHI::MemoryStatisticsBuilder&) override {}
            void UpdateCpuTimingStatisticsInternal() const override {}
            AZStd::chrono::microseconds GpuTimestampToMicroseconds([[maybe_unused]] uint64_t gpuTimestamp, [[maybe_unused]] AZ::RHI::HardwareQueueClass queueClass) const override
            {
                return AZStd::chrono::microseconds();
            }
            void FillFormatsCapabilitiesInternal([[maybe_unused]] FormatCapabilitiesList& formatsCapabilities) override {}
            AZ::RHI::ResultCode InitializeLimits() override { return AZ::RHI::ResultCode::Success; }
            void PreShutdown() override {}
            AZ::RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const AZ::RHI::ImageDescriptor& descriptor) override { return AZ::RHI::ResourceMemoryRequirements{}; };
            AZ::RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const AZ::RHI::BufferDescriptor& descriptor) override { return AZ::RHI::ResourceMemoryRequirements{}; };
            void ObjectCollectionNotify(AZ::RHI::ObjectCollectorNotifyFunction notifyFunction) override {}
            AZ::RHI::ShadingRateImageValue ConvertShadingRate([[maybe_unused]] AZ::RHI::ShadingRate rate) const override
            {
                return AZ::RHI::ShadingRateImageValue{};
            }
        };

        class ImageView
            : public AZ::RHI::SingleDeviceImageView
        {
        public:
            AZ_CLASS_ALLOCATOR(ImageView, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::SingleDeviceResource&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InvalidateInternal() override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
        };

        class Image
            : public AZ::RHI::SingleDeviceImage
        {
        public:
            AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator);

        private:
            void GetSubresourceLayoutsInternal(
                const AZ::RHI::ImageSubresourceRange&, AZ::RHI::SingleDeviceImageSubresourceLayout*, size_t*) const override
            {
            }
            bool IsStreamableInternal() const override {return true;};
        };

        class BufferView
            : public AZ::RHI::SingleDeviceBufferView
        {
        public:
            AZ_CLASS_ALLOCATOR(BufferView, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::SingleDeviceResource&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InvalidateInternal() override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
        };

        class Buffer
            : public AZ::RHI::SingleDeviceBuffer
        {
            friend class BufferPool;
        public:
            AZ_CLASS_ALLOCATOR(Buffer, AZ::SystemAllocator);

            bool IsMapped() const  { return m_isMapped; }

            void* Map()
            {
                m_isMapped = true;
                return m_data.data();
            }

            void Unmap()
            {
                m_isMapped = false;
            }

            const AZStd::vector<uint8_t>& GetData() const
            {
                return m_data;
            }

        private:
            bool m_isMapped = false;
            AZStd::vector<uint8_t> m_data;
        };

        class BufferPool
            : public AZ::RHI::SingleDeviceBufferPool
        {
        public:
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::BufferPoolDescriptor&) override { return AZ::RHI::ResultCode::Success;}

            AZ::RHI::ResultCode InitBufferInternal(AZ::RHI::SingleDeviceBuffer& bufferBase, const AZ::RHI::BufferDescriptor& descriptor) override
            {
                AZ_Assert(IsInitialized(), "Buffer Pool is not initialized");

                Buffer& buffer = static_cast<Buffer&>(bufferBase);
                buffer.m_data.resize(descriptor.m_byteCount);

                return AZ::RHI::ResultCode::Success;
            }

            void ShutdownResourceInternal(AZ::RHI::SingleDeviceResource& resourceBase) override
            {
                Buffer& buffer = static_cast<Buffer&>(resourceBase);
                buffer.m_data.clear();
            }

            AZ::RHI::ResultCode MapBufferInternal(const AZ::RHI::SingleDeviceBufferMapRequest& request, AZ::RHI::SingleDeviceBufferMapResponse& response) override
            {
                Buffer& buffer = static_cast<Buffer&>(*request.m_buffer);
                response.m_data = buffer.Map();
                return AZ::RHI::ResultCode::Success;
            }

            void UnmapBufferInternal(AZ::RHI::SingleDeviceBuffer& bufferBase) override
            {
                Buffer& buffer = static_cast<Buffer&>(bufferBase);
                buffer.Unmap();
            }

            AZ::RHI::ResultCode OrphanBufferInternal(AZ::RHI::SingleDeviceBuffer&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode StreamBufferInternal([[maybe_unused]] const AZ::RHI::SingleDeviceBufferStreamRequest& request) override { return AZ::RHI::ResultCode::Success; }
            void ComputeFragmentation() const override {}
        };

        class ImagePool
            : public AZ::RHI::SingleDeviceImagePool
        {
        public:
            AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ImagePoolDescriptor&) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            AZ::RHI::ResultCode UpdateImageContentsInternal(const AZ::RHI::SingleDeviceImageUpdateRequest&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitImageInternal(const AZ::RHI::SingleDeviceImageInitRequest&) override
            {
                return AZ::RHI::ResultCode::Success;
            }
            void ShutdownResourceInternal(AZ::RHI::SingleDeviceResource&) override {}
        };

        class StreamingImagePool
            : public AZ::RHI::SingleDeviceStreamingImagePool
        {
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator);

            void ComputeFragmentation() const override {}

        private:
            AZ::RHI::ResultCode InitImageInternal([[maybe_unused]] const AZ::RHI::SingleDeviceStreamingImageInitRequest& request) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            void ShutdownResourceInternal(AZ::RHI::SingleDeviceResource&) override {}
            AZ::RHI::ResultCode ExpandImageInternal(const AZ::RHI::SingleDeviceStreamingImageExpandRequest&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode TrimImageInternal(AZ::RHI::SingleDeviceImage&, uint32_t) override { return AZ::RHI::ResultCode::Success; }
        };

        class SwapChain
            : public AZ::RHI::SingleDeviceSwapChain
        {
        public:
            AZ_CLASS_ALLOCATOR(SwapChain, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::SwapChainDescriptor&, [[maybe_unused]] AZ::RHI::SwapChainDimensions* outNativeDimensions) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitImageInternal(const InitImageRequest&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode ResizeInternal([[maybe_unused]] const AZ::RHI::SwapChainDimensions& dimensions, [[maybe_unused]] AZ::RHI::SwapChainDimensions* nativeDimensions) override { return AZ::RHI::ResultCode::Success; };
            uint32_t PresentInternal() override { return 0; };
        };

        class Fence
            : public AZ::RHI::SingleDeviceFence
        {
        public:
            AZ_CLASS_ALLOCATOR(Fence, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, AZ::RHI::FenceState) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            void SignalOnCpuInternal() override {}
            void WaitOnCpuInternal() const override {};
            void ResetInternal() override {}
            AZ::RHI::FenceState GetFenceStateInternal() const override { return AZ::RHI::FenceState::Reset; }
        };

        class ShaderResourceGroupPool
            : public AZ::RHI::SingleDeviceShaderResourceGroupPool
        {
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ShaderResourceGroupPoolDescriptor&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitGroupInternal(AZ::RHI::SingleDeviceShaderResourceGroup&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode CompileGroupInternal(AZ::RHI::SingleDeviceShaderResourceGroup&,const AZ::RHI::SingleDeviceShaderResourceGroupData&) override { return AZ::RHI::ResultCode::Success; }
        };

        class ShaderResourceGroup
            : public AZ::RHI::SingleDeviceShaderResourceGroup
        {
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator);
        };

        class PipelineLibrary
            : public AZ::RHI::SingleDevicePipelineLibrary
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, [[maybe_unused]] const AZ::RHI::SingleDevicePipelineLibraryDescriptor& descriptor) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            AZ::RHI::ResultCode MergeIntoInternal(AZStd::span<const AZ::RHI::SingleDevicePipelineLibrary* const>) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ConstPtr<AZ::RHI::PipelineLibraryData> GetSerializedDataInternal() const override { return nullptr; }
            bool SaveSerializedDataInternal([[maybe_unused]] const AZStd::string& filePath) const { return true;}
        };

        class ShaderStageFunction
            : public AZ::RHI::ShaderStageFunction
        {
        public:
            AZ_RTTI(ShaderStageFunction, "{644DBC98-C864-488C-BBA8-0137C210C1E2}", AZ::RHI::ShaderStageFunction);
            AZ_CLASS_ALLOCATOR(ShaderStageFunction, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode FinalizeInternal() override { return AZ::RHI::ResultCode::Success; }
        };

        class PipelineState
            : public AZ::RHI::SingleDevicePipelineState
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForDraw&, AZ::RHI::SingleDevicePipelineLibrary*) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForDispatch&, AZ::RHI::SingleDevicePipelineLibrary*) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForRayTracing&, AZ::RHI::SingleDevicePipelineLibrary*) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
        };

        class Scope
            : public AZ::RHI::Scope
        {
        public:
            AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator);

        private:
            void InitInternal() override {}
            void ActivateInternal() override {}
            void CompileInternal() override {}
            void DeactivateInternal() override {}
            void ShutdownInternal() override {}
        };

        class FrameGraphCompiler
            : public AZ::RHI::FrameGraphCompiler
        {
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphCompiler, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::MessageOutcome CompileInternal(const AZ::RHI::FrameGraphCompileRequest&) override { return AZ::Success(); }
            void ShutdownInternal() override {}
        };

        class FrameGraphExecuter
            : public AZ::RHI::FrameGraphExecuter
        {
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuter, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(const AZ::RHI::FrameGraphExecuterDescriptor&) override { return AZ::RHI::ResultCode::Success; }
            void BeginInternal(const AZ::RHI::FrameGraph&) override {}
            void ExecuteGroupInternal(AZ::RHI::FrameGraphExecuteGroup&) override {}
            void EndInternal() override {}
            void ShutdownInternal() override {}
        };

        class TransientAttachmentPool
            : public AZ::RHI::SingleDeviceTransientAttachmentPool
        {
        public:
            AZ_CLASS_ALLOCATOR(TransientAttachmentPool, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::TransientAttachmentPoolDescriptor&) override { return AZ::RHI::ResultCode::Success; }
            void BeginInternal([[maybe_unused]] const AZ::RHI::TransientAttachmentPoolCompileFlags flags, [[maybe_unused]] const AZ::RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint) override {}
            AZ::RHI::SingleDeviceImage* ActivateImage(const AZ::RHI::TransientImageDescriptor&) override { return nullptr; }
            AZ::RHI::SingleDeviceBuffer* ActivateBuffer(const AZ::RHI::TransientBufferDescriptor&) override { return nullptr; }
            void DeactivateBuffer(const AZ::RHI::AttachmentId&) override {}
            void DeactivateImage(const AZ::RHI::AttachmentId&) override {}
            void EndInternal() override {}
            void ShutdownInternal() override {}
        };

        class Query
            : public AZ::RHI::SingleDeviceQuery
        {
            friend class QueryPool;
        public:
            AZ_CLASS_ALLOCATOR(Query, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode BeginInternal([[maybe_unused]] AZ::RHI::CommandList& commandList, [[maybe_unused]] AZ::RHI::QueryControlFlags flags) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode EndInternal([[maybe_unused]] AZ::RHI::CommandList& commandList) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode WriteTimestampInternal([[maybe_unused]] AZ::RHI::CommandList& commandList) override { return AZ::RHI::ResultCode::Success; };
        };

        class QueryPool
            : public AZ::RHI::SingleDeviceQueryPool
        {
        public:
            AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal([[maybe_unused]] AZ::RHI::Device& device, [[maybe_unused]] const AZ::RHI::QueryPoolDescriptor& descriptor) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitQueryInternal([[maybe_unused]] AZ::RHI::SingleDeviceQuery& query) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode GetResultsInternal(
                [[maybe_unused]] uint32_t startIndex,
                [[maybe_unused]] uint32_t queryCount,
                [[maybe_unused]] uint64_t* results,
                [[maybe_unused]] uint32_t resultsCount,
                [[maybe_unused]] AZ::RHI::QueryResultFlagBits flags) override
            { return AZ::RHI::ResultCode::Success; }
        };

        class IndirectBufferWriter
            : public AZ::RHI::SingleDeviceIndirectBufferWriter
        {
        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferWriter, AZ::ThreadPoolAllocator);

        private:
            void SetVertexViewInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::SingleDeviceStreamBufferView& view) override {}
            void SetIndexViewInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::SingleDeviceIndexBufferView& view) override {}
            void DrawInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::DrawLinear& arguments) override {}
            void DrawIndexedInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::DrawIndexed& arguments) override {}
            void DispatchInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::DispatchDirect& arguments) override {}
            void SetRootConstantsInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const uint8_t* data, [[maybe_unused]] uint32_t byteSize) override {}
        };

        class IndirectBufferSignature
            : public AZ::RHI::SingleDeviceIndirectBufferSignature
        {
        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferSignature, AZ::ThreadPoolAllocator);

        private:
            AZ::RHI::ResultCode InitInternal([[maybe_unused]] AZ::RHI::Device& device, [[maybe_unused]] const AZ::RHI::SingleDeviceIndirectBufferSignatureDescriptor& descriptor) override { return AZ::RHI::ResultCode::Success; }
            uint32_t GetByteStrideInternal() const override { return 0; }
            uint32_t GetOffsetInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index) const override { return 0; }
            void ShutdownInternal() override {}
        };
    }
}
