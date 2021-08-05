/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Auto-generates virtual function declarations for getters and setters of specified parameters and overrides

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        virtual ValueType Get##Name() const = 0;                                                        \
        virtual void Set##Name(ValueType val) = 0;                                                      \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName,  OverrideValueType)                         \
        virtual OverrideValueType Get##Name##Override() const = 0;                                      \
        virtual void Set##Name##Override(OverrideValueType val) = 0;                                    \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
