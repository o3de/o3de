/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QDateTime>
#include <AzToolsFramework/UI/Logging/LogEntry.h>
#include <AzToolsFramework/UI/Logging/LogLine.h>

namespace AzToolsFramework
{
    namespace Logging
    {
        AZ::Crc32 LogEntry::s_messageField = AZ_CRC_CE("message");
        const char* LogEntry::s_messageFieldName = "message";
        AZ::Crc32 LogEntry::s_windowField = AZ_CRC_CE("window");
        const char* LogEntry::s_windowFieldName = "window";

        LogEntry::Field::Field(Field&& rhs)
            : m_name(AZStd::move(rhs.m_name))
            , m_value(AZStd::move(rhs.m_value))
        {
        }

        LogEntry::Field& LogEntry::Field::operator=(Field&& rhs)
        {
            m_name = AZStd::move(rhs.m_name);
            m_value = AZStd::move(rhs.m_value);
            return *this;
        }
        
        const LogEntry::FieldStorage& LogEntry::GetFields() const
        {
            return m_fields;
        }

        AZ::u64 LogEntry::GetRecordedTime() const
        {
            return m_recordedTime;
        }

        LogEntry::Severity LogEntry::GetSeverity() const
        {
            return m_severity;
        }

        void LogEntry::AddField(const Field& field)
        {
            AZ::Crc32 key(field.m_name.c_str(), field.m_name.size(), true);
            m_fields[key] = field;
        }

        void LogEntry::AddField(Field&& field)
        {
            AZ::Crc32 key(field.m_name.c_str(), field.m_name.size(), true);
            m_fields[key] = AZStd::move(field);
        }

        void LogEntry::SetRecordedTime(AZ::u64 timeStamp)
        {
            m_recordedTime = timeStamp;
        }
        
        void LogEntry::RecordTime()
        {
            m_recordedTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        }

        void LogEntry::SetSeverity(Severity severity)
        {
            m_severity = severity;
        }

        void LogEntry::Clear()
        {
            m_fields.clear();
            m_recordedTime = 0;
            m_severity = Severity::Message;
        }

        bool LogEntry::IsDefault() const
        {
            return m_recordedTime == 0 && m_severity == Severity::Message && m_fields.empty();
        }

        bool LogEntry::IsCommonField(AZ::Crc32 fieldName)
        {
            return fieldName == s_messageField || fieldName == s_windowField;
        }

        bool LogEntry::ParseLog(const char* log, AZ::u64 logLength, AZStd::function<void(LogEntry&)> newEntryCallback)
        {
            if (!newEntryCallback)
            {
                return false;
            }

            LogEntry currentEntry;

            bool processingResult = true;
            bool parseResult = LogLine::ParseLog(log, logLength,
                [&](LogLine& logLine)
                {
                    if (logLine.GetLogType() == LogLine::TYPE_CONTEXT)
                    {
                        processingResult = processingResult &&
                            ParseContext(currentEntry.m_fields, logLine.GetLogMessage());
                    }
                    else
                    {
                        processingResult = processingResult &&
                            CopyLineInformation(currentEntry, logLine);
                        newEntryCallback(currentEntry);
                        currentEntry.Clear();
                    }
                }
            );

            if (!currentEntry.IsDefault())
            {
                newEntryCallback(currentEntry);
            }

            return parseResult && processingResult;
        }

        bool LogEntry::ParseLog(AZStd::vector<LogEntry>& entryList, const char* log, AZ::u64 logLength)
        {
            return ParseLog(log, logLength,
                [&](LogEntry& logEntry)
                {
                    entryList.push_back(logEntry);
                }
            );
        }

        bool LogEntry::ParseContext(FieldStorage& fields, const AZStd::string& line)
        {
            size_t openMarker = line.find_first_of('[');
            if (openMarker == AZStd::string::npos)
            {
                // Context doesn't contain opening marker '[' so ignore.
                return false;
            }

            size_t closeMarker = line.find_first_of(']', openMarker + 1);
            if (closeMarker == AZStd::string::npos)
            {
                // Context doesn't contain closing marker ']' so ignore.
                return false;
            }

            Field field;
            field.m_name = line.substr(openMarker + 1, closeMarker - openMarker - 1);
            // Offset of 4 to compensate for the "] = " characters.
            if (closeMarker + 4 < line.size())
            {
                field.m_value = line.substr(closeMarker + 4);
            }
            AZ::Crc32 key(field.m_name.c_str(), field.m_name.size(), true);
            fields[key] = AZStd::move(field);
            
            return true;
        }

        bool LogEntry::CopyLineInformation(LogEntry& target, const LogLine& source)
        {
            if (!source.GetLogMessage().empty())
            {
                Field field;
                field.m_name = s_messageFieldName;
                field.m_value = source.GetLogMessage();
                target.m_fields[s_messageField] = AZStd::move(field);
            }

            if (!source.GetLogWindow().empty())
            {
                Field field;
                field.m_name = s_windowFieldName;
                field.m_value = source.GetLogWindow();
                target.m_fields[s_windowField] = AZStd::move(field);
            }

            target.m_recordedTime = source.GetLogTime();
            switch (source.GetLogType())
            {
            case LogLine::TYPE_ERROR:
                target.m_severity = Severity::Error;
                break;
            case LogLine::TYPE_WARNING:
                target.m_severity = Severity::Warning;
                break;
            case LogLine::TYPE_MESSAGE:
                // fall through
            case LogLine::TYPE_DEBUG:
                target.m_severity = Severity::Message;
                break;
            default:
                AZ_Assert(false, "Unknown log type: %i.", source.GetLogType());
                return false;
            }

            return true;
        }
    } // Logging
} // AzToolsFramework
