/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/CoreLightsConstants.h>

namespace AZ
{
    namespace Render
    {
        struct DirectionalLightComponentConfig final
            : public ComponentConfig
        {
            AZ_RTTI(DirectionalLightConfiguration, "EB01B835-F9FE-4FF0-BDC4-455462BFE769", ComponentConfig);
            static void Reflect(ReflectContext* context);

            // The following functions provide information to an EditContext...

            //! Returns characters for a suffix for the light type including a space. " lm" for lumens for example.
            const char* GetIntensitySuffix() const;

            //! Returns the minimum intensity value allowed depending on the m_intensityMode
            float GetIntensityMin() const;

            //! Returns the maximum intensity value allowed depending on the m_intensityMode
            float GetIntensityMax() const;

            //! Returns the minimum intensity value for UI depending on the m_intensityMode, but users may still type in a lesser value depending on GetIntensityMin().
            float GetIntensitySoftMin() const;

            //! Returns the maximum intensity value for UI depending on the m_intensityMode, but users may still type in a greater value depending on GetIntensityMin().
            float GetIntensitySoftMax() const;

            AZ::Color m_color = AZ::Color::CreateOne();

            //! Lux or Ev100
            PhotometricUnit m_intensityMode = PhotometricUnit::Ev100Illuminance;
            //! Intensity in lux or Ev100 (depending on m_intensityMode)
            float m_intensity = 4.0f;

            //! Angular diameter of light in degrees, should be small. The sun is about 0.5.
            float m_angularDiameter = 0.5f;

            //! EntityId of the camera specifying view frustum to create shadowmaps.
            EntityId m_cameraEntityId{ EntityId::InvalidEntityId };

            //! Far depth clips for shadows.
            float m_shadowFarClipDistance = 100.f;

            //! Width/Height of shadowmap images.
            ShadowmapSize m_shadowmapSize = ShadowmapSize::Size1024;

            //! Number of cascades.
            uint32_t m_cascadeCount = 4;

            //! Flag to switch splitting of shadowmap frustum to cascades automatically or not.
            //! If true, m_shadowmapFrustumSplitSchemeRatio is used.
            //! If false, m_cascadeFarDepths is used.
            bool m_isShadowmapFrustumSplitAutomatic = true;

            //! Ratio to lerp between the two types of frustum splitting scheme.
            //!   0 = Uniform scheme which will split the Frustum evenly across all cascades.
            //!   1 = Logarithmic scheme which is designed to split the frustum in a logarithmic fashion
            //!       in order to enable us to produce a more optimal perspective aliasing across the frustum.
            //! This is valid only when m_shadowmapFrustumSplitIsAutomatic = true.
            float m_shadowmapFrustumSplitSchemeRatio = 0.9f;

            //! Far depth for each cascade.
            //! Note that near depth of a cascade equals the far depth of the previous cascade.
            //! This is valid only when m_shadowmapFrustumSplitIsAutomatic = false.
            AZ::Vector4 m_cascadeFarDepths
            {
                m_shadowFarClipDistance * 1 / Shadow::MaxNumberOfCascades,
                m_shadowFarClipDistance * 2 / Shadow::MaxNumberOfCascades,
                m_shadowFarClipDistance * 3 / Shadow::MaxNumberOfCascades,
                m_shadowFarClipDistance * 4 / Shadow::MaxNumberOfCascades
            };

            //! Height of camera from the ground.
            //! The position of view frustum is corrected using cameraHeight
            //! to get better quality of shadow around the area close to the camera.
            //! To enable the correction, m_isCascadeCorrectionEnabled=true is required.
            float m_groundHeight = 0.f;

            //! Flag specifying whether view frustum positions are corrected.
            //! The calculation of it is caused when the position of configuration of the camera is changed.
            bool m_isCascadeCorrectionEnabled = false;

            //! Flag specifying whether debug coloring is added.
            bool m_isDebugColoringEnabled = false;

            //! Method of shadow's filtering.
            ShadowFilterMethod m_shadowFilterMethod = ShadowFilterMethod::None;

            // Reduces acne by biasing the shadowmap lookup along the geometric normal.
            float m_normalShadowBias = 0.0f;

            //! Sample Count for filtering (from 4 to 64)
            //! It is used only when the pixel is predicted as on the boundary.
            uint16_t m_filteringSampleCount = 32;

            //! Whether not to enable the receiver plane bias.
            //! This uses partial derivatives to reduce shadow acne when using large pcf kernels.
            bool m_receiverPlaneBiasEnabled = true;

            //! Reduces shadow acne by applying a small amount of offset along shadow-space z.
            float m_shadowBias = 0.0f;

            bool IsSplitManual() const;
            bool IsSplitAutomatic() const;
            bool IsCascadeCorrectionDisabled() const;
            bool IsShadowFilteringDisabled() const;
            bool IsShadowPcfDisabled() const;
            bool IsEsmDisabled() const;
        };
    } // namespace Render
} // namespace AZ
