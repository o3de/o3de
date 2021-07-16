/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Macros below are of the form:
// PARAM(NAME, MEMBER_NAME, DEFAULT_VALUE, ...)

// --- SSAO COMPUTE ---

// Whether SSAO effect is enabled
AZ_GFX_BOOL_PARAM(Enabled, m_enabled, true)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(bool, Enabled, m_enabled)

// The strength multiplier for the SSAO effect
AZ_GFX_FLOAT_PARAM(Strength, m_strength, Ssao::DefaultStrength)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, Strength, m_strength)

// The sampling radius calculated in screen UV space
AZ_GFX_FLOAT_PARAM(SamplingRadius, m_samplingRadius, Ssao::DefaultSamplingRadius)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, SamplingRadius, m_samplingRadius)


// --- SSAO BLUR ---

// Whether to enable the blur passes
AZ_GFX_BOOL_PARAM(EnableBlur, m_enableBlur, true)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(bool, EnableBlur, m_enableBlur)

//! How much a value is reduced from pixel to pixel on a perfectly flat surface
AZ_GFX_FLOAT_PARAM(BlurConstFalloff, m_blurConstFalloff, Ssao::DefaultBlurConstFalloff)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, BlurConstFalloff, m_blurConstFalloff)

//! Threshold used to reduce computed depth difference during blur and thus the depth falloff
//! Can be thought of as a bias that blurs curved surfaces more like flat surfaces
//! but generally not needed and can be set to 0.0f
AZ_GFX_FLOAT_PARAM(BlurDepthFalloffThreshold, m_blurDepthFalloffThreshold, Ssao::DefaultBlurDepthFalloffThreshold)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, BlurDepthFalloffThreshold, m_blurDepthFalloffThreshold)

//! How much the difference in depth slopes between pixels affects the blur falloff.
//! The higher this value, the sharper edges will appear
AZ_GFX_FLOAT_PARAM(BlurDepthFalloffStrength, m_blurDepthFalloffStrength, Ssao::DefaultBlurDepthFalloffStrength)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, BlurDepthFalloffStrength, m_blurDepthFalloffStrength)


// --- SSAO DOWNSAMPLE ---

// Whether to downsample the depth buffer before SSAO and upsample the result
AZ_GFX_BOOL_PARAM(EnableDownsample, m_enableDownsample, true)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(bool, EnableDownsample, m_enableDownsample)
