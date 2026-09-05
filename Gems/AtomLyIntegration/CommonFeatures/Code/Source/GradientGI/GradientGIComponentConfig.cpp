/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        void GradientGIComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<GradientGIComponentConfig, ComponentConfig>()
                    ->Version(4)
                    ->Field("HighColor",     &GradientGIComponentConfig::m_highColor)
                    ->Field("MidColor",      &GradientGIComponentConfig::m_midColor)
                    ->Field("LowColor",      &GradientGIComponentConfig::m_lowColor)
                    ->Field("Exposure",      &GradientGIComponentConfig::m_exposure)
                    ->Field("FaceResolution",&GradientGIComponentConfig::m_faceResolution)
                    ->Field("UpdateMode",    &GradientGIComponentConfig::m_updateMode)
                    ->Field("DetailTexture", &GradientGIComponentConfig::m_detailTexture)
                    ->Field("DetailMapping", &GradientGIComponentConfig::m_detailMapping)
                    ->Field("DetailBlend",   &GradientGIComponentConfig::m_detailBlend)
                    ->Field("DetailStrength",&GradientGIComponentConfig::m_detailStrength)
                    ->Field("SpecularTexture",  &GradientGIComponentConfig::m_specularTexture)
                    ->Field("SpecularMapping",  &GradientGIComponentConfig::m_specularMapping)
                    ->Field("SpecularBlend",    &GradientGIComponentConfig::m_specularBlend)
                    ->Field("SpecularStrength", &GradientGIComponentConfig::m_specularStrength)
                    ;
            }
        }

        AZ::Crc32 GradientGIComponentConfig::GetGpuLayerVisibility() const
        {
            // Texture layers are produced by the GPU compute pass, which only runs in
            // Dynamic mode; hide them in Static (CPU) mode.
            return m_updateMode == GradientGIUpdateMode::Dynamic
                ? AZ::Edit::PropertyVisibility::Show
                : AZ::Edit::PropertyVisibility::Hide;
        }

    } // namespace Render
} // namespace AZ
