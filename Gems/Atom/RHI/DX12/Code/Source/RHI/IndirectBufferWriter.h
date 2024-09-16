/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceIndirectBufferWriter.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace DX12
    {
        class Image;

        //! DX12 Implementation of the RHI IndirectBufferWriter.
        //! It writes DX12 Indirect Commands into the a buffer or memory location.
        //! It supports Tier2 indirect commands.
        class IndirectBufferWriter final
            : public RHI::DeviceIndirectBufferWriter
        {
            using Base = RHI::DeviceIndirectBufferWriter;
        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferWriter, AZ::ThreadPoolAllocator);
            AZ_RTTI(IndirectBufferWriter, "{A83429FE-19AD-423C-BEEA-884AA31DCD77}", Base);

            static RHI::Ptr<IndirectBufferWriter> Create();

        private:
            IndirectBufferWriter() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceIndirectBufferWriter
            void SetVertexViewInternal(RHI::IndirectCommandIndex index, const RHI::DeviceStreamBufferView& view) override;
            void SetIndexViewInternal(RHI::IndirectCommandIndex index, const RHI::DeviceIndexBufferView& view) override;
            void DrawInternal(RHI::IndirectCommandIndex index, const RHI::DrawLinear& arguments, const RHI::DrawInstanceArguments& drawInstanceArgs) override;
            void DrawIndexedInternal(RHI::IndirectCommandIndex index, const RHI::DrawIndexed& arguments, const RHI::DrawInstanceArguments& drawInstanceArgs) override;
            void DispatchInternal(RHI::IndirectCommandIndex index, const RHI::DispatchDirect& arguments) override;
            void SetRootConstantsInternal(RHI::IndirectCommandIndex index, const uint8_t* data, uint32_t byteSize) override;
            //////////////////////////////////////////////////////////////////////////

            uint8_t* GetCommandTargetMemory(RHI::IndirectCommandIndex index) const;
        };
    }
}
