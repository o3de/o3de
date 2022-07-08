/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Auto-generates override function declarations for getters and setters of specified parameters and overrides

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType Get##Name() const override { return MemberName; }                                     \
        void Set##Name(ValueType val) override { MemberName = val; }                                    \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        OverrideValueType Get##Name##Override() const override { return MemberName##Override; }         \
        void Set##Name##Override(OverrideValueType val) override { MemberName##Override = val; }        \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
