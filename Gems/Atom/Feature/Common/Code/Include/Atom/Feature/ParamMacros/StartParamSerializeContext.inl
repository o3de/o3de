/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

