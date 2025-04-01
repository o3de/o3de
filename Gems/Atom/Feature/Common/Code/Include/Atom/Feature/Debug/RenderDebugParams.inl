/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Macros below are of the form:
// PARAM(NAME, MEMBER_NAME, DEFAULT_VALUE, ...)

// Whether to turn on render debugging
AZ_GFX_BOOL_PARAM(Enabled, m_enabled, true)

// Debug view mode
AZ_GFX_COMMON_PARAM(AZ::Render::RenderDebugViewMode, RenderDebugViewMode, m_renderDebugViewMode, AZ::Render::RenderDebugViewMode::None)

// Lighting Views
AZ_GFX_COMMON_PARAM(AZ::Render::RenderDebugLightingType, RenderDebugLightingType, m_renderDebugLightingType, AZ::Render::RenderDebugLightingType::DiffuseAndSpecular)
AZ_GFX_COMMON_PARAM(AZ::Render::RenderDebugLightingSource, RenderDebugLightingSource, m_renderDebugLightingSource, AZ::Render::RenderDebugLightingSource::DirectAndIndirect)

// Lighting Overrides
AZ_GFX_FLOAT_PARAM(DebugLightingAzimuth, m_debugLightingAzimuth, 0.0f)
AZ_GFX_FLOAT_PARAM(DebugLightingElevation, m_debugLightingElevation, 60.0f)
AZ_GFX_VEC3_PARAM(DebugLightingColor, m_debugLightingColor, Vector3(1, 1, 1))
AZ_GFX_FLOAT_PARAM(DebugLightingIntensity, m_debugLightingIntensity, 2)

// Debug options
AZ_GFX_BOOL_PARAM(OverrideBaseColor,        m_overrideBaseColor,        false)
AZ_GFX_BOOL_PARAM(OverrideRoughness,        m_overrideRoughness,        false)
AZ_GFX_BOOL_PARAM(OverrideMetallic,         m_overrideMetallic,         false)
AZ_GFX_BOOL_PARAM(EnableNormalMaps,         m_enableNormalMaps,         true)
AZ_GFX_BOOL_PARAM(EnableDetailNormalMaps,   m_enableDetailNormalMaps,   true)

// Material Overrides
AZ_GFX_VEC3_PARAM(MaterialBaseColorOverride, m_materialBaseColorOverride, Vector3(0.5, 0.5, 0.5))
AZ_GFX_FLOAT_PARAM(MaterialRoughnessOverride, m_materialRoughnessOverride, 0.5f)
AZ_GFX_FLOAT_PARAM(MaterialMetallicOverride, m_materialMetallicOverride, 0.0f)

// Custom Debug options
AZ_GFX_BOOL_PARAM(CustomDebugOption01, m_customDebugOption01, false)
AZ_GFX_BOOL_PARAM(CustomDebugOption02, m_customDebugOption02, false)
AZ_GFX_BOOL_PARAM(CustomDebugOption03, m_customDebugOption03, false)
AZ_GFX_BOOL_PARAM(CustomDebugOption04, m_customDebugOption04, false)

// Custom Debug Floats
AZ_GFX_FLOAT_PARAM(CustomDebugFloat01, m_customDebugFloat01, 0.0f)
AZ_GFX_FLOAT_PARAM(CustomDebugFloat02, m_customDebugFloat02, 0.0f)
AZ_GFX_FLOAT_PARAM(CustomDebugFloat03, m_customDebugFloat03, 0.0f)
AZ_GFX_FLOAT_PARAM(CustomDebugFloat04, m_customDebugFloat04, 0.0f)
AZ_GFX_FLOAT_PARAM(CustomDebugFloat05, m_customDebugFloat05, 0.0f)
AZ_GFX_FLOAT_PARAM(CustomDebugFloat06, m_customDebugFloat06, 0.0f)
AZ_GFX_FLOAT_PARAM(CustomDebugFloat07, m_customDebugFloat07, 0.0f)
AZ_GFX_FLOAT_PARAM(CustomDebugFloat08, m_customDebugFloat08, 0.0f)
AZ_GFX_FLOAT_PARAM(CustomDebugFloat09, m_customDebugFloat09, 0.0f)

