/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/RHI/IndirectBufferSignature.h>
#include <Atom/RHI/IndirectBufferWriter.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <gmock/gmock.h>

namespace UnitTest
{
    class IndirectBufferWriter
        : public AZ::RHI::IndirectBufferWriter
    {
    public:
        AZ_CLASS_ALLOCATOR(IndirectBufferWriter, AZ::SystemAllocator, 0);

        uint8_t* GetData() const { return GetTargetMemory(); };

        MOCK_METHOD2(SetVertexViewInternal, void(AZ::RHI::IndirectCommandIndex index, const AZ::RHI::StreamBufferView& view));
        MOCK_METHOD2(SetIndexViewInternal, void(AZ::RHI::IndirectCommandIndex index, const AZ::RHI::IndexBufferView& view));
        MOCK_METHOD2(DrawInternal, void(AZ::RHI::IndirectCommandIndex index, const AZ::RHI::DrawLinear& arguments));
        MOCK_METHOD2(DrawIndexedInternal, void(AZ::RHI::IndirectCommandIndex index, const AZ::RHI::DrawIndexed& arguments));
        MOCK_METHOD2(DispatchInternal, void(AZ::RHI::IndirectCommandIndex index, const AZ::RHI::DispatchDirect& arguments));
        MOCK_METHOD3(SetRootConstantsInternal, void(AZ::RHI::IndirectCommandIndex index, const uint8_t* data, uint32_t byteSize));
    };

    using NiceIndirectBufferWriter = ::testing::NiceMock<IndirectBufferWriter>;

    class IndirectBufferSignature
        : public AZ::RHI::IndirectBufferSignature
    {
    public:
        AZ_CLASS_ALLOCATOR(IndirectBufferSignature, AZ::SystemAllocator, 0);

        MOCK_METHOD2(InitInternal, AZ::RHI::ResultCode(AZ::RHI::Device& device, const AZ::RHI::IndirectBufferSignatureDescriptor& descriptor));
        MOCK_CONST_METHOD0(GetByteStrideInternal, uint32_t());
        MOCK_CONST_METHOD1(GetOffsetInternal, uint32_t(AZ::RHI::IndirectCommandIndex index));
        MOCK_METHOD0(ShutdownInternal, void());
    };

    using NiceIndirectBufferSignature = ::testing::NiceMock<IndirectBufferSignature>;
}
