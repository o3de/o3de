/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MCoreCommandManager.h"
#include "LogManager.h"
#include "CommandManagerCallback.h"
#include "StringConversions.h"

//#define MCORE_COMMANDMANAGER_PERFORMANCE

namespace MCore
{
    CommandManager::CommandHistoryEntry::CommandHistoryEntry(CommandGroup* group, Command* command, const CommandLine& parameters, AZ::u32 historyItemNr)
    {
        mCommandGroup       = group;
        mExecutedCommand    = command;
        mParameters         = parameters;
        m_historyItemNr     = historyItemNr;
    }

    CommandManager::CommandHistoryEntry::~CommandHistoryEntry()
    {
        // remark: the mCommand and mCommandGroup are automatically deleted after popping from the history
    }

    AZStd::string CommandManager::CommandHistoryEntry::ToString(CommandGroup* group, Command* command, AZ::u32 historyItemNr)
    {
        if (group)
        {
            return AZStd::string::format("%.3d - %s", historyItemNr, group->GetGroupName());
        }
        else if (command)
        {
            return AZStd::string::format("%.3d - %s", historyItemNr, command->GetHistoryName());
        }

        return "";
    }

    AZStd::string CommandManager::CommandHistoryEntry::ToString() const
    {
        return ToString(mCommandGroup, mExecutedCommand, m_historyItemNr);
    }

    CommandManager::CommandManager()
    {
        mCommands.reserve(128);

        // set default values
        mMaxHistoryEntries      = 100;
        mHistoryIndex           = -1;
        m_totalNumHistoryItems   = 0;

        // preallocate history entries
        mCommandHistory.reserve(mMaxHistoryEntries);

        m_commandsInExecution = 0;
    }

    CommandManager::~CommandManager()
    {
        for (auto element : mRegisteredCommands)
        {
            Command* command = element.second;
            delete command;
        }
        mRegisteredCommands.clear();

        // remove all callbacks
        RemoveCallbacks();

        // destroy the command history
        while (!mCommandHistory.empty())
        {
            PopCommandHistory();
        }
    }

    // push a command group on the history
    void CommandManager::PushCommandHistory(CommandGroup* commandGroup)
    {
        // if we reached the maximum number of history entries remove the oldest one
        if (mCommandHistory.size() >= mMaxHistoryEntries)
        {
            PopCommandHistory();
            mHistoryIndex = static_cast<int32>(mCommandHistory.size()) - 1;
        }

        if (!mCommandHistory.empty())
        {
            // remove unneeded commandsnumToRemove
            const size_t numToRemove = mCommandHistory.size() - mHistoryIndex - 1;
            for (CommandManagerCallback* managerCallback : mCallbacks)
            {
                for (size_t a = 0; a < numToRemove; ++a)
                {
                    managerCallback->OnRemoveCommand(mHistoryIndex + 1);
                }
            }

            for (size_t a = 0; a < numToRemove; ++a)
            {
                delete mCommandHistory[mHistoryIndex + 1].mExecutedCommand;
                delete mCommandHistory[mHistoryIndex + 1].mCommandGroup;
                mCommandHistory.erase(mCommandHistory.begin() + mHistoryIndex + 1);
            }
        }

        // resize the command history
        mCommandHistory.resize(mHistoryIndex + 1);

        // add a command history entry
        m_totalNumHistoryItems++;
        mCommandHistory.push_back(CommandHistoryEntry(commandGroup, nullptr, CommandLine(), m_totalNumHistoryItems));

        // increase the history index
        mHistoryIndex++;

        // perform callbacks
        for (CommandManagerCallback* managerCallback : mCallbacks)
        {
            managerCallback->OnAddCommandToHistory(mHistoryIndex, commandGroup, nullptr, CommandLine());
        }
    }

    // save command in the history
    void CommandManager::PushCommandHistory(Command* command, const CommandLine& parameters)
    {
        if (!mCommandHistory.empty())
        {
            // if we reached the maximum number of history entries remove the oldest one
            if (mCommandHistory.size() >= mMaxHistoryEntries)
            {
                PopCommandHistory();
                mHistoryIndex = static_cast<int32>(mCommandHistory.size()) - 1;
            }

            // remove unneeded commands
            const size_t numToRemove = mCommandHistory.size() - mHistoryIndex - 1;
            for (CommandManagerCallback* managerCallback : mCallbacks)
            {
                for (size_t a = 0; a < numToRemove; ++a)
                {
                    managerCallback->OnRemoveCommand(mHistoryIndex + 1);
                }
            }

            for (size_t a = 0; a < numToRemove; ++a)
            {
                delete mCommandHistory[mHistoryIndex + 1].mExecutedCommand;
                delete mCommandHistory[mHistoryIndex + 1].mCommandGroup;
                mCommandHistory.erase(mCommandHistory.begin() + mHistoryIndex + 1);
            }
        }

        // resize the command history
        mCommandHistory.resize(mHistoryIndex + 1);

        // add a command history entry
        m_totalNumHistoryItems++;
        mCommandHistory.push_back(CommandHistoryEntry(nullptr, command, parameters, m_totalNumHistoryItems));

        // increase the history index
        mHistoryIndex++;

        // perform callbacks
        for (CommandManagerCallback* managerCallback : mCallbacks)
        {
            managerCallback->OnAddCommandToHistory(mHistoryIndex, nullptr, command, parameters);
        }
    }

