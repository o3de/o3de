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

// Brightness threshold for bloom
AZ_GFX_FLOAT_PARAM(Threshold, m_threshold, Bloom::DefaultThreshold)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, Threshold, m_threshold)

// Soft knee value for smooth threshold falloff
AZ_GFX_FLOAT_PARAM(Knee, m_knee, Bloom::DefaultKnee)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, Knee, m_knee)

AZ_GFX_FLOAT_PARAM(Intensity, m_intensity, Bloom::DefaultIntensity)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, Intensity, m_intensity)

AZ_GFX_BOOL_PARAM(BicubicEnabled, m_enableBicubic, Bloom::DefaultEnableBicubicFilter)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(bool, BicubicEnabled, m_enableBicubic)

AZ_GFX_FLOAT_PARAM(KernelSizeScale, m_kernelSizeScale, Bloom::DefaultKernelSizeScale)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, KernelSizeScale, m_kernelSizeScale)

// Kernel width for each downsampled blurring stage, the value is the ratio of kernel size and screen width
// for example for a 80x100 image with ratio 0.04 the kernel size is 100 x 0.04 = 4 pixel, i.e. 4x4
AZ_GFX_FLOAT_PARAM(KernelSizeStage0, m_kernelSizeStage0, Bloom::DefaultScreenPercentStage0)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, KernelSizeStage0, m_kernelSizeStage0)
AZ_GFX_FLOAT_PARAM(KernelSizeStage1, m_kernelSizeStage1, Bloom::DefaultScreenPercentStage1)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, KernelSizeStage1, m_kernelSizeStage1)
AZ_GFX_FLOAT_PARAM(KernelSizeStage2, m_kernelSizeStage2, Bloom::DefaultScreenPercentStage2)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, KernelSizeStage2, m_kernelSizeStage2)
AZ_GFX_FLOAT_PARAM(KernelSizeStage3, m_kernelSizeStage3, Bloom::DefaultScreenPercentStage3)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, KernelSizeStage3, m_kernelSizeStage3)
AZ_GFX_FLOAT_PARAM(KernelSizeStage4, m_kernelSizeStage4, Bloom::DefaultScreenPercentStage4)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, KernelSizeStage4, m_kernelSizeStage4)

// Tint color for each downsample stage
AZ_GFX_VEC3_PARAM(TintStage0, m_tintStage0, Bloom::DefaultTint)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(AZ::Vector3, TintStage0, m_tintStage0)
AZ_GFX_VEC3_PARAM(TintStage1, m_tintStage1, Bloom::DefaultTint)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(AZ::Vector3, TintStage1, m_tintStage1)
AZ_GFX_VEC3_PARAM(TintStage2, m_tintStage2, Bloom::DefaultTint)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(AZ::Vector3, TintStage2, m_tintStage2)
AZ_GFX_VEC3_PARAM(TintStage3, m_tintStage3, Bloom::DefaultTint)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(AZ::Vector3, TintStage3, m_tintStage3)
AZ_GFX_VEC3_PARAM(TintStage4, m_tintStage4, Bloom::DefaultTint)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(AZ::Vector3, TintStage4, m_tintStage4)
