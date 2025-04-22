/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DevicePipelineLibrary.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Metal
    {
        class PipelineLibrary final
            : public RHI::DevicePipelineLibrary
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator);
            AZ_DISABLE_COPY_MOVE(PipelineLibrary);

            static RHI::Ptr<PipelineLibrary> Create();
            id<MTLBinaryArchive> GetNativePipelineCache() const;
            id<MTLRenderPipelineState> CreateGraphicsPipelineState(uint64_t hash, MTLRenderPipelineDescriptor* pipelineStateDesc, NSError** error);
            id<MTLComputePipelineState> CreateComputePipelineState(uint64_t hash, MTLComputePipelineDescriptor* pipelineStateDesc, NSError** error);
            
        private:
            PipelineLibrary() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DevicePipelineLibrary
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::DevicePipelineLibraryDescriptor& descriptor) override;
            void ShutdownInternal() override;
            RHI::ResultCode MergeIntoInternal(AZStd::span<const RHI::DevicePipelineLibrary* const> libraries) override;
            RHI::ConstPtr<RHI::PipelineLibraryData> GetSerializedDataInternal() const override;
            bool IsMergeRequired() const;
            bool SaveSerializedDataInternal(const AZStd::string& filePath) const;
            //////////////////////////////////////////////////////////////////////////

            RHI::DevicePipelineLibraryDescriptor m_descriptor;
            id<MTLBinaryArchive> m_mtlBinaryArchive = nil;
            mutable AZStd::mutex m_mutex;
            
            // Internally tracks additions to the library. Used when merging libraries together.
            AZStd::unordered_map<uint64_t, MTLRenderPipelineDescriptor*> m_renderPipelineStates;
            AZStd::unordered_map<uint64_t, MTLComputePipelineDescriptor*> m_computePipelineStates;
        };
    }
}
