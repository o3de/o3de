/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <string>
#include <vector>

#include <AzCore/UnitTest/TestTypes.h>

#include <MCore/Source/CommandLine.h>

namespace EMotionFX
{
    using CommandLineFixtureParameter = std::pair<
        std::string,
        std::vector<std::pair<std::string, std::string>>
    >;

    class CommandLineFixture
        : public UnitTest::LeakDetectionFixture
        , public ::testing::WithParamInterface<CommandLineFixtureParameter>
    {
    };

    TEST_P(CommandLineFixture, TestCommandLine)
    {
        const CommandLineFixtureParameter& expected = GetParam();
        MCore::CommandLine commandLine(expected.first.c_str());

        std::vector<std::pair<std::string, std::string>> gotParameters;
        for (uint32 i = 0; i < commandLine.GetNumParameters(); ++i)
        {
            const AZStd::string& gotName = commandLine.GetParameterName(i);
            const AZStd::string& gotValue = commandLine.CheckIfHasValue(gotName.c_str()) ? commandLine.GetParameterValue(i) : "";
            gotParameters.emplace_back(
                std::make_pair(
                    std::string(gotName.c_str(), gotName.size()),
                    std::string(gotValue.c_str(), gotValue.size())
                )
            );
        }

        EXPECT_THAT(gotParameters, ::testing::Pointwise(::testing::Eq(), expected.second));
    }

    static const std::vector<CommandLineFixtureParameter> commandLineTestData
    {
        {
            {R"str(-xres 800 -yres 1024 -threshold 0.145 -culling false -fullscreen)str"},
            {
                {"xres", "800"},
                {"yres", "1024"},
                {"threshold", "0.145"},
                {"culling", "false"},
            }
        },
        {
            {R"str(-fullscreen -xres 800 -yres 1024 -threshold 0.145 -culling false)str"},
            {
                {"fullscreen", "-xres 800"},
                {"yres", "1024"},
                {"threshold", "0.145"},
                {"culling", "false"},
            }
        },
        {
            {R"str(-motionSetID 0 -idString <undefined> -newIDString %" -updateMotionNodeStringIDs true)str"},
            {
                {"motionSetID", "0"},
                {"idString", "<undefined>"},
                {"newIDString", R"s(%" -updateMotionNodeStringIDs true)s"},
            }
        },
        {
            {R"str(-newName "")str"},
            {
                {"newName", R"str()str"},
            }
        },
        {
            {R"str(-newName {})str"},
            {
                {"newName", R"str()str"},
            }
        },
        {
            {R"str(-newName "{}")str"},
            {
                {"newName", R"str({})str"},
            }
        },
        {
            {R"str(-newName {""})str"},
            {
                {"newName", R"str()str"},
            }
        },
        {
            // utf-8 smiley
            {u8"-newName \U0001F604"},
            {
                {"newName", u8"\U0001F604"},
            }
        },
    };

    INSTANTIATE_TEST_CASE_P(TestCommandLine, CommandLineFixture, ::testing::ValuesIn(commandLineTestData));
} // namespace EMotionFX
