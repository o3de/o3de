/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/GradientGlobalIllumination/GradientGlobalIlluminationComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ::Render
{
    void GradientGlobalIlluminationComponentConfig::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<GradientGlobalIlluminationComponentConfig, ComponentConfig>()
                ->Version(1)
                ->Field("HighColor", &GradientGlobalIlluminationComponentConfig::m_highColor)
                ->Field("MidColor", &GradientGlobalIlluminationComponentConfig::m_midColor)
                ->Field("LowColor", &GradientGlobalIlluminationComponentConfig::m_lowColor)
                ->Field("Exposure", &GradientGlobalIlluminationComponentConfig::m_exposure)
                ->Field("FaceResolution", &GradientGlobalIlluminationComponentConfig::m_faceResolution)
                ->Field("UpdateMode", &GradientGlobalIlluminationComponentConfig::m_updateMode)
                ->Field("DetailTexture", &GradientGlobalIlluminationComponentConfig::m_detailTexture)
                ->Field("DetailMapping", &GradientGlobalIlluminationComponentConfig::m_detailMapping)
                ->Field("DetailBlend", &GradientGlobalIlluminationComponentConfig::m_detailBlend)
                ->Field("DetailStrength", &GradientGlobalIlluminationComponentConfig::m_detailStrength)
                ->Field("SpecularTexture", &GradientGlobalIlluminationComponentConfig::m_specularTexture)
                ->Field("SpecularMapping", &GradientGlobalIlluminationComponentConfig::m_specularMapping)
                ->Field("SpecularBlend", &GradientGlobalIlluminationComponentConfig::m_specularBlend)
                ->Field("SpecularStrength", &GradientGlobalIlluminationComponentConfig::m_specularStrength);
        }
    }

    AZ::Crc32 GradientGlobalIlluminationComponentConfig::GetGpuLayerVisibility() const
    {
        return m_updateMode == GradientGlobalIlluminationUpdateMode::Dynamic
            ? AZ::Edit::PropertyVisibility::Show
            : AZ::Edit::PropertyVisibility::Hide;
    }
} // namespace AZ::Render
