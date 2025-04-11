/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LOGENTRY_H
#define LOGENTRY_H

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>

namespace AzToolsFramework
{
    namespace Logging
    {
        class LogLine;
        class LogEntry
        {
        public:
            enum class Severity
            {
                Error,
                Warning,
                Message
            };

            struct Field
            {
                Field() = default;
                Field(const Field& rhs) = default;
                Field(Field&& rhs);

                Field& operator=(const Field& rhs) = default;
                Field& operator=(Field&& rhs);

                AZStd::string m_name;
                AZStd::string m_value;
            };
            
            AZ_CLASS_ALLOCATOR(LogEntry, AZ::SystemAllocator);

            using FieldStorage = AZStd::unordered_map<AZ::Crc32, Field>;

            const FieldStorage& GetFields() const;
            AZ::u64 GetRecordedTime() const;
            Severity GetSeverity() const;

            // Adds a new entry to the field table. If the given fieldName is in use, it will be overwritten.
            void AddField(const Field& field);
            // Adds a new entry to the field table. If the given fieldName is in use, it will be overwritten.
            void AddField(Field&& field);
            void SetRecordedTime(AZ::u64 timeStamp);
            // Sets the recorded time to the current time.
            void RecordTime();
            void SetSeverity(Severity severity);
            
            void Clear();
            bool IsDefault() const;

            static bool IsCommonField(AZ::Crc32 fieldName);

            static bool ParseLog(const char* log, AZ::u64 logLength, AZStd::function<void(LogEntry&)> newEntryCallback);
            static bool ParseLog(AZStd::vector<LogEntry>& entryList, const char* log, AZ::u64 logLength);

            // Common fields
            static AZ::Crc32 s_messageField;
            static const char* s_messageFieldName;
            static AZ::Crc32 s_windowField;
            static const char* s_windowFieldName;

        private:
            static bool ParseContext(FieldStorage& fields, const AZStd::string& line);
            static bool CopyLineInformation(LogEntry& target, const LogLine& source);

            FieldStorage m_fields;
            AZ::u64 m_recordedTime = 0; // MSecs since epoch (Qt style)
            Severity m_severity = Severity::Message;
        };
    } // Logging
} // AzToolsFramework

#endif
