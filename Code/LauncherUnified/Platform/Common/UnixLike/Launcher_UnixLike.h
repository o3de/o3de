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

