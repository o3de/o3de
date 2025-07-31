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
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(bool, Enabled, m_enabled)

// Strength
AZ_GFX_FLOAT_PARAM(Strength, m_strength, MotionBlur::DefaultStrength)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, Strength, m_strength)

// Sample quality
AZ_GFX_COMMON_PARAM(MotionBlur::SampleQuality, SampleQuality, m_sampleQuality, MotionBlur::SampleQuality::High)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(MotionBlur::SampleQuality, SampleQuality, m_sampleQuality)
