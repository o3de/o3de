/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Auto-generates setter and getter definitions for specified parameters and overrides

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        virtual ValueType Get##Name() const { return MemberName; }                                      \
        virtual void Set##Name(ValueType val) { MemberName = val; }                                     \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        virtual OverrideValueType Get##Name##Override() const { return MemberName##Override; }          \
        virtual void Set##Name##Override(OverrideValueType val) { MemberName##Override = val; }         \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
