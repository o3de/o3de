/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <cstddef>

namespace O3DELauncher
{
    // Increase the core and stack limits
    bool IncreaseResourceLimits();

    // Get the absolute path for any given input path if possible. If an absolute path cannot be
    // resolved, then return an empty string.
    const char* GetAbsolutePath(char* absolutePathBuffer, size_t absolutePathBufferSize, const char* inputPath);

}

