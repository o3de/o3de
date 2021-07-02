/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Maps all PARAM declarations to AZ_GFX_COMMON_PARAM, allowing the user to handle all types with a single definition for AZ_GFX_COMMON_PARAM
//
// Why don't we always use AZ_GFX_COMMON_PARAM instead of the specif macros in the first place?
// Because this way allows for greater flexibility for users to define custom behavior for different types should
// they so choose. Doing it like this from the start is easy and much less costly than a refactor down the road.

#define AZ_GFX_BOOL_PARAM(Name, MemberName, DefaultValue)                                               \
        AZ_GFX_COMMON_PARAM(bool, Name, MemberName, DefaultValue)                                       \

#define AZ_GFX_UINT32_PARAM(Name, MemberName, DefaultValue)                                             \
        AZ_GFX_COMMON_PARAM(uint32_t, Name, MemberName, DefaultValue)                                   \

#define AZ_GFX_FLOAT_PARAM(Name, MemberName, DefaultValue)                                              \
        AZ_GFX_COMMON_PARAM(float, Name, MemberName, DefaultValue)                                      \

#define AZ_GFX_VEC2_PARAM(Name, MemberName, DefaultValue)                                               \
        AZ_GFX_COMMON_PARAM(AZ::Vector2, Name, MemberName, DefaultValue)                                \

#define AZ_GFX_VEC3_PARAM(Name, MemberName, DefaultValue)                                               \
        AZ_GFX_COMMON_PARAM(AZ::Vector3, Name, MemberName, DefaultValue)                                \

#define AZ_GFX_VEC4_PARAM(Name, MemberName, DefaultValue)                                               \
        AZ_GFX_COMMON_PARAM(AZ::Vector4, Name, MemberName, DefaultValue)                                \

#define AZ_GFX_TEXTURE2D_PARAM(Name, MemberName, DefaultValue)                                          \
        AZ_GFX_COMMON_PARAM(AZStd::string, Name, MemberName, DefaultValue)                              \
