/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/Debug/TraceContextBufferedFormatter.h>

#ifdef AZ_ENABLE_TRACE_CONTEXT

#include <AzCore/Math/Uuid.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzToolsFramework/Debug/TraceContextStackInterface.h>

#include <inttypes.h>

#endif // AZ_ENABLE_TRACE_CONTEXT

namespace AzToolsFramework
{
    namespace Debug
    {

#ifdef AZ_ENABLE_TRACE_CONTEXT

        int TraceContextBufferedFormatter::Print(char* buffer, size_t bufferSize, const TraceContextStackInterface& stack, bool printUuids, size_t startIndex)
        {
            if (bufferSize == 0)
            {
                return -1;
            }
            
            // Make sure there's always a terminator, even if nothing has been written.
            buffer[0] = 0;

            size_t stackSize = stack.GetStackCount();
            for (size_t i = startIndex; i < stackSize; ++i)
            {
                int written = 0;
                switch (stack.GetType(i))
                {
                case TraceContextStackInterface::ContentType::StringType:
                    written = azsnprintf(buffer, bufferSize, "%s=%s\n", stack.GetKey(i), stack.GetStringValue(i));
                    break;
                case TraceContextStackInterface::ContentType::BoolType:
                    written = azsnprintf(buffer, bufferSize, "%s=%c\n", stack.GetKey(i), (stack.GetBoolValue(i) ? '1' : '0'));
                    break;
                case TraceContextStackInterface::ContentType::IntType:
                    written = azsnprintf(buffer, bufferSize, "%s=%" PRIi64 "\n", stack.GetKey(i), stack.GetIntValue(i));
                    break;
                case TraceContextStackInterface::ContentType::UintType:
                    written = azsnprintf(buffer, bufferSize, "%s=%" PRIu64 "\n", stack.GetKey(i), stack.GetUIntValue(i));
                    break;
                case TraceContextStackInterface::ContentType::FloatType:
                    written = azsnprintf(buffer, bufferSize, "%s=%f\n", stack.GetKey(i), stack.GetFloatValue(i));
                    break;
                case TraceContextStackInterface::ContentType::DoubleType:
                    written = azsnprintf(buffer, bufferSize, "%s=%f\n", stack.GetKey(i), stack.GetDoubleValue(i));
                    break;
                case TraceContextStackInterface::ContentType::UuidType:
                    if (printUuids)
                    {
                        written = azsnprintf(buffer, bufferSize, "%s=", stack.GetKey(i));
                        if (written > 0)
                        {
                            int uuidWritten = PrintUuid(buffer + written, bufferSize - written, stack.GetUuidValue(i));
                            written = (uuidWritten < 0 ? -1 : (written + uuidWritten));
                        }
                        break;
                    }
                    else
                    {
                        continue;
                    }
                case TraceContextStackInterface::ContentType::Undefined:
                    written = azsnprintf(buffer, bufferSize, "<UNDEFINED>\n");
                    break;
                default:
                    written = azsnprintf(buffer, bufferSize, "<UNKNOWN>\n");
                    break;
                }

                // If successful azsnprintf will return the number of characters that were
                //      written, so move the buffer forward and reduce the available space.
                //      Otherwise see if there's anything written that needs to be recovered
                //      or to simply move to the next entry upon re-entry.
                if (written > 0)
                {
                    buffer += written;
                    bufferSize -= written;
                }
                else
                {
                    // If the startIndex is the same as the current index, this is the first
                    //      entry to be written. It means this is the largest the buffer will
                    //      ever get, so leave whatever has been written in place. Do however
                    //      add a newline.
                    if (startIndex == i)
                    {
                        if (bufferSize >= 2)
                        {
                            buffer[bufferSize - 2] = '\n';
                            buffer[bufferSize - 1] = 0;
                        }
                        return aznumeric_caster(i + 1);
                    }
                    else
                    {
                        if (bufferSize > 0)
                        {
                            // Remove whatever part has been written as it's not complete.
                            *buffer = 0;
                        }
                    }
                    return aznumeric_caster(i);
                }
            }
            return -1;
        }

        int TraceContextBufferedFormatter::PrintUuid(char* buffer, size_t bufferSize, const AZ::Uuid& uuid)
        {
            int written = uuid.ToString(buffer, aznumeric_caster(bufferSize), false);
            if (written > 0)
            {
                if (bufferSize > written)
                {
                    buffer[written - 1] = '\n';
                    buffer[written] = 0;
                    return written + 1;
                }
            }
            return -1;
        }

#else // AZ_ENABLE_TRACE_CONTEXT

        int TraceContextBufferedFormatter::Print(char* /*buffer*/,  size_t /*bufferSize*/, 
            const TraceContextStackInterface& /*stack*/, bool /*printUuids*/, size_t /*startIndex*/)
        {
            return -1;
        }

#endif // AZ_ENABLE_TRACE_CONTEXT
    } // Debug
} // AzToolsFramework
