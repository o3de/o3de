/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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


