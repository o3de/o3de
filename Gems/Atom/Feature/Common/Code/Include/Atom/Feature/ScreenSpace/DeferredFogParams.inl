/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Macros below are of the form:
// PARAM(NAME, MEMBER_NAME, DEFAULT_VALUE, ...)

AZ_GFX_VEC3_PARAM(FogColor, m_fogColor, Vector3(0.45, 0.45, 0.6) )

AZ_GFX_FLOAT_PARAM(FogStartDistance, m_fogStartDistance, 1.0f)
AZ_GFX_FLOAT_PARAM(FogEndDistance, m_fogEndDistance, 5.0f )
AZ_GFX_FLOAT_PARAM(FogMinHeight, m_fogMinHeight, 0.01f)
AZ_GFX_FLOAT_PARAM(FogMaxHeight, m_fogMaxHeight, 1.0f)

AZ_GFX_FLOAT_PARAM(FogDensity, m_fogDensity, 0.33f)
AZ_GFX_FLOAT_PARAM(FogDensityClamp, m_fogDensityClamp, 1.0f)
AZ_GFX_COMMON_PARAM(Render::FogMode, FogMode, m_fogMode, FogMode::Linear)

AZ_GFX_TEXTURE2D_PARAM(NoiseTexture, m_noiseTexture, "textures/cloudnoise_01.jpg.streamingimage")

// First noise octave
AZ_GFX_VEC2_PARAM(NoiseTexCoordScale, m_noiseScaleUV, Vector2(0.01f, 0.01f) )
AZ_GFX_VEC2_PARAM(NoiseTexCoordVelocity, m_noiseVelocityUV, Vector2(0.002f, 0.0032f) )

// Second noise octave
AZ_GFX_VEC2_PARAM(NoiseTexCoord2Scale, m_noiseScaleUV2, Vector2(0.0239f, 0.0239f))
AZ_GFX_VEC2_PARAM(NoiseTexCoord2Velocity, m_noiseVelocityUV2, Vector2(0.00275f, -0.004f))

// Amount of blend between octaves: noise = (1 - blend) * octave1 + blend * octave2
AZ_GFX_FLOAT_PARAM(OctavesBlendFactor, m_octavesBlendFactor, 0.4f)
