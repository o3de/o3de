/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <Debug/RenderDebugSettings.h>
#include <Debug/RenderDebugFeatureProcessor.h>

namespace AZ::Render
{
    RenderDebugSettings::RenderDebugSettings([[maybe_unused]] RenderDebugFeatureProcessor* featureProcessor)
    {
    }

    void RenderDebugSettings::Simulate()
    {
        UpdateOptionsMask();
    }

    void RenderDebugSettings::UpdateOptionsMask()
    {
        m_optionsMask = 0;

        // Enabled
        m_optionsMask |= (u32)GetEnabled() << (u32)RenderDebugOptions::DebugEnabled;

        // Material Overrides
        m_optionsMask |= (u32)GetOverrideBaseColor() << (u32)RenderDebugOptions::OverrideBaseColor;
        m_optionsMask |= (u32)GetOverrideRoughness() << (u32)RenderDebugOptions::OverrideRoughness;
        m_optionsMask |= (u32)GetOverrideMetallic() << (u32)RenderDebugOptions::OverrideMetallic;

        // Normal Maps
        m_optionsMask |= (u32)GetEnableNormalMaps() << (u32)RenderDebugOptions::EnableNormalMaps;
        m_optionsMask |= (u32)GetEnableDetailNormalMaps() << (u32)RenderDebugOptions::EnableDetailNormalMaps;

        // Debug Light
        bool useDebugLight = GetRenderDebugLightingSource() == RenderDebugLightingSource::DebugLight;
        m_optionsMask |= (u32)useDebugLight << (u32)RenderDebugOptions::UseDebugLight;

        // Direct & Indirect Lighting

        bool diffuseLightingEnabled = GetRenderDebugLightingType() == RenderDebugLightingType::Diffuse ||
                                      GetRenderDebugLightingType() == RenderDebugLightingType::DiffuseAndSpecular;

        bool specularLightingEnabled = GetRenderDebugLightingType() == RenderDebugLightingType::Specular ||
                                       GetRenderDebugLightingType() == RenderDebugLightingType::DiffuseAndSpecular;

        bool directLightingEnabled = GetRenderDebugLightingSource() == RenderDebugLightingSource::Direct ||
                                     GetRenderDebugLightingSource() == RenderDebugLightingSource::DirectAndIndirect;

        bool indirectLightingEnabled = GetRenderDebugLightingSource() == RenderDebugLightingSource::Indirect ||
                                       GetRenderDebugLightingSource() == RenderDebugLightingSource::DirectAndIndirect;

        if (useDebugLight)
        {
            directLightingEnabled = indirectLightingEnabled = false;
        }

        if (GetRenderDebugViewMode() != RenderDebugViewMode::None)
        {
            diffuseLightingEnabled = specularLightingEnabled = directLightingEnabled = indirectLightingEnabled = false;
        }

        m_optionsMask |= (u32)diffuseLightingEnabled  << (u32)RenderDebugOptions::EnableDiffuseLighting;
        m_optionsMask |= (u32)specularLightingEnabled << (u32)RenderDebugOptions::EnableSpecularLighting;
        m_optionsMask |= (u32)directLightingEnabled   << (u32)RenderDebugOptions::EnableDirectLighting;
        m_optionsMask |= (u32)indirectLightingEnabled << (u32)RenderDebugOptions::EnableIndirectLighting;

        // Custom Debug Options
        m_optionsMask |= (u32)GetCustomDebugOption01() << (u32)RenderDebugOptions::CustomDebugOption01;
        m_optionsMask |= (u32)GetCustomDebugOption02() << (u32)RenderDebugOptions::CustomDebugOption02;
        m_optionsMask |= (u32)GetCustomDebugOption03() << (u32)RenderDebugOptions::CustomDebugOption03;
        m_optionsMask |= (u32)GetCustomDebugOption04() << (u32)RenderDebugOptions::CustomDebugOption04;
    }

}
