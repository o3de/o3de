/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CommandGroup.h"


namespace MCore
{
    CommandGroup::CommandGroup()
    {
        ReserveCommands(5);
        SetContinueAfterError(true);
        SetReturnFalseAfterError(false);
    }

    CommandGroup::CommandGroup(const AZStd::string& groupName, size_t numCommandsToReserve)
    {
        SetGroupName(groupName);
        ReserveCommands(numCommandsToReserve);
        SetContinueAfterError(true);
        SetAddToHistoryAfterError(true);
    }

    CommandGroup::~CommandGroup()
    {
        RemoveAllCommands(true);
    }

    void CommandGroup::ReserveCommands(size_t numToReserve)
    {
        if (numToReserve > 0)
        {
            m_commands.reserve(numToReserve);
        }
    }

    void CommandGroup::AddCommandString(const char* commandString)
    {
        m_commands.emplace_back(CommandEntry());
        m_commands.back().m_commandString = commandString;
    }

    void CommandGroup::AddCommandString(const AZStd::string& commandString)
    {
        m_commands.emplace_back(CommandEntry());
        m_commands.back().m_commandString = commandString;
    }

    void CommandGroup::AddCommand(MCore::Command* command)
    {
        m_commands.emplace_back(CommandEntry());
        m_commands.back().m_command = command;
    }

    const char* CommandGroup::GetCommandString(size_t index) const
    {
        return m_commands[index].m_commandString.c_str();
    }

    const AZStd::string& CommandGroup::GetCommandStringAsString(size_t index)   const
    {
        return m_commands[index].m_commandString;
    }

    Command* CommandGroup::GetCommand(size_t index)
    {
        return m_commands[index].m_command;
    }

    const CommandLine& CommandGroup::GetParameters(size_t index) const
    {
        return m_commands[index].m_commandLine;
    }

    const char* CommandGroup::GetGroupName() const
    {
        return m_groupName.c_str();
    }

    const AZStd::string& CommandGroup::GetGroupNameString() const
    {
        return m_groupName;
    }

    void CommandGroup::SetGroupName(const char* groupName)
    {
        m_groupName = groupName;
    }

    void CommandGroup::SetGroupName(const AZStd::string& groupName)
    {
        m_groupName = groupName;
    }

    void CommandGroup::SetCommandString(size_t index, const char* commandString)
    {
        m_commands[index].m_commandString = commandString;
    }

    void CommandGroup::SetParameters(size_t index, const CommandLine& params)
    {
        m_commands[index].m_commandLine = params;
    }

    void CommandGroup::SetCommand(size_t index, Command* command)
    {
        m_commands[index].m_command = command;
    }

    size_t CommandGroup::GetNumCommands() const
    {
        return m_commands.size();
    }

    void CommandGroup::RemoveAllCommands(bool delFromMem)
    {
        if (delFromMem)
        {
            for (CommandEntry& commandEntry : m_commands)
            {
                delete commandEntry.m_command;
            }
        }

        m_commands.clear();
    }

    CommandGroup* CommandGroup::Clone() const
    {
        CommandGroup* newGroup          = new CommandGroup(m_groupName, 0);
        newGroup->m_commands             = m_commands;
        newGroup->m_historyAfterError    = m_historyAfterError;
        newGroup->m_continueAfterError   = m_continueAfterError;
        newGroup->m_returnFalseAfterError = m_returnFalseAfterError;
        return newGroup;
    }

    // continue execution of the remaining commands after one fails to execute?
    void CommandGroup::SetContinueAfterError(bool continueAfter)
    {
        m_continueAfterError = continueAfter;
    }

    // add group to the history even when one internal command failed to execute?
    void CommandGroup::SetAddToHistoryAfterError(bool addAfterError)
    {
        m_historyAfterError = addAfterError;
    }

    // check to see if we continue executing internal commands even if one failed
    bool CommandGroup::GetContinueAfterError() const
    {
        return m_continueAfterError;
    }

    // check if we add this group to the history, even if one internal command failed
    bool CommandGroup::GetAddToHistoryAfterError() const
    {
        return m_historyAfterError;
    }

    // set if the command group shall return false after an error occurred or not
    void CommandGroup::SetReturnFalseAfterError(bool returnAfterError)
    {
        m_returnFalseAfterError = returnAfterError;
    }

    // returns true in case the group returns false when executing it
    bool CommandGroup::GetReturnFalseAfterError() const
    {
        return m_returnFalseAfterError;
    }
} // namespace MCore
