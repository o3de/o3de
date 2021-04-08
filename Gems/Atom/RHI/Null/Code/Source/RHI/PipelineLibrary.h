/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RHI/PipelineLibrary.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Null
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
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::PipelineLibraryData* serializedData) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            RHI::ResultCode MergeIntoInternal([[maybe_unused]] AZStd::array_view<const RHI::PipelineLibrary*> libraries) override { return RHI::ResultCode::Success;}
            RHI::ConstPtr<RHI::PipelineLibraryData> GetSerializedDataInternal() const override { return nullptr;}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
