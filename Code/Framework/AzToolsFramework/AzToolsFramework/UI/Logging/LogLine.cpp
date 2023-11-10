/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Logging/LogLine.h>

#include <AzCore/XML/rapidxml.h>
#include <AzCore/std/containers/list.h>
#include <AzFramework/Logging/LogFile.h>
#include <AzToolsFramework/UI/Logging/LogPanel_Panel.h>
#include <AzToolsFramework/UI/Logging/LoggingCommon.h>
#include <AzToolsFramework/UI/Logging/LogControl.h>

#include <QColor>
#include <QDateTime>
#include <QLocale>

namespace AzToolsFramework
{
    namespace Logging
    {
        static bool IsNumeral(char c)
        {
            return (c >= '0') && (c <= '0');
        }

        static AZStd::string RemoveRCFormatting(const AZStd::string& message)
        {
            // RC logs errors that start with an optional type qualifier, then a time stamp, then a message.
            // Most importantly, the first 4 characters are either a type qualifier, a colon and 2 spaces, or 4 spaces
            // followed by a time stamp which is 1 or 2 numerals, a colon, 2 numerals, and either nothing (meaning a zero length
            // message) or a space, followed by a message
            //
            // Examples:
            // 
            //E:  0:00 Message with multiple words
            //    1:00 Message with multiple words
            // 
            // This was initially done as a regex, but the format is so prescribed and fixed, and regex's so slow,
            // that I switched to this. It also avoids problems caused by using static regex objects, which pre-allocate
            // and then don't free at the right time. Also, it should be extremely fast this way.

            size_t index = 0;
            if (index >= message.size())
            {
                return message;
            }

            char nextValidCharacter = ':'; // next valid character can be a : or a space, depending on what comes first
            switch (message[index])
            {
                case 'E':
                case 'W':
                case 'C':
                break;

                case ' ':
                    nextValidCharacter = ' ';
                break;

                default:
                    return message;
                break;
            }

            index++;
            if (index >= message.size())
            {
                return message;
            }

            if (message[index] != nextValidCharacter)
            {
                return message;
            }

            index++;
            if (index >= message.size())
            {
                return message;
            }

            for (int i = 0; i < 2; i++)
            {
                if (message[index] != ' ')
                {
                    return message;
                }

                index++;
                if (index >= message.size())
                {
                    return message;
                }
            }

            if (!IsNumeral(message[index]))
            {
                return message;
            }

            // one digit; check if we have two for the hours
            index++;
            if (index >= message.size())
            {
                return message;
            }

            if (IsNumeral(message[index]))
            {
                // only one extra digit; skip to :
                index++;
                if (index >= message.size())
                {
                    return message;
                }
            }

            // have to have the hours : minutes separator by this point
            if (message[index] != ':')
            {
                return message;
            }
            
            index++;
            if (index >= message.size())
            {
                return message;
            }

            if (!IsNumeral(message[index]))
            {
                return message;
            }

            index++;
            if (index >= message.size())
            {
                return message;
            }
            

            if (!IsNumeral(message[index]))
            {
                return message;
            }

            // If we're at the end of the string, return the zero length string
            index++;
            if (index >= message.size())
            {
                return "";
            }

            // otherwise, check for space
            if (message[index] != ' ')
            {
                return message;
            }

            index++;

            // explicitly check for a zero length string after the last space; substr can throw an exception otherwise
            if (index >= message.size())
            {
                return "";
            }

            // otherwise, take the remaining string
            return message.substr(index);
        }

        static AZStd::string RemoveAssetBuilderFormatting(const AZStd::string& message)
        {
            if(message.size() < 2)
            {
                // A message shorter than 2 characters can't possibly have the characters we're looking to strip off
                return message;
            }

            // We're looking for prefixes of the format [WES]:<optional space>.  The space will be missing if the prefix is the only thing on the line
            // The prefixes exist so we can figure out the message severity/type, but they just add noise when displaying the log in the AP GUI
            switch(message[0])
            {
                case 'W': // warning
                case 'E': // error
                case 'S': // summary
                    break;
                default:
                    return message;
            }

            if (message[1] != ':')
            {
                return message;
            }

            if(message.size() == 2)
            {
                return "";
            }

            if(message[2] == ' ')
            {
                // Prefix matched, return the message without the prefix
                return message.substr(3);
            }

            return message;
        }

