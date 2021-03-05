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

#include <AzToolsFramework/Debug/TraceContextLogFormatter.h>
#include <TraceDrillerHook.h>

namespace AZ
{
    namespace RC
    {
        TraceDrillerHook::TraceDrillerHook()
            : m_errorCount(0)
        {
            BusConnect();
        }
        
        TraceDrillerHook::~TraceDrillerHook()
        {
            BusDisconnect();
        }

        bool TraceDrillerHook::OnPreAssert(const char* fileName, int line, const char* func, const char* message)
        {
            AZ_UNUSED(fileName);
            AZ_UNUSED(line);
            AZ_UNUSED(func);

            DumpContextStack();
            m_errorCount++;
            RCLogError("%.*s", CalculateLineLength(message), message);
            return true;
        }

        bool TraceDrillerHook::OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message)
        {
            AZ_UNUSED(window);
            AZ_UNUSED(fileName);
            AZ_UNUSED(line);
            AZ_UNUSED(func);
            
            DumpContextStack();
            m_errorCount++;
            RCLogError("%.*s", CalculateLineLength(message), message);
            return true;
        }

        bool TraceDrillerHook::OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message)
        {
            AZ_UNUSED(window);
            AZ_UNUSED(fileName);
            AZ_UNUSED(line);
            AZ_UNUSED(func);

            DumpContextStack();
            RCLogWarning("%.*s", CalculateLineLength(message), message);
            return true;
        }

        bool TraceDrillerHook::OnPrintf(const char* window, const char* message)
        {
            DumpContextStack();
            
            // "%.*s" specifier only supports int for its size
            int messageLineLen = aznumeric_cast<int>(CalculateLineLength(message));
            if (AzFramework::StringFunc::Equal(window, SceneAPI::Utilities::ErrorWindow))
            {
                m_errorCount++;
                RCLogError("%.*s", messageLineLen, message);
            }
            else if (AzFramework::StringFunc::Equal(window, SceneAPI::Utilities::WarningWindow))
            {
                RCLogWarning("%.*s", messageLineLen, message);
            }
            else
            {
                RCLog("%.*s", messageLineLen, message);
            }
            return true;
        }

        size_t TraceDrillerHook::GetErrorCount() const
        {
            return m_errorCount;
        }

        void TraceDrillerHook::DumpContextStack() const
        {
            AZStd::shared_ptr<const AzToolsFramework::Debug::TraceContextStack> stack = m_stacks.GetCurrentStack();
            if (stack)
            {
                AZStd::string line;
                size_t stackSize = stack->GetStackCount();
                for (size_t i = 0; i < stackSize; ++i)
                {
                    line.clear();
                    if (stack->GetType(i) == AzToolsFramework::Debug::TraceContextStackInterface::ContentType::UuidType)
                    {
                        continue;
                    }

                    AzToolsFramework::Debug::TraceContextLogFormatter::PrintLine(line, *stack, i);
                    RCLogContext(line.c_str());
                }
            }
        }

        size_t TraceDrillerHook::CalculateLineLength(const char* message) const
        {
            size_t length = strlen(message);
            while ((message[length - 1] == '\n' || message[length - 1] == '\r' ) && length > 1)
            {
                length--;
            }
            return length;
        }
    } // RC
} // AZ
