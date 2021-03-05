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

// Auto-generates code to copy settings from a given source
//
// Note: To use this you must first #define COPY_SOURCE as the pointer to the settings you are copying from

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        MemberName = COPY_SOURCE->Get##Name();                                                          \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        MemberName##Override = COPY_SOURCE->Get##Name##Override();                                      \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
