/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/optional.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandLine.h>
#include <MCore/Source/MCoreCommandManager.h>
#include <MCore/Source/CommandManagerCallback.h>
#include <Tests/MCoreSystemFixture.h>
#include <Tests/Mocks/Command.h>
#include <Tests/Mocks/CommandManagerCallback.h>

namespace EMotionFX
{
    class TestCommand
        : public MCore::Command
    {
    public:
        TestCommand(const AZStd::optional<float>& newValue, float* data = nullptr, MCore::Command* orgCommand = nullptr)
            : MCore::Command(TestCommand::s_commandName, orgCommand),
            m_data(data),
            m_value(newValue)
        {
        }

        bool Execute([[maybe_unused]] const MCore::CommandLine& parameters, [[maybe_unused]] AZStd::string& outResult) override
        {
            ExecuteParameter<float>(m_oldValue, m_value, *m_data);
            return true;
        }

        bool Undo([[maybe_unused]] const MCore::CommandLine& parameters, [[maybe_unused]] AZStd::string& outResult) override
        {
            if (m_value.has_value())
            {
                *m_data = m_oldValue.value();
            }
            return true;
        }

        void InitSyntax() override
        {
            MCore::CommandSyntax& syntax = GetSyntax();
            syntax.AddParameter("value", "Test value.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.0");
        }

        bool SetCommandParameters(const MCore::CommandLine& parameters) override
        {
            if (parameters.CheckIfHasParameter("value"))
            {
                m_value = parameters.GetValueAsFloat("value", this);
            }
            return true;
        }

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Test Command"; }
        const char* GetDescription() const override { return "Unit test command"; }
        MCore::Command* Create() override { return new TestCommand(m_value, m_data, this); }

    public:
        static const char* s_commandName;

    private:
        float* m_data;
        AZStd::optional<float> m_value;
        AZStd::optional<float> m_oldValue;
    };
    const char* TestCommand::s_commandName = "TestCommand";

    enum TestCommandExecutionMethod
    {
        Execute,
        Undo,
        Redo
    };

    class CommandGroupTester
    {
    public:
        CommandGroupTester()
        {
            m_commandManager.RegisterCommand(new TestCommand(AZStd::nullopt, &m_value));
            m_managerCallbackMock = new MCore::CommandManagerCallbackMock();
            m_commandManager.RegisterCallback(m_managerCallbackMock);
            m_callbackMock = new MCore::CommandCallbackMock(/*executePreUndo=*/true);
            m_commandManager.RegisterCommandCallback(TestCommand::s_commandName, m_callbackMock);
        }

        void TestCommandGroup(MCore::CommandGroup& commandGroup, int numCommandsInGroupVerification, float valueBefore, float valueAfter, TestCommandExecutionMethod commandExecutionTestMethod)
        {
            EXPECT_EQ(commandGroup.GetNumCommands(), numCommandsInGroupVerification);
            const int numCommandsInGroup = static_cast<int>(commandGroup.GetNumCommands());
            AZStd::string result;

            SetManagerCallbackExpectations(m_managerCallbackMock, numCommandsInGroup, commandExecutionTestMethod);
            SetCommandCallbackExpectations(m_callbackMock, numCommandsInGroup, commandExecutionTestMethod);

            // 1. Execute
            EXPECT_TRUE(m_commandManager.ExecuteCommandGroup(commandGroup, result));
            EXPECT_FLOAT_EQ(m_value, valueAfter);

            // 2. Undo
            if (commandExecutionTestMethod == TestCommandExecutionMethod::Undo ||
                commandExecutionTestMethod == TestCommandExecutionMethod::Redo)
            {
                EXPECT_TRUE(m_commandManager.Undo(result));
                EXPECT_FLOAT_EQ(m_value, valueBefore);
            }

            // 3. Redo
            if (commandExecutionTestMethod == TestCommandExecutionMethod::Redo)
            {
                EXPECT_TRUE(m_commandManager.Redo(result));
                EXPECT_FLOAT_EQ(m_value, valueAfter);
            }
        }

