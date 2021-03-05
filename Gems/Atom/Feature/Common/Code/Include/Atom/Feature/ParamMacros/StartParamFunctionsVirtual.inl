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

// Auto-generates virtual function declarations for getters and setters of specified parameters and overrides

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        virtual ValueType Get##Name() const = 0;                                                        \
        virtual void Set##Name(ValueType val) = 0;                                                      \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName,  OverrideValueType)                         \
        virtual OverrideValueType Get##Name##Override() const = 0;                                      \
        virtual void Set##Name##Override(OverrideValueType val) = 0;                                    \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
