/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Device.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/IndirectBufferSignature.h>
#include <Atom/RHI/IndirectBufferWriter.h>
#include <Atom/RHI/StreamingImagePool.h>
#include <Atom/RHI/SwapChain.h>
#include <Atom/RHI/Fence.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/PipelineLibrary.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/Query.h>
#include <Atom/RHI/QueryPool.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/FrameGraphCompiler.h>
#include <Atom/RHI/FrameGraphExecuter.h>
#include <Atom/RHI/TransientAttachmentPool.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>

namespace UnitTest
{
    namespace StubRHI
    {
        class PhysicalDevice
            : public AZ::RHI::PhysicalDevice
        {
        public:
            AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator, 0);
            static AZ::RHI::PhysicalDeviceList Enumerate();

        private:
            PhysicalDevice();
        };

        class Device
            : public AZ::RHI::Device
        {
        public:
            AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator, 0);
            Device();

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::PhysicalDevice&) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            void BeginFrameInternal() override {}
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
        };

        class ImageView
            : public AZ::RHI::ImageView
        {
        public:
            AZ_CLASS_ALLOCATOR(ImageView, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::Resource&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InvalidateInternal() override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
        };

        class Image
            : public AZ::RHI::Image
        {
        public:
            AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator, 0);

        private:
            void GetSubresourceLayoutsInternal(const AZ::RHI::ImageSubresourceRange&, AZ::RHI::ImageSubresourceLayoutPlaced*, size_t*) const override {}
        };

        class BufferView
            : public AZ::RHI::BufferView
        {
        public:
            AZ_CLASS_ALLOCATOR(BufferView, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::Resource&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InvalidateInternal() override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
        };

        class Buffer
            : public AZ::RHI::Buffer
        {
            friend class BufferPool;
        public:
            AZ_CLASS_ALLOCATOR(Buffer, AZ::SystemAllocator, 0);

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
            : public AZ::RHI::BufferPool
        {
        public:
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::BufferPoolDescriptor&) override { return AZ::RHI::ResultCode::Success;}
            
            AZ::RHI::ResultCode InitBufferInternal(AZ::RHI::Buffer& bufferBase, const AZ::RHI::BufferDescriptor& descriptor) override
            {
                AZ_Assert(IsInitialized(), "Buffer Pool is not initialized");

                Buffer& buffer = static_cast<Buffer&>(bufferBase);
                buffer.m_data.resize(descriptor.m_byteCount);

                return AZ::RHI::ResultCode::Success;
            }

            void ShutdownResourceInternal(AZ::RHI::Resource& resourceBase) override
            {
                Buffer& buffer = static_cast<Buffer&>(resourceBase);
                buffer.m_data.clear();
            }

            AZ::RHI::ResultCode MapBufferInternal(const AZ::RHI::BufferMapRequest& request, AZ::RHI::BufferMapResponse& response) override
            {
                Buffer& buffer = static_cast<Buffer&>(*request.m_buffer);
                response.m_data = buffer.Map();
                return AZ::RHI::ResultCode::Success;
            }

            void UnmapBufferInternal(AZ::RHI::Buffer& bufferBase) override
            {
                Buffer& buffer = static_cast<Buffer&>(bufferBase);
                buffer.Unmap();
            }

            AZ::RHI::ResultCode OrphanBufferInternal(AZ::RHI::Buffer&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode StreamBufferInternal([[maybe_unused]] const AZ::RHI::BufferStreamRequest& request) override { return AZ::RHI::ResultCode::Success; }
        };

        class ImagePool
            : public AZ::RHI::ImagePool
        {
        public:
            AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ImagePoolDescriptor&) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            AZ::RHI::ResultCode UpdateImageContentsInternal(const AZ::RHI::ImageUpdateRequest&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitImageInternal(const AZ::RHI::ImageInitRequest&) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownResourceInternal(AZ::RHI::Resource&) override {}
        };

        class StreamingImagePool
            : public AZ::RHI::StreamingImagePool
        {
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitImageInternal([[maybe_unused]] const AZ::RHI::StreamingImageInitRequest& request) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            void ShutdownResourceInternal(AZ::RHI::Resource&) override {}
            AZ::RHI::ResultCode ExpandImageInternal(const AZ::RHI::StreamingImageExpandRequest&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode TrimImageInternal(AZ::RHI::Image&, uint32_t) override { return AZ::RHI::ResultCode::Success; }
        };

        class SwapChain
            : public AZ::RHI::SwapChain
        {
        public:
            AZ_CLASS_ALLOCATOR(SwapChain, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::SwapChainDescriptor&, [[maybe_unused]] AZ::RHI::SwapChainDimensions* outNativeDimensions) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitImageInternal(const InitImageRequest&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode ResizeInternal([[maybe_unused]] const AZ::RHI::SwapChainDimensions& dimensions, [[maybe_unused]] AZ::RHI::SwapChainDimensions* nativeDimensions) override { return AZ::RHI::ResultCode::Success; };
            uint32_t PresentInternal() override { return 0; };
        };

        class Fence
            : public AZ::RHI::Fence
        {
        public:
            AZ_CLASS_ALLOCATOR(Fence, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, AZ::RHI::FenceState) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            void SignalOnCpuInternal() override {}
            void WaitOnCpuInternal() const override {};
            void ResetInternal() override {}
            AZ::RHI::FenceState GetFenceStateInternal() const override { return AZ::RHI::FenceState::Reset; }
        };

        class ShaderResourceGroupPool
            : public AZ::RHI::ShaderResourceGroupPool
        {
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ShaderResourceGroupPoolDescriptor&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitGroupInternal(AZ::RHI::ShaderResourceGroup&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode CompileGroupInternal(AZ::RHI::ShaderResourceGroup&,const AZ::RHI::ShaderResourceGroupData&) override { return AZ::RHI::ResultCode::Success; }
        };

        class ShaderResourceGroup
            : public AZ::RHI::ShaderResourceGroup
        {
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator, 0);
        };

        class PipelineLibrary
            : public AZ::RHI::PipelineLibrary
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineLibraryData*) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            AZ::RHI::ResultCode MergeIntoInternal(AZStd::array_view<const AZ::RHI::PipelineLibrary*>) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ConstPtr<AZ::RHI::PipelineLibraryData> GetSerializedDataInternal() const override { return nullptr; }
        };

        class ShaderStageFunction
            : public AZ::RHI::ShaderStageFunction
        {
        public:
            AZ_RTTI(ShaderStageFunction, "{644DBC98-C864-488C-BBA8-0137C210C1E2}", AZ::RHI::ShaderStageFunction);
            AZ_CLASS_ALLOCATOR(ShaderStageFunction, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode FinalizeInternal() override { return AZ::RHI::ResultCode::Success; }
        };

        class PipelineState
            : public AZ::RHI::PipelineState
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForDraw&, AZ::RHI::PipelineLibrary*) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForDispatch&, AZ::RHI::PipelineLibrary*) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForRayTracing&, AZ::RHI::PipelineLibrary*) override { return AZ::RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
        };

        class Scope
            : public AZ::RHI::Scope
        {
        public:
            AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator, 0);

        private:
            void InitInternal() override {}
            void ActivateInternal() override {}
            void CompileInternal([[maybe_unused]] AZ::RHI::Device& device) override {}
            void DeactivateInternal() override {}
            void ShutdownInternal() override {}
        };

        class FrameGraphCompiler
            : public AZ::RHI::FrameGraphCompiler
        {
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphCompiler, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::MessageOutcome CompileInternal(const AZ::RHI::FrameGraphCompileRequest&) override { return AZ::Success(); }
            void ShutdownInternal() override {}
        };

        class FrameGraphExecuter
            : public AZ::RHI::FrameGraphExecuter
        {
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuter, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(const AZ::RHI::FrameGraphExecuterDescriptor&) override { return AZ::RHI::ResultCode::Success; }
            void BeginInternal(const AZ::RHI::FrameGraph&) override {}
            void ExecuteGroupInternal(AZ::RHI::FrameGraphExecuteGroup&) override {}
            void EndInternal() override {}
            void ShutdownInternal() override {}
        };

        class TransientAttachmentPool
            : public AZ::RHI::TransientAttachmentPool
        {
        public:
            AZ_CLASS_ALLOCATOR(TransientAttachmentPool, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::TransientAttachmentPoolDescriptor&) override { return AZ::RHI::ResultCode::Success; }
            void BeginInternal([[maybe_unused]] const AZ::RHI::TransientAttachmentPoolCompileFlags flags, [[maybe_unused]] const AZ::RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint) override {}
            AZ::RHI::Image* ActivateImage(const AZ::RHI::TransientImageDescriptor&) override { return nullptr; }
            AZ::RHI::Buffer* ActivateBuffer(const AZ::RHI::TransientBufferDescriptor&) override { return nullptr; }
            void DeactivateBuffer(const AZ::RHI::AttachmentId&) override {}
            void DeactivateImage(const AZ::RHI::AttachmentId&) override {}
            void EndInternal() override {}
            void ShutdownInternal() override {}
        };

        class Query
            : public AZ::RHI::Query
        {
            friend class QueryPool;
        public:
            AZ_CLASS_ALLOCATOR(Query, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode BeginInternal([[maybe_unused]] AZ::RHI::CommandList& commandList, [[maybe_unused]] AZ::RHI::QueryControlFlags flags) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode EndInternal([[maybe_unused]] AZ::RHI::CommandList& commandList) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode WriteTimestampInternal([[maybe_unused]] AZ::RHI::CommandList& commandList) override { return AZ::RHI::ResultCode::Success; };
        };

        class QueryPool
            : public AZ::RHI::QueryPool
        {
        public:
            AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal([[maybe_unused]] AZ::RHI::Device& device, [[maybe_unused]] const AZ::RHI::QueryPoolDescriptor& descriptor) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode InitQueryInternal([[maybe_unused]] AZ::RHI::Query& query) override { return AZ::RHI::ResultCode::Success; }
            AZ::RHI::ResultCode GetResultsInternal(
                [[maybe_unused]] uint32_t startIndex, 
                [[maybe_unused]] uint32_t queryCount, 
                [[maybe_unused]] uint64_t* results, 
                [[maybe_unused]] uint32_t resultsCount, 
                [[maybe_unused]] AZ::RHI::QueryResultFlagBits flags) override 
            { return AZ::RHI::ResultCode::Success; }
        };

        class IndirectBufferWriter
            : public AZ::RHI::IndirectBufferWriter
        {
        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferWriter, AZ::ThreadPoolAllocator, 0);

        private:
            void SetVertexViewInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::StreamBufferView& view) override {}
            void SetIndexViewInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::IndexBufferView& view) override {}
            void DrawInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::DrawLinear& arguments) override {}
            void DrawIndexedInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::DrawIndexed& arguments) override {}
            void DispatchInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const AZ::RHI::DispatchDirect& arguments) override {}
            void SetRootConstantsInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index, [[maybe_unused]] const uint8_t* data, [[maybe_unused]] uint32_t byteSize) override {}
        };

        class IndirectBufferSignature
            : public AZ::RHI::IndirectBufferSignature
        {
        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferSignature, AZ::ThreadPoolAllocator, 0);

        private:
            AZ::RHI::ResultCode InitInternal([[maybe_unused]] AZ::RHI::Device& device, [[maybe_unused]] const AZ::RHI::IndirectBufferSignatureDescriptor& descriptor) override { return AZ::RHI::ResultCode::Success; }
            uint32_t GetByteStrideInternal() const override { return 0; }
            uint32_t GetOffsetInternal([[maybe_unused]] AZ::RHI::IndirectCommandIndex index) const override { return 0; }
            void ShutdownInternal() override {}
        };
    }
}