    // remove the oldest entry in the history stack
    void CommandManager::PopCommandHistory()
    {
        // perform callbacks
        for (CommandManagerCallback* managerCallback : mCallbacks)
        {
            managerCallback->OnRemoveCommand(0);
        }

        // destroy the command and remove it from the command history
        Command* command = mCommandHistory.front().mExecutedCommand;
        if (command)
        {
            delete command;
        }
        else
        {
            delete mCommandHistory.front().mCommandGroup;
        }

        mCommandHistory.erase(mCommandHistory.begin());
    }

    bool CommandManager::ExecuteCommand(const AZStd::string& command, AZStd::string& outCommandResult, bool addToHistory, Command** outExecutedCommand, CommandLine* outExecutedParameters, bool callFromCommandGroup, bool clearErrors, bool handleErrors)
    {
        AZStd::string commandResult;
        bool result = ExecuteCommand(command.c_str(), commandResult, addToHistory, outExecutedCommand, outExecutedParameters, callFromCommandGroup, clearErrors, handleErrors);
        outCommandResult = commandResult.c_str();
        return result;
    }

    bool CommandManager::ShouldDeleteCommand(Command* commandObject, bool commandExecutionResult, bool callFromCommandGroup, bool addToHistory)
    {
        // Remove failed commands.
        if (!commandExecutionResult)
        {
            return true;
        }

        // Remove commands that are NOT undoable.
        if (!commandObject->GetIsUndoable())
        {
            return true;
        }

        // Remove individually executed commands that are NOT added to the history (e.g. calling commands inside commands).
        if (!callFromCommandGroup && (!addToHistory))
        {
            return true;
        }

        return false;
    }

    // parse and execute command
    bool CommandManager::ExecuteCommand(const char* command, AZStd::string& outCommandResult, bool addToHistory, Command** outExecutedCommand, CommandLine* outExecutedParameters, bool callFromCommandGroup, bool clearErrors, bool handleErrors)
    {
        // store the executed command
        if (outExecutedCommand)
        {
            *outExecutedCommand = nullptr;
        }

        if (outExecutedParameters)
        {
            *outExecutedParameters = CommandLine();
        }

        // build a local string object from the command
        AZStd::string commandString(command);

        // remove all spaces on the left and right
        AzFramework::StringFunc::TrimWhiteSpace(commandString, true, true);

        // check if the string actually contains text
        if (commandString.empty())
        {
            outCommandResult = "Command string is empty.";
            if (outExecutedCommand)
            {
                *outExecutedCommand = nullptr;
            }
            return false;
        }

        // find the first space after the command
        const size_t wordSeparatorIndex = commandString.find_first_of(MCore::CharacterConstants::wordSeparators);

        // get the command name
        const AZStd::string commandName = commandString.substr(0, wordSeparatorIndex);

        // find the corresponding command and execute it
        Command* commandObject = FindCommand(commandName);
        if (commandObject == nullptr)
        {
            if (outExecutedCommand)
            {
                *outExecutedCommand = nullptr;
            }
            outCommandResult = "Command has not been found, please make sure you have registered the command before using it.";
            return false;
        }

        // get the parameters
        AZStd::string commandParameters;
        if (wordSeparatorIndex != AZStd::string::npos)
        {
            commandParameters = commandString.substr(wordSeparatorIndex + 1);
        }
        AzFramework::StringFunc::TrimWhiteSpace(commandParameters, true, true);

        // show help when wanted
        if (AzFramework::StringFunc::Equal(commandParameters.c_str(), "-help", false /* no case */))
        {
            if (outExecutedCommand)
            {
                *outExecutedCommand = nullptr;
            }
            commandObject->GetSyntax().LogSyntax();
            return true;
        }

        // build the command line
        CommandLine commandLine(commandParameters);

        // check on syntax errors first
        outCommandResult.clear();
        if (commandObject->GetSyntax().CheckIfIsValid(commandLine, outCommandResult) == false)
        {
            if (outExecutedCommand)
            {
                *outExecutedCommand = nullptr;
            }
            return false;
        }

        // create and execute the command
        Command* newCommand = commandObject->Create();
        newCommand->SetCommandParameters(commandLine);
        const bool result = ExecuteCommand(newCommand,
            commandLine,
            outCommandResult,
            addToHistory,
            callFromCommandGroup,
            clearErrors,
            handleErrors,
            /*autoDeleteCommand=*/false);

        // delete the command object directly if we don't want to store it in the history
        if (ShouldDeleteCommand(newCommand, result, callFromCommandGroup, addToHistory))
        {
            delete newCommand;
            if (outExecutedCommand)
            {
                *outExecutedCommand = nullptr;
            }
            return result;
        }

        // store the executed command
        if (outExecutedCommand)
        {
            *outExecutedCommand = newCommand;
        }

        if (outExecutedParameters)
        {
            *outExecutedParameters = commandLine;
        }

        // return the command result
        return result;
    }

