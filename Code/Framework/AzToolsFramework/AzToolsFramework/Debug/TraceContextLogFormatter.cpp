/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/Debug/TraceContextLogFormatter.h>

#ifdef AZ_ENABLE_TRACE_CONTEXT

#include <AzToolsFramework/Debug/TraceContextStackInterface.h>

#endif // AZ_ENABLE_TRACE_CONTEXT

#include <cinttypes>

namespace AzToolsFramework
{
    namespace Debug
    {
#ifdef AZ_ENABLE_TRACE_CONTEXT

        void TraceContextLogFormatter::Print(AZStd::string& out, const TraceContextStackInterface& stack, bool printUuids)
        {
            size_t stackSize = stack.GetStackCount();
            for (size_t i = 0; i < stackSize; ++i)
            {
                if (!printUuids && stack.GetType(i) == TraceContextStackInterface::ContentType::UuidType)
                {
                    continue;
                }

                PrintLine(out, stack, i);
                out += '\n';
            }
        }

        void TraceContextLogFormatter::PrintLine(AZStd::string& out, const TraceContextStackInterface& stack, size_t index)
        {
            // Don't assert on this as it could cause an infinite loop of asserts that trigger trace context reporting.
            if (index >= stack.GetStackCount())
            {
                return;
            }

            out += AZStd::string::format("[%s] = ", stack.GetKey(index));
            PrintValue(out, stack, index);
        }

        void TraceContextLogFormatter::PrintValue(AZStd::string& out, const TraceContextStackInterface& stack, size_t index)
        {
            // Don't assert on this as it could cause an infinite loop of asserts that trigger trace context reporting.
            if (index >= stack.GetStackCount())
            {
                return;
            }

            switch (stack.GetType(index))
            {
            case TraceContextStackInterface::ContentType::StringType:
                out += stack.GetStringValue(index);
                break;
            case TraceContextStackInterface::ContentType::BoolType:
                out += stack.GetBoolValue(index) ? "true" : "false";
                break;
            case TraceContextStackInterface::ContentType::IntType:
                out += AZStd::string::format("%" PRId64, stack.GetIntValue(index));
                break;
            case TraceContextStackInterface::ContentType::UintType:
                out += AZStd::string::format("%" PRIu64, stack.GetUIntValue(index));
                break;
            case TraceContextStackInterface::ContentType::FloatType:
                out += AZStd::string::format("%f", stack.GetFloatValue(index));
                break;
            case TraceContextStackInterface::ContentType::DoubleType:
                out += AZStd::string::format("%f", stack.GetDoubleValue(index));
                break;
            case TraceContextStackInterface::ContentType::UuidType:
                out += stack.GetUuidValue(index).ToString<AZStd::string>(false).c_str();
                break;
            case TraceContextStackInterface::ContentType::Undefined:
                out += "<undefined value type>";
                break;
            default:
                out += "<unknown value type>";
                break;
            }
        }

#else // AZ_ENABLE_TRACE_CONTEXT

        void TraceContextLogFormatter::Print(AZStd::string& /*out*/, const TraceContextStackInterface& /*stack*/, bool /*printUuids*/)
        {
        }

        void TraceContextLogFormatter::PrintLine(AZStd::string& /*out*/, const TraceContextStackInterface& /*stack*/, size_t /*index*/)
        {
        }
        
        void TraceContextLogFormatter::PrintValue(AZStd::string& /*out*/, const TraceContextStackInterface& /*stack*/, size_t /*index*/)
        {
        }

#endif // AZ_ENABLE_TRACE_CONTEXT
    } // Debug
} // AzToolsFramework
