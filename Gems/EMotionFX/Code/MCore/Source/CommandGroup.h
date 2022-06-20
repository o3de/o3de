/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include "StandardHeaders.h"
#include "CommandLine.h"
#include "Command.h"


namespace MCore
{
    /**
     * The command group class, which executes a set of commands internally, while showing just one item inside the command history.
     * Undoing a command group would undo all commands inside the group.
     */
    class MCORE_API CommandGroup
    {
        MCORE_MEMORYOBJECTCATEGORY(Command, MCore::MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_COMMANDSYSTEM);

    public:
        /**
         * Default constructor.
         * You can set the name of the group with SetGroupName() manually.
         */
        CommandGroup();

        /**
         * The constructor.
         * @param groupName The name of the command group, which is the name that will appear inside the command history.
         * @param numCommandsToReserve Pre-allocate memory for a given amount of commands. This can be used to prevent memory reallocations.
         */
        CommandGroup(const AZStd::string& groupName, size_t numCommandsToReserve = 3);

        /**
         * The destructor.
         * Please note that this does NOT delete the actual Command objects after the group has been executed. They are deleted by the command manager automatically.
         */
        ~CommandGroup();

        /**
         * Add a command string to the group.
         * This will be added to the back of the list of commands to be executed when executing this group.
         * @param commandString The full command string, like you would execute using CommandManager::ExecuteCommand(...).
         */
        void AddCommandString(const char* commandString);

        void AddCommand(MCore::Command* command);

        /**
         * Add a command string to the group.
         * This will be added to the back of the list of commands to be executed when executing this group.
         * @param commandString The full command string, like you would execute using CommandManager::ExecuteCommand(...).
         */
        void AddCommandString(const AZStd::string& commandString);

        /**
         * Get the command execution string for a given command inside the group.
         * @param index The command number to get the string for. This must be in range of [0..GetNumCommands()-1].
         * @result The command string for the specified command.
         */
        const char* GetCommandString(size_t index) const;

        /**
         * Get the command execution string for a given command inside the group.
         * @param index The command number to get the string for. This must be in range of [0..GetNumCommands()-1].
         * @result The command string for the specified command.
         */
        const AZStd::string& GetCommandStringAsString(size_t index) const;

        /**
         * Get a given command.
         * The return value can be nullptr in case the group hasn't yet been executed or when the execution of this command failed.
         * @param index The command number to get the pointer to. This must be in range of [0..GetNumCommands()-1].
         * @result A pointer to the command, which can be nullptr.
         */
        Command* GetCommand(size_t index);

        /**
         * Get the parameter commandline object for the given command.
         * This is only set after executing the command. Otherwise the parameter list will be empty.
         * @param index The command number ot get the parameter list for. This must be in range of [0..GetNumCommands()-1].
         * @result The CommandLine object which represents the parameter list used during execution of the command.
         */
        const CommandLine& GetParameters(size_t index) const;

        /**
         * Get the name of the group.
         * This is the name that will appear inside the command history also.
         * @result The name of the group.
         */
        const char* GetGroupName() const;

        /**
         * Get the group name in form of a string object.
         * This is the name that will appear inside the command history also.
         * @result The name of the group.
         */
        const AZStd::string& GetGroupNameString() const;

        /**
         * Set the name of the group, which is the name as it will appear inside the command history.
         * @param groupName The name of the group.
         */
        void SetGroupName(const char* groupName);

        /**
         * Set the name of the group, which is the name as it will appear inside the command history.
         * @param groupName The name of the group.
         */
        void SetGroupName(const AZStd::string& groupName);

        /**
         * Set the command line string for a given command.
         * @param index The command number to change the command string for. This must be in range of [0..GetNumCommands()-1].
         * @param commandString The command string to use. This is the string you would pass to CommandManager::ExecuteCommand(...).
         */
        void SetCommandString(size_t index, const char* commandString);

