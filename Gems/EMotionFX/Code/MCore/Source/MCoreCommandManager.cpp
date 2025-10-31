/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MCoreCommandManager.h"

#include <AzCore/Serialization/Locale.h>

#include "LogManager.h"
#include "CommandManagerCallback.h"
#include "StringConversions.h"

//#define MCORE_COMMANDMANAGER_PERFORMANCE

namespace MCore
{
    CommandManager::CommandHistoryEntry::CommandHistoryEntry(CommandGroup* group, Command* command, const CommandLine& parameters, size_t historyItemNr)
    {
        m_commandGroup       = group;
        m_executedCommand    = command;
        m_parameters         = parameters;
        m_historyItemNr     = historyItemNr;
    }

    CommandManager::CommandHistoryEntry::~CommandHistoryEntry()
    {
    }

    AZStd::string CommandManager::CommandHistoryEntry::ToString(CommandGroup* group, Command* command, size_t historyItemNr)
    {
        if (group)
        {
            return AZStd::string::format("%.3zu - %s", historyItemNr, group->GetGroupName());
        }
        else if (command)
        {
            return AZStd::string::format("%.3zu - %s", historyItemNr, command->GetHistoryName());
        }

        return "";
    }

    AZStd::string CommandManager::CommandHistoryEntry::ToString() const
    {
        return ToString(m_commandGroup, m_executedCommand, m_historyItemNr);
    }

    CommandManager::CommandManager()
    {
        m_commands.reserve(128);

        // set default values
        m_maxHistoryEntries      = 100;
        m_historyIndex           = -1;
        m_totalNumHistoryItems   = 0;

        // preallocate history entries
        m_commandHistory.reserve(m_maxHistoryEntries);

        m_commandsInExecution = 0;
    }

    CommandManager::~CommandManager()
    {
        for (auto element : m_registeredCommands)
        {
            Command* command = element.second;
            delete command;
        }
        m_registeredCommands.clear();

        // remove all callbacks
        RemoveCallbacks();

        // destroy the command history
        while (!m_commandHistory.empty())
        {
            PopCommandHistory();
        }
    }

    // push a command group on the history
    void CommandManager::PushCommandHistory(CommandGroup* commandGroup)
    {
        // if we reached the maximum number of history entries remove the oldest one
        if (m_commandHistory.size() >= m_maxHistoryEntries)
        {
            PopCommandHistory();
            m_historyIndex = static_cast<ptrdiff_t>(m_commandHistory.size()) - 1;
        }

        if (!m_commandHistory.empty())
        {
            // remove unneeded commandsnumToRemove
            const size_t numToRemove = m_commandHistory.size() - m_historyIndex - 1;
            for (CommandManagerCallback* managerCallback : m_callbacks)
            {
                for (size_t a = 0; a < numToRemove; ++a)
                {
                    managerCallback->OnRemoveCommand(m_historyIndex + 1);
                }
            }

            for (size_t a = 0; a < numToRemove; ++a)
            {
                delete m_commandHistory[m_historyIndex + 1].m_executedCommand;
                delete m_commandHistory[m_historyIndex + 1].m_commandGroup;
                m_commandHistory.erase(m_commandHistory.begin() + m_historyIndex + 1);
            }
        }

        // resize the command history
        m_commandHistory.resize(m_historyIndex + 1);

        // add a command history entry
        m_totalNumHistoryItems++;
        m_commandHistory.push_back(CommandHistoryEntry(commandGroup, nullptr, CommandLine(), m_totalNumHistoryItems));

        // increase the history index
        m_historyIndex++;

        // perform callbacks
        for (CommandManagerCallback* managerCallback : m_callbacks)
        {
            managerCallback->OnAddCommandToHistory(m_historyIndex, commandGroup, nullptr, CommandLine());
        }
    }

