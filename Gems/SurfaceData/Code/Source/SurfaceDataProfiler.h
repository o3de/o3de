/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Profiler.h>

AZ_DECLARE_BUDGET(SurfaceData);

//#define ENABLE_SURFACE_DATA_PROFILE_VERBOSE
#ifdef ENABLE_SURFACE_DATA_PROFILE_VERBOSE
// Add verbose profile markers
#define SURFACE_DATA_PROFILE_SCOPE_VERBOSE(...) AZ_PROFILE_SCOPE(SurfaceData, __VA_ARGS__);
#define SURFACE_DATA_PROFILE_FUNCTION_VERBOSE AZ_PROFILE_FUNCTION(SurfaceData);
#else
// Define ENABLE_SURFACE_DATA_PROFILE_VERBOSE to get verbose profile markers
#define SURFACE_DATA_PROFILE_SCOPE_VERBOSE(...)
#define SURFACE_DATA_PROFILE_FUNCTION_VERBOSE
#endif
