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
    namespace DX12
    {
        class PipelineLibrary final
            : public RHI::DevicePipelineLibrary
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator);
            AZ_DISABLE_COPY_MOVE(PipelineLibrary);

            static RHI::Ptr<PipelineLibrary> Create();

            RHI::Ptr<ID3D12PipelineState> CreateGraphicsPipelineState(uint64_t hash, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc);
            RHI::Ptr<ID3D12PipelineState> CreateComputePipelineState(uint64_t hash, const D3D12_COMPUTE_PIPELINE_STATE_DESC& pipelineStateDesc);

        private:
            PipelineLibrary() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DevicePipelineLibrary
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::DevicePipelineLibraryDescriptor& descriptor) override;
            void ShutdownInternal() override;
            RHI::ResultCode MergeIntoInternal(AZStd::span<const RHI::DevicePipelineLibrary* const> libraries) override;
            RHI::ConstPtr<RHI::PipelineLibraryData> GetSerializedDataInternal() const override;
            bool IsMergeRequired() const;
            bool SaveSerializedDataInternal(const AZStd::string& filePath) const override;
            //////////////////////////////////////////////////////////////////////////

            ID3D12DeviceX* m_dx12Device = nullptr;

#if defined (AZ_DX12_USE_PIPELINE_LIBRARY)
            // This is stored because the DX12 library doesn't actually copy the data.
            RHI::ConstPtr<RHI::PipelineLibraryData> m_serializedData;

            mutable AZStd::mutex m_mutex;
            RHI::Ptr<ID3D12PipelineLibraryX> m_library;

            // Internally tracks additions to the library. Used when merging libraries together.
            AZStd::unordered_map<AZStd::wstring, RHI::Ptr<ID3D12PipelineState>> m_pipelineStates;
#endif
        };
    }
}
