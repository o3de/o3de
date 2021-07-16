/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/RHI/PipelineLibrary.h>
#include <Atom/RHI/PipelineState.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class PipelineLibrary
        : public AZ::RHI::PipelineLibrary
    {
    public:
        AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator, 0);

        AZStd::unordered_map<uint64_t, const AZ::RHI::PipelineState*> m_pipelineStates;

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineLibraryData*) override { return AZ::RHI::ResultCode::Success; }
        void ShutdownInternal() override;
        AZ::RHI::ResultCode MergeIntoInternal(AZStd::array_view<const AZ::RHI::PipelineLibrary*>) override;
        AZ::RHI::ConstPtr<AZ::RHI::PipelineLibraryData> GetSerializedDataInternal() const override { return nullptr; }
    };

    class PipelineState
        : public AZ::RHI::PipelineState
    {
    public:
        AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator, 0);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForDraw&, AZ::RHI::PipelineLibrary*) override;
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForDispatch&, AZ::RHI::PipelineLibrary*) override;
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForRayTracing&, AZ::RHI::PipelineLibrary*) override;
        void ShutdownInternal() override {}
    };
}
