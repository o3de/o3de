#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/functional.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QVariant>

namespace AzToolsFramework
{
    namespace LogPanel
    {
        class GenericLogPanel;
    }

    namespace Logging
    {
        //! LogLine is a structured version of raw text logged with our various logging mechanisms. It is meant to be just a
        //! blind data holder, so you can change its fields directly before submitting it to a model, or just create it via
        //! the constructor.  Its only not a struct because it has some functions on it to help.
        class LogLine
        {
        public:
            AZ_CLASS_ALLOCATOR(LogLine, AZ::SystemAllocator);

            enum LogType
            {
                TYPE_DEBUG, // really low priority
                TYPE_MESSAGE, // normal priority
                TYPE_WARNING, 
                TYPE_ERROR, 
                TYPE_CONTEXT, // Additional information
            };

            // ========= Constructors\Destructor =========
            // Construction and copying operations are supported:
            LogLine() = default;
            LogLine(const char* inMessage, const char* inWindow, LogType inType, AZ::u64 inTime, void* data = nullptr, uintptr_t threadId = 0);
            LogLine(const LogLine& other);
            LogLine& operator=(const LogLine& other);
            
            // Move operations are used when consumed by the log data models, so once you submit it, it will be emptied out.
            LogLine(LogLine&& other);
            LogLine& operator=(LogLine&& other);

            // Destructor
            ~LogLine() = default; // note:  non virtual, this needs to be very lightweight

            // ========= Accessors =========
            const AZStd::string& GetLogMessage() const;
            const AZStd::string& GetLogWindow() const;
            LogType GetLogType() const;
            AZ::u64 GetLogTime() const;
            uintptr_t GetLogThreadId() const;


            void* GetUserData() const;
            void SetUserData(void* data);

            // ========= Utilities =========
            AZStd::string ToString(); // for writing it out to logs and copy/paste

            // (internal use)
            QVariant data(int column, int role) const;

            /*! Log parsing note
            For speed's sake, the panels prefer to not allocate any dynamic memory where possible. To enable that
            to work as quickly and as efficiently as possible, one version of ParseLog allows you to provide a callback
            for every successfully parsed log line.
            Because we are parsing out LogLine& and not dynamically allocating storage for these, the other parse function
            will take in a linked list of LogLines to avoid unnecessary class moves or copies. */
            static bool ParseLog(const char* entireLog, AZ::u64 logLength, AZStd::function<void(LogLine&)> logLineAdder);
            static bool ParseLog(AZStd::list<LogLine>& logList,  const char* entireLog, AZ::u64 logLength);
            static bool ParseLine(LogLine& outLine, const AZStd::string& line);
            static bool ParseContextLogLine(const LogLine& line, std::pair<QString, QString>& result);

        private:
            // "Processing" a log line happens once, and its where we check to see if it contains any special tags
            // or rich text,  to change its severity or log type.
            void Process();

        private:
            AZStd::string m_message;
            AZStd::string m_window;
            AZ::u64 m_messageTime = 0; // MSecs since epoch (Qt style)
            LogType m_type = TYPE_MESSAGE;
            bool m_isRichText = false;
            bool m_processed = false;
            uintptr_t m_threadId = 0;
            void* m_userData = nullptr;
        };
    }
}

Q_DECLARE_METATYPE(const AzToolsFramework::Logging::LogLine*);
