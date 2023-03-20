/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Material/Material.h>

namespace AZ
{
    namespace Render
    {
        class ImageBasedLightFeatureProcessorInterface;
        class SkyBoxFeatureProcessorInterface;
        class ExposureControlSettingsInterface;

        //! ExposureControlConfig describes exposure settings that can be added to a LightingPreset
        struct ExposureControlConfig final
        {
            AZ_TYPE_INFO(AZ::Render::ExposureControlConfig, "{C6FD75F7-58BA-46CE-8FBA-2D64CB4ECFF9}");
            static void Reflect(AZ::ReflectContext* context);

            // Exposure Control Constants...
            enum class ExposureControlType :uint32_t
            {
                ManualOnly = 0,
                EyeAdaptation,
                ExposureControlTypeMax
            };

            uint32_t m_exposureControlType = 0;
            float m_manualCompensationValue = 0.0f;
            float m_autoExposureMin = -10.0;
            float m_autoExposureMax = 10.0f;
            float m_autoExposureSpeedUp = 3.0;
            float m_autoExposureSpeedDown = 1.0;
        };

        //! LightConfig describes a directional light that can be added to a LightingPreset
        struct LightConfig final
        {
            AZ_TYPE_INFO(AZ::Render::LightConfig, "{02644F52-9483-47A8-9028-37671695C34E}");
            static void Reflect(AZ::ReflectContext* context);

            // Setting default direction to produce a visible shadow
            AZ::Vector3 m_direction = AZ::Vector3(1.0f / 3.0f, 1.0f / 3.0f, -1.0f / 3.0f);
            AZ::Color m_color = AZ::Color::CreateOne();
            float m_intensity = 1.0f;
            uint16_t m_shadowCascadeCount = 4;
            float m_shadowRatioLogarithmUniform = 1.0f;
            float m_shadowFarClipDistance = 20.0f;
            ShadowmapSize m_shadowmapSize = ShadowmapSize::Size2048;
            bool m_enableShadowDebugColoring = false;
        };

        //! LightingPreset describes a lighting environment that can be applied to the viewport
        struct LightingPreset final
        {
            AZ_TYPE_INFO(AZ::Render::LightingPreset, "{6EEACBC0-2D97-414C-8E87-088E7BA231A9}");
            AZ_CLASS_ALLOCATOR(LightingPreset, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            static constexpr char const Extension[] = "lightingpreset.azasset";
            AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_iblDiffuseImageAsset;
            AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_iblSpecularImageAsset;
            AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_skyboxImageAsset;
            AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_alternateSkyboxImageAsset;
            float m_iblExposure = 0.0f;
            float m_skyboxExposure = 0.0f;
            ExposureControlConfig m_exposure;
            AZStd::vector<LightConfig> m_lights;
            float m_shadowCatcherOpacity = 0.5f;

            // Apply the lighting config to the currect scene through feature processors.
            // Shader catcher material is optional.
            void ApplyLightingPreset(
                ImageBasedLightFeatureProcessorInterface* iblFeatureProcessor,
                SkyBoxFeatureProcessorInterface* skyboxFeatureProcessor,
                ExposureControlSettingsInterface* exposureControlSettingsInterface,
                DirectionalLightFeatureProcessorInterface* directionalLightFeatureProcessor,
                const Camera::Configuration& cameraConfig,
                AZStd::vector<DirectionalLightFeatureProcessorInterface::LightHandle>& lightHandles,
                bool enableAlternateSkybox) const;
        };

        using LightingPresetPtr = AZStd::shared_ptr<LightingPreset>;
        using LightingPresetPtrVector = AZStd::vector<LightingPresetPtr>;
    } // namespace Render
} // namespace AZ
