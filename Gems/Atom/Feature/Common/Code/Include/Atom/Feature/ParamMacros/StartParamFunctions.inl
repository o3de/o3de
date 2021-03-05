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

// Auto-generates setter and getter definitions for specified parameters and overrides

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        virtual ValueType Get##Name() const { return MemberName; }                                      \
        virtual void Set##Name(ValueType val) { MemberName = val; }                                     \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        virtual OverrideValueType Get##Name##Override() const { return MemberName##Override; }          \
        virtual void Set##Name##Override(OverrideValueType val) { MemberName##Override = val; }         \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