    // use this version when calling a command from inside a command execute or undo function
    bool CommandManager::ExecuteCommandInsideCommand(const char* command, AZStd::string& outCommandResult)
    {
        return ExecuteCommand(command,
            outCommandResult,
            /*addToHistory=*/false,
            /*outExecutedCommand=*/nullptr,
            /*outExecutedParameters=*/nullptr,
            /*callFromCommandGroup=*/false,
            /*clearErrors=*/false,
            /*handleErrors=*/false);
    }

    bool CommandManager::ExecuteCommandInsideCommand(const AZStd::string& command, AZStd::string& outCommandResult)
    {
        return ExecuteCommand(command,
            outCommandResult,
            /*addToHistory=*/false,
            /*outExecutedCommand=*/nullptr,
            /*outExecutedParameters=*/nullptr,
            /*clearErrors=*/false,
            /*handleErrors=*/false);
    }

    bool CommandManager::ExecuteCommandInsideCommand(Command* command, AZStd::string& outCommandResult)
    {
        return ExecuteCommand(command,
            CommandLine(),
            outCommandResult,
            /*addToHistory=*/false,
            /*callFromCommandGroup=*/false,
            /*clearErrors=*/false,
            /*handleErrors=*/false,
            /*autoDeleteCommand=*/true);
    }

    bool CommandManager::ExecuteCommandOrAddToGroup(const AZStd::string& command, MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        if (!commandGroup)
        {
            bool commandResult;
            AZStd::string commandResultString;
            if (executeInsideCommand)
            {
                commandResult = ExecuteCommandInsideCommand(command, commandResultString);
            }
            else
            {
                commandResult = ExecuteCommand(command, commandResultString);
            }

            if (!commandResult)
            {
                AZ_Error("EMotionFX", false, commandResultString.c_str());
                return false;
            }
        }
        else
        {
            commandGroup->AddCommandString(command);
        }

        return true;
    }

    bool CommandManager::ExecuteCommandOrAddToGroup(Command* command, MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        if (!commandGroup)
        {
            bool commandResult;
            AZStd::string commandResultString;
            if (executeInsideCommand)
            {
                commandResult = ExecuteCommandInsideCommand(command, commandResultString);
            }
            else
            {
                commandResult = ExecuteCommand(command, commandResultString);
            }

            if (!commandResult)
            {
                AZ_Error("EMotionFX", false, commandResultString.c_str());
                return false;
            }
        }
        else
        {
            commandGroup->AddCommand(command);
        }

        return true;
    }

