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

#include <PostProcessing/TaaPass.h>

namespace AZ::Render
{
    
    RPI::Ptr<TaaPass> TaaPass::Create(const RPI::PassDescriptor& descriptor)
    {
        RPI::Ptr<TaaPass> pass = aznew TaaPass(descriptor);
        return pass;
    }
    
    TaaPass::TaaPass(const RPI::PassDescriptor& descriptor)
        : RPI::ComputePass(descriptor)
    {

    }

    void TaaPass::CompileResources(const RHI::FrameGraphCompileContext& /*context*/)
    {

    }
    
    void TaaPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& /*context*/)
    {

    }
    
    void TaaPass::ResetInternal()
    {

    }

    void TaaPass::BuildAttachmentsInternal()
    {

    }

} // namespace AZ::Render
