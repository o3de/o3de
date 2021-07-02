/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
