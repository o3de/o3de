/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Auto-generates member definition for specified parameters and overrides

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType MemberName = DefaultValue;                                                            \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        OverrideValueType MemberName##Override = (OverrideValueType)1;                                  \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
