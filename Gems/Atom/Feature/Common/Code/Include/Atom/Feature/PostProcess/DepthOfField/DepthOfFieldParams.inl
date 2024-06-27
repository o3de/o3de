/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Macros below are of the form:
// PARAM(NAME, MEMBER_NAME, DEFAULT_VALUE, ...)

AZ_GFX_COMMON_PARAM(EntityId, CameraEntityId, m_cameraEntityId, EntityId())
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(EntityId, CameraEntityId, m_cameraEntityId)

AZ_GFX_BOOL_PARAM(Enabled, m_enabled, false)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(bool, Enabled, m_enabled)

AZ_GFX_UINT32_PARAM(QualityLevel, m_qualityLevel, DepthOfField::QualityLevelMax - 1)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(uint32_t, QualityLevel, m_qualityLevel)

AZ_GFX_FLOAT_PARAM(ApertureF, m_apertureF, 0.5f)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, ApertureF, m_apertureF)

AZ_GFX_FLOAT_PARAM(FNumber, m_fNumber, 0.0f)

AZ_GFX_FLOAT_PARAM(FocusDistance, m_focusDistance, 100.0f)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, FocusDistance, m_focusDistance)

// Auto Focus Params...
AZ_GFX_BOOL_PARAM(EnableAutoFocus, m_enableAutoFocus, true)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(bool, EnableAutoFocus, m_enableAutoFocus)

AZ_GFX_COMMON_PARAM(EntityId, FocusedEntityId, m_focusedEntityId, EntityId())
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(EntityId, FocusedEntityId, m_focusedEntityId)

AZ_GFX_VEC2_PARAM(AutoFocusScreenPosition, m_autoFocusScreenPosition, AZ::Vector2(0.5f, 0.5f))
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(AZ::Vector2, AutoFocusScreenPosition, m_autoFocusScreenPosition)

AZ_GFX_FLOAT_PARAM(AutoFocusSensitivity, m_autoFocusSensitivity, 1.0f)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, AutoFocusSensitivity, m_autoFocusSensitivity)

AZ_GFX_FLOAT_PARAM(AutoFocusSpeed, m_autoFocusSpeed, DepthOfField::AutoFocusSpeedMax)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, AutoFocusSpeed, m_autoFocusSpeed)

AZ_GFX_FLOAT_PARAM(AutoFocusDelay, m_autoFocusDelay, 0.0f)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, AutoFocusDelay, m_autoFocusDelay)

// Debugging parameters...
AZ_GFX_BOOL_PARAM(EnableDebugColoring, m_enableDebugColoring, false)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(bool, EnableDebugColoring, m_enableDebugColoring)
