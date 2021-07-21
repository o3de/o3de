/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
