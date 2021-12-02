/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Module/Environment.h>
#include <IAudioInterfacesCommonData.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioLogType
    {
        eALT_ASSERT,
        eALT_ERROR,
        eALT_WARNING,
        eALT_COMMENT,
        eALT_ALWAYS,
    };

    namespace Log
    {
        enum Options : AZ::u8
        {
            None = 0,
            Errors = (1 << 0),
            Warnings = (1 << 1),
            Comments = (1 << 2),
        };

        inline AZ::u32 s_audioLogOptions = 0;

#if defined(ENABLE_AUDIO_LOGGING)
        // Eventually will get rid of the CAudioLogger class and objects and convert
        // all audio log calls to use Audio::Log::Print.
        // Audio::Log::Print will take variadic arguments, but for now format the
        // arguments in CAudioLogger::Log and call this PrintMsg.
        inline void PrintMsg(const EAudioLogType type, const char* const message)
        {
            static constexpr const char* AudioWindow = "Audio";

            switch (type)
            {
                case eALT_ASSERT:
                {
                    AZ_Assert(false, message);
                    break;
                }
                case eALT_ERROR:
                {
                    if ((s_audioLogOptions & Log::Options::Errors) != 0)
                    {
                        AZ_Error(AudioWindow, false, message);
                    }
                    break;
                }
                case eALT_WARNING:
                {
                    if ((s_audioLogOptions & Log::Options::Warnings) != 0)
                    {
                        AZ_Warning(AudioWindow, false, message);
                    }
                    break;
                }
                case eALT_COMMENT:
                {
                    if ((s_audioLogOptions & Log::Options::Comments) != 0)
                    {
                        AZ_TracePrintf(AudioWindow, message);
                    }
                    break;
                }
                case eALT_ALWAYS:
                {
                    AZ_Printf(AudioWindow, message);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
#endif
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title CAudioLogger>
    // Summary:
    //      A simple logger wrapper that uses independent verbosity
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioLogger
    {
    public:
        CAudioLogger() = default;
        ~CAudioLogger() = default;

        CAudioLogger(const CAudioLogger&) = delete;
        CAudioLogger& operator=(const CAudioLogger&) = delete;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title Log>
        // Summary:
        //      Log a message
        // Arguments:
        //      type        - log message type (e.g. eALT_ERROR, eALT_WARNING, eALT_COMMENT, etc)
        //      format, ... - printf-style format string and its arguments
        ///////////////////////////////////////////////////////////////////////////////////////////////
        void Log([[maybe_unused]] const EAudioLogType type, [[maybe_unused]] const char* const format, ...) const
        {
        #if defined(ENABLE_AUDIO_LOGGING)
            if (format && format[0] != '\0')
            {
                const size_t BUFFER_LEN = 1024;
                char buffer[BUFFER_LEN];

                va_list argList;
                va_start(argList, format);
                azvsnprintf(buffer, BUFFER_LEN, format, argList);
                va_end(argList);

                buffer[BUFFER_LEN - 1] = '\0';

                Audio::Log::PrintMsg(type, buffer);
            }
        #endif // ENABLE_AUDIO_LOGGING
        }
    };

} // namespace Audio
