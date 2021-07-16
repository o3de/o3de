/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/IndirectBufferSignature.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        //! Vulkan implementation of the RHI IndirectBufferSignature class.
        //! Vulkan doesn't have an object that represents a signature, so this class doesn't
        //! create any platform objects. We use it to expose the strides of the indirect commands.
        class IndirectBufferSignature final
            : public RHI::IndirectBufferSignature
        {
            using Base = RHI::IndirectBufferSignature;
        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferSignature, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(IndirectBufferSignature, "{29CAB46E-351D-4BC1-A93B-B16EA0496645}", Base);

            static RHI::Ptr<IndirectBufferSignature> Create();

        private:
            IndirectBufferSignature() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::IndirectBufferSignature
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::IndirectBufferSignatureDescriptor& descriptor) override;
            uint32_t GetByteStrideInternal() const override;
            uint32_t GetOffsetInternal(RHI::IndirectCommandIndex index) const override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            uint32_t m_stride = 0;
            AZStd::vector<uint32_t> m_offsets;
        };
    }
}
