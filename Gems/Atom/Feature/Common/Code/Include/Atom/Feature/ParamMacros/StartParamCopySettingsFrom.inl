/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Auto-generates code to copy settings from a given source
//
// Note: To use this you must first #define COPY_SOURCE as the pointer to the settings you are copying from

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        MemberName = COPY_SOURCE->Get##Name();                                                          \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        MemberName##Override = COPY_SOURCE->Get##Name##Override();                                      \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
