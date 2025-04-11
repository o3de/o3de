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
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceFence.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/DeviceImagePool.h>
#include <Atom/RHI/DeviceIndirectBufferSignature.h>
#include <Atom/RHI/DeviceIndirectBufferWriter.h>
#include <Atom/RHI/DevicePipelineLibrary.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/DeviceQuery.h>
#include <Atom/RHI/DeviceQueryPool.h>
#include <Atom/RHI/DeviceShaderResourceGroup.h>
#include <Atom/RHI/DeviceShaderResourceGroupPool.h>
#include <Atom/RHI/DeviceStreamingImagePool.h>
#include <Atom/RHI/DeviceSwapChain.h>
#include <Atom/RHI/DeviceTransientAttachmentPool.h>

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
            AZStd::pair<uint64_t, uint64_t> GetCalibratedTimestamp([[maybe_unused]] AZ::RHI::HardwareQueueClass queueClass) override
            {
                return { 0ull, AZStd::chrono::microseconds().count() };
            };
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
            : public AZ::RHI::DeviceImageView
        {
        public:
            AZ_CLASS_ALLOCATOR(ImageView, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::DeviceResource&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InvalidateInternal() override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
        };

        class Image
            : public AZ::RHI::DeviceImage
        {
        public:
            AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator);

        private:
            void GetSubresourceLayoutsInternal(
                const AZ::RHI::ImageSubresourceRange&, AZ::RHI::DeviceImageSubresourceLayout*, size_t*) const override
            {
            }
            bool IsStreamableInternal() const override {return true;};
        };

        class BufferView
            : public AZ::RHI::DeviceBufferView
        {
        public:
            AZ_CLASS_ALLOCATOR(BufferView, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::DeviceResource&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InvalidateInternal() override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
        };

        class Buffer
            : public AZ::RHI::DeviceBuffer
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
            : public AZ::RHI::DeviceBufferPool
        {
        public:
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::BufferPoolDescriptor&) override { return AZ::RHI::ResultCode::Success;}

            AZ::RHI::ResultCode InitBufferInternal(AZ::RHI::DeviceBuffer& bufferBase, const AZ::RHI::BufferDescriptor& descriptor) override
            {
                AZ_Assert(IsInitialized(), "Buffer Pool is not initialized");

                Buffer& buffer = static_cast<Buffer&>(bufferBase);
                buffer.m_data.resize(descriptor.m_byteCount);

                return AZ::RHI::ResultCode::Success;
            }

            void ShutdownResourceInternal(AZ::RHI::DeviceResource& resourceBase) override
            {
                Buffer& buffer = static_cast<Buffer&>(resourceBase);
                buffer.m_data.clear();
            }

            AZ::RHI::ResultCode MapBufferInternal(const AZ::RHI::DeviceBufferMapRequest& request, AZ::RHI::DeviceBufferMapResponse& response) override
            {
                Buffer& buffer = static_cast<Buffer&>(*request.m_buffer);
                response.m_data = buffer.Map();
                return AZ::RHI::ResultCode::Success;
            }

            void UnmapBufferInternal(AZ::RHI::DeviceBuffer& bufferBase) override
            {
                Buffer& buffer = static_cast<Buffer&>(bufferBase);
                buffer.Unmap();
            }

            AZ::RHI::ResultCode OrphanBufferInternal(AZ::RHI::DeviceBuffer&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode StreamBufferInternal([[maybe_unused]] const AZ::RHI::DeviceBufferStreamRequest& request) override { return AZ::RHI::ResultCode::Success; }
            void ComputeFragmentation() const override {}
        };

        class ImagePool
            : public AZ::RHI::DeviceImagePool
        {
        public:
            AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ImagePoolDescriptor&) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            AZ::RHI::ResultCode UpdateImageContentsInternal(const AZ::RHI::DeviceImageUpdateRequest&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitImageInternal(const AZ::RHI::DeviceImageInitRequest&) override
            {
                return AZ::RHI::ResultCode::Success;
            }
            void ShutdownResourceInternal(AZ::RHI::DeviceResource&) override {}
        };

        class StreamingImagePool
            : public AZ::RHI::DeviceStreamingImagePool
        {
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator);

            void ComputeFragmentation() const override {}

        private:
            AZ::RHI::ResultCode InitImageInternal([[maybe_unused]] const AZ::RHI::DeviceStreamingImageInitRequest& request) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            void ShutdownResourceInternal(AZ::RHI::DeviceResource&) override {}
            AZ::RHI::ResultCode ExpandImageInternal(const AZ::RHI::DeviceStreamingImageExpandRequest&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode TrimImageInternal(AZ::RHI::DeviceImage&, uint32_t) override { return AZ::RHI::ResultCode::Success; }
        };

        class SwapChain
            : public AZ::RHI::DeviceSwapChain
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
            : public AZ::RHI::DeviceFence
        {
        public:
            AZ_CLASS_ALLOCATOR(Fence, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, AZ::RHI::FenceState, [[maybe_unused]] bool usedForWaitingOnDevice) override
            {
                return AZ::RHI::ResultCode::Success;
            }
            void ShutdownInternal() override {}
            void SignalOnCpuInternal() override {}
            void WaitOnCpuInternal() const override {};
            void ResetInternal() override {}
            AZ::RHI::FenceState GetFenceStateInternal() const override { return AZ::RHI::FenceState::Reset; }
        };

        class ShaderResourceGroupPool
            : public AZ::RHI::DeviceShaderResourceGroupPool
        {
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ShaderResourceGroupPoolDescriptor&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitGroupInternal(AZ::RHI::DeviceShaderResourceGroup&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode CompileGroupInternal(AZ::RHI::DeviceShaderResourceGroup&,const AZ::RHI::DeviceShaderResourceGroupData&) override { return AZ::RHI::ResultCode::Success; }
        };

        class ShaderResourceGroup
            : public AZ::RHI::DeviceShaderResourceGroup
        {
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator);
        };

        class PipelineLibrary
            : public AZ::RHI::DevicePipelineLibrary
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, [[maybe_unused]] const AZ::RHI::DevicePipelineLibraryDescriptor& descriptor) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            AZ::RHI::ResultCode MergeIntoInternal(AZStd::span<const AZ::RHI::DevicePipelineLibrary* const>) override { return AZ::RHI::ResultCode::Success; }
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
            : public AZ::RHI::DevicePipelineState
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForDraw&, AZ::RHI::DevicePipelineLibrary*) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForDispatch&, AZ::RHI::DevicePipelineLibrary*) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForRayTracing&, AZ::RHI::DevicePipelineLibrary*) override { return AZ::RHI::ResultCode::Success; }
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
            AZ::RHI::ResultCode InitInternal() override { return AZ::RHI::ResultCode::Success; }
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
            : public AZ::RHI::DeviceTransientAttachmentPool
        {
        public:
            AZ_CLASS_ALLOCATOR(TransientAttachmentPool, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::TransientAttachmentPoolDescriptor&) override { return AZ::RHI::ResultCode::Success; }
            void BeginInternal([[maybe_unused]] const AZ::RHI::TransientAttachmentPoolCompileFlags flags, [[maybe_unused]] const AZ::RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint) override {}
            AZ::RHI::DeviceImage* ActivateImage(const AZ::RHI::TransientImageDescriptor&) override { return nullptr; }
            AZ::RHI::DeviceBuffer* ActivateBuffer(const AZ::RHI::TransientBufferDescriptor&) override { return nullptr; }
            void DeactivateBuffer(const AZ::RHI::AttachmentId&) override {}
            void DeactivateImage(const AZ::RHI::AttachmentId&) override {}
            void EndInternal() override {}
            void ShutdownInternal() override {}
        };

        class Query
            : public AZ::RHI::DeviceQuery
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
            : public AZ::RHI::DeviceQueryPool
        {
        public:
            AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator);

        private:
            AZ::RHI::ResultCode InitInternal([[maybe_unused]] AZ::RHI::Device& device, [[maybe_unused]] const AZ::RHI::QueryPoolDescriptor& descriptor) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitQueryInternal([[maybe_unused]] AZ::RHI::DeviceQuery& query) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode GetResultsInternal(
                [[maybe_unused]] uint32_t startIndex,
                [[maybe_unused]] uint32_t queryCount,
                [[maybe_unused]] uint64_t* results,
                [[maybe_unused]] uint32_t resultsCount,
                [[maybe_unused]] AZ::RHI::QueryResultFlagBits flags) override
            { return AZ::RHI::ResultCode::Success; }
        };

        class IndirectBufferWriter
            : public AZ::RHI::DeviceIndirectBufferWriter
        {
        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferWriter, AZ::ThreadPoolAllocator);

        private:
            void SetVertexViewInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::DeviceStreamBufferView& view) override {}
            void SetIndexViewInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::DeviceIndexBufferView& view) override {}
            void DrawInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::DrawLinear& arguments, [[maybe_unused]] const AZ::RHI::DrawInstanceArguments& drawInstanceArgs) override {}
            void DrawIndexedInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::DrawIndexed& arguments, [[maybe_unused]] const AZ::RHI::DrawInstanceArguments& drawInstanceArgs) override {}
            void DispatchInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::DispatchDirect& arguments) override {}
            void SetRootConstantsInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const uint8_t* data, [[maybe_unused]] uint32_t byteSize) override {}
        };

        class IndirectBufferSignature
            : public AZ::RHI::DeviceIndirectBufferSignature
        {
        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferSignature, AZ::ThreadPoolAllocator);

        private:
            AZ::RHI::ResultCode InitInternal([[maybe_unused]] AZ::RHI::Device& device, [[maybe_unused]] const AZ::RHI::DeviceIndirectBufferSignatureDescriptor& descriptor) override { return AZ::RHI::ResultCode::Success; }
            uint32_t GetByteStrideInternal() const override { return 0; }
            uint32_t GetOffsetInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index) const override { return 0; }
            void ShutdownInternal() override {}
        };
    }
}
