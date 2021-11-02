/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom_Feature_Traits_Platform.h>
#if AZ_TRAIT_LUXCORE_SUPPORTED

#include "LuxCoreTexture.h"
#include <Atom/Feature/LuxCore/LuxCoreTexturePass.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        LuxCoreTexture::LuxCoreTexture(AZ::Data::Instance<AZ::RPI::Image> image, LuxCoreTextureType type)
        {
            Init(image, type);
        }

        LuxCoreTexture::LuxCoreTexture(const LuxCoreTexture &texture)
        {
            Init(texture.m_texture, texture.m_type);
        }

        LuxCoreTexture::~LuxCoreTexture()
        {
            if (m_rtPipeline)
            {
                m_rtPipeline->RemoveFromScene();
                m_rtPipeline = nullptr;
            }
            
            if (m_texture)
            {
                m_texture = nullptr;
            }
        }

        void LuxCoreTexture::Init(AZ::Data::Instance<AZ::RPI::Image> image, LuxCoreTextureType type)
        {
            m_textureAssetId = image->GetAssetId();
            m_texture = image;
            m_type = type;

            if (m_type == LuxCoreTextureType::IBL)
            {
                m_luxCoreTextureName = "scene.lights." + m_textureAssetId.ToString<AZStd::string>();
                m_luxCoreTexture = m_luxCoreTexture << luxrays::Property(std::string(m_luxCoreTextureName.data()) + ".type")("infinite");
            }
            else
            {
                m_luxCoreTextureName = "scene.textures." + m_textureAssetId.ToString<AZStd::string>();
                m_luxCoreTexture = m_luxCoreTexture << luxrays::Property(std::string(m_luxCoreTextureName.data()) + ".type")("imagemap");
            }

            m_luxCoreTexture = m_luxCoreTexture << luxrays::Property(std::string(m_luxCoreTextureName.data()) + ".file")(std::string(m_textureAssetId.ToString<AZStd::string>().data()));
            m_textureChannels = 4;

            AddRenderTargetPipeline();
        }

        void LuxCoreTexture::AddRenderTargetPipeline()
        {
            // Render Texture pipeline
            AZ::RPI::RenderPipelineDescriptor pipelineDesc;
            pipelineDesc.m_name = m_textureAssetId.ToString<AZStd::string>();
            pipelineDesc.m_rootPassTemplate = "LuxCoreTexturePassTemplate";
            m_rtPipeline = AZ::RPI::RenderPipeline::CreateRenderPipeline(pipelineDesc);

            // Set source texture
            AZ::RPI::Pass* rootPass = m_rtPipeline->GetRootPass().get();
            AZ_Assert(rootPass != nullptr, "Failed to get root pass for render target pipeline");
            LuxCoreTexturePass* parentPass = static_cast<LuxCoreTexturePass*>(rootPass);

            // Setup call back to save read back data to m_textureData
            RPI::AttachmentReadback::CallbackFunction callback =
                [this](const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
            {
                RHI::ImageSubresourceLayout imageLayout = RHI::GetImageSubresourceLayout(readbackResult.m_imageDescriptor.m_size,
                    readbackResult.m_imageDescriptor.m_format);
                m_textureData.resize_no_construct(imageLayout.m_bytesPerImage);
                memcpy(m_textureData.data(), readbackResult.m_dataBuffer->data(), imageLayout.m_bytesPerImage);

                m_textureReadbackComplete = true;
            };
            parentPass->SetReadbackCallback(callback);

            switch (m_type)
            {
            case LuxCoreTextureType::Default:
                // assume a 8 bits linear texture
                parentPass->SetSourceTexture(m_texture, RHI::Format::R8G8B8A8_UNORM);
                break;
            case LuxCoreTextureType::IBL:
                // assume its a float image if its an IBL source
                parentPass->SetSourceTexture(m_texture, RHI::Format::R32G32B32A32_FLOAT);
                break;
            case LuxCoreTextureType::Albedo:
                // albedo texture is in sRGB space
                parentPass->SetSourceTexture(m_texture, RHI::Format::R8G8B8A8_UNORM_SRGB);
                break;
            case LuxCoreTextureType::Normal:
                // Normal texture needs special handling
                parentPass->SetIsNormalTexture(true);
                parentPass->SetSourceTexture(m_texture, RHI::Format::R8G8B8A8_UNORM);
                break;
            }
            
            const auto mainScene = AZ::RPI::RPISystemInterface::Get()->GetSceneByName(AZ::Name("RPI"));
            if (mainScene)
            {
                mainScene->AddRenderPipeline(m_rtPipeline);
            }
        }

        bool LuxCoreTexture::IsIBLTexture()
        {
            return m_type == LuxCoreTextureType::IBL;
        }

        void* LuxCoreTexture::GetRawDataPointer()
        {
            return (void*)m_textureData.data();
        }

        unsigned int LuxCoreTexture::GetTextureWidth()
        {
            return m_texture->GetRHIImage()->GetDescriptor().m_size.m_width;
        }

        unsigned int LuxCoreTexture::GetTextureHeight()
        {
            return m_texture->GetRHIImage()->GetDescriptor().m_size.m_height;
        }

        unsigned int LuxCoreTexture::GetTextureChannels()
        {
            return m_textureChannels;
        }

        luxrays::Properties LuxCoreTexture::GetLuxCoreTextureProperties()
        {
            return m_luxCoreTexture;
        }

        bool LuxCoreTexture::IsTextureReady()
        {
            return m_textureReadbackComplete;
        }
    }
}
#endif
