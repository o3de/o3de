/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Macros below are of the form:
// PARAM(NAME, MEMBER_NAME, DEFAULT_VALUE, ...)

// Whether to turn on ray tracing debugging
AZ_GFX_BOOL_PARAM(Enabled, m_enabled, true)

// Which property to display for the hit surface (InstanceIndex, PrimitiveIndex, barycentrics, ...)
AZ_GFX_COMMON_PARAM(RayTracingDebugViewMode, DebugViewMode, m_debugViewMode, RayTracingDebugViewMode::InstanceIndex)