    // save command in the history
    void CommandManager::PushCommandHistory(Command* command, const CommandLine& parameters)
    {
        if (!m_commandHistory.empty())
        {
            // if we reached the maximum number of history entries remove the oldest one
            if (m_commandHistory.size() >= m_maxHistoryEntries)
            {
                PopCommandHistory();
                m_historyIndex = static_cast<ptrdiff_t>(m_commandHistory.size()) - 1;
            }

            // remove unneeded commands
            const size_t numToRemove = m_commandHistory.size() - m_historyIndex - 1;
            for (CommandManagerCallback* managerCallback : m_callbacks)
            {
                for (size_t a = 0; a < numToRemove; ++a)
                {
                    managerCallback->OnRemoveCommand(m_historyIndex + 1);
                }
            }

            for (size_t a = 0; a < numToRemove; ++a)
            {
                delete m_commandHistory[m_historyIndex + 1].m_executedCommand;
                delete m_commandHistory[m_historyIndex + 1].m_commandGroup;
                m_commandHistory.erase(m_commandHistory.begin() + m_historyIndex + 1);
            }
        }

        // resize the command history
        m_commandHistory.resize(m_historyIndex + 1);

        // add a command history entry
        m_totalNumHistoryItems++;
        m_commandHistory.push_back(CommandHistoryEntry(nullptr, command, parameters, m_totalNumHistoryItems));

        // increase the history index
        m_historyIndex++;

        // perform callbacks
        for (CommandManagerCallback* managerCallback : m_callbacks)
        {
            managerCallback->OnAddCommandToHistory(m_historyIndex, nullptr, command, parameters);
        }
    }

