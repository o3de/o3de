/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Macros below are of the form:
// PARAM(NAME, MEMBER_NAME, DEFAULT_VALUE, ...)

AZ_GFX_BOOL_PARAM(Enabled, m_enabled, false)
AZ_GFX_COMMON_PARAM(Data::Asset<RPI::AnyAsset>, ColorGradingLut, m_colorGradingLut, {})
AZ_GFX_COMMON_PARAM(AZ::Render::ShaperPresetType, ShaperPresetType, m_shaperPresetType, AZ::Render::ShaperPresetType::Log2_48Nits)
AZ_GFX_COMMON_PARAM(float, CustomMinExposure, m_customMinExposure, -6.5)
AZ_GFX_COMMON_PARAM(float, CustomMaxExposure, m_customMaxExposure, 6.5)
AZ_GFX_FLOAT_PARAM(ColorGradingLutIntensity, m_colorGradingLutIntensity, 1.0)
AZ_GFX_FLOAT_PARAM(ColorGradingLutOverride, m_colorGradingLutOverride, 1.0)