        LogLine::LogLine(const char* inMessage, const char* inWindow, LogType inType, AZ::u64 inTime, void* data, uintptr_t threadId)
            : m_message(inMessage)
            , m_window(inWindow)
            , m_type(inType)
            , m_messageTime(inTime)
            , m_processed(false)
            , m_userData(data)
            , m_threadId(threadId)
        {
            Process();
        }

        LogLine::LogLine(const LogLine& other)
        {
            *this = other;
        }

        LogLine::LogLine(LogLine&& other)
        {
            *this = AZStd::move(other);
        }

        LogLine& LogLine::operator=(LogLine&& other)
        {
            if (this != &other)
            {
                m_message = AZStd::move(other.m_message);
                m_window = AZStd::move(other.m_window);
                m_type = other.m_type;
                m_messageTime = other.m_messageTime;
                m_isRichText = other.m_isRichText;
                m_processed = other.m_processed;
                m_threadId = other.m_threadId;
                m_userData = other.m_userData;
            }
            return *this;
        }

        LogLine& LogLine::operator=(const LogLine& other)
        {
            if (this != &other)
            {
                m_message = other.m_message;
                m_window = other.m_window;
                m_type = other.m_type;
                m_messageTime = other.m_messageTime;
                m_isRichText = other.m_isRichText;
                m_processed = other.m_processed;
                m_threadId = other.m_threadId;
                m_userData = other.m_userData;
            }
            return *this;
        }

        void LogLine::Process()
        {
            if (m_processed)
            {
                return;
            }

            m_processed = true;
            // strip trailing newlines and whitespace:

            while ((!m_message.empty()) && (strchr("\n\r\t ", m_message[m_message.length() - 1])))
            {
                m_message.resize(m_message.length() - 1);
            }

            // detect richtext currently by presence of span at the beginning, or <a
            if (
                (azstrnicmp(m_message.c_str(), "<span ", 6) == 0) ||
                (azstrnicmp(m_message.c_str(), "<a ", 3) == 0)
                )
            {
                m_isRichText = true;
            }

            // the message also provides a hint, until we have a form of context harvesting:
            if ((strncmp(m_message.c_str(), "W:", 2) == 0) || (azstrnicmp(m_message.c_str(), "warn:", 5) == 0) || (azstrnicmp(m_message.c_str(), "warning:", 8) == 0))
            {
                m_type = TYPE_WARNING;
            }
            else if ((strncmp(m_message.c_str(), "E:", 2) == 0) || (azstrnicmp(m_message.c_str(), "err:", 4) == 0) || (azstrnicmp(m_message.c_str(), "error:", 6) == 0))
            {
                m_type = TYPE_ERROR;
            }
            else if ((strncmp(m_message.c_str(), "C:", 2) == 0) || (azstrnicmp(m_message.c_str(), "ctx:", 4) == 0) || (azstrnicmp(m_message.c_str(), "context:", 8) == 0))
            {
                m_type = TYPE_CONTEXT;
            }

            // note that we don't set rich text to false here.  The user is welcome to set that to true before calling add.
            // if we have not yet escalated and its rich text, then see if the rich text escalates it:
            // we don't bother checking if the type is ALREADY not just normal or debug
            if ((m_isRichText) && ((m_type == TYPE_MESSAGE) || (m_type == TYPE_DEBUG)))
            {
                // the rich text allows us to add attributes the surrounding span
                // one of those attributes allows us to change the apparent severity level.
                using namespace AZ::rapidxml;
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(m_message.c_str()));
                xml_node<>* cur_node = doc.first_node("span", 4, false);
                if (cur_node)
                {
                    xml_attribute<>* attrib = cur_node->first_attribute("severity", 8, false);
                    if (attrib)
                    {
                        AZStd::string attribValue(attrib->value(), attrib->value() + attrib->value_size());
                        if (azstrnicmp(attribValue.c_str(), "err", 3) == 0)
                        {
                            m_type = TYPE_ERROR;
                        }
                        else if (azstrnicmp(attribValue.c_str(), "warn", 4) == 0)
                        {
                            m_type = TYPE_WARNING;
                        }
                    }
                }
            }

