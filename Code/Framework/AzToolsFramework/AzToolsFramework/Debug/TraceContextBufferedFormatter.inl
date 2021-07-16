/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AzToolsFramework
{
    namespace Debug
    {
        template<size_t size>
        inline int TraceContextBufferedFormatter::Print(char(&buffer)[size], const TraceContextStackInterface& stack, bool printUuids, size_t startIndex)
        {
            return Print(buffer, size, stack, printUuids, startIndex);
        }
    } // Debug
} // AzToolsFramework
