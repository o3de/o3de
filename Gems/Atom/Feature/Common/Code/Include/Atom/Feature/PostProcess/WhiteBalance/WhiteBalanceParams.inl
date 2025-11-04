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

// Strength of effect
AZ_GFX_FLOAT_PARAM(Temperature, m_temperature, WhiteBalance::DefaultTemperature)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, Temperature, m_temperature)

// Blending
AZ_GFX_FLOAT_PARAM(Tint, m_tint, WhiteBalance::DefaultTint)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, Tint, m_tint)
