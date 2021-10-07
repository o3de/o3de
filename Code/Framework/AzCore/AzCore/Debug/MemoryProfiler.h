/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#ifndef AZ_PROFILE_MEMORY_ALLOC
// No other profiler has defined the performance markers AZ_PROFILE_MEMORY_ALLOC (and friends), fall back to a Driller implementation (currently empty)
#   define AZ_PROFILE_MEMORY_ALLOC(category, address, size, context)
#   define AZ_PROFILE_MEMORY_ALLOC_EX(category, filename, lineNumber, address, size, context)
#   define AZ_PROFILE_MEMORY_FREE(category, address)
#   define AZ_PROFILE_MEMORY_FREE_EX(category, filename, lineNumber, address)
#endif
