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

#include "ReflectionCopyFrameBufferPass.h"
#include "ReflectionScreenSpaceBlurPass.h"
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ReflectionCopyFrameBufferPass> ReflectionCopyFrameBufferPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ReflectionCopyFrameBufferPass> pass = aznew ReflectionCopyFrameBufferPass(descriptor);
            return AZStd::move(pass);
        }

        ReflectionCopyFrameBufferPass::ReflectionCopyFrameBufferPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionCopyFrameBufferPass::BuildInternal()
        {
            RPI::PassHierarchyFilter passFilter(AZ::Name("ReflectionScreenSpaceBlurPass"));
            const AZStd::vector<RPI::Pass*>& passes = RPI::PassSystemInterface::Get()->FindPasses(passFilter);
            if (!passes.empty())
            {
                Render::ReflectionScreenSpaceBlurPass* blurPass = azrtti_cast<ReflectionScreenSpaceBlurPass*>(passes.front());
                Data::Instance<RPI::AttachmentImage>& frameBufferAttachment = blurPass->GetFrameBufferImageAttachment();

                RPI::PassAttachmentBinding& outputBinding = GetOutputBinding(0);
                AttachImageToSlot(outputBinding.m_name, frameBufferAttachment);
            }

            FullscreenTrianglePass::BuildInternal();
        }
    }   // namespace RPI
}   // namespace AZ