            m_message = RemoveRCFormatting(m_message);
            m_message = RemoveAssetBuilderFormatting(m_message);
        }

        const AZStd::string& LogLine::GetLogMessage() const
        {
            return m_message;
        }

        const AZStd::string& LogLine::GetLogWindow() const
        {
            return m_window;
        }

        LogLine::LogType LogLine::GetLogType() const
        {
            return m_type;
        }

        AZ::u64 LogLine::GetLogTime() const
        {
            return m_messageTime;
        }

        uintptr_t LogLine::GetLogThreadId() const
        {
            return m_threadId;
        }

        void* LogLine::GetUserData() const
        {
            return m_userData;
        }
        void LogLine::SetUserData(void* data)
        {
            m_userData = data;
        }

        AZStd::string LogLine::ToString()
        {
            if (!m_processed)
            {
                Process();
            }

            using namespace AZ::Debug;

            const AZStd::string separator = " | ";
            const AZStd::string Origin = "origin: ";
            const AZStd::string EOL = "\n";

            AZStd::string dateTime = QLocale::system().toString(QDateTime::fromMSecsSinceEpoch(m_messageTime), QLocale::ShortFormat).toUtf8().data();
            AZStd::string severity;
            switch (m_type)
            {
            case LogLine::TYPE_DEBUG:
                severity = "DEBUG";
                break;
            case LogLine::TYPE_ERROR:
                severity = "ERROR";
                break;
            case LogLine::TYPE_WARNING:
                severity = "Warn";
                break;
            case LogLine::TYPE_MESSAGE:
                severity = "Info";
                break;
            case LogLine::TYPE_CONTEXT:
                severity = "Context";
                break;
            }
            return dateTime + separator + severity + separator + Origin + m_window.c_str() + separator + m_message.c_str() + EOL;
        }

        QVariant LogLine::data(int column, int role) const
        {
            using namespace AZ::Debug;
            if (role == LogPanel::ExtraRoles::LogLineRole)
            {
                return QVariant::fromValue<const LogLine*>(this);
            }
            if (role == LogPanel::ExtraRoles::RichTextRole) // the renderer is asking whether this cell is rich text or not.  Return a true or false.
            {
                return m_isRichText;
            }
            else if (role == Qt::DecorationRole) // the renderer is asking whether or not we want to display an icon in this cell.  Return an icon or null.
            {
                if (column == 0)
                {
                    switch (m_type)
                    {
                    case LogLine::TYPE_ERROR:
                        return LogPanel::BaseLogView::GetErrorIcon();
                    case LogLine::TYPE_WARNING:
                        return LogPanel::BaseLogView::GetWarningIcon();
                    case LogLine::TYPE_MESSAGE:
                        return LogPanel::BaseLogView::GetInformationIcon();
                    case LogLine::TYPE_DEBUG:
                        return LogPanel::BaseLogView::GetDebugIcon();
                    case LogLine::TYPE_CONTEXT:
                        return LogPanel::BaseLogView::GetInformationIcon();
                    }
                }
            }
            else if (role == Qt::DisplayRole) // the renderer wants to know what text to show in this cell.  Return a string or null
            {
                if (column == 0) // icon has no text
                {
                    return QVariant(QString());
                }
                if (column == 1)
                {
                    return QVariant(QLocale::system().toString(QDateTime::fromMSecsSinceEpoch(m_messageTime), QLocale::ShortFormat));
                }
                else if (column == 2) // window
                {
                    return QVariant(m_window.c_str());
                }
                else if (column == 3) // message
                {
                    return QVariant(m_message.c_str());
                }
            }
            else if (role == Qt::BackgroundRole) // the renderer wants to know what the background color of this cell should be.  REturn a color or null (to use default)
            {
                switch (m_type)
                {
                case LogLine::TYPE_ERROR:
                    return QVariant(QColor::fromRgb(64, 0, 0));
                    break;
                case LogLine::TYPE_WARNING:
                    return QVariant(QColor::fromRgb(64, 64, 0));
                    break;
                case LogLine::TYPE_CONTEXT:
                    return QVariant(QColor::fromRgb(32, 0, 32));
                    break;
                }
            }
            else if (role == Qt::ForegroundRole) // the renderer wants to know what the text color of this cell should be.  Return a color or empty QVariant()
            {
                switch (m_type)
                {
                case LogLine::TYPE_MESSAGE:
                    return QVariant(QColor::fromRgb(255, 255, 255));
                case LogLine::TYPE_ERROR:
                    return QVariant(QColor::fromRgb(255, 192, 192));
                case LogLine::TYPE_WARNING:
                    return QVariant(QColor::fromRgb(255, 255,192));
                case LogLine::TYPE_DEBUG:
                    return QVariant(QColor::fromRgb(128, 128, 128));
                case LogLine::TYPE_CONTEXT:
                    return QVariant(QColor::fromRgb(255, 192, 255));
                }
            }
            return QVariant();
        }
        
