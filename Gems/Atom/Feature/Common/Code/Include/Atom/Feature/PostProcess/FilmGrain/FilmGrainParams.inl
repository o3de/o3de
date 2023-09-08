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

// Intensity of effect
AZ_GFX_FLOAT_PARAM(Intensity, m_intensity, FilmGrain::DefaultIntensity)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, Intensity, m_intensity)

// Dampening
AZ_GFX_FLOAT_PARAM(LuminanceDampening, m_luminanceDampening, FilmGrain::DefaultLuminanceDampening)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, LuminanceDampening, m_luminanceDampening)

// Scaling
AZ_GFX_FLOAT_PARAM(TilingScale, m_tilingScale, FilmGrain::DefaultTilingScale)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, TilingScale, m_tilingScale)

// Grain Source
AZ_GFX_TEXTURE2D_PARAM(GrainPath, m_grainPath, FilmGrain::DefaultGrainPath)
