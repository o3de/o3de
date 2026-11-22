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
#include <AzCore/Asset/AssetCommon.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AtomLyIntegration/CommonFeatures/GradientGlobalIllumination/GradientGlobalIlluminationComponentConstants.h>

namespace AZ::Render
{
    class GradientGlobalIlluminationComponentConfig final
        : public ComponentConfig
    {
    public:
        AZ_RTTI(GradientGlobalIlluminationComponentConfig, "{C3D4E5F6-A7B8-9012-CDEF-123456789012}", ComponentConfig);
        AZ_CLASS_ALLOCATOR(GradientGlobalIlluminationComponentConfig, SystemAllocator);

        static void Reflect(ReflectContext* context);

        AZ::Crc32 GetGpuLayerVisibility() const;

    public:
        Color m_lowColor = Color(0.05f, 0.06f, 0.08f, 1.0f);
        Color m_midColor = Color(0.20f, 0.30f, 0.55f, 1.0f);
        Color m_highColor = Color(0.85f, 0.95f, 1.0f, 1.0f);

        float m_exposure = 0.0f;
        uint32_t m_faceResolution = 64;
        GradientGlobalIlluminationUpdateMode m_updateMode = GradientGlobalIlluminationUpdateMode::Static;

        // GPU texture layers.

        Data::Asset<RPI::StreamingImageAsset> m_detailTexture;
        GradientGlobalIlluminationTextureMapping m_detailMapping = GradientGlobalIlluminationTextureMapping::Stretched;
        GradientGlobalIlluminationBlendMode m_detailBlend = GradientGlobalIlluminationBlendMode::Overlay;
        float m_detailStrength = 1.0f;

        Data::Asset<RPI::StreamingImageAsset> m_specularTexture;
        GradientGlobalIlluminationTextureMapping m_specularMapping = GradientGlobalIlluminationTextureMapping::Stretched;
        GradientGlobalIlluminationBlendMode m_specularBlend = GradientGlobalIlluminationBlendMode::Overlay;
        float m_specularStrength = 1.0f;
    };
} // namespace AZ::Render
