/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Macros below are of the form:
// PARAM(NAME, MEMBER_NAME, DEFAULT_VALUE, ...)

AZ_GFX_BOOL_PARAM(Enabled, m_enabled, true)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(bool, Enabled, m_enabled)

AZ_GFX_COMMON_PARAM(AZ::Render::ExposureControl::ExposureControlType, ExposureControlType, m_exposureControlType, AZ::Render::ExposureControl::ExposureControlType::ManualOnly)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(AZ::Render::ExposureControl::ExposureControlType, ExposureControlType, m_exposureControlType)

AZ_GFX_FLOAT_PARAM(ManualCompensation, m_manualCompensationValue, 0.0f)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, ManualCompensation, m_manualCompensationValue)

AZ_GFX_FLOAT_PARAM(EyeAdaptationExposureMin, m_autoExposureMin, -16.0f)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, EyeAdaptationExposureMin, m_autoExposureMin)

AZ_GFX_FLOAT_PARAM(EyeAdaptationExposureMax, m_autoExposureMax, 16.0f)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, EyeAdaptationExposureMax, m_autoExposureMax)

AZ_GFX_FLOAT_PARAM(EyeAdaptationSpeedUp, m_autoExposureSpeedUp, 3.0)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, EyeAdaptationSpeedUp, m_autoExposureSpeedUp)

AZ_GFX_FLOAT_PARAM(EyeAdaptationSpeedDown, m_autoExposureSpeedDown, 1.0)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, EyeAdaptationSpeedDown, m_autoExposureSpeedDown)

AZ_GFX_BOOL_PARAM(HeatmapEnabled, m_heatmapEnabled, false)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(bool, HeatmapEnabled, m_heatmapEnabled)

