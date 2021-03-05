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

// Maps all OVERRIDE declarations to AZ_GFX_COMMON_OVERRIDE, allowing the user to create a single definition
// using AZ_GFX_COMMON_OVERRIDE in order to handle all the types

#define AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(ValueType, Name, MemberName)                                     \
        AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, bool)                                       \

#define AZ_GFX_INTEGER_PARAM_FLOAT_OVERRIDE(ValueType, Name, MemberName)                                \
        AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, float)                                      \

#define AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(ValueType, Name, MemberName)                                  \
        AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, float)                                      \

#define AZ_GFX_STRING_PARAM_STRING_OVERRIDE(ValueType, Name, MemberName)                                \
        AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, AZStd::string)                              \


