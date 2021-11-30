/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#undef RC_INVOKED

#include <Atom/Feature/Utils/LightingPreset.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        void ExposureControlConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ExposureControlConfig>()
                    ->Version(4)
                    ->Field("compensateValue", &ExposureControlConfig::m_manualCompensationValue)
                    ->Field("exposureControlType", &ExposureControlConfig::m_exposureControlType)
                    ->Field("autoExposureMin", &ExposureControlConfig::m_autoExposureMin)
                    ->Field("autoExposureMax", &ExposureControlConfig::m_autoExposureMax)
                    ->Field("autoExposureSpeedUp", &ExposureControlConfig::m_autoExposureSpeedUp)
                    ->Field("autoExposureSpeedDown", &ExposureControlConfig::m_autoExposureSpeedDown)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<ExposureControlConfig>("ExposureControlConfig")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Constructor()
                    ->Constructor<const ExposureControlConfig&>()
                    ->Property("compensateValue", BehaviorValueProperty(&ExposureControlConfig::m_manualCompensationValue))
                    ->Property("exposureControlType", BehaviorValueProperty(&ExposureControlConfig::m_exposureControlType))
                    ->Property("autoExposureMin", BehaviorValueProperty(&ExposureControlConfig::m_autoExposureMin))
                    ->Property("autoExposureMax", BehaviorValueProperty(&ExposureControlConfig::m_autoExposureMax))
                    ->Property("autoExposureSpeedUp", BehaviorValueProperty(&ExposureControlConfig::m_autoExposureSpeedUp))
                    ->Property("autoExposureSpeedDown", BehaviorValueProperty(&ExposureControlConfig::m_autoExposureSpeedDown))
                    ;
            }
        }

        void LightConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<LightConfig>()
                    ->Version(2)
                    ->Field("direction", &LightConfig::m_direction)
                    ->Field("color", &LightConfig::m_color)
                    ->Field("intensity", &LightConfig::m_intensity)
                    ->Field("shadowCascadeCount", &LightConfig::m_shadowCascadeCount)
                    ->Field("shadowRatioLogarithmUniform", &LightConfig::m_shadowRatioLogarithmUniform)
                    ->Field("shadowFarClipDistance", &LightConfig::m_shadowFarClipDistance)
                    ->Field("shadowmapSize", &LightConfig::m_shadowmapSize)
                    ->Field("enableShadowDebugColoring", &LightConfig::m_enableShadowDebugColoring)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<LightConfig>("LightConfig")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Constructor()
                    ->Constructor<const LightConfig&>()
                    ->Property("direction", BehaviorValueProperty(&LightConfig::m_direction))
                    ->Property("color", BehaviorValueProperty(&LightConfig::m_color))
                    ->Property("intensity", BehaviorValueProperty(&LightConfig::m_intensity))
                    ->Property("shadowCascadeCount", BehaviorValueProperty(&LightConfig::m_shadowCascadeCount))
                    ->Property("shadowRatioLogarithmUniform", BehaviorValueProperty(&LightConfig::m_shadowRatioLogarithmUniform))
                    ->Property("shadowFarClipDistance", BehaviorValueProperty(&LightConfig::m_shadowFarClipDistance))
                    ->Property("shadowmapSize", BehaviorValueProperty(&LightConfig::m_shadowmapSize))
                    ->Property("enableShadowDebugColoring", BehaviorValueProperty(&LightConfig::m_enableShadowDebugColoring))
                    ;
            }
        }

        void LightingPreset::Reflect(AZ::ReflectContext* context)
        {
            ExposureControlConfig::Reflect(context);
            LightConfig::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<AZStd::vector<LightConfig>>();

                serializeContext->Class<LightingPreset>()
                    ->Version(5)
                    ->Field("displayName", &LightingPreset::m_displayName)
                    ->Field("iblDiffuseImageAsset", &LightingPreset::m_iblDiffuseImageAsset)
                    ->Field("iblSpecularImageAsset", &LightingPreset::m_iblSpecularImageAsset)
                    ->Field("skyboxImageAsset", &LightingPreset::m_skyboxImageAsset)
                    ->Field("alternateSkyboxImageAsset", &LightingPreset::m_alternateSkyboxImageAsset)
                    ->Field("iblExposure", &LightingPreset::m_iblExposure)
                    ->Field("skyboxExposure", &LightingPreset::m_skyboxExposure)
                    ->Field("shadowCatcherOpacity", &LightingPreset::m_shadowCatcherOpacity)
                    ->Field("exposure", &LightingPreset::m_exposure)
                    ->Field("lights", &LightingPreset::m_lights)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<LightingPreset>("LightingPreset")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Constructor()
                    ->Constructor<const LightingPreset&>()
                    ->Property("displayName", BehaviorValueProperty(&LightingPreset::m_displayName))
                    ->Property("alternateSkyboxImageAsset", BehaviorValueProperty(&LightingPreset::m_alternateSkyboxImageAsset))
                    ->Property("skyboxImageAsset", BehaviorValueProperty(&LightingPreset::m_skyboxImageAsset))
                    ->Property("iblSpecularImageAsset", BehaviorValueProperty(&LightingPreset::m_iblSpecularImageAsset))
                    ->Property("iblDiffuseImageAsset", BehaviorValueProperty(&LightingPreset::m_iblDiffuseImageAsset))
                    ->Property("iblExposure", BehaviorValueProperty(&LightingPreset::m_iblExposure))
                    ->Property("skyboxExposure", BehaviorValueProperty(&LightingPreset::m_skyboxExposure))
                    ->Property("exposure", BehaviorValueProperty(&LightingPreset::m_exposure))
                    ->Property("lights", BehaviorValueProperty(&LightingPreset::m_lights))
                    ->Property("shadowCatcherOpacity", BehaviorValueProperty(&LightingPreset::m_shadowCatcherOpacity))
                    ;
            }
        }

        void LightingPreset::ApplyLightingPreset(
            ImageBasedLightFeatureProcessorInterface* iblFeatureProcessor,
            SkyBoxFeatureProcessorInterface* skyboxFeatureProcessor,
            ExposureControlSettingsInterface* exposureControlSettingsInterface,
            DirectionalLightFeatureProcessorInterface* directionalLightFeatureProcessor,
            const Camera::Configuration& cameraConfig,
            AZStd::vector<DirectionalLightFeatureProcessorInterface::LightHandle>& lightHandles,
            Data::Instance<RPI::Material> shadowCatcherMaterial,
            RPI::MaterialPropertyIndex shadowCatcherOpacityPropertyIndex,
            bool enableAlternateSkybox) const
        {
            if (iblFeatureProcessor)
            {
                iblFeatureProcessor->SetDiffuseImage(m_iblDiffuseImageAsset);
                iblFeatureProcessor->SetSpecularImage(m_iblSpecularImageAsset);
                iblFeatureProcessor->SetExposure(m_iblExposure);
            }

            if (skyboxFeatureProcessor)
            {
                auto skyboxAsset = (enableAlternateSkybox && m_alternateSkyboxImageAsset.GetId().IsValid()) ? m_alternateSkyboxImageAsset : m_skyboxImageAsset;
                skyboxFeatureProcessor->SetCubemap(RPI::StreamingImage::FindOrCreate(skyboxAsset));
                skyboxFeatureProcessor->SetCubemapExposure(m_skyboxExposure);
            }

            if (exposureControlSettingsInterface)
            {
                exposureControlSettingsInterface->SetExposureControlType(static_cast<ExposureControl::ExposureControlType>(m_exposure.m_exposureControlType));
                exposureControlSettingsInterface->SetManualCompensation(m_exposure.m_manualCompensationValue);
                exposureControlSettingsInterface->SetEyeAdaptationExposureMin(m_exposure.m_autoExposureMin);
                exposureControlSettingsInterface->SetEyeAdaptationExposureMax(m_exposure.m_autoExposureMax);
                exposureControlSettingsInterface->SetEyeAdaptationSpeedUp(m_exposure.m_autoExposureSpeedUp);
                exposureControlSettingsInterface->SetEyeAdaptationSpeedDown(m_exposure.m_autoExposureSpeedDown);
            }

            if (directionalLightFeatureProcessor)
            {
                // Destroy previous lights
                for (DirectionalLightFeatureProcessorInterface::LightHandle& handle : lightHandles)
                {
                    directionalLightFeatureProcessor->ReleaseLight(handle);
                }
                lightHandles.clear();

                // Create new lights
                for (const auto& lightConfig : m_lights)
                {
                    const DirectionalLightFeatureProcessorInterface::LightHandle lightHandle = directionalLightFeatureProcessor->AcquireLight();

                    PhotometricColor<PhotometricUnit::Lux> lightColor(lightConfig.m_color * lightConfig.m_intensity);

                    directionalLightFeatureProcessor->SetDirection(lightHandle, lightConfig.m_direction);
                    directionalLightFeatureProcessor->SetRgbIntensity(lightHandle, lightColor);
                    directionalLightFeatureProcessor->SetCascadeCount(lightHandle, lightConfig.m_shadowCascadeCount);
                    directionalLightFeatureProcessor->SetShadowmapFrustumSplitSchemeRatio(lightHandle, lightConfig.m_shadowRatioLogarithmUniform);
                    directionalLightFeatureProcessor->SetShadowFarClipDistance(lightHandle, lightConfig.m_shadowFarClipDistance);
                    directionalLightFeatureProcessor->SetShadowmapSize(lightHandle, lightConfig.m_shadowmapSize);

                    int flags = lightConfig.m_enableShadowDebugColoring ?
                        DirectionalLightFeatureProcessorInterface::DebugDrawFlags::DebugDrawAll :
                        DirectionalLightFeatureProcessorInterface::DebugDrawFlags::DebugDrawNone;
                    directionalLightFeatureProcessor->SetDebugFlags(lightHandle, static_cast<DirectionalLightFeatureProcessorInterface::DebugDrawFlags>(flags));

                    directionalLightFeatureProcessor->SetCameraConfiguration(lightHandle, cameraConfig);

                    lightHandles.push_back(lightHandle);
                }
            }

            if (shadowCatcherMaterial && shadowCatcherOpacityPropertyIndex.IsValid())
            {
                shadowCatcherMaterial->SetPropertyValue(shadowCatcherOpacityPropertyIndex, m_shadowCatcherOpacity);
            }
        }
    } // namespace Render
} // namespace AZ
