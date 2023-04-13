/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Settings/TextParser.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/fixed_unordered_map.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    struct TextFileParams
    {
        AZStd::string_view m_testTextFileName;
        AZStd::string_view m_testTextContents;
        // The following test below will not have more than 20
        AZStd::fixed_vector<AZStd::string_view, 20> m_expectedLines;
    };

    class TextParserTestFixture
        : public LeakDetectionFixture
    {
    protected:
        AZStd::string m_textBuffer;
        AZ::IO::ByteContainerStream<AZStd::string> m_textStream{ &m_textBuffer };
    };

    // Parameterized test fixture for the TextFileParams
    class TextParserParamFixture
        : public TextParserTestFixture
        , public ::testing::WithParamInterface<TextFileParams>
    {
        void SetUp() override
        {
            auto textFileParam = GetParam();

            // Create the test text stream
            m_textStream.Write(textFileParam.m_testTextContents.size(),
                textFileParam.m_testTextContents.data());
            // Seek back to the beginning of the stream for test to read written data
            m_textStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }
    };

    TEST_F(TextParserTestFixture, ParseTextFile_WithEmptyFunction_Fails)
    {
        AZ::Settings::TextParserSettings parserSettings{ AZ::Settings::TextParserSettings::ParseTextEntryFunc{} };

        m_textBuffer = R"(project_path=/TestProject
            engine_path=/TestEngine
        )";
        EXPECT_FALSE(AZ::Settings::ParseTextFile(m_textStream, parserSettings));
    }

    TEST_F(TextParserTestFixture, ParseTextFile_WithParseTextEntryFunctionWhichAlwaysReturnsFalse_Fails)
    {
        auto parseTextEntry = [](AZStd::string_view)
        {
            return false;
        };

        AZ::Settings::TextParserSettings parserSettings{ parseTextEntry };

        m_textBuffer = R"(project_path=/TestProject
            engine_path=/TestEngine
        )";
        EXPECT_FALSE(AZ::Settings::ParseTextFile(m_textStream, parserSettings));

    }

    TEST_F(TextParserTestFixture, ParseTextFile_WithLineLargerThan4096_Fails)
    {
        auto parseTextEntry = [](AZStd::string_view)
        {
            return true;
        };

        AZ::Settings::TextParserSettings parserSettings{ parseTextEntry };

        m_textBuffer = "project_path=/TestProject\n";

        // append a line longer than 4096 characters
        m_textBuffer += "foo=";
        m_textBuffer.append(4096, 'a');
        m_textBuffer += '\n';

        EXPECT_FALSE(AZ::Settings::ParseTextFile(m_textStream, parserSettings));
    }

    TEST_F(TextParserTestFixture, ParseTextFile_WithAllLinesSmallerThan4097_Succeeds)
    {
        auto parseTextEntry = [](AZStd::string_view)
        {
            return true;
        };

        AZ::Settings::TextParserSettings parserSettings{ parseTextEntry };

        m_textBuffer = "project_path=/TestProject\n";

        // append only 4000 characters
        m_textBuffer += "foo=";
        m_textBuffer.append(4000, 'a');
        m_textBuffer += '\n';

        EXPECT_TRUE(AZ::Settings::ParseTextFile(m_textStream, parserSettings));
    }

    TEST_P(TextParserParamFixture, ParseTextFile_ParseContents_Successfully)
    {
        auto textFileParam = GetParam();

        // Parse Text File and write output to a vector for testing
        using ParseSettingsVector = AZStd::fixed_vector<AZStd::string_view, 20>;
        ParseSettingsVector parseSettingsVector;
        auto parseTextEntry = [&parseSettingsVector](AZStd::string_view textEntry)
        {
            parseSettingsVector.emplace_back(textEntry);
            return true;
        };

        AZ::Settings::TextParserSettings parserSettings{ parseTextEntry };


        auto parseOutcome = AZ::Settings::ParseTextFile(m_textStream, parserSettings);
        EXPECT_TRUE(parseOutcome);

        // Validate that parse lines matches the expected lines
        EXPECT_THAT(parseSettingsVector, ::testing::ContainerEq(textFileParam.m_expectedLines));
    }

INSTANTIATE_TEST_CASE_P(
    ReadTextFile,
    TextParserParamFixture,
    ::testing::Values(
        // Processes a fake text file
        // and properly terminates the file with a newline
        TextFileParams{ "fake_uuid.text", R"(
engine.json
project.json
levels/defaultlevel.prefab

)"
        , AZStd::fixed_vector<AZStd::string_view, 20>{
            AZStd::string_view{ "engine.json" },
            AZStd::string_view{ "project.json" },
            AZStd::string_view{ "levels/defaultlevel.prefab" }
        }},
        // Parses a fake text file
        // and does not end with a newline
        TextFileParams{ "fake_names.text", R"(
shader
material
document property editor)"
        , AZStd::fixed_vector<AZStd::string_view, 20>{
            AZStd::string_view{ "shader" },
            AZStd::string_view{ "material" },
            AZStd::string_view{ "document property editor" }
        }}
        )
    );


}