    // remove the oldest entry in the history stack
    void CommandManager::PopCommandHistory()
    {
        // perform callbacks
        for (CommandManagerCallback* managerCallback : m_callbacks)
        {
            managerCallback->OnRemoveCommand(0);
        }

        // destroy the command and remove it from the command history
        Command* command = m_commandHistory.front().m_executedCommand;
        if (command)
        {
            delete command;
        }
        else
        {
            delete m_commandHistory.front().m_commandGroup;
        }

        m_commandHistory.erase(m_commandHistory.begin());
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
        for (CommandManagerCallback* managerCallback : m_callbacks)
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
                    if (static_cast<ptrdiff_t>(i) < relativeIndex)
                    {
                        MCore::LogError("Execution of command '%s' failed, command trying to access results from %d commands back, but there are only %d", commandString.c_str(), relativeIndex, i - 1);
                        hadError = true;
                        break;
                    }

                    tmpStr = commandString.substr(lastResultIndex, rightPercentagePos - lastResultIndex + 1);
                    AZStd::string replaceStr = intermediateCommandResults[i - relativeIndex];
                    if (replaceStr.empty())
                    {
                        replaceStr = "-1";
                    }
                    AzFramework::StringFunc::Replace(commandString, tmpStr.c_str(), replaceStr.c_str());
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
                        for (CommandManagerCallback* managerCallback : m_callbacks)
                        {
                            managerCallback->OnPostExecuteCommandGroup(&commandGroup, false);
                        }

                        // Let the callbacks handle error reporting (e.g. show an error report window).
                        if (handleErrors && !m_errors.empty())
                        {
                            // Execute error report callbacks.
                            for (CommandManagerCallback* managerCallback : m_callbacks)
                            {
                                managerCallback->OnShowErrorReport(m_errors);
                            }
                        }

                        // Clear errors after reporting if specified.
                        if (clearErrors)
                        {
                            m_errors.clear();
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
        for (CommandManagerCallback* managerCallback : m_callbacks)
        {
            managerCallback->OnPostExecuteCommandGroup(&commandGroup, true);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        const bool errorsOccured = !m_errors.empty();
        if (handleErrors && errorsOccured)
        {
            // Execute error report callbacks.
            for (CommandManagerCallback* managerCallback : m_callbacks)
            {
                managerCallback->OnShowErrorReport(m_errors);
            }
        }

        // Return the result of the last command
        // TODO: once we convert to AZStd::string we can do a swap here
        outCommandResult = intermediateCommandResults.back();

        // Clear errors after reporting if specified.
        if (clearErrors)
        {
            m_errors.clear();
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
        size_t      numFailed   = 0;

        // get the number of callbacks and iterate through them
        const size_t numCommandCallbacks = orgCommand->GetNumCallbacks();
        for (size_t i = 0; i < numCommandCallbacks; ++i)
        {
            // get the current callback
            Command::Callback* callback = orgCommand->GetCallback(i);

            if (preUndo)
            {
                for (CommandManagerCallback* managerCallback : m_callbacks)
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
                for (CommandManagerCallback* managerCallback : m_callbacks)
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
        size_t      numFailed   = 0;

        // get the number of callbacks and iterate through them
        const size_t numCommandCallbacks = orgCommand->GetNumCallbacks();
        for (size_t i = 0; i < numCommandCallbacks; ++i)
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
        if (m_commandHistory.empty() && m_historyIndex >= 0)
        {
            outCommandResult = "Cannot undo command. The command history is empty";
            return false;
        }

        // get the last called command from the command history
        const CommandHistoryEntry& lastEntry = m_commandHistory[m_historyIndex];
        Command* command = lastEntry.m_executedCommand;

        ++m_commandsInExecution;

        // if we are dealing with a regular command
        bool result = true;
        if (command)
        {
            // execute pre-undo callbacks
            ExecuteUndoCallbacks(command, lastEntry.m_parameters, true);

            // undo the command, get the result and reset it
            result = command->Undo(lastEntry.m_parameters, outCommandResult);

            // execute post-undo callbacks
            ExecuteUndoCallbacks(command, lastEntry.m_parameters, false);
        }
        // we are dealing with a command group
        else
        {
            CommandGroup* group = lastEntry.m_commandGroup;
            MCORE_ASSERT(group);

            // perform callbacks
            for (CommandManagerCallback* managerCallback : m_callbacks)
            {
                managerCallback->OnPreExecuteCommandGroup(group, true);
            }

            const ptrdiff_t numCommands = static_cast<ptrdiff_t>(group->GetNumCommands()) - 1;
            for (ptrdiff_t g = numCommands; g >= 0; --g)
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
            for (CommandManagerCallback* managerCallback : m_callbacks)
            {
                managerCallback->OnPostExecuteCommandGroup(group, result);
            }
        }

        // go one step back in the command history
        m_historyIndex--;

        // perform callbacks
        for (CommandManagerCallback* managerCallback : m_callbacks)
        {
            managerCallback->OnSetCurrentCommand(m_historyIndex);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        if (!m_errors.empty())
        {
            for (CommandManagerCallback* managerCallback : m_callbacks)
            {
                managerCallback->OnShowErrorReport(m_errors);
            }
            m_errors.clear();
        }

        --m_commandsInExecution;

        return result;
    }

    // redo the last undoed command
    bool CommandManager::Redo(AZStd::string& outCommandResult)
    {
        // get the last called command from the command history
        const CommandHistoryEntry& lastEntry = m_commandHistory[m_historyIndex + 1];

        // if we just redo one single command
        bool result = true;
        if (lastEntry.m_executedCommand)
        {
            // redo the command, get the result and reset it
            result = ExecuteCommand(lastEntry.m_executedCommand,
                lastEntry.m_parameters,
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
            CommandGroup* group = lastEntry.m_commandGroup;
            AZ_Assert(group, "Cannot redo. Command group is not valid.");

            for (CommandManagerCallback* managerCallback : m_callbacks)
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

            for (CommandManagerCallback* managerCallback : m_callbacks)
            {
                managerCallback->OnPostExecuteCommandGroup(group, result);
            }
        }

        // go one step forward in the command history
        m_historyIndex++;

        // perform callbacks
        for (CommandManagerCallback* managerCallback : m_callbacks)
        {
            managerCallback->OnSetCurrentCommand(m_historyIndex);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        if (!m_errors.empty())
        {
            for (CommandManagerCallback* managerCallback : m_callbacks)
            {
                managerCallback->OnShowErrorReport(m_errors);
            }
            m_errors.clear();
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
        m_registeredCommands.insert(AZStd::make_pair(command->GetNameString(), command));

        // we're going to insert the command in a sorted way now
        bool found = false;
        const size_t numCommands = m_commands.size();
        for (size_t i = 0; i < numCommands; ++i)
        {
            if (azstricmp(m_commands[i]->GetName(), command->GetName()) > 0)
            {
                found = true;
                m_commands.insert(m_commands.begin() + i, command);
                break;
            }
        }

        // if no insert location has been found, add it to the back of the array
        if (!found)
        {
            m_commands.push_back(command);
        }

        // initialize the command syntax
        command->InitSyntax();

        // success
        return true;
    }

    Command* CommandManager::FindCommand(const AZStd::string& commandName)
    {
        auto iterator = m_registeredCommands.find(commandName);
        if (iterator == m_registeredCommands.end())
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
        AZ::Locale::ScopedSerializationLocale localeScope;  // make sure '%f' uses the "C" Locale.

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
        for (CommandManagerCallback* managerCallback : m_callbacks)
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
        for (CommandManagerCallback* managerCallback : m_callbacks)
        {
            managerCallback->OnPostExecuteCommand(nullptr, command, commandLine, result, outCommandResult);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        if (handleErrors && !m_errors.empty())
        {
            // Execute error report callbacks.
            for (CommandManagerCallback* managerCallback : m_callbacks)
            {
                managerCallback->OnShowErrorReport(m_errors);
            }
        }

        // Clear errors after reporting if specified.
        if (clearErrors)
        {
            m_errors.clear();
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
        const size_t numHistoryEntries = m_commandHistory.size();
        LogDetailedInfo("Command History (%d entries) - oldest (top entry) to newest (bottom entry):", numHistoryEntries);

        // print the command history entries
        for (size_t i = 0; i < numHistoryEntries; ++i)
        {
            AZStd::string text = AZStd::string::format("%.3zu: name='%s', num parameters=%zu", i, m_commandHistory[i].m_executedCommand->GetName(), m_commandHistory[i].m_parameters.GetNumParameters());
            if (i == static_cast<size_t>(m_historyIndex))
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
        for (CommandManagerCallback* managerCallback : m_callbacks)
        {
            delete managerCallback;
        }

        m_callbacks.clear();
    }

    void CommandManager::RegisterCallback(CommandManagerCallback* callback)
    {
        m_callbacks.push_back(callback);
    }

    void CommandManager::RemoveCallback(CommandManagerCallback* callback, bool delFromMem)
    {
        m_callbacks.erase(AZStd::remove(m_callbacks.begin(), m_callbacks.end(), callback), m_callbacks.end());

        if (delFromMem)
        {
            delete callback;
        }
    }

    size_t CommandManager::GetNumCallbacks() const
    {
        return m_callbacks.size();
    }

    CommandManagerCallback* CommandManager::GetCallback(size_t index)
    {
        return m_callbacks[index];
    }

    // set the max num history items
    void CommandManager::SetMaxHistoryItems(size_t maxItems)
    {
        maxItems = AZStd::max(size_t{1}, maxItems);
        m_maxHistoryEntries = maxItems;

        while (m_commandHistory.size() > m_maxHistoryEntries)
        {
            PopCommandHistory();
            m_historyIndex = static_cast<ptrdiff_t>(m_commandHistory.size()) - 1;
        }
    }

    size_t CommandManager::GetMaxHistoryItems() const
    {
        return m_maxHistoryEntries;
    }

    ptrdiff_t CommandManager::GetHistoryIndex() const
    {
        return m_historyIndex;
    }

    size_t CommandManager::GetNumHistoryItems() const
    {
        return m_commandHistory.size();
    }

    const CommandManager::CommandHistoryEntry& CommandManager::GetHistoryItem(size_t index) const
    {
        return m_commandHistory[index];
    }

    Command* CommandManager::GetHistoryCommand(size_t historyIndex)
    {
        return m_commandHistory[historyIndex].m_executedCommand;
    }

    void CommandManager::ClearHistory()
    {
        // clear the command history
        while (!m_commandHistory.empty())
        {
            PopCommandHistory();
        }

        // reset the history index
        m_historyIndex = -1;
    }

    const CommandLine& CommandManager::GetHistoryCommandLine(size_t historyIndex) const
    {
        return m_commandHistory[historyIndex].m_parameters;
    }

    size_t CommandManager::GetNumRegisteredCommands() const
    {
        return m_commands.size();
    }

    Command* CommandManager::GetCommand(size_t index)
    {
        return m_commands[index];
    }

    // delete the given callback from all commands
    void CommandManager::RemoveCommandCallback(Command::Callback* callback, bool delFromMem)
    {
        for (Command* command : m_commands)
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
        const bool errorsOccured = !m_errors.empty();
        if (errorsOccured)
        {
            // Execute error report callbacks.
            for (CommandManagerCallback* managerCallback : m_callbacks)
            {
                managerCallback->OnShowErrorReport(m_errors);
            }
        }

        // clear errors after reporting
        m_errors.clear();

        return errorsOccured;
    }
} // namespace MCore
