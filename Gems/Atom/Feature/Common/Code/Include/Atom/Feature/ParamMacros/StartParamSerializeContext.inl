/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Auto-generates serialize context reflection code for the specified parameters and overrides
//
// Note: To use this you must first define SERIALIZE_CLASS as the class you are serializing
// Example: #define SERIALIZE_CLASS DepthOfFieldSettings

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ->Field(#Name, &SERIALIZE_CLASS::MemberName)                                                    \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        ->Field(#Name "Override", &SERIALIZE_CLASS::MemberName##Override)                               \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>