        static void SetManagerCallbackExpectations(MCore::CommandManagerCallbackMock* managerCallback, int numCommandsInGroup, TestCommandExecutionMethod executionMethod)
        {
            int numExecutedCommands = -1;
            int numExecutedCommandGroups = -1;
            int numUndoCommands = -1;
            int numSetCurrentCommandCalls = -1;
            switch (executionMethod)
            {
            case TestCommandExecutionMethod::Execute:
                numExecutedCommandGroups = 1;
                numExecutedCommands = numCommandsInGroup;
                numUndoCommands = 0;
                numSetCurrentCommandCalls = 0;
                break;
            case TestCommandExecutionMethod::Undo:
                numExecutedCommandGroups = 2; // Undo is a parameter of the execute command group callback and thus we're expecting two calls.
                numExecutedCommands = numCommandsInGroup;
                numUndoCommands = numCommandsInGroup;
                numSetCurrentCommandCalls = 1;
                break;
            case TestCommandExecutionMethod::Redo:
                numExecutedCommandGroups = 3;
                numExecutedCommands = 2 * numCommandsInGroup; // On redo, all commands are executed again.
                numUndoCommands = numCommandsInGroup; // Now new undo calls, but we called undo before we can redo.
                numSetCurrentCommandCalls = 2;
                break;
            default:
                EXPECT_TRUE(false) << "Invalid execution method passed.";
            }

            EXPECT_CALL(*managerCallback, OnPreExecuteCommandGroup(::testing::_, ::testing::_)).Times(numExecutedCommandGroups);
            EXPECT_CALL(*managerCallback, OnPostExecuteCommandGroup(::testing::_, ::testing::_)).Times(numExecutedCommandGroups);

            EXPECT_CALL(*managerCallback, OnPreExecuteCommand(::testing::_, ::testing::_, ::testing::_)).Times(numExecutedCommands);
            EXPECT_CALL(*managerCallback, OnPostExecuteCommand(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(numExecutedCommands);

            EXPECT_CALL(*managerCallback, OnPreUndoCommand(::testing::_, ::testing::_)).Times(numUndoCommands);
            EXPECT_CALL(*managerCallback, OnPostUndoCommand(::testing::_, ::testing::_)).Times(numUndoCommands);

            // We are adding only one item to the history with an execute call, the command group itself.
            const int numNewHistoryItems = 1;
            EXPECT_CALL(*managerCallback, OnAddCommandToHistory(::testing::_, ::testing::_, ::testing::_, ::testing::_))
                .Times(numNewHistoryItems);

            // Set current command gets called for undo as well as redo operations.
            EXPECT_CALL(*managerCallback, OnSetCurrentCommand(::testing::_))
                .Times(numSetCurrentCommandCalls);
        }

        static void SetCommandCallbackExpectations(MCore::CommandCallbackMock* callback, int numTimes, TestCommandExecutionMethod executionMethod)
        {
            const int numExecuteCalls = (executionMethod == TestCommandExecutionMethod::Redo) ? 2 * numTimes : numTimes;
            EXPECT_CALL(*callback, Execute(::testing::_, ::testing::_))
                .Times(numExecuteCalls)
                .WillRepeatedly(::testing::Return(true));

            if (executionMethod == TestCommandExecutionMethod::Execute)
            {
                EXPECT_CALL(*callback, Undo(::testing::_, ::testing::_))
                    .Times(0);
            }
            else
            {
                EXPECT_CALL(*callback, Undo(::testing::_, ::testing::_))
                    .Times(numTimes)
                    .WillRepeatedly(::testing::Return(true));
            }
        }

    public:
        MCore::CommandManager m_commandManager;
        MCore::CommandManagerCallbackMock* m_managerCallbackMock = nullptr;
        MCore::CommandCallbackMock* m_callbackMock = nullptr;
        float m_value = 0.0f;
    };

    class CommandGroupFixture
        : public MCoreSystemFixture
        , public ::testing::WithParamInterface<TestCommandExecutionMethod>
    {
    };

    TEST_P(CommandGroupFixture, StringBasedCommandGroup_OneCommand)
    {
        MCore::CommandGroup commandGroup;
        commandGroup.AddCommandString("TestCommand -value 1.0");

        CommandGroupTester tester;
        tester.TestCommandGroup(commandGroup, 1, 0.0f, 1.0f, GetParam());
    }

    TEST_P(CommandGroupFixture, StringBasedCommandGroup_MultipleCommands)
    {
        MCore::CommandGroup commandGroup;
        commandGroup.AddCommandString("TestCommand -value 1.0");
        commandGroup.AddCommandString("TestCommand -value 2.0");
        commandGroup.AddCommandString("TestCommand -value 3.0");
        
        CommandGroupTester tester;
        tester.TestCommandGroup(commandGroup, 3, 0.0f, 3.0f, GetParam());
    }

    TEST_P(CommandGroupFixture, ObjectBasedCommandGroup_OneCommand)
    {
        CommandGroupTester tester;

        MCore::CommandGroup commandGroup;
        commandGroup.AddCommand(new TestCommand(1.0f, &tester.m_value));

        tester.TestCommandGroup(commandGroup, 1, 0.0f, 1.0f, GetParam());
    }

    TEST_P(CommandGroupFixture, ObjectBasedCommandGroup_MultipleCommands)
    {
        CommandGroupTester tester;

        MCore::CommandGroup commandGroup;
        commandGroup.AddCommand(new TestCommand(1.0f, &tester.m_value));
        commandGroup.AddCommand(new TestCommand(2.0f, &tester.m_value));
        commandGroup.AddCommand(new TestCommand(3.0f, &tester.m_value));

        tester.TestCommandGroup(commandGroup, 3, 0.0f, 3.0f, GetParam());
    }

    TEST_P(CommandGroupFixture, ObjectBasedCommandGroup_Mixed)
    {
        CommandGroupTester tester;

        MCore::CommandGroup commandGroup;
        commandGroup.AddCommandString("TestCommand -value 1.0");
        commandGroup.AddCommand(new TestCommand(2.0f, &tester.m_value));
        commandGroup.AddCommandString("TestCommand -value 3.0");
        commandGroup.AddCommand(new TestCommand(4.0f, &tester.m_value));

        tester.TestCommandGroup(commandGroup, 4, 0.0f, 4.0f, GetParam());
    }

    INSTANTIATE_TEST_CASE_P(CommandGroupTests,
        CommandGroupFixture,
        ::testing::ValuesIn({ TestCommandExecutionMethod::Execute, TestCommandExecutionMethod::Undo, TestCommandExecutionMethod::Redo}));
} // namespace EMotionFX
