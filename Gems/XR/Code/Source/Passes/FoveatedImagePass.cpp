/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Passes/FoveatedImagePass.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace XR
{    
    AZ::RPI::Ptr<FoveatedImagePass> FoveatedImagePass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<FoveatedImagePass> pass = aznew FoveatedImagePass(descriptor);
        return pass;
    }
    
    FoveatedImagePass::FoveatedImagePass(const AZ::RPI::PassDescriptor& descriptor)
        : Base(descriptor)
    {
        const FoveatedImagePassData* passData = AZ::RPI::PassUtils::GetPassData<FoveatedImagePassData>(descriptor);
        if (passData)
        {
            m_foveatedLevel = passData->m_foveatedLevel;
        }
    }
        
    void FoveatedImagePass::ResetInternal()
    {
        m_foveatedImage.reset();
        m_foveatedAttachment.reset();
        Base::ResetInternal();
    }

    void FoveatedImagePass::BuildInternal()
    {
        AZ::RHI::XRRenderingInterface* xrSystem = AZ::RHI::RHISystemInterface::Get()->GetXRSystem();
        if (xrSystem)
        {
            m_foveatedAttachment = FindAttachment(AZ::Name("FoveatedImage"));
            // Update the image attachment descriptor to sync up size and format
            m_foveatedAttachment->Update(true);
            AZ::RHI::ImageDescriptor& imageDesc = m_foveatedAttachment->m_descriptor.m_image;

            // Calculate the size of the shading rate attachment.
            AZ::RHI::Device* device = AZ::RHI::RHISystemInterface::Get()->GetDevice();
            const auto& tileSize = device->GetLimits().m_shadingRateTileSize;
            const uint32_t width = aznumeric_cast<uint32_t>(ceil(static_cast<float>(imageDesc.m_size.m_width) / tileSize.m_width));
            const uint32_t height = aznumeric_cast<uint32_t>(ceil(static_cast<float>(imageDesc.m_size.m_height) / tileSize.m_height));
            AZ::RPI::AttachmentImage* currentImage = azrtti_cast<AZ::RPI::AttachmentImage*>(m_foveatedAttachment->m_importedResource.get());

            // If there's a resource already and the size didn't change, just keep using the old AttachmentImage.
            if (!m_foveatedAttachment->m_importedResource || AZ::RHI::Size(width, height, 1) != currentImage->GetDescriptor().m_size)
            {
                AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

                imageDesc.m_size.m_width = width;
                imageDesc.m_size.m_height = height;
                imageDesc.m_bindFlags |= AZ::RHI::ImageBindFlags::ShadingRate;

                if (imageDesc.m_format == AZ::RHI::Format::Unknown)
                {
                    // Find the appropriate format for the image
                    for (uint32_t i = 0; i < static_cast<uint32_t>(AZ::RHI::Format::Count); ++i)
                    {
                        AZ::RHI::Format format = static_cast<AZ::RHI::Format>(i);
                        AZ::RHI::FormatCapabilities capabilities = device->GetFormatCapabilities(format);
                        if (AZ::RHI::CheckBitsAll(capabilities, AZ::RHI::FormatCapabilities::ShadingRate))
                        {
                            imageDesc.m_format = format;
                            break;
                        }
                    }
                }

                // The ImageViewDescriptor must be specified to make sure the frame graph compiler doesn't treat this as a transient image.
                AZ::RHI::ImageViewDescriptor viewDesc = AZ::RHI::ImageViewDescriptor::Create(imageDesc.m_format, 0, 0);
                viewDesc.m_aspectFlags = AZ::RHI::ImageAspectFlags::Color;

                AZStd::string imageName = AZ::RPI::ConcatPassString(GetPathName(), m_foveatedAttachment->m_path);
                auto attachmentImage = AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, AZ::Name(imageName), nullptr, &viewDesc);

                if (attachmentImage)
                {
                    // Fill up the contents of the shading rate image
                    xrSystem->InitVariableRateShadingImageContent(attachmentImage->GetRHIImage(), m_foveatedLevel);

                    m_foveatedAttachment->m_path = attachmentImage->GetAttachmentId();
                    m_foveatedAttachment->m_importedResource = attachmentImage;
                    m_foveatedImage = attachmentImage;
                }
            }
        }
        Base::BuildInternal();
    }
} // namespace XR
