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

// Description: Utilities and functions used when cryphysics is disabled

#pragma once

// Assert if CryPhysics is disabled and no functionality replacement has been implemented
// 1: Runtime AZ_Error
// 2: Runtime Assertion
// 3: Compilation error
// Other: Do nothing
#define ENABLE_CRY_PHYSICS_REPLACEMENT_ASSERT 0

#if (ENABLE_CRY_PHYSICS_REPLACEMENT_ASSERT == 1)
#define CRY_PHYSICS_REPLACEMENT_ASSERT() AZ_Error("CryPhysics", false, __FUNCTION__ " - CRYPHYSICS REPLACEMENT NOT IMPLEMENTED")
#elif (ENABLE_CRY_PHYSICS_REPLACEMENT_ASSERT == 2)
#define CRY_PHYSICS_REPLACEMENT_ASSERT() AZ_Assert(false, "CRYPHYSICS REPLACEMENT NOT IMPLEMENTED")
#elif (ENABLE_CRY_PHYSICS_REPLACEMENT_ASSERT == 3)
#define CRY_PHYSICS_REPLACEMENT_ASSERT() static_assert(false, __FUNCTION__ " - CRYPHYSICS REPLACEMENT NOT IMPLEMENTED")
#else
#define CRY_PHYSICS_REPLACEMENT_ASSERT() 
#endif
