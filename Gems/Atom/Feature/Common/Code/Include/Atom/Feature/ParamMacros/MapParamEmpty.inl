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
