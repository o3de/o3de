/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Defines all OVERRIDE declarations as empty. Useful when invoking a file that has both PARAM 
// and OVERRIDE declarations but only the PARAM declarations are needed for the use case

#define AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(ValueType, Name, MemberName)
#define AZ_GFX_INTEGER_PARAM_FLOAT_OVERRIDE(ValueType, Name, MemberName)
#define AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(ValueType, Name, MemberName)

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)
