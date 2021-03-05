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

// Note: To use this you must first #define COPY_TARGET as the pointer to the settings you are copying to
// Example: (see PostFxLayerComponentConfig::CopySettingsTo)
// #define COPY_TARGET settings

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        COPY_TARGET->Set##Name(MemberName);                                                             \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        COPY_TARGET->Set##Name##Override(MemberName##Override);                                         \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