        bool LogLine::ParseLog(const char* entireLog, AZ::u64 logLength, AZStd::function<void(LogLine&)> logLineAdder)
        {
            // default log file parse assumes newline separated.

            if (logLength < 1)
            {
                return false;
            }

            AZStd::string parseBuffer;

            const char* start = entireLog;
            AZ::u64 pos = 0;
            AZ::u64 numChars = 0;
            while (pos < logLength)
            {
                char currentChar = entireLog[pos];
                if ((currentChar == '\n') || (currentChar == '\r') || (pos == (logLength - 1)))
                {
                    if (numChars > 0)
                    {
                        // we have found a newline or EOF.
                        parseBuffer.assign(start, numChars);

                        LogLine output;
                        if (ParseLine(output, parseBuffer))
                        {
                            logLineAdder(output);
                        }
                    }
                    numChars = 0;
                    ++pos;
                    start = entireLog + pos;
                }
                else
                {
                    ++pos;
                    ++numChars;
                }
            }

            return true;
        }

        bool LogLine::ParseLog(AZStd::list<LogLine>& logList,  const char* entireLog, AZ::u64 logLength)
        {
            return ParseLog(entireLog, logLength, 
                [&](LogLine& logLine)
                {
                    logList.push_back(logLine);
                }
            );
        }

