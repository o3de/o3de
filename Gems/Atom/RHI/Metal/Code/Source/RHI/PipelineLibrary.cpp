/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Device.h>
#include <RHI/PipelineLibrary.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<PipelineLibrary> PipelineLibrary::Create()
        {
            return aznew PipelineLibrary;
        }

        RHI::ResultCode PipelineLibrary::InitInternal(RHI::Device& deviceBase, const RHI::PipelineLibraryData* serializedData)
        {
            return RHI::ResultCode::Success;
        }

        void PipelineLibrary::ShutdownInternal()
        {
        }

        RHI::ResultCode PipelineLibrary::MergeIntoInternal(AZStd::array_view<const RHI::PipelineLibrary*> pipelineLibraries)
        {
            return RHI::ResultCode::Success;
        }

        RHI::ConstPtr<RHI::PipelineLibraryData> PipelineLibrary::GetSerializedDataInternal() const
        {
            return nullptr;
        }
    }
}
