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

namespace AssetMemoryAnalyzer
{
    namespace Data
    {
        struct CodePoint;
    }

    namespace FormatUtils
    {
        // Formats a location in code to a single line of human-readable text. Returns a pointer to the resulting string.
        // WARNING: Returns pointer to an internal static buffer for performance. Single-threaded access only!
        extern const char* FormatCodePoint(const Data::CodePoint& cp);

        // Formats a byte value to be easily read in kilobytes. Returns a pointer to the resulting string.
        // WARNING: Returns pointer to an internal static buffer for performance. Single-threaded access only!
        extern const char* FormatKB(size_t bytes);
    }
}
