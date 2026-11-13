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
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        class GradientGIComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(GradientGIComponentConfig, "{C3D4E5F6-A7B8-9012-CDEF-123456789012}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(GradientGIComponentConfig, SystemAllocator);

            static void Reflect(ReflectContext* context);

            // =====================================================================
            // Gradient Colors
            // =====================================================================

            Color m_lowColor  = Color(0.05f, 0.06f, 0.08f, 1.0f);
            Color m_midColor  = Color(0.20f, 0.30f, 0.55f, 1.0f);
            Color m_highColor = Color(0.85f, 0.95f, 1.0f,  1.0f);

            // =====================================================================
            // Settings
            // =====================================================================

            float             m_exposure       = 0.0f;
            uint32_t          m_faceResolution = 64;
            GradientGIUpdateMode m_updateMode  = GradientGIUpdateMode::Static;

            // =====================================================================
            // Detail Texture Layer (GPU/Dynamic mode only)
            // =====================================================================

            Data::Asset<RPI::StreamingImageAsset> m_detailTexture;
            GradientGITextureMapping m_detailMapping = GradientGITextureMapping::Stretched;
            GradientGIBlendMode      m_detailBlend    = GradientGIBlendMode::Overlay;
            float                    m_detailStrength = 1.0f;

            // =====================================================================
            // Specular Texture Layer (GPU/Dynamic mode only)
            // =====================================================================

            Data::Asset<RPI::StreamingImageAsset> m_specularTexture;
            GradientGITextureMapping m_specularMapping = GradientGITextureMapping::Stretched;
            GradientGIBlendMode      m_specularBlend    = GradientGIBlendMode::Overlay;
            float                    m_specularStrength = 1.0f;

            //! Editor visibility: the texture layers only function in GPU/Dynamic mode,
            //! so they are hidden in CPU/Static mode. Returns a PropertyVisibility Crc.
            AZ::Crc32 GetGpuLayerVisibility() const;
        };

    } // namespace Render
} // namespace AZ
