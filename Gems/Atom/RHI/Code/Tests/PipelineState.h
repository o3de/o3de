/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/RHI/SingleDevicePipelineLibrary.h>
#include <Atom/RHI/SingleDevicePipelineState.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class PipelineLibrary
        : public AZ::RHI::SingleDevicePipelineLibrary
    {
    public:
        AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator);

        AZStd::unordered_map<uint64_t, const AZ::RHI::SingleDevicePipelineState*> m_pipelineStates;

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::SingleDevicePipelineLibraryDescriptor&) override { return AZ::RHI::ResultCode::Success; }
        void ShutdownInternal() override;
        AZ::RHI::ResultCode MergeIntoInternal(AZStd::span<const AZ::RHI::SingleDevicePipelineLibrary* const>) override;
        AZ::RHI::ConstPtr<AZ::RHI::PipelineLibraryData> GetSerializedDataInternal() const override { return nullptr; }
        bool SaveSerializedDataInternal([[maybe_unused]] const AZStd::string& filePath) const override { return false; }
    };

    class PipelineState
        : public AZ::RHI::SingleDevicePipelineState
    {
    public:
        AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForDraw&, AZ::RHI::SingleDevicePipelineLibrary*) override;
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForDispatch&, AZ::RHI::SingleDevicePipelineLibrary*) override;
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::PipelineStateDescriptorForRayTracing&, AZ::RHI::SingleDevicePipelineLibrary*) override;
        void ShutdownInternal() override {}
    };
}
