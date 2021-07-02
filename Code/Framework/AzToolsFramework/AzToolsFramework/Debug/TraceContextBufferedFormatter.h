/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>

namespace AZ
{
    struct Uuid;
}

namespace AzToolsFramework
{
    namespace Debug
    {
        class TraceContextStackInterface;

        // TraceContexBufferedFormatter takes a trace context stack and prints it to the given buffer. 
        //      It's aimed to be used with small to micro sized character buffers. (At least 50-100 
        //      characters is advised.) If the entire context couldn't be written to the buffer, Build 
        //      can be called repeatedly with the returned index to continue printing the buffer. If a 
        //      single context entry doesn't fit in the buffer, TraceContexBufferedFormatter will attempt 
        //      to write as much data as can be fitted in the buffer.
        //
        //      Typical usage looks like:
        //          TraceContextSingleStackHandler stackHandler;
        //          ...
        //          TraceContexBufferedFormatter buffered;
        //          char buffer[64];
        //          int index = 0;
        //          do
        //          {
        //              index = buffered.Build(buffer, stackHandler.GetStack(), true, index);
        //              Print(buffer);
        //          } while (index >= 0);
        //
        //      Example output:
        //          String=text
        //          Integer=42
        //          Float=3.141500
        //          Uuid=E2C7EEFA-B1CA-465F-A4BC-30514F76B7B5

        class TraceContextBufferedFormatter
        {
        public:
            // Prints the trace context to the given buffer, tags.
            //      If printUuids is true, the uuid of objects and tags is printed as well.
            //      Use startIndex to continue from a specific entry.
            //      Returns the index of the next entry to be written or -1 if no entries are left.
            static int Print(char* buffer, size_t bufferSize, const TraceContextStackInterface& stack, bool printUuids, size_t startIndex = 0);
            
            template<size_t size>
            static inline int Print(char(&buffer)[size], const TraceContextStackInterface& stack, bool printUuids, size_t startIndex = 0);

        private:
            static int PrintUuid(char* buffer, size_t bufferSize, const AZ::Uuid& uuid);
        };
    } // Debug
} // AzToolsFramework

#include <AzToolsFramework/Debug/TraceContextBufferedFormatter.inl>
