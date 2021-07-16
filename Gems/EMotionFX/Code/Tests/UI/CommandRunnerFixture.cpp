/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/UI/CommandRunnerFixture.h>

namespace EMotionFX
{
    void CommandRunnerFixtureBase::TearDown()
    {
        m_results.clear();
        m_results.shrink_to_fit();
        UIFixture::TearDown();
    }

    void CommandRunnerFixtureBase::ExecuteCommands(std::vector<std::string> commands)
    {
        AZStd::string result;
        for (const auto& commandStr : commands)
        {
            if (commandStr == "UNDO")
            {
                EXPECT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << "Undo: " << result.c_str();
            }
            else if (commandStr == "REDO")
            {
                EXPECT_TRUE(CommandSystem::GetCommandManager()->Redo(result)) << "Redo: " << result.c_str();
            }
            else
            {
                EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(commandStr.c_str(), result)) << commandStr.c_str() << ": " << result.c_str();
            }
            m_results.emplace_back(result);
        }
    }

    const AZStd::vector<AZStd::string>& CommandRunnerFixtureBase::GetResults()
    {
        return m_results;
    }

    TEST_P(CommandRunnerFixture, ExecuteCommands)
    {
        ExecuteCommands(GetParam());
    }
} // end namespace EMotionFX
