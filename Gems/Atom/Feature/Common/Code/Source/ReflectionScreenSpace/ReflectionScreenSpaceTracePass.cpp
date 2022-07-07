/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceTracePass.h"
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ReflectionScreenSpaceTracePass> ReflectionScreenSpaceTracePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ReflectionScreenSpaceTracePass> pass = aznew ReflectionScreenSpaceTracePass(descriptor);
            return AZStd::move(pass);
        }

        ReflectionScreenSpaceTracePass::ReflectionScreenSpaceTracePass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionScreenSpaceTracePass::BuildInternal()
        {
            Data::Instance<RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

            // retrieve the previous frame image attachment from the pass
            AZ_Assert(m_ownedAttachments.size() == 3, "ReflectionScreenSpaceTracePass must have the following attachment images defined: ReflectionImage, DownSampledDepthImage, and PreviousFrameImage");
            RPI::Ptr<RPI::PassAttachment> previousFrameImageAttachment = m_ownedAttachments[2];
            
            // update the image attachment descriptor to sync up size and format
            previousFrameImageAttachment->Update();
            
            // change the lifetime since we want it to live between frames
            previousFrameImageAttachment->m_lifetime = RHI::AttachmentLifetimeType::Imported;
            
            // set the bind flags
            RHI::ImageDescriptor& imageDesc = previousFrameImageAttachment->m_descriptor.m_image;
            imageDesc.m_bindFlags |= RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite;
            
            // create the image attachment
            RHI::ClearValue clearValue = RHI::ClearValue::CreateVector4Float(0, 0, 0, 0);
            m_previousFrameImageAttachment = RPI::AttachmentImage::Create(*pool.get(), imageDesc, Name(previousFrameImageAttachment->m_path.GetCStr()), &clearValue, nullptr);

            previousFrameImageAttachment->m_path = m_previousFrameImageAttachment->GetAttachmentId();
            previousFrameImageAttachment->m_importedResource = m_previousFrameImageAttachment;
        }

    }   // namespace RPI
}   // namespace AZ
