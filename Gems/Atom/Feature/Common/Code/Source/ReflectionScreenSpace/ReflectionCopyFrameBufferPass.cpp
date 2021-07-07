/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
