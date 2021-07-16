/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Defines all PARAM declarations as empty. Useful when invoking a file that has both PARAM 
// and OVERRIDE declarations but only the OVERRIDE declarations are needed for the use case

#define AZ_GFX_BOOL_PARAM(Name, MemberName, DefaultValue)
#define AZ_GFX_FLOAT_PARAM(Name, MemberName, DefaultValue)
#define AZ_GFX_UINT32_PARAM(Name, MemberName, DefaultValue)
#define AZ_GFX_VEC2_PARAM(Name, MemberName, DefaultValue)
#define AZ_GFX_VEC3_PARAM(Name, MemberName, DefaultValue)
#define AZ_GFX_VEC4_PARAM(Name, MemberName, DefaultValue)
#define AZ_GFX_TEXTURE2D_PARAM(Name, MemberName, DefaultValue)

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)
