/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            mCommands.reserve(numToReserve);
        }
    }

    void CommandGroup::AddCommandString(const char* commandString)
    {
        mCommands.emplace_back(CommandEntry());
        mCommands.back().mCommandString = commandString;
    }

    void CommandGroup::AddCommandString(const AZStd::string& commandString)
    {
        mCommands.emplace_back(CommandEntry());
        mCommands.back().mCommandString = commandString;
    }

    void CommandGroup::AddCommand(MCore::Command* command)
    {
        mCommands.emplace_back(CommandEntry());
        mCommands.back().mCommand = command;
    }

    const char* CommandGroup::GetCommandString(size_t index) const
    {
        return mCommands[index].mCommandString.c_str();
    }

    const AZStd::string& CommandGroup::GetCommandStringAsString(size_t index)   const
    {
        return mCommands[index].mCommandString;
    }

    Command* CommandGroup::GetCommand(size_t index)
    {
        return mCommands[index].mCommand;
    }

    const CommandLine& CommandGroup::GetParameters(size_t index) const
    {
        return mCommands[index].mCommandLine;
    }

    const char* CommandGroup::GetGroupName() const
    {
        return mGroupName.c_str();
    }

    const AZStd::string& CommandGroup::GetGroupNameString() const
    {
        return mGroupName;
    }

    void CommandGroup::SetGroupName(const char* groupName)
    {
        mGroupName = groupName;
    }

    void CommandGroup::SetGroupName(const AZStd::string& groupName)
    {
        mGroupName = groupName;
    }

    void CommandGroup::SetCommandString(size_t index, const char* commandString)
    {
        mCommands[index].mCommandString = commandString;
    }

    void CommandGroup::SetParameters(size_t index, const CommandLine& params)
    {
        mCommands[index].mCommandLine = params;
    }

    void CommandGroup::SetCommand(size_t index, Command* command)
    {
        mCommands[index].mCommand = command;
    }

    size_t CommandGroup::GetNumCommands() const
    {
        return mCommands.size();
    }

    void CommandGroup::RemoveAllCommands(bool delFromMem)
    {
        if (delFromMem)
        {
            for (CommandEntry& commandEntry : mCommands)
            {
                delete commandEntry.mCommand;
            }
        }

        mCommands.clear();
    }

    CommandGroup* CommandGroup::Clone() const
    {
        CommandGroup* newGroup          = new CommandGroup(mGroupName, 0);
        newGroup->mCommands             = mCommands;
        newGroup->mHistoryAfterError    = mHistoryAfterError;
        newGroup->mContinueAfterError   = mContinueAfterError;
        newGroup->mReturnFalseAfterError = mReturnFalseAfterError;
        return newGroup;
    }

    // continue execution of the remaining commands after one fails to execute?
    void CommandGroup::SetContinueAfterError(bool continueAfter)
    {
        mContinueAfterError = continueAfter;
    }

    // add group to the history even when one internal command failed to execute?
    void CommandGroup::SetAddToHistoryAfterError(bool addAfterError)
    {
        mHistoryAfterError = addAfterError;
    }

    // check to see if we continue executing internal commands even if one failed
    bool CommandGroup::GetContinueAfterError() const
    {
        return mContinueAfterError;
    }

    // check if we add this group to the history, even if one internal command failed
    bool CommandGroup::GetAddToHistoryAfterError() const
    {
        return mHistoryAfterError;
    }

    // set if the command group shall return false after an error occurred or not
    void CommandGroup::SetReturnFalseAfterError(bool returnAfterError)
    {
        mReturnFalseAfterError = returnAfterError;
    }

    // returns true in case the group returns false when executing it
    bool CommandGroup::GetReturnFalseAfterError() const
    {
        return mReturnFalseAfterError;
    }
} // namespace MCore
