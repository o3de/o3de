/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// VEG_PROFILE_ENABLED is defined in the wscript
// VEG_PROFILE_ENABLED is only defined in the Vegetation gem by default
#if defined(VEG_PROFILE_ENABLED)
#define VEG_PROFILE_METHOD(Method) Method
#else
#define VEG_PROFILE_METHOD(...) // no-op
#endif

