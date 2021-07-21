/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/Debug/Trace.h>

#ifdef AZ_ENABLE_TRACING
#define AZ_ENABLE_SHADER_RELOAD_DEBUG_TRACKER
#endif

namespace AZ
{
    namespace RPI
    {
        //! Utility for printing trace statements about the callstack when doing shader and material hot-reload activities.
        //! (nothing about this class is necessarily specific to shader and material hot-reload though, so the class could
        //!  be generalized if needed elsewhere).
        class ShaderReloadDebugTracker final
        {
        public:
            static bool IsEnabled();

            //! Begin a code section. Will print a "[BEGIN] <sectionName>" header, and all subsequent calls will be indented.
            template<typename ... Args>
            static void BeginSection([[maybe_unused]] const char* sectionNameFormat, [[maybe_unused]] Args... args)
            {
#ifdef AZ_ENABLE_SHADER_RELOAD_DEBUG_TRACKER
                if (IsEnabled())
                {
                    const AZStd::string sectionName = AZStd::string::format(sectionNameFormat, args...);
                    AZ_TracePrintf("ShaderReloadDebug", "%*s [BEGIN] %s \n", s_indent, "", sectionName.c_str());
                    s_indent += IndentSpaces;
                }
#endif
            }

            //! Ends a code section. Will print "[_END_] <sectionName>", and un-indent subsequent messages by one level.
            template<typename ... Args>
            static void EndSection([[maybe_unused]] const char* sectionNameFormat, [[maybe_unused]] Args... args)
            {
#ifdef AZ_ENABLE_SHADER_RELOAD_DEBUG_TRACKER
                if (IsEnabled())
                {
                    const AZStd::string sectionName = AZStd::string::format(sectionNameFormat, args...);
                    s_indent -= IndentSpaces;
                    AZ_TracePrintf("ShaderReloadDebug", "%*s [_END_] %s \n", s_indent, "", sectionName.c_str());
                }
#endif
            }
            
            //! Prints a generic message at the appropriate indent level.
            template<typename ... Args>
            static void Printf([[maybe_unused]] const char* format, [[maybe_unused]] Args... args)
            {
#ifdef AZ_ENABLE_SHADER_RELOAD_DEBUG_TRACKER
                if (IsEnabled())
                {
                    const AZStd::string message = AZStd::string::format(format, args...);

                    AZ_TracePrintf("ShaderReloadDebug", "%*s %s \n", s_indent, "", message.c_str());
                }
#endif
            }

            //! Use this utility to call BeginSection(), and automatically call EndSection() when the object goes out of scope.
            class ScopedSection final
            {
            public:
                template<typename ... Args>
                ScopedSection(const char* sectionNameFormat, Args... args)
                {
                    m_sectionName = AZStd::string::format(sectionNameFormat, args...);
                    ShaderReloadDebugTracker::BeginSection("%s", m_sectionName.c_str());
                }

                ~ScopedSection();

            private:
                AZStd::string m_sectionName;
            };

        private:
            static bool s_enabled;
            static int s_indent;
            static constexpr int IndentSpaces = 4;
        };

    } // namespace RPI
} // namespace AZ