        /**
         * Set the parameter list for a given command.
         * Normally you will not be using this function as this is automatically set by the command manager.
         * @param index The command number to change the parameter list for. This must be in range of [0..GetNumCommands()-1]. Please note that this isn't needed to be set normally.
         *              With normal usage you would only specify the command string and nothing more.
         * @param params The parameter list.
         */
        void SetParameters(size_t index, const CommandLine& params);

        /**
         * Set the command pointer for a given command number.
         * Normally you will not be using this function as this is automatically set by the command manager.
         * @param index The command number to set the command pointer for. This must be in range of [0..GetNumCommands()-1].
         * @param command The command to use.
         */
        void SetCommand(size_t index, Command* command);

        /**
         * Set if we want to continue when one of the internal commands fails.
         * If not, it will stop executing further commands.
         * The default value is true.
         * @param continueAfter Set to true to continue even after an error, or false to stop when one of the commands fails to execute.
         */
        void SetContinueAfterError(bool continueAfter);

        /**
         * Set whether to add the group to the command history if one of the commands failed to execute or not.
         * The default value is true.
         * @param addAfterError Set to true if you wish the group to be added to the command history, even if an error occurred.
         */
        void SetAddToHistoryAfterError(bool addAfterError);

        /**
         * Set whether the group returns false after calling CommandManager::ExecuteCommandGroup().
         * The default value is false.
         * @param returnAfterError Set to true if you wish the group to fail and return false in case an error occurred.
         */
        void SetReturnFalseAfterError(bool returnAfterError);

        /**
         * Check if we want to continue when one of the internal commands fails.
         * If not, it will stop executing further commands.
         * The default is true.
         * @result Returns false when the internal command execution will stop at the first error, otherwise true is returned.
         */
        bool GetContinueAfterError() const;

        /**
         * Check whether this command group will be added to the history, even when one of the internal commands fails to execute.
         * The default is true.
         * @result Returns false when the group will not be added to the history when one of the internal commands fails to execute, otherwise true is returned.
         */
        bool GetAddToHistoryAfterError() const;

        /**
         * Check whether the group returns false after calling CommandManager::ExecuteCommandGroup().
         * The default value is false.
         * @return Returns true if the group return false in case an error occurred.
         */
        bool GetReturnFalseAfterError() const;

        /**
         * Get the number of commands inside this group.
         * Using AddCommandString will increase the number returned by this method.
         * @result The number of commands inside this group.
         */
        size_t GetNumCommands() const;

        bool IsEmpty() const { return GetNumCommands() == 0; }

        /**
         * Remove all commands from the group.
         * @param delFromMem Set to true if you wish to delete the Command objects from memory as well. Normally you shouldn't do this as the command manager handles this.
         */
        void RemoveAllCommands(bool delFromMem = false);
        void Clear(bool delFromMem = false) { RemoveAllCommands(delFromMem); }

        /**
         * Clone this command group.
         * @result The clone of this command group.
         */
        CommandGroup* Clone() const;

        /**
         * Reserve space for a given amount of commands, to prevent reallocs.
         * @param[in] numToReserve The number of command strings to reserve memory for. This will prevent reallocs when adding new commands.
         */
        void ReserveCommands(size_t numToReserve);

    private:
        /**
         * The command entry.
         * This contains information about the commands that have to be executed, or information needed to undo and redo the commands.
         */
        struct MCORE_API CommandEntry
        {
            MCore::Command*     m_command = nullptr; /**< The command object, which gets set when you execute the group inside the command manager. */
            MCore::CommandLine  m_commandLine{};     /**< The command line that was used when executing this command. */
            AZStd::string       m_commandString{};   /**< The command string that we will execute. */
        };

        AZStd::vector<CommandEntry> m_commands;          /**< The set of commands inside the group. */
        AZStd::string           m_groupName;             /**< The name of the group as it will appear inside the command history. */
        bool                    m_continueAfterError;    /**< */
        bool                    m_historyAfterError;     /**< */
        bool                    m_returnFalseAfterError;
    };
} // namespace MCore