        bool LogLine::ParseLine(LogLine& outLine, const AZStd::string& line)
        {
            // the standard log outputs (if machine readable format) to 
            // ~~MS_SINCE_EPOCH~~SEVERITY~~THREADID~~WINDOW~~MESSAGE
            // we can always add additional parsers here, or parse modes, etc.
            // we're also going to assume if it doesn't have tags like that, its a plain message log line
            // for speed, we'll manually walk through the line ourselves.
            // we also want to avoid actually allocating memory here on the heap.


            size_t lineSize = line.size();
            if (lineSize == 0)
            {
                // this can't be a valid line.
                return false;
            }

            // pass over it once to see if it has all the required fields:

            // TIME     SEV      THREAD   WINDOW   MESSAGE

            enum FieldIndexes
            {
                Idx_Time,
                Idx_Severity,
                Idx_Thread,
                Idx_Window,
                Idx_Message,
                NUM_FIELD_INDEXES,
            };

            size_t fieldBegins[NUM_FIELD_INDEXES];
            size_t fieldSizes[NUM_FIELD_INDEXES];
            int fieldIndex = 0;

            memset(fieldBegins, 0, sizeof(size_t) * NUM_FIELD_INDEXES);
            memset(fieldSizes, 0, sizeof(size_t) * NUM_FIELD_INDEXES);
            
            if (strncmp(line.c_str(), "~~", 2) == 0)
            {
                // its probably in the delimited format here:
                
                size_t fieldStartPos = 2; // start after the opening ~'s
                bool inDelimiter = false;
                for (size_t pos = 2; pos < lineSize - 1; ++pos)
                {
                    if (fieldIndex >= NUM_FIELD_INDEXES)
                    {
                        break; // found all the fields.
                    }
                    char currentChar = line[pos];

                    if (currentChar == '~')
                    {
                        // this could be a delimiter.
                        if (inDelimiter)
                        {
                            // this is the second delimiter char in a row, so its the end delimiter.
                            // current field is over! 
                            fieldSizes[fieldIndex] = (pos - fieldStartPos) - 1;
                            fieldBegins[fieldIndex] = fieldStartPos;
                            ++fieldIndex;
                            fieldStartPos = pos + 1;
                            inDelimiter = false;

                            // if we just moved on to the last field, which is the message, its just the rest of the string...
                            if (fieldIndex == Idx_Message)
                            {
                                fieldBegins[fieldIndex] = fieldStartPos;
                                fieldSizes[fieldIndex] = lineSize - fieldStartPos; // the rest of the line.
                                break;
                            }
                        }
                        else
                        {
                            inDelimiter = true;
                        }
                    }
                    else
                    {
                        inDelimiter = false;
                        // some other char.
                    }
                }
            }

            const int maxFieldSize = 80;

            // if everything went okay with parsing:
            if ((fieldBegins[Idx_Message] != 0) && (fieldSizes[Idx_Time] < maxFieldSize) && (fieldSizes[Idx_Severity] < maxFieldSize) && (fieldSizes[Idx_Thread] < maxFieldSize) && (fieldSizes[Idx_Window] < maxFieldSize))
            {
                char tempScratch[maxFieldSize];
                azstrncpy(tempScratch, AZ_ARRAY_SIZE(tempScratch), line.c_str() + fieldBegins[Idx_Time], fieldSizes[Idx_Time]);
                tempScratch[fieldSizes[Idx_Time]] = 0;

                bool convertedOK = false;
                AZ::u64 msSinceEpoch = QString(tempScratch).toULongLong(&convertedOK, 10);
                if (convertedOK)
                {
                    outLine.m_messageTime = msSinceEpoch;
                }
                else
                {
                    outLine.m_messageTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
                }
                
                azstrncpy(tempScratch, AZ_ARRAY_SIZE(tempScratch), line.c_str() + fieldBegins[Idx_Severity], fieldSizes[Idx_Severity]);
                tempScratch[fieldSizes[Idx_Severity]] = 0;
                int severityValue = QString(tempScratch).toInt(&convertedOK, 10);
                if (convertedOK)
                {
                    switch (severityValue)
                    {
                    case AzFramework::LogFile::SEV_DEBUG:
                        outLine.m_type = Logging::LogLine::TYPE_DEBUG;
                        break;
                    case AzFramework::LogFile::SEV_NORMAL:
                        outLine.m_type = Logging::LogLine::TYPE_MESSAGE;
                        break;
                    case AzFramework::LogFile::SEV_WARNING:
                        outLine.m_type = Logging::LogLine::TYPE_WARNING;
                        break;
                    default:
                        outLine.m_type = Logging::LogLine::TYPE_ERROR;
                        break;
                    }
                }

                azstrncpy(tempScratch, AZ_ARRAY_SIZE(tempScratch), line.c_str() + fieldBegins[Idx_Thread], fieldSizes[Idx_Thread]);
                tempScratch[fieldSizes[Idx_Thread]] = 0;
                convertedOK = false;
                uintptr_t threadId = static_cast<uintptr_t>(QString(tempScratch).toUInt(&convertedOK, 16));
                if (convertedOK)
                {
                    outLine.m_threadId = threadId;
                }

                outLine.m_window.assign(line.c_str() + fieldBegins[Idx_Window], fieldSizes[Idx_Window]);
                outLine.m_message.assign(line.c_str() + fieldBegins[Idx_Message], fieldSizes[Idx_Message]);
                outLine.Process();
                return true;
            }
            else
            {
                // assume some random unknown format log message line.
                outLine.m_messageTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
                outLine.m_message = line.c_str();
                outLine.m_window = "Message";
                outLine.Process();
                return true;
            }
        }

        bool LogLine::ParseContextLogLine(const LogLine& line, std::pair<QString, QString>& result)
        {
            // Kind of parsed content: C: [Source] = Value.....
            static QRegularExpression rx("C:\\s*\\[([^\\n\\r\\[\\]]+)\\]\\s*=\\s*(.*)");
            const QRegularExpressionMatch match = rx.match(QString::fromLatin1(line.GetLogMessage().c_str()));

            if (match.hasMatch())
            {
                result.first = match.captured(1).trimmed();
                result.second = match.captured(2).trimmed();
                return true;
            }

            return false;
        }


    }
}
