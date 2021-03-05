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

#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessor.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <Atom/RHI/CpuProfiler.h>

#include <AzCore/Debug/EventTrace.h>

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
            
            // Find SRG index for global IBL
            m_specularEnvMapIndex = m_sceneSrg->FindShaderInputImageIndex(Name{"m_specularEnvMap"});
            m_diffuseEnvMapIndex = m_sceneSrg->FindShaderInputImageIndex(Name{"m_diffuseEnvMap"});
            m_iblExposureConstantIndex = m_sceneSrg->FindShaderInputConstantIndex(Name{"m_iblExposure"});
            m_iblOrientationConstantIndex = m_sceneSrg->FindShaderInputConstantIndex(Name{"m_iblOrientation"});

            // Load default specular and diffuse cubemaps
            // These are assigned when Global IBL is disabled or removed from the scene to prevent a Vulkan TDR.
            // [GFX-TODO][ATOM-4181] This can be removed after Vulkan is changed to automatically handle this issue.
            LoadDefaultCubeMaps();
        }

        void ImageBasedLightFeatureProcessor::Deactivate()
        {
            m_iblExposureConstantIndex = {};
            m_diffuseEnvMapIndex = {};
            m_specularEnvMapIndex = {};
            m_sceneSrg = {};
        }

        void ImageBasedLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_ATOM_PROFILE_FUNCTION("RPI", "ImageBasedLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            m_sceneSrg->SetImage(m_specularEnvMapIndex, m_specular);
            m_sceneSrg->SetImage(m_diffuseEnvMapIndex, m_diffuse);
            m_sceneSrg->SetConstant(m_iblExposureConstantIndex, m_exposure);
            m_sceneSrg->SetConstant(m_iblOrientationConstantIndex, m_orientation);
        }

        void ImageBasedLightFeatureProcessor::SetSpecularImage(const Data::Asset<RPI::StreamingImageAsset>& imageAsset)
        {
            m_specular = GetInstanceForImage(imageAsset, m_defaultSpecularImage);
        }

        void ImageBasedLightFeatureProcessor::SetDiffuseImage(const Data::Asset<RPI::StreamingImageAsset>& imageAsset)
        {
            m_diffuse = GetInstanceForImage(imageAsset, m_defaultDiffuseImage);
        }

        void ImageBasedLightFeatureProcessor::SetExposure(float exposure)
        {
            m_exposure = exposure;
        }

        void ImageBasedLightFeatureProcessor::SetOrientation(const Quaternion& orientation)
        {
            m_orientation = orientation;
        }

        void ImageBasedLightFeatureProcessor::Reset()
        {
            m_specular = m_defaultSpecularImage;
            m_diffuse = m_defaultDiffuseImage;
            m_exposure = 0;
        }

        void ImageBasedLightFeatureProcessor::LoadDefaultCubeMaps()
        {
            const constexpr char* DefaultSpecularCubeMapPath = "textures/default/default_iblglobalcm_iblspecular.dds.streamingimage";
            const constexpr char* DefaultDiffuseCubeMapPath = "textures/default/default_iblglobalcm_ibldiffuse.dds.streamingimage";

            Data::AssetId specularAssetId;
            Data::AssetCatalogRequestBus::BroadcastResult(
                specularAssetId,
                &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                DefaultSpecularCubeMapPath,
                azrtti_typeid<AZ::RPI::StreamingImageAsset>(),
                false);

            Data::AssetId diffuseAssetId;
            Data::AssetCatalogRequestBus::BroadcastResult(
                diffuseAssetId,
                &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                DefaultDiffuseCubeMapPath,
                azrtti_typeid<AZ::RPI::StreamingImageAsset>(),
                false);

            auto specularAsset = Data::AssetManager::Instance().GetAsset<RPI::StreamingImageAsset>(specularAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
            auto diffuseAsset = Data::AssetManager::Instance().GetAsset<RPI::StreamingImageAsset>(diffuseAssetId, AZ::Data::AssetLoadBehavior::PreLoad);

            specularAsset.BlockUntilLoadComplete();
            diffuseAsset.BlockUntilLoadComplete();

            m_defaultSpecularImage = RPI::StreamingImage::FindOrCreate(specularAsset);
            AZ_Assert(m_defaultSpecularImage, "Failed to load default specular cubemap");

            m_defaultDiffuseImage = RPI::StreamingImage::FindOrCreate(diffuseAsset);
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
