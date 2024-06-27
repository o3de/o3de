/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceIndirectBufferSignature.h>
#include <Atom/RHI.Reflect/IndirectBufferLayout.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/DX12.h>

namespace AZ
{
    namespace DX12
    {
        //! DX12 implementation of the RHI IndirectBufferSignature. 
        //! It represents the DX12 object ID3D12CommandSignature when doing indirect rendering. 
        class IndirectBufferSignature final
            : public RHI::DeviceIndirectBufferSignature
        {
            using Base = RHI::DeviceIndirectBufferSignature;
        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferSignature, AZ::ThreadPoolAllocator);
            AZ_RTTI(IndirectBufferSignature, "{3BAE9C56-555B-4145-96B6-07C81FF9D3AC}", Base);

            static RHI::Ptr<IndirectBufferSignature> Create();

            ID3D12CommandSignature* Get() const;

        private:
            IndirectBufferSignature() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceIndirectBufferSignature
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::DeviceIndirectBufferSignatureDescriptor& descriptor) override;
            uint32_t GetByteStrideInternal() const override;
            uint32_t GetOffsetInternal(RHI::IndirectCommandIndex index) const override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::Ptr<ID3D12CommandSignature> m_signature;

            uint32_t m_stride = 0;
            AZStd::vector<uint32_t> m_offsets;
        };
    }
}
