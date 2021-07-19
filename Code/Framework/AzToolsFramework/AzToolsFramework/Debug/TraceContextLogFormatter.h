/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/ObjectStream.h>

namespace AZ
{
    struct Uuid;
}

namespace AzToolsFramework
{
    namespace Debug
    {
        class TraceContextStackInterface;

        // TraceContextLogFormatter takes the trace context for the current thread and prints
        //      it to a string. It's aimed to create a humanly readable output to provide 
        //      contextual information alongside a message. By default uuids are ignored, but 
        //      optionally these can be printed as well.
        //
        //      Typical output will look like:
        //          [String] = text
        //          [Integer] = 42
        //          [Float] = 3.141500
        //          [Uuid] = B0E342BA-0D79-412D-ABDE-11FC29D4875C

        class TraceContextLogFormatter
        {
        public:
            // Prints all entries in the trace context stack to a string, except any object or tags.
            static void Print(AZStd::string& out, const TraceContextStackInterface& stack, bool printUuids = false);
            // Appends a single entry from the trace context stack to a string.
            static void PrintLine(AZStd::string& out, const TraceContextStackInterface& stack, size_t index);
            // Appends a single value from the trace context stack to a string.
            static void PrintValue(AZStd::string& out, const TraceContextStackInterface& stack, size_t index);
        };
    } // Debug
} // AzToolsFramework
