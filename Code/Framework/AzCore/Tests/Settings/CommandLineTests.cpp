/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Settings/CommandLine.h>

namespace UnitTest
{
    class CommandLineTests
        : public LeakDetectionFixture
    {
    };

    TEST_F(CommandLineTests, CommandLineParser_Sanity)
    {
        AZ::CommandLine cmd;
        EXPECT_FALSE(cmd.HasSwitch(""));
        EXPECT_EQ(cmd.GetNumSwitchValues("haha"), 0);
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_EQ(cmd.GetSwitchValue("haha", 0), AZStd::string());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_EQ(cmd.GetNumMiscValues(), 0);
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_EQ(cmd.GetMiscValue(1), AZStd::string());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(CommandLineTests, CommandLineParser_Switches_Simple)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", "--switch1", "test", "--switch2", "test2", "--switch3", "tEST3"
        };

        cmd.Parse(7, const_cast<char**>(argValues));

        EXPECT_FALSE(cmd.HasSwitch("switch4"));
        EXPECT_TRUE(cmd.HasSwitch("switch3"));
        EXPECT_TRUE(cmd.HasSwitch("sWITCH2")); // expect case insensitive
        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetNumSwitchValues("switch1"), 1);
        EXPECT_EQ(cmd.GetNumSwitchValues("switch2"), 1);
        EXPECT_EQ(cmd.GetNumSwitchValues("switch3"), 1);

        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "test");
        EXPECT_EQ(cmd.GetSwitchValue("switch2", 0), "test2");
        EXPECT_EQ(cmd.GetSwitchValue("switch3", 0), "tEST3"); // retain case in values.

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 1), AZStd::string());
        EXPECT_EQ(cmd.GetSwitchValue("switch2", 1), AZStd::string());
        EXPECT_EQ(cmd.GetSwitchValue("switch3", 1), AZStd::string());
        AZ_TEST_STOP_TRACE_SUPPRESSION(3);
    }

    TEST_F(CommandLineTests, CommandLineParser_MiscValues_Simple)
    {
        AZ::CommandLine cmd{ AZ::Settings::CommandLineOptionPrefixArray{ "-" } };

        const char* argValues[] = {
            "programname.exe", "-switch1", "test", "miscvalue1", "miscvalue2"
        };

        cmd.Parse(5, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetNumSwitchValues("switch1"), 1);
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "test");
        EXPECT_EQ(cmd.GetNumMiscValues(), 2);
        EXPECT_EQ(cmd.GetMiscValue(0), "miscvalue1");
        EXPECT_EQ(cmd.GetMiscValue(1), "miscvalue2");
    }

    TEST_F(CommandLineTests, CommandLineParser_Complex)
    {
        AZ::CommandLine cmd{ AZ::Settings::CommandLineOptionPrefixArray{ "--", "-", "/" } };

        const char* argValues[] = {
            "programname.exe", "-switch1", "test", "--switch1", "test2", "/switch2", "otherswitch", "miscvalue", "/switch3=abc,def", "miscvalue2", "/switch3", "hij,klm"
        };

        cmd.Parse(12, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_TRUE(cmd.HasSwitch("switch2"));
        EXPECT_EQ(cmd.GetNumMiscValues(), 2);
        EXPECT_EQ(cmd.GetMiscValue(0), "miscvalue");
        EXPECT_EQ(cmd.GetMiscValue(1), "miscvalue2");
        EXPECT_EQ(cmd.GetNumSwitchValues("switch1"), 2);
        EXPECT_EQ(cmd.GetNumSwitchValues("switch2"), 1);
        EXPECT_EQ(cmd.GetNumSwitchValues("switch3"), 4);
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "test");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 1), "test2");
        EXPECT_EQ(cmd.GetSwitchValue("switch2", 0), "otherswitch");
        EXPECT_EQ(cmd.GetSwitchValue("switch3", 0), "abc");
        EXPECT_EQ(cmd.GetSwitchValue("switch3", 1), "def");
        EXPECT_EQ(cmd.GetSwitchValue("switch3", 2), "hij");
        EXPECT_EQ(cmd.GetSwitchValue("switch3", 3), "klm");
    }

    TEST_F(CommandLineTests, CommandLineParser_WhitespaceTolerant)
    {
        AZ::CommandLine cmd{ AZ::Settings::CommandLineOptionPrefixArray{ "--", "-", "/" } };

        const char* argValues[] = {
            "programname.exe", "/switch1 ", "test ", " /switch1", " test2", " --switch1", " abc, def ", " /switch1 = abc, def " 
        };

        cmd.Parse(8, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "test");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 1), "test2");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 2), "abc");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 3), "def");

        // note:  Every switch must appear in the order it is given, even duplicates.
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 4), "abc");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 5), "def");
    }

    TEST_F(CommandLineTests, CommandLineParser_CustomCommandOption_IsUsedInsteadOfDefault)
    {
        AZ::CommandLine cmd{ AZ::Settings::CommandLineOptionPrefixArray{ "+" } };

        const char* argValues[] =
        {
            "programname.exe", "-fakeswitch1", "test", "--fakeswitch2", "test2", "+realswitch1", "othervalue", "miscvalue",
            "/fakeswitch3=abc,def", "miscvalue2", "+-realswitch2", "More,Real"
        };

        cmd.Parse(aznumeric_cast<int>(AZStd::size(argValues)), const_cast<char**>(AZStd::data(argValues)));

        EXPECT_FALSE(cmd.HasSwitch("fakeswitch1"));
        EXPECT_FALSE(cmd.HasSwitch("fakeswitch2"));
        EXPECT_FALSE(cmd.HasSwitch("fakeswitch3"));
        EXPECT_TRUE(cmd.HasSwitch("realswitch1"));
        EXPECT_TRUE(cmd.HasSwitch("-realswitch2"));
        EXPECT_EQ(7, cmd.GetNumMiscValues());
        EXPECT_EQ("-fakeswitch1", cmd.GetMiscValue(0));
        EXPECT_EQ("test", cmd.GetMiscValue(1));
        EXPECT_EQ("--fakeswitch2", cmd.GetMiscValue(2));
        EXPECT_EQ("test2", cmd.GetMiscValue(3));
        EXPECT_EQ("miscvalue", cmd.GetMiscValue(4));
        EXPECT_EQ("/fakeswitch3=abc,def", cmd.GetMiscValue(5));
        EXPECT_EQ("miscvalue2", cmd.GetMiscValue(6));
        EXPECT_EQ(1, cmd.GetNumSwitchValues("realswitch1"));
        EXPECT_EQ(2, cmd.GetNumSwitchValues("-realswitch2"));
        EXPECT_EQ("othervalue", cmd.GetSwitchValue("realswitch1", 0));
        EXPECT_EQ("More", cmd.GetSwitchValue("-realswitch2", 0));
        EXPECT_EQ("Real", cmd.GetSwitchValue("-realswitch2", 1));

    }

    TEST_F(CommandLineTests, CommandLineParser_QuoteBoundNoEqualWithComma_Success)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1 " ,"\"acb,def\""
        };

        cmd.Parse(3, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "acb,def");

    }

    TEST_F(CommandLineTests, CommandLineParser_QuoteBoundEqualWithCommaNoSpace_Success)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1=\"abc,fde\""
        };

        cmd.Parse(2, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "abc,fde");
    }

    TEST_F(CommandLineTests, CommandLineParser_QuoteBoundEqualWithCommaSpace_Success)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1=\"abc, def\""
        };

        cmd.Parse(2, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "abc, def");
    }

    TEST_F(CommandLineTests, CommandLineParser_SingleQuoteEqualWithCommaSpace_Tokenized)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1=\"abc, def"
        };

        cmd.Parse(2, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "\"abc");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 1), "def");

    }

    TEST_F(CommandLineTests, CommandLineParser_SingleQuoteEqualWithCommaNoSpace_Tokenized)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1=\"abc,def"
        };

        cmd.Parse(2, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "\"abc");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 1), "def");

    }


    TEST_F(CommandLineTests, CommandLineParser_SingleQuoteNoEqualWithCommaNoSpace_Tokenized)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1", " \"abc,def" 
        };

        cmd.Parse(3, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "\"abc");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 1), "def");

    }


    TEST_F(CommandLineTests, CommandLineParser_SingleQuoteNoEqualWithSemicolonNoSpace_Tokenized)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1", " \"abc;def"
        };

        cmd.Parse(3, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "\"abc");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 1), "def");
    }


    TEST_F(CommandLineTests, CommandLineParser_SingleQuoteNoEqualWithCommaSpace_Tokenized)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1", "\"abc, def"
        };

        cmd.Parse(3, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "\"abc");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 1), "def");

    }

    TEST_F(CommandLineTests, CommandLineParser_DoubleQuoteNoEqual_Blank)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", "-switch1", "\"\""
        };

        cmd.Parse(3, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "");

    }

    TEST_F(CommandLineTests, CommandLineParser_DoubleQuote_Blank)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1=\"\"", " -switch2", "\"\""
        };

        cmd.Parse(4, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "");

        EXPECT_TRUE(cmd.HasSwitch("switch2"));
        EXPECT_EQ(cmd.GetSwitchValue("switch2", 0), "");
    }

    TEST_F(CommandLineTests, CommandLineParser_DoubleQuoteEqualComma_Comma)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1=\",\""
        };

        cmd.Parse(2, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), ",");

    }

    TEST_F(CommandLineTests, CommandLineParser_SingleDoubleQuote_Success)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1=\"", " -switch2", "\""
        };

        cmd.Parse(4, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "\"");

        EXPECT_TRUE(cmd.HasSwitch("switch2"));
        EXPECT_EQ(cmd.GetSwitchValue("switch2", 0), "\"");
    }

    // Verify things can "work the previous way" - what if my desired parameter starts and ends with quotes?
    // -- You can simply start or end with the token and things will work
    TEST_F(CommandLineTests, CommandLineParser_QuoteCommaStartOrEndWithCommaNoSpace_Tokenized)
    {
        AZ::CommandLine cmd;

        const char* argValues[] = {
            "programname.exe", " -switch1=\"abc,fde\",", "-switch2", " ,\"cba, edf\""
        };

        cmd.Parse(4, const_cast<char**>(argValues));

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "\"abc");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 1), "fde\"");

        EXPECT_TRUE(cmd.HasSwitch("switch1"));
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 0), "\"abc");
        EXPECT_EQ(cmd.GetSwitchValue("switch1", 1), "fde\"");

        EXPECT_TRUE(cmd.HasSwitch("switch2"));
        EXPECT_EQ(cmd.GetSwitchValue("switch2", 0), "\"cba");
        EXPECT_EQ(cmd.GetSwitchValue("switch2", 1), "edf\"");
    }

    TEST_F(CommandLineTests, CommandLineParser_DumpingCommandLineAndParsingAgain_ResultsInEquivalentCommandLine)
    {
        AZ::CommandLine origCommandLine{ AZ::Settings::CommandLineOptionPrefixArray{ "-", "/" } };

        const char* argValues[] =
        {
            "programname.exe", "/gamefolder", "/RemoteIp", "10.0.0.1", "-ScanFolders", R"(\a\b\c,\d\e\f)", "Foo", "Bat"
        };

        origCommandLine.Parse(aznumeric_cast<int>(AZStd::size(argValues)), const_cast<char**>(AZStd::data(argValues)));
        AZ::CommandLine::ParamContainer dumpedCommandLine;
        origCommandLine.Dump(dumpedCommandLine);

        AZ::CommandLine newCommandLine{ AZ::Settings::CommandLineOptionPrefixArray{ "-", "/" } };
        newCommandLine.Parse(dumpedCommandLine);

        AZStd::initializer_list<AZ::CommandLine> commandLines{ origCommandLine, newCommandLine };
        for (const auto& commandLine : commandLines)
        {
            EXPECT_TRUE(commandLine.HasSwitch("gamefolder"));
            EXPECT_EQ(1, commandLine.GetNumSwitchValues("gamefolder"));
            EXPECT_STREQ("", commandLine.GetSwitchValue("gamefolder", 0).c_str());

            EXPECT_TRUE(commandLine.HasSwitch("remoteip"));
            EXPECT_EQ(1, commandLine.GetNumSwitchValues("remoteip"));
            EXPECT_STREQ("10.0.0.1", commandLine.GetSwitchValue("remoteip", 0).c_str());

            EXPECT_TRUE(commandLine.HasSwitch("scanfolders"));
            EXPECT_EQ(2, commandLine.GetNumSwitchValues("scanfolders"));
            EXPECT_STREQ(R"(\a\b\c)", commandLine.GetSwitchValue("scanfolders", 0).c_str());
            EXPECT_STREQ(R"(\d\e\f)", commandLine.GetSwitchValue("scanfolders", 1).c_str());

            EXPECT_EQ(2, commandLine.GetNumMiscValues());
            EXPECT_STREQ("Foo", commandLine.GetMiscValue(0).c_str());
            EXPECT_STREQ("Bat", commandLine.GetMiscValue(1).c_str());
        }
    }

    TEST_F(CommandLineTests, CommandLineParser_GetSwitchValue_WithNoArgument_ReturnsLastValue)
    {
        AZ::CommandLine commandLine{ AZ::Settings::CommandLineOptionPrefixArray{ "--", "-" } };

        constexpr AZStd::string_view argValues[] =
        {
            "programname.exe", "--foo=1", "--foo", "2"
        };

        commandLine.Parse(argValues);

        ASSERT_GE(commandLine.GetNumSwitchValues("foo"), 2);
        EXPECT_STREQ("2", commandLine.GetSwitchValue("foo").c_str());
        EXPECT_STREQ("2", commandLine.GetSwitchValue("foo", 1).c_str());
        EXPECT_STREQ("1", commandLine.GetSwitchValue("foo", 0).c_str());
    }

    TEST_F(CommandLineTests, ArgumentsParsed_AfterDoubleDash_ArePositionalArgumentsOnly)
    {
        AZ::CommandLine commandLine{ AZ::Settings::CommandLineOptionPrefixArray{ "--", "-" } };

        constexpr AZStd::string_view argValues[] =
        {
            "programname.exe", "--foo=1", "--", "--foo=2", "bar", "--", "baz"
        };

        commandLine.Parse(argValues);

        EXPECT_EQ(commandLine.GetNumSwitchValues("foo"), 1);
        EXPECT_EQ("1", commandLine.GetSwitchValue("foo"));
        ASSERT_EQ(5, commandLine.GetNumMiscValues());
        // The first Misc entry is the executable name "programname.exe"
        // ignore checking that entry since it is not relevant to this test
        EXPECT_EQ("--foo=2",commandLine.GetMiscValue(1));
        EXPECT_EQ("bar", commandLine.GetMiscValue(2));
        EXPECT_EQ("--", commandLine.GetMiscValue(3));
        EXPECT_EQ("baz", commandLine.GetMiscValue(4));
    }

    class CommandLineParserTests
        : public LeakDetectionFixture
    {
    };
        TEST_F(CommandLineParserTests, CanParseAllTokensAfterOptionAsValue)
        {
            AZ::Settings::CommandLineParserSettings parserSettings;

            struct Argument
            {
                AZStd::string m_option;
                AZStd::string m_value;
            };
            AZStd::vector<Argument> parsedArguments;
            parserSettings.m_parseCommandLineEntryFunc = [&parsedArguments](const AZ::Settings::CommandLineArgument& argument)
            {
                parsedArguments.push_back(Argument{ argument.m_option, argument.m_value });
                return true;
            };

            constexpr AZStd::string_view argValues[] =
            {
                "programname.exe", "--foo=1", "--", "--foo=2", "bar", "--", "baz"
            };

            auto parseOutcome = AZ::Settings::ParseCommandLine(argValues, parserSettings);
            EXPECT_TRUE(parseOutcome);

            ASSERT_EQ(6, parsedArguments.size());

            EXPECT_TRUE(parsedArguments[0].m_option.empty());
            EXPECT_EQ("programname.exe", parsedArguments[0].m_value);
            EXPECT_EQ("foo", parsedArguments[1].m_option);
            EXPECT_EQ("1", parsedArguments[1].m_value);
            EXPECT_TRUE(parsedArguments[2].m_option.empty());
            EXPECT_EQ("--foo=2", parsedArguments[2].m_value);
            EXPECT_TRUE(parsedArguments[3].m_option.empty());
            EXPECT_EQ("bar", parsedArguments[3].m_value);
            EXPECT_TRUE(parsedArguments[4].m_option.empty());
            EXPECT_EQ("--", parsedArguments[4].m_value);
            EXPECT_TRUE(parsedArguments[5].m_option.empty());
            EXPECT_EQ("baz", parsedArguments[5].m_value);
        }

}   // namespace UnitTest
