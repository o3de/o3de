/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzToolsFramework/UI/Logging/LogLine.h>

using namespace AZ;
using namespace AzToolsFramework;
using namespace AzToolsFramework::Logging;

namespace UnitTest
{
    static const char* s_logPrefix = "~~1541632104059~~1~~8240~~RC Builder~~";
    static const char* s_logWindow = "RC Builder";

    TEST(LogLines, BasicTest)
    {
        const char* messages[] = {
            R"X(Executing RC.EXE: '"E:\lyengine\dev\windows\bin\profile\rc.exe" "E:/Directory/File.tga")X",
            R"X(Executing RC.EXE with working directory : '')X",
            R"X(ResourceCompiler  64 - bit  DEBUG)X",
            R"X(Platform support : PC, PowerVR)X",
            R"X(Version 1.1.8.6  Nov  5 2018 13 : 28 : 28)X"
        };

        AZStd::string textBuffer;
        for (const char* message : messages)
        {
            if (textBuffer.size() > 0)
            {
                textBuffer.append("\n");
            }

            textBuffer.append(s_logPrefix);
            textBuffer.append(message);
        }

        AZStd::list<LogLine> lines;
        LogLine::ParseLog(lines, textBuffer.c_str(), textBuffer.size() + 1);

        EXPECT_EQ(lines.size(), sizeof(messages) / sizeof(messages[0]));

        size_t index = 0;
        for (LogLine& line : lines)
        {
            EXPECT_STREQ(s_logWindow, line.GetLogWindow().c_str());
            EXPECT_STREQ(line.GetLogMessage().c_str(), messages[index]);
            EXPECT_EQ(line.GetLogType(), LogLine::TYPE_MESSAGE);
            index++;
        }
    }

    TEST(LogLines, Junk)
    {
        const char* messages[] = {
            R"X(small string)X",
            R"X(tiny)X",
            R"X(unformatted string)X",
        };

        AZStd::string textBuffer;
        for (const char* message : messages)
        {
            if (textBuffer.size() > 0)
            {
                textBuffer.append("\n");
            }

            textBuffer.append(s_logPrefix);
            textBuffer.append(message);
        }

        AZStd::list<LogLine> lines;
        LogLine::ParseLog(lines, textBuffer.c_str(), textBuffer.size() + 1);

        EXPECT_EQ(lines.size(), sizeof(messages) / sizeof(messages[0]));

        size_t index = 0;
        for (LogLine& line : lines)
        {
            EXPECT_STREQ(s_logWindow, line.GetLogWindow().c_str());
            EXPECT_STREQ(line.GetLogMessage().c_str(), messages[index]);
            EXPECT_EQ(line.GetLogType(), LogLine::TYPE_MESSAGE);
            index++;
        }
    }

    TEST(LogLines, RCParsingWithoutType)
    {
        const char* message = "Memory: working set 15.6Mb (peak 15.6Mb), pagefile 35.9Mb (peak 35.9Mb)";
        const char* timeStampWithProperRCSpacing = "    0:00 "; // <-- exact number of spaces specified by RC
        const char* timeStampWithWrongRCSpacing =  " 0:00 "; // <-- a different number of spaces

        AZStd::string textBuffer;

        textBuffer.append(AZStd::string::format("%s%s%s", s_logPrefix, timeStampWithProperRCSpacing, message));
        textBuffer.append("\n");

        AZStd::string messageWithTimeStampNotParsed = AZStd::string::format("%s%s", timeStampWithWrongRCSpacing, message);
        textBuffer.append(AZStd::string::format("%s%s", s_logPrefix, messageWithTimeStampNotParsed.c_str()));

        AZStd::list<LogLine> lines;
        LogLine::ParseLog(lines, textBuffer.c_str(), textBuffer.size() + 1);

        EXPECT_EQ(lines.size(), 2);

        LogLine& lineWithRCFormatting = lines.front();
        EXPECT_STREQ(s_logWindow, lineWithRCFormatting.GetLogWindow().c_str());
        EXPECT_STREQ(lineWithRCFormatting.GetLogMessage().c_str(), message);
        EXPECT_EQ(lineWithRCFormatting.GetLogType(), LogLine::TYPE_MESSAGE);

        LogLine& lineWithoutRCFormatting = lines.back();
        EXPECT_STREQ(s_logWindow, lineWithoutRCFormatting.GetLogWindow().c_str());
        EXPECT_STREQ(lineWithoutRCFormatting.GetLogMessage().c_str(), messageWithTimeStampNotParsed.c_str());
        EXPECT_EQ(lineWithoutRCFormatting.GetLogType(), LogLine::TYPE_MESSAGE);
    }

