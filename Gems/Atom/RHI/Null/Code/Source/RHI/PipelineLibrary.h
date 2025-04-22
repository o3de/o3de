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
    namespace Null
    {
        class PipelineLibrary final
            : public RHI::DevicePipelineLibrary
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator);
            AZ_DISABLE_COPY_MOVE(PipelineLibrary);

            static RHI::Ptr<PipelineLibrary> Create();

        private:
            PipelineLibrary() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DevicePipelineLibrary
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::DevicePipelineLibraryDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            RHI::ResultCode MergeIntoInternal([[maybe_unused]] AZStd::span<const RHI::DevicePipelineLibrary* const> libraries) override { return RHI::ResultCode::Success;}
            RHI::ConstPtr<RHI::PipelineLibraryData> GetSerializedDataInternal() const override { return nullptr;}
            bool SaveSerializedDataInternal([[maybe_unused]] const AZStd::string& filePath) const override { return true;}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
