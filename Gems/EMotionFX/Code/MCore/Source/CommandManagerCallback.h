/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include "StandardHeaders.h"
#include "CommandLine.h"


namespace MCore
{
    // forward declarations
    class Command;
    class CommandGroup;

    /**
     * The command manager callback class.
     * Specific events are triggered by the command manager through this callback class.
     * For example you can get notified when a given command is being executed. This can be used to for example link the command manager to a graphical user interface.
     * It could show the command history in which the user can step back and forth to undo and redo commands.
     */
    class MCORE_API CommandManagerCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(CommandManagerCallback, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_COMMANDSYSTEM)

    public:
        /**
         * The constructor.
         */
        CommandManagerCallback()  {}

        /**
         * The destructor.
         */
        virtual ~CommandManagerCallback() {}

        /**
         * This callback is executed before we are going to execute a given command.
         * @param group The group that is about to be executed, or nullptr when it is not a group but a regular command.
         * @param command The command that is about to be executed.
         * @param commandLine The command line that is going to be used when executing the command.
         */
        virtual void OnPreExecuteCommand(CommandGroup* group, Command* command, const CommandLine& commandLine) = 0;

        /**
         * This callback is executed after we have executed a given command.
         * @param group The group that just executed, or nullptr when it is not a group but a regular command.
         * @param command The command that has just been executed.
         * @param commandLine The command line that was used when executing the command.
         * @param wasSuccess This is set to true when the command execution was successful, or false when it failed.
         * @param outResult The string that contains the result of the execution, or the error when the execution failed.
         */
        virtual void OnPostExecuteCommand(CommandGroup* group, Command* command, const CommandLine& commandLine, bool wasSuccess, const AZStd::string& outResult) = 0;

        virtual void OnPreUndoCommand(MCore::Command* command, const MCore::CommandLine& commandLine) { AZ_UNUSED(command); AZ_UNUSED(commandLine); }
        virtual void OnPostUndoCommand(MCore::Command* command, const MCore::CommandLine& commandLine) { AZ_UNUSED(command); AZ_UNUSED(commandLine); }

        /**
         * This callback is executed before we are going to execute a given command group.
         * @param group The group that is about to be executed, or nullptr when it is not a group but a regular command.
         * @param undo True in case the command group has been executed already and is now being undone, false in case the command group is normally executed.
         */
        virtual void OnPreExecuteCommandGroup(CommandGroup* group, bool undo) = 0;

        /**
         * This callback is executed after we have executed a given command group.
         * @param group The group that just executed, or nullptr when it is not a group but a regular command.
         * @param wasSuccess This is set to true when the command execution was successful, or false when it failed.
         */
        virtual void OnPostExecuteCommandGroup(CommandGroup* group, bool wasSuccess) = 0;

        /**
         * This callback is executed when a new item is added to the command history.
         * @param historyIndex The history index where the new item was added.
         * @param group The group that was added to the history list, or nullptr when it is not a group but a regular command.
         * @param command The command that is linked with this history item.
         * @param commandLine The command line that is linked to this history item.
         */
        virtual void OnAddCommandToHistory(size_t historyIndex, CommandGroup* group, Command* command, const CommandLine& commandLine) = 0;

        /**
         * This callback is executed when a command is being removed from the command history.
         * @param historyIndex The history index of the command that is being removed.
         */
        virtual void OnRemoveCommand(size_t historyIndex) = 0;

        /**
         * This callback is executed when we step back or forth in the command history.
         * @param index The new history index which will be the current state the system will be in.
         */
        virtual void OnSetCurrentCommand(size_t index) = 0;

        /**
         * This callback is executed before the error array is getting cleared and the interfaces shall show some error reporting window or something similar.
         * @errors[in] errors The errors that happened and shall be made visible to the user.
         */
        virtual void OnShowErrorReport(const AZStd::vector<AZStd::string>& errors) { MCORE_UNUSED(errors); }
    };
} // namespace MCore