    TEST(LogLines, RCParsingToEmptyLine)
    {
        const char* timeStampWithProperRCSpacing = "    0:00"; // <-- exact number of spaces specified by RC, but no space on the end

        AZStd::string textBuffer;

        textBuffer.append(AZStd::string::format("%s%s", s_logPrefix, timeStampWithProperRCSpacing));

        AZStd::list<LogLine> lines;
        LogLine::ParseLog(lines, textBuffer.c_str(), textBuffer.size() + 1);

        EXPECT_EQ(lines.size(), 1);

        LogLine& lineWithRCFormatting = lines.front();
        EXPECT_STREQ(s_logWindow, lineWithRCFormatting.GetLogWindow().c_str());
        EXPECT_STREQ(lineWithRCFormatting.GetLogMessage().c_str(), "");
        EXPECT_EQ(lineWithRCFormatting.GetLogType(), LogLine::TYPE_MESSAGE);
    }

    TEST(LogLines, RCParsingWithType)
    {
        const char* rcPrefix = "E:  0:00 ";
        const char* message = R"X(CImageCompiler::ProcessImplementation: LoadInput(file:'E:\Directory\File.tga', ext:'tga') failed)X";

        AZStd::string textBuffer;

        textBuffer.append(AZStd::string::format("%s%s%s", s_logPrefix, rcPrefix, message));

        AZStd::list<LogLine> lines;
        LogLine::ParseLog(lines, textBuffer.c_str(), textBuffer.size() + 1);

        EXPECT_EQ(lines.size(), 1);

        LogLine& line = lines.front();
        EXPECT_STREQ(s_logWindow, line.GetLogWindow().c_str());
        EXPECT_STREQ(line.GetLogMessage().c_str(), message);
        EXPECT_EQ(line.GetLogType(), LogLine::TYPE_ERROR);
    }

    static AZStd::string CreateContextLine(const char* context, const char* data)
    {
        return AZStd::string::format("C: [%s] = %s", context, data);
    }

    TEST(LogLines, ContextParsing)
    {
        std::pair<const char*, const char*> contextInfos[] = {
            std::make_pair("Source", "scriptcanvas / AntiAlias.scriptcanvas"),
            std::make_pair("Platforms", "pc")
        };
        const char* messages[] = {
            R"X(C: [Source] = scriptcanvas / AntiAlias.scriptcanvas)X",
            R"X(C: [Platforms] = pc)X"
        };

        AZStd::string textBuffer;
        for (auto& contextInfo : contextInfos)
        {
            if (textBuffer.size() > 0)
            {
                textBuffer.append("\n");
            }

            textBuffer.append(s_logPrefix);
            textBuffer.append(CreateContextLine(contextInfo.first, contextInfo.second));
        }

        AZStd::list<LogLine> lines;
        LogLine::ParseLog(lines, textBuffer.c_str(), textBuffer.size() + 1);

        EXPECT_EQ(lines.size(), sizeof(messages) / sizeof(messages[0]));

        size_t index = 0;
        for (LogLine& line : lines)
        {
            EXPECT_EQ(AZStd::string(s_logWindow), line.GetLogWindow());

            AZStd::string message = CreateContextLine(contextInfos[index].first, contextInfos[index].second);
            EXPECT_STREQ(line.GetLogMessage().c_str(), message.c_str());
            EXPECT_EQ(line.GetLogType(), LogLine::TYPE_CONTEXT);

            std::pair<QString, QString> result;
            bool contextLineParsed = LogLine::ParseContextLogLine(line, result);
            EXPECT_TRUE(contextLineParsed);

            const char* expectedContext = contextInfos[index].first;
            const char* expectedData = contextInfos[index].second;

            QByteArray parsedContext = result.first.toUtf8();
            QByteArray parsedData = result.second.toUtf8();

            EXPECT_STREQ(expectedContext, parsedContext.data());
            EXPECT_STREQ(expectedData, parsedData.data());

            index++;
        }
    }
}
