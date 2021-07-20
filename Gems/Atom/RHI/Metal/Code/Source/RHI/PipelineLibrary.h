/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PipelineLibrary.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Metal
    {
        class PipelineLibrary final
            : public RHI::PipelineLibrary
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator, 0);
            AZ_DISABLE_COPY_MOVE(PipelineLibrary);

            static RHI::Ptr<PipelineLibrary> Create();

        private:
            PipelineLibrary() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::PipelineLibrary
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineLibraryData* serializedData) override;
            void ShutdownInternal() override;
            RHI::ResultCode MergeIntoInternal(AZStd::array_view<const RHI::PipelineLibrary*> libraries) override;
            RHI::ConstPtr<RHI::PipelineLibraryData> GetSerializedDataInternal() const override;
            //////////////////////////////////////////////////////////////////////////

        };
    }
}