    // execute a group of commands
    bool CommandManager::ExecuteCommandGroup(CommandGroup& commandGroup, AZStd::string& outCommandResult, bool addToHistory, bool clearErrors, bool handleErrors)
    {
        // if there is nothing to do
        const size_t numCommands = commandGroup.GetNumCommands();
        if (numCommands == 0)
        {
            return true;
        }

        ++m_commandsInExecution;

        // execute command manager callbacks
        for (CommandManagerCallback* managerCallback : mCallbacks)
        {
            managerCallback->OnPreExecuteCommandGroup(&commandGroup, false);
        }

        // clear the output result
        AZStd::vector<AZStd::string> intermediateCommandResults;
        intermediateCommandResults.resize(numCommands);

        // execute all commands inside the group
        bool hadError = false;
        CommandGroup* newGroup = commandGroup.Clone();
        AZStd::string commandString;
        AZStd::string tmpStr;

        for (size_t i = 0; i < numCommands; ++i)
        {
            Command* executedCommand = commandGroup.GetCommand(i);
            bool result = false;

            // Command-object based
            if (executedCommand)
            {
                // Set the original command. This is needed for the command callbacks to function
                MCore::Command* orgCommand = FindCommand(executedCommand->GetNameString());
                executedCommand->SetOriginalCommand(orgCommand);

                result = ExecuteCommand(executedCommand,
                    CommandLine(),
                    intermediateCommandResults[i],
                    /*addToHistory=*/false,
                    /*callFromCommandGroup=*/true,
                    /*clearErrors=*/false,
                    /*handleErrors=*/false,
                    /*autoDeleteCommand=*/false);

                // Ownership transfer, move the command object from the former to the new command group.
                commandGroup.SetCommand(i, nullptr);
                newGroup->SetCommand(i, executedCommand);
            }
            // String-based command
            else
            {
                commandString = commandGroup.GetCommandString(i);

                // feed the last result into the next command
                size_t lastResultIndex = commandString.find("%LASTRESULT");
                bool replaceHappen = false;
                while (lastResultIndex != AZStd::string::npos)
                {
                    // Find what index the string is referring to
                    // 11 is the amount of characters in "%LASTRESULT"
                    const size_t rightPercentagePos = commandString.find("%", lastResultIndex + 11);
                    if (rightPercentagePos == AZStd::string::npos)
                    {
                        // if the right pound is not found, stop replacing, the command is ill-formed
                        MCore::LogError("Execution of command '%s' failed, right '%' delimiter was not found", commandString.c_str());
                        hadError = true;
                        break;
                    }

                    // Get the relative index of the command execution we want the result from
                    // For example, %LASTRESULT2% means we want the result of two commands ago
                    // %LASTRESULT% == %LASTRESULT1% which is the result of the last command
                    int32 relativeIndex = 1;

                    // 11 is the amount of characters in "%LASTRESULT"
                    tmpStr = commandString.substr(lastResultIndex + 11, rightPercentagePos - 11 - lastResultIndex);
                    if (!tmpStr.empty())
                    {
                        // check if it can be safely converted to number
                        if (!AzFramework::StringFunc::LooksLikeInt(tmpStr.c_str(), &relativeIndex))
                        {
                            MCore::LogError("Execution of command '%s' failed, characters between '%LASTRESULT' and '%' cannot be converted to integer", commandString.c_str());
                            hadError = true;
                            break;
                        }
                        if (relativeIndex == 0)
                        {
                            MCore::LogError("Execution of command '%s' failed, command trying to access its own result", commandString.c_str());
                            hadError = true;
                            break;
                        }
                    }
                    if (static_cast<int32>(i) < relativeIndex)
                    {
                        MCore::LogError("Execution of command '%s' failed, command trying to access results from %d commands back, but there are only %d", commandString.c_str(), relativeIndex, i - 1);
                        hadError = true;
                        break;
                    }

                    tmpStr = commandString.substr(lastResultIndex, rightPercentagePos - lastResultIndex + 1);
                    AzFramework::StringFunc::Replace(commandString, tmpStr.c_str(), intermediateCommandResults[i - relativeIndex].c_str());
                    replaceHappen = true;

                    // Search again in case the command group is referring to other results
                    lastResultIndex = commandString.find("%LASTRESULT");
                }
                if (replaceHappen)
                {
                    commandGroup.SetCommandString(i, commandString.c_str());
                }

                CommandLine executedParameters;
                result = ExecuteCommand(commandString, intermediateCommandResults[i], /*addToHistory=*/false, &executedCommand, &executedParameters, /*callFromCommandGroup=*/true, /*clearErrors=*/false, /*handleErrors=*/false);

                // String-based commands create a new command object after parsing the command string.
                // The new command object needs to be transfered to the new command group along with the executed command parameters.
                newGroup->SetParameters(i, executedParameters);
                newGroup->SetCommand(i, executedCommand);
            }

            if (!result)
            {
                AZ_Error("EMotionFX", false, "Execution of command '%s' failed (result='%s')", commandString.c_str(), intermediateCommandResults[i].c_str());
                AddError(intermediateCommandResults[i]);
                hadError = true;

                if (!commandGroup.GetContinueAfterError())
                {
                    if (commandGroup.GetAddToHistoryAfterError() == false || addToHistory == false)
                    {
                        delete newGroup;

                        // execute command manager callbacks
                        for (CommandManagerCallback* managerCallback : mCallbacks)
                        {
                            managerCallback->OnPostExecuteCommandGroup(&commandGroup, false);
                        }

                        // Let the callbacks handle error reporting (e.g. show an error report window).
                        if (handleErrors && !mErrors.empty())
                        {
                            // Execute error report callbacks.
                            for (CommandManagerCallback* managerCallback : mCallbacks)
                            {
                                managerCallback->OnShowErrorReport(mErrors);
                            }
                        }

                        // Clear errors after reporting if specified.
                        if (clearErrors)
                        {
                            mErrors.clear();
                        }

                        --m_commandsInExecution;
                        return false;
                    }
                    else
                    {
                        for (size_t c = i; c < numCommands; ++c)
                        {
                            newGroup->SetCommand(c, nullptr);
                        }
                        break;
                    }
                }
            }
        }

        //  if we want to add the group to the history
        if (hadError == false)
        {
            if (addToHistory) // also check if all commands inside the group are undoable?
            {
                PushCommandHistory(newGroup);
            }
            else
            {
                delete newGroup; // delete the cloned command group in case we don't want to add it to the history
            }
        }
        else
        {
            if (commandGroup.GetAddToHistoryAfterError() && addToHistory)
            {
                PushCommandHistory(newGroup);
            }
            else
            {
                delete newGroup; // delete the cloned command group in case we don't want to add it to the history
            }
        }

        // execute command manager callbacks
        for (CommandManagerCallback* managerCallback : mCallbacks)
        {
            managerCallback->OnPostExecuteCommandGroup(&commandGroup, true);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        const bool errorsOccured = !mErrors.empty();
        if (handleErrors && errorsOccured)
        {
            // Execute error report callbacks.
            for (CommandManagerCallback* managerCallback : mCallbacks)
            {
                managerCallback->OnShowErrorReport(mErrors);
            }
        }

        // Return the result of the last command
        // TODO: once we convert to AZStd::string we can do a swap here
        outCommandResult = intermediateCommandResults.back();

        // Clear errors after reporting if specified.
        if (clearErrors)
        {
            mErrors.clear();
        }

        --m_commandsInExecution;

        // check if we need to let the whole command group fail in case an error occured
        if (errorsOccured && commandGroup.GetReturnFalseAfterError())
        {
            return false;
        }

        return true;
    }

    // use this version when calling a command group from inside another command
    bool CommandManager::ExecuteCommandGroupInsideCommand(CommandGroup& commandGroup, AZStd::string& outCommandResult)
    {
        return ExecuteCommandGroup(commandGroup, outCommandResult, false, false, false);
    }

    // internal helper functions to easily execute all undo callbacks for a given command
    void CommandManager::ExecuteUndoCallbacks(Command* command, const CommandLine& parameters, bool preUndo)
    {
        Command*    orgCommand  = command->GetOriginalCommand();
        uint32      numFailed   = 0;

        // get the number of callbacks and iterate through them
        const uint32 numCommandCallbacks = orgCommand->GetNumCallbacks();
        for (uint32 i = 0; i < numCommandCallbacks; ++i)
        {
            // get the current callback
            Command::Callback* callback = orgCommand->GetCallback(i);

            if (preUndo)
            {
                for (CommandManagerCallback* managerCallback : mCallbacks)
                {
                    managerCallback->OnPreUndoCommand(command, parameters);
                }
            }

            // check if we need to execute the callback
            if (callback->GetExecutePreUndo() == preUndo)
            {
                if (callback->Undo(command, parameters) == false)
                {
                    numFailed++;
                }
            }

            if (!preUndo)
            {
                for (CommandManagerCallback* managerCallback : mCallbacks)
                {
                    managerCallback->OnPostUndoCommand(command, parameters);
                }
            }
        }

        if (numFailed > 0)
        {
            LogWarning("%d out of %d %s-undo callbacks of command '%s' (%s) returned a failure.", numFailed, numCommandCallbacks, preUndo ? "pre" : "post", command->GetName(), command->GetHistoryName());
        }
    }

    // internal helper functions to easily execute all command callbacks for a given command
    void CommandManager::ExecuteCommandCallbacks(Command* command, const CommandLine& parameters, bool preCommand)
    {
        Command*    orgCommand  = command->GetOriginalCommand();
        uint32      numFailed   = 0;

        // get the number of callbacks and iterate through them
        const uint32 numCommandCallbacks = orgCommand->GetNumCallbacks();
        for (uint32 i = 0; i < numCommandCallbacks; ++i)
        {
            // get the current callback
            Command::Callback* callback = orgCommand->GetCallback(i);

            // check if we need to execute the callback
            if (callback->GetExecutePreCommand() == preCommand)
            {
                if (callback->Execute(command, parameters) == false)
                {
                    numFailed++;
                }
            }
        }

        if (numFailed > 0)
        {
            LogWarning("%d out of %d %s-command callbacks of command '%s' (%s) returned a failure.", numFailed, numCommandCallbacks, preCommand ? "pre" : "post", command->GetName(), command->GetHistoryName());
        }
    }

    bool CommandManager::Undo(AZStd::string& outCommandResult)
    {
        // check if we can undo
        if (mCommandHistory.empty() && mHistoryIndex >= 0)
        {
            outCommandResult = "Cannot undo command. The command history is empty";
            return false;
        }

        // get the last called command from the command history
        const CommandHistoryEntry& lastEntry = mCommandHistory[mHistoryIndex];
        Command* command = lastEntry.mExecutedCommand;

        ++m_commandsInExecution;

        // if we are dealing with a regular command
        bool result = true;
        if (command)
        {
            // execute pre-undo callbacks
            ExecuteUndoCallbacks(command, lastEntry.mParameters, true);

            // undo the command, get the result and reset it
            result = command->Undo(lastEntry.mParameters, outCommandResult);

            // execute post-undo callbacks
            ExecuteUndoCallbacks(command, lastEntry.mParameters, false);
        }
        // we are dealing with a command group
        else
        {
            CommandGroup* group = lastEntry.mCommandGroup;
            MCORE_ASSERT(group);

            // perform callbacks
            for (CommandManagerCallback* managerCallback : mCallbacks)
            {
                managerCallback->OnPreExecuteCommandGroup(group, true);
            }

            const int32 numCommands = static_cast<int32>(group->GetNumCommands() - 1);
            for (int32 g = numCommands; g >= 0; --g)
            {
                Command* groupCommand = group->GetCommand(g);
                if (groupCommand == nullptr)
                {
                    continue;
                }

                ++m_commandsInExecution;

                // execute pre-undo callbacks
                ExecuteUndoCallbacks(groupCommand, group->GetParameters(g), true);

                // undo the command, get the result and reset it
                // TODO: add option to stop execution of undo's when one fails?
                if (groupCommand->Undo(group->GetParameters(g), outCommandResult) == false)
                {
                    result = false;
                }

                // execute post-undo callbacks
                ExecuteUndoCallbacks(groupCommand, group->GetParameters(g), false);

                --m_commandsInExecution;
            }

            // perform callbacks
            for (CommandManagerCallback* managerCallback : mCallbacks)
            {
                managerCallback->OnPostExecuteCommandGroup(group, result);
            }
        }

        // go one step back in the command history
        mHistoryIndex--;

        // perform callbacks
        for (CommandManagerCallback* managerCallback : mCallbacks)
        {
            managerCallback->OnSetCurrentCommand(mHistoryIndex);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        if (!mErrors.empty())
        {
            for (CommandManagerCallback* managerCallback : mCallbacks)
            {
                managerCallback->OnShowErrorReport(mErrors);
            }
            mErrors.clear();
        }

        --m_commandsInExecution;

        return result;
    }

    // redo the last undoed command
    bool CommandManager::Redo(AZStd::string& outCommandResult)
    {
        /*  // check if there are still commands to undo in the history
            if (mHistoryIndex >= mCommandHistory.GetLength())
            {
                outCommandResult = "Cannot redo command. Either the history is empty or the history index is out of range.";
                return false;
            }*/

        // get the last called command from the command history
        const CommandHistoryEntry& lastEntry = mCommandHistory[mHistoryIndex + 1];

        // if we just redo one single command
        bool result = true;
        if (lastEntry.mExecutedCommand)
        {
            // redo the command, get the result and reset it
            result = ExecuteCommand(lastEntry.mExecutedCommand,
                lastEntry.mParameters,
                outCommandResult,
                /*addToHistory=*/false,
                /*callFromCommandGroup=*/false,
                /*clearErrors=*/true,
                /*handleErrors=*/true,
                /*autoDeleteCommand=*/false);
        }
        // Redo command group
        else
        {
            ++m_commandsInExecution;
            CommandGroup* group = lastEntry.mCommandGroup;
            AZ_Assert(group, "Cannot redo. Command group is not valid.");

            for (CommandManagerCallback* managerCallback : mCallbacks)
            {
                managerCallback->OnPreExecuteCommandGroup(group, false);
            }

            // redo all commands inside the group
            const size_t numCommands = group->GetNumCommands();
            for (size_t g = 0; g < numCommands; ++g)
            {
                if (group->GetCommand(g))
                {
                    if (!ExecuteCommand(group->GetCommand(g),
                        group->GetParameters(g),
                        outCommandResult,
                        /*addToHistory=*/false,
                        /*callFromCommandGroup=*/false,
                        /*clearErrors=*/true,
                        /*handleErrors=*/true,
                        /*autoDeleteCommand=*/false))
                    {
                        result = false;
                    }
                }
            }
            --m_commandsInExecution;

            for (CommandManagerCallback* managerCallback : mCallbacks)
            {
                managerCallback->OnPostExecuteCommandGroup(group, result);
            }
        }

        // go one step forward in the command history
        mHistoryIndex++;

        // perform callbacks
        for (CommandManagerCallback* managerCallback : mCallbacks)
        {
            managerCallback->OnSetCurrentCommand(mHistoryIndex);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        if (!mErrors.empty())
        {
            for (CommandManagerCallback* managerCallback : mCallbacks)
            {
                managerCallback->OnShowErrorReport(mErrors);
            }
            mErrors.clear();
        }

        return result;
    }

    bool CommandManager::RegisterCommand(Command* command)
    {
        // check if the command object is valid
        if (command == nullptr)
        {
            LogError("Cannot register command. Command object is not valid.");
            return false;
        }

        // check if the string actually contains text
        if (command->GetNameString().empty())
        {
            LogError("Cannot register command. Command name is empty.");
            return false;
        }

        // check if the command is already registered
        Command* commandObject = FindCommand(command->GetNameString());
        if (commandObject)
        {
            LogError("Cannot register command. There is already a command registered as '%s'.", command->GetName());
            return false;
        }

        // add the command to the hash table
        mRegisteredCommands.insert(AZStd::make_pair<AZStd::string, Command*>(command->GetNameString(), command));

        // we're going to insert the command in a sorted way now
        bool found = false;
        const size_t numCommands = mCommands.size();
        for (size_t i = 0; i < numCommands; ++i)
        {
            if (azstricmp(mCommands[i]->GetName(), command->GetName()) > 0)
            {
                found = true;
                mCommands.insert(mCommands.begin() + i, command);
                break;
            }
        }

        // if no insert location has been found, add it to the back of the array
        if (!found)
        {
            mCommands.push_back(command);
        }

        // initialize the command syntax
        command->InitSyntax();

        // success
        return true;
    }

    Command* CommandManager::FindCommand(const AZStd::string& commandName)
    {
        auto iterator = mRegisteredCommands.find(commandName);
        if (iterator == mRegisteredCommands.end())
        {
            return nullptr;
        }

        return iterator->second;
    }

    // execute command object and store the history entry if the command is undoable
    bool CommandManager::ExecuteCommand(Command* command, AZStd::string& outCommandResult, bool addToHistory, bool clearErrors, bool handleErrors, bool autoDeleteCommand)
    {
        return ExecuteCommand(command, CommandLine(), outCommandResult, addToHistory, /*callFromCommandGroup=*/false, clearErrors, handleErrors, autoDeleteCommand);
    }

    bool CommandManager::ExecuteCommand(Command* command,
        const CommandLine& commandLine,
        AZStd::string& outCommandResult,
        bool addToHistory,
        bool callFromCommandGroup,
        bool clearErrors,
        bool handleErrors,
        bool autoDeleteCommand)
    {
#ifdef MCORE_COMMANDMANAGER_PERFORMANCE
        Timer commandTimer;
#endif

        if (!command)
        {
            return false;
        }

#ifdef MCORE_COMMANDMANAGER_PERFORMANCE
        Timer preCallbacksTimer;
#endif
        ++m_commandsInExecution;

        // Set the original command. This is needed for the command callbacks to function
        if (!command->GetOriginalCommand() || (command->GetOriginalCommand() == command))
        {
            MCore::Command* orgCommand = FindCommand(command->GetNameString());
            command->SetOriginalCommand(orgCommand);
        }

        // execute command manager callbacks
        for (CommandManagerCallback* managerCallback : mCallbacks)
        {
            managerCallback->OnPreExecuteCommand(nullptr, command, commandLine);
        }

        // execute pre-command callbacks
        ExecuteCommandCallbacks(command, commandLine, true);

#ifdef MCORE_COMMANDMANAGER_PERFORMANCE
        LogInfo("   + PreExecuteCallbacks(%d): %.2f ms.", command->GetOriginalCommand()->CalcNumPreCommandCallbacks(), preCallbacksTimer.GetTime() * 1000);
        Timer executeTimer;
#endif

        // execute the command object, get the result and reset it
        outCommandResult.clear();
        const bool result = command->Execute(commandLine, outCommandResult);

#ifdef MCORE_COMMANDMANAGER_PERFORMANCE
        LogInfo("   + Execution: %.2f ms.", executeTimer.GetTime() * 1000);
        Timer postCallbacksTimer;
#endif

        // if it was successful, execute all the post-command callbacks
        if (result)
        {
            ExecuteCommandCallbacks(command, commandLine, false);
        }

        // save command in the command history if it is undoable
        if (addToHistory && command->GetIsUndoable() && result)
        {
            PushCommandHistory(command, commandLine);
        }

        // execute all post execute callbacks
        for (CommandManagerCallback* managerCallback : mCallbacks)
        {
            managerCallback->OnPostExecuteCommand(nullptr, command, commandLine, result, outCommandResult);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        if (handleErrors && !mErrors.empty())
        {
            // Execute error report callbacks.
            for (CommandManagerCallback* managerCallback : mCallbacks)
            {
                managerCallback->OnShowErrorReport(mErrors);
            }
        }

        // Clear errors after reporting if specified.
        if (clearErrors)
        {
            mErrors.clear();
        }

#ifdef MCORE_COMMANDMANAGER_PERFORMANCE
        LogInfo("   + PostExecuteCallbacks(%d): %.2f ms.", command->GetOriginalCommand()->CalcNumPostCommandCallbacks(), postCallbacksTimer.GetTime() * 1000);
        LogInfo("   + Total time: %.2f ms.", commandTimer.GetTime() * 1000);
#endif
        --m_commandsInExecution;

        // Delete the command in case we
        if (autoDeleteCommand &&
            ShouldDeleteCommand(command, result, callFromCommandGroup, addToHistory))
        {
            delete command;
        }

        return result;
    }

    // log the command history to the console
    void CommandManager::LogCommandHistory()
    {
        LogDetailedInfo("----------------------------------");

        // get the number of entries in the command history
        const size_t numHistoryEntries = mCommandHistory.size();
        LogDetailedInfo("Command History (%d entries) - oldest (top entry) to newest (bottom entry):", numHistoryEntries);

        // print the command history entries
        for (size_t i = 0; i < numHistoryEntries; ++i)
        {
            AZStd::string text = AZStd::string::format("%.3zu: name='%s', num parameters=%u", i, mCommandHistory[i].mExecutedCommand->GetName(), mCommandHistory[i].mParameters.GetNumParameters());
            if (i == (uint32)mHistoryIndex)
            {
                LogDetailedInfo("-> %s", text.c_str());
            }
            else
            {
                LogDetailedInfo(text.c_str());
            }
        }

        LogDetailedInfo("----------------------------------");
    }

    void CommandManager::RemoveCallbacks()
    {
        for (CommandManagerCallback* managerCallback : mCallbacks)
        {
            delete managerCallback;
        }

        mCallbacks.clear();
    }

    void CommandManager::RegisterCallback(CommandManagerCallback* callback)
    {
        mCallbacks.push_back(callback);
    }

    void CommandManager::RemoveCallback(CommandManagerCallback* callback, bool delFromMem)
    {
        mCallbacks.erase(AZStd::remove(mCallbacks.begin(), mCallbacks.end(), callback), mCallbacks.end());

        if (delFromMem)
        {
            delete callback;
        }
    }

    size_t CommandManager::GetNumCallbacks() const
    {
        return mCallbacks.size();
    }

    CommandManagerCallback* CommandManager::GetCallback(size_t index)
    {
        return mCallbacks[index];
    }

    // set the max num history items
    void CommandManager::SetMaxHistoryItems(uint32 maxItems)
    {
        maxItems = AZStd::max(1u, maxItems);
        mMaxHistoryEntries = maxItems;

        while (mCommandHistory.size() > mMaxHistoryEntries)
        {
            PopCommandHistory();
            mHistoryIndex = static_cast<int32>(mCommandHistory.size()) - 1;
        }
    }

    size_t CommandManager::GetMaxHistoryItems() const
    {
        return mMaxHistoryEntries;
    }

    int32 CommandManager::GetHistoryIndex() const
    {
        return mHistoryIndex;
    }

    size_t CommandManager::GetNumHistoryItems() const
    {
        return mCommandHistory.size();
    }

    const CommandManager::CommandHistoryEntry& CommandManager::GetHistoryItem(size_t index) const
    {
        return mCommandHistory[index];
    }

    Command* CommandManager::GetHistoryCommand(size_t historyIndex)
    {
        return mCommandHistory[historyIndex].mExecutedCommand;
    }

    void CommandManager::ClearHistory()
    {
        // clear the command history
        while (!mCommandHistory.empty())
        {
            PopCommandHistory();
        }

        // reset the history index
        mHistoryIndex = -1;
    }

    const CommandLine& CommandManager::GetHistoryCommandLine(uint32 historyIndex) const
    {
        return mCommandHistory[historyIndex].mParameters;
    }

    size_t CommandManager::GetNumRegisteredCommands() const
    {
        return mCommands.size();
    }

    Command* CommandManager::GetCommand(size_t index)
    {
        return mCommands[index];
    }

    // delete the given callback from all commands
    void CommandManager::RemoveCommandCallback(Command::Callback* callback, bool delFromMem)
    {
        for (Command* command : mCommands)
        {
            command->RemoveCallback(callback, false); // false = don't delete from memory
        }
        if (delFromMem)
        {
            delete callback;
        }
    }

    // delete the callback from a given command
    void CommandManager::RemoveCommandCallback(const char* commandName, Command::Callback* callback, bool delFromMem)
    {
        // try to find the command
        Command* command = FindCommand(commandName);
        if (command)
        {
            command->RemoveCallback(callback, false); // false = don't delete from memory
        }
        if (delFromMem)
        {
            delete callback;
        }
    }

    // register a given callback to a command
    bool CommandManager::RegisterCommandCallback(const char* commandName, Command::Callback* callback)
    {
        // try to find the command
        Command* command = FindCommand(commandName);
        if (command == nullptr)
        {
            return false;
        }

        // if this command hasn't already registered this callback, add it
        if (command->CheckIfHasCallback(callback) == false)
        {
            command->AddCallback(callback);
            return true;
        }

        // command already has been registered
        return false;
    }

    // check if an error occurred and calls the error handling callbacks
    bool CommandManager::ShowErrorReport()
    {
        // Let the callbacks handle error reporting (e.g. show an error report window).
        const bool errorsOccured = !mErrors.empty();
        if (errorsOccured)
        {
            // Execute error report callbacks.
            for (CommandManagerCallback* managerCallback : mCallbacks)
            {
                managerCallback->OnShowErrorReport(mErrors);
            }
        }

        // clear errors after reporting
        mErrors.clear();

        return errorsOccured;
    }
} // namespace MCore
