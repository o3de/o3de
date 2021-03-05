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
#include "Atom_RHI_Metal_precompiled.h"

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
