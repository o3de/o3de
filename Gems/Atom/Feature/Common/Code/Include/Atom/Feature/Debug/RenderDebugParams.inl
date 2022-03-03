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

// Debug options and view mode
AZ_GFX_UINT32_PARAM(RenderDebugOptions, m_renderDebugOptions, 0)
AZ_GFX_COMMON_PARAM(AZ::Render::RenderDebugViewMode, RenderDebugViewMode, m_renderDebugViewMode, AZ::Render::RenderDebugViewMode::None)

// Material Overrides
AZ_GFX_VEC3_PARAM(MaterialAlbedoOverride, m_materialAlbedoOverride, Vector3(1, 1, 1))
AZ_GFX_FLOAT_PARAM(MaterialRoughnessOverride, m_materialRoughnessOverride, 0.5f)
AZ_GFX_FLOAT_PARAM(MaterialMetallicOverride, m_materialMetallicOverride, 0.0f)

// Lighting Overrides
AZ_GFX_FLOAT_PARAM(DebugLightingAzimuth, m_debugLightingAzimuth, 0.0f)
AZ_GFX_FLOAT_PARAM(DebugLightingElevation, m_debugLightingElevation, 90.0f)
AZ_GFX_VEC3_PARAM(DebugLightingIntensity, m_debugLightingIntensity, Vector3(1, 1, 1))

