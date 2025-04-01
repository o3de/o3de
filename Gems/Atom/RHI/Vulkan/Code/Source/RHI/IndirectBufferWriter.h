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
    namespace Vulkan
    {
        //! Vulkan implementation of the helper RHI class IndirectBufferWriter.
        //! It writes indirect commands into a memory location following the layout of
        //! the Vulkan's commands.
        //! It only supports Tier1 commands.
        class IndirectBufferWriter final
            : public RHI::DeviceIndirectBufferWriter
        {
            using Base = RHI::DeviceIndirectBufferWriter;
        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferWriter, AZ::ThreadPoolAllocator);
            AZ_RTTI(IndirectBufferWriter, "{089BDED9-EDF3-4C72-9B52-57926DD29BBA}", Base);

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

            uint8_t* GetCommandMemoryMap(RHI::IndirectCommandIndex index) const;
        };
    }
}
