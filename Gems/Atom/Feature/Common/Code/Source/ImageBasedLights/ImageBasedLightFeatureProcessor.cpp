/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageBasedLights/ImageBasedLightFeatureProcessor.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

namespace AZ
{
    namespace Render
    {
        void ImageBasedLightFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<ImageBasedLightFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void ImageBasedLightFeatureProcessor::Activate()
        {
            m_sceneSrg = GetParentScene()->GetShaderResourceGroup();
            
            // Load default specular and diffuse cubemaps
            // These are assigned when Global IBL is disabled or removed from the scene to prevent a Vulkan TDR.
            // [GFX-TODO][ATOM-4181] This can be removed after Vulkan is changed to automatically handle this issue.
            LoadDefaultCubeMaps();
        }

        void ImageBasedLightFeatureProcessor::Deactivate()
        {
            m_iblOrientationConstantIndex.Reset();
            m_iblExposureConstantIndex.Reset();
            m_diffuseEnvMapIndex.Reset();
            m_specularEnvMapIndex.Reset();
            SetGradientOverride({}, {}, 0);
            m_sceneSrg = {};
        }

        void ImageBasedLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "ImageBasedLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            m_sceneSrg->SetImage(m_specularEnvMapIndex, m_specular);
            m_sceneSrg->SetImage(m_diffuseEnvMapIndex, m_diffuse);
            m_sceneSrg->SetConstant(m_iblExposureConstantIndex, m_exposure);
            m_sceneSrg->SetConstant(m_iblOrientationConstantIndex, m_orientation);
        }

        void ImageBasedLightFeatureProcessor::SetSpecularImage(const Data::Asset<RPI::StreamingImageAsset>& imageAsset)
        {
            m_skylightSpecular = GetInstanceForImage(imageAsset, m_defaultSpecularImage);
            if (!m_gradientActive)
            {
                m_specular = m_skylightSpecular;
            }
        }

        void ImageBasedLightFeatureProcessor::SetDiffuseImage(const Data::Asset<RPI::StreamingImageAsset>& imageAsset)
        {
            m_skylightDiffuse = GetInstanceForImage(imageAsset, m_defaultDiffuseImage);
            if (!m_gradientActive)
            {
                m_diffuse = m_skylightDiffuse;
            }
        }

        void ImageBasedLightFeatureProcessor::SetExposure(float exposure)
        {
            m_skylightExposure = exposure;
            if (!m_gradientActive)
            {
                m_exposure = m_skylightExposure;
            }
        }

        void ImageBasedLightFeatureProcessor::SetOrientation(const Quaternion& orientation)
        {
            m_orientation = orientation;
        }

        void ImageBasedLightFeatureProcessor::SetGradientOverride(
            const Data::Instance<RPI::Image>& diffuse,
            const Data::Instance<RPI::Image>& specular,
            float exposure)
        {
            if (!diffuse)
            {
                if (m_gradientActive)
                {
                    m_gradientActive = false;
                    ApplySkylight();
                }
                return;
            }

            if (!m_gradientActive)
            {
                if (!m_skylightDiffuse)
                {
                    m_skylightDiffuse = m_defaultDiffuseImage;
                }
                if (!m_skylightSpecular)
                {
                    m_skylightSpecular = m_defaultSpecularImage;
                }
                m_gradientActive = true;
            }
            m_diffuse = diffuse;
            m_specular = specular;
            m_exposure = exposure;
        }

        void ImageBasedLightFeatureProcessor::Reset()
        {
            m_skylightSpecular = m_defaultSpecularImage;
            m_skylightDiffuse = m_defaultDiffuseImage;
            m_skylightExposure = 0;
            if (!m_gradientActive)
            {
                ApplySkylight();
            }
        }

        void ImageBasedLightFeatureProcessor::ApplySkylight()
        {
            m_specular = m_skylightSpecular;
            m_diffuse = m_skylightDiffuse;
            m_exposure = m_skylightExposure;
        }

        void ImageBasedLightFeatureProcessor::LoadDefaultCubeMaps()
        {
            const constexpr char* DefaultSpecularCubeMapPath = "textures/default/default_iblglobalcm_iblspecular.dds.streamingimage";
            const constexpr char* DefaultDiffuseCubeMapPath = "textures/default/default_iblglobalcm_ibldiffuse.dds.streamingimage";

            m_defaultSpecularImage = RPI::LoadStreamingTexture(DefaultSpecularCubeMapPath);
            AZ_Assert(m_defaultSpecularImage, "Failed to load default specular cubemap");

            m_defaultDiffuseImage = RPI::LoadStreamingTexture(DefaultDiffuseCubeMapPath);
            AZ_Assert(m_defaultDiffuseImage, "Failed to load default diffuse cubemap");
        }

        Data::Instance<RPI::Image> ImageBasedLightFeatureProcessor::GetInstanceForImage(const Data::Asset<RPI::StreamingImageAsset>& imageAsset, const Data::Instance<RPI::Image>& defaultImage)
        {
            Data::Instance<RPI::Image> image;
            if (imageAsset.GetId().IsValid())
            {
                image = RPI::StreamingImage::FindOrCreate(imageAsset);
                if (image && !ValidateIsCubemap(image))
                {
                    image = defaultImage;
                }
            }
            return image;
        }

        bool ImageBasedLightFeatureProcessor::ValidateIsCubemap(Data::Instance<RPI::Image> image)
        {
            const RHI::ImageDescriptor& desc = image->GetRHIImage()->GetDescriptor();
            return (desc.m_isCubemap || desc.m_arraySize == 6);
        }
    } // namespace Render
} // namespace AZ
