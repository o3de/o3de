/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Profiler.h>

AZ_DECLARE_BUDGET(Vegetation);

// VEG_PROFILE_ENABLED is defined in the wscript
// VEG_PROFILE_ENABLED is only defined in the Vegetation gem by default
#if defined(VEG_PROFILE_ENABLED)
#define VEG_PROFILE_METHOD(Method) Method
#else
#define VEG_PROFILE_METHOD(...) // no-op
#endif

//#define ENABLE_VEGETATION_PROFILE_VERBOSE
#ifdef ENABLE_VEGETATION_PROFILE_VERBOSE
// Add verbose profile markers
#define VEGETATION_PROFILE_SCOPE_VERBOSE(...) AZ_PROFILE_SCOPE(Vegetation, __VA_ARGS__);
#define VEGETATION_PROFILE_FUNCTION_VERBOSE AZ_PROFILE_FUNCTION(Vegetation);
#else
// Define ENABLE_VEGETATION_PROFILE_VERBOSE to get verbose profile markers
#define VEGETATION_PROFILE_SCOPE_VERBOSE(...)
#define VEGETATION_PROFILE_FUNCTION_VERBOSE
#endif
