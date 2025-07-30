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

// The strength multiplier for the SSAO effect
AZ_GFX_FLOAT_PARAM(SsaoStrength, m_ssaoStrength, Ssao::DefaultSsaoStrength)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, SsaoStrength, m_ssaoStrength)

// The sampling radius calculated in screen UV space
AZ_GFX_FLOAT_PARAM(SsaoSamplingRadius, m_ssaoSamplingRadius, Ssao::DefaultSsaoSamplingRadius)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, SsaoSamplingRadius, m_ssaoSamplingRadius)
