/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/DirectionalLightComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/limits.h>

namespace AZ
{
    namespace Render
    {
        void DirectionalLightComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DirectionalLightComponentConfig, ComponentConfig>()
                    ->Version(8)
                    ->Field("Color", &DirectionalLightComponentConfig::m_color)
                    ->Field("IntensityMode", &DirectionalLightComponentConfig::m_intensityMode)
                    ->Field("Intensity", &DirectionalLightComponentConfig::m_intensity)
                    ->Field("AngularDiameter", &DirectionalLightComponentConfig::m_angularDiameter)
                    ->Field("CameraEntityId", &DirectionalLightComponentConfig::m_cameraEntityId)
                    ->Field("ShadowFarClipDistance", &DirectionalLightComponentConfig::m_shadowFarClipDistance)
                    ->Field("ShadowmapSize", &DirectionalLightComponentConfig::m_shadowmapSize)
                    ->Field("CascadeCount", &DirectionalLightComponentConfig::m_cascadeCount)
                    ->Field("SplitAutomatic", &DirectionalLightComponentConfig::m_isShadowmapFrustumSplitAutomatic)
                    ->Field("SplitRatio", &DirectionalLightComponentConfig::m_shadowmapFrustumSplitSchemeRatio)
                    ->Field("CascadeFarDepths", &DirectionalLightComponentConfig::m_cascadeFarDepths)
                    ->Field("GroundHeight", &DirectionalLightComponentConfig::m_groundHeight)
                    ->Field("IsCascadeCorrectionEnabled", &DirectionalLightComponentConfig::m_isCascadeCorrectionEnabled)
                    ->Field("IsDebugColoringEnabled", &DirectionalLightComponentConfig::m_isDebugColoringEnabled)
                    ->Field("ShadowFilterMethod", &DirectionalLightComponentConfig::m_shadowFilterMethod)
                    ->Field("PcfFilteringSampleCount", &DirectionalLightComponentConfig::m_filteringSampleCount)
                    ->Field("ShadowReceiverPlaneBiasEnabled", &DirectionalLightComponentConfig::m_receiverPlaneBiasEnabled)
                    ->Field("Shadow Bias", &DirectionalLightComponentConfig::m_shadowBias)
                    ->Field("Normal Shadow Bias", &DirectionalLightComponentConfig::m_normalShadowBias);
            }
        }

        const char* DirectionalLightComponentConfig::GetIntensitySuffix() const
        {
            return PhotometricValue::GetTypeSuffix(m_intensityMode);
        }

        float DirectionalLightComponentConfig::GetIntensityMin() const
        {
            switch (m_intensityMode)
            {
            case PhotometricUnit::Lux:
                return 0.0f;
            case PhotometricUnit::Ev100Illuminance:
                return AZStd::numeric_limits<float>::lowest();
            }
            return 0.0f;
        }

        float DirectionalLightComponentConfig::GetIntensityMax() const
        {
            // While there is no hard-max, a max must be included when there is a hard min.
            return AZStd::numeric_limits<float>::max();
        }

        float DirectionalLightComponentConfig::GetIntensitySoftMin() const
        {
            switch (m_intensityMode)
            {
            case PhotometricUnit::Lux:
                return 0.0f;
            case PhotometricUnit::Ev100Illuminance:
                return -4.0f;
            }
            return 0.0f;
        }

        float DirectionalLightComponentConfig::GetIntensitySoftMax() const
        {
            switch (m_intensityMode)
            {
            case PhotometricUnit::Lux:
                return 200'000.0f;
            case PhotometricUnit::Ev100Illuminance:
                return 16.0f;
            }
            return 0.0f;
        }

        bool DirectionalLightComponentConfig::IsSplitManual() const
        {
            return !m_isShadowmapFrustumSplitAutomatic;
        }

        bool DirectionalLightComponentConfig::IsSplitAutomatic() const
        {
            return m_isShadowmapFrustumSplitAutomatic;
        }

        bool DirectionalLightComponentConfig::IsCascadeCorrectionDisabled() const
        {
            return (m_cascadeCount == 1 || !m_isCascadeCorrectionEnabled);
        }

        bool DirectionalLightComponentConfig::IsShadowFilteringDisabled() const
        {
            return (m_shadowFilterMethod == ShadowFilterMethod::None);
        }

        bool DirectionalLightComponentConfig::IsShadowPcfDisabled() const
        {
            return !(m_shadowFilterMethod == ShadowFilterMethod::Pcf ||
                m_shadowFilterMethod == ShadowFilterMethod::EsmPcf);
        }

        bool DirectionalLightComponentConfig::IsEsmDisabled() const
        {
            return !(m_shadowFilterMethod == ShadowFilterMethod::Esm || m_shadowFilterMethod == ShadowFilterMethod::EsmPcf);
        }
    } // namespace Render
} // namespace AZ
