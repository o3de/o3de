/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include "StandardHeaders.h"
#include <AzCore/std/containers/vector.h>
#include "Command.h"
#include "CommandGroup.h"


namespace MCore
{
    // forward declarations
    class CommandManagerCallback;

    /**
     * The command manager.
     * This manager has a set of registered commands (MCore::Command) that can be executed and undo-ed and redo-ed through this manager.
     * Also it keeps a command history. It also supports the usage of command groups, which batch sets of commands as single commands inside the command history.
     */
    class MCORE_API CommandManager
    {
        MCORE_MEMORYOBJECTCATEGORY(CommandManager, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_COMMANDSYSTEM)

    public:
        /**
         * The command history entry stores all information which is relevant for a called command.
         * This information is needed for the undo/redo usage.
         */
        struct MCORE_API CommandHistoryEntry
        {
            CommandHistoryEntry()
                : m_commandGroup(nullptr)
                , m_executedCommand(nullptr)
            {}

            /**
             * Extended Constructor.
             * @param group The command group pointer. When set to nullptr it will use the single command object instead of the group.
             * @param command The command instance that has been created at execution time. When set to nullptr it will assume it is a group, and it will use the group you specified.
             * @param parameters The command arguments.
             */
            CommandHistoryEntry(CommandGroup* group, Command* command, const CommandLine& parameters, size_t historyItemNr);

            ~CommandHistoryEntry();

            static AZStd::string ToString(CommandGroup* group, Command* command, size_t historyItemNr);
            AZStd::string ToString() const;

            CommandGroup*   m_commandGroup;          /**< A pointer to the command group, or nullptr when no group is used (in that case it uses a single command). */
            Command*        m_executedCommand;       /**< A pointer to the command object, or nullptr when no command is used (in that case it uses a group). */
            CommandLine     m_parameters;            /**< The used command arguments, unused in case no command is used (in that case it uses a group). */
            size_t          m_historyItemNr;        /**< The global history item number. This number will neither change depending on the size of the history queue nor with undo/redo. */
        };


        /**
         * The default constructor.
         */
        CommandManager();

        /**
         * The destructor.
         */
        virtual ~CommandManager();

        /**
         * Execute a command.
         * @param command The command string including all arguments.
         * @param outCommandResult The return/result value of the command.
         * @param addToHistory When set to true it is being added to the command history and can be undone.
         * @param outExecutedCommand This will contain the pointer to the command that was executed, or nullptr when no command was excuted. Please note that the pointer
         *                           that will be stored in this parameter will be invalid when the command failed to execute.
         * @param outExecutedParamters This will contain the command line parameter list of the executed command.
         * @param[in] callFromCommandGroup True in case the command is called from a command group. False in case the command to be called is not part of a command group and called only by itself.
         * @return True if the command succeeded, false if not.
         */
        bool ExecuteCommand(const char* command, AZStd::string& outCommandResult, bool addToHistory = true, Command** outExecutedCommand = nullptr, CommandLine* outExecutedParamters = nullptr, bool callFromCommandGroup = false, bool clearErrors = true, bool handleErrors = true);
        bool ExecuteCommand(const AZStd::string& command, AZStd::string& outCommandResult, bool addToHistory = true, Command** outExecutedCommand = nullptr, CommandLine* outExecutedParamters = nullptr, bool callFromCommandGroup = false, bool clearErrors = true, bool handleErrors = true);
        bool ExecuteCommand(Command* command,
            AZStd::string& outCommandResult,
            bool addToHistory = true,
            bool clearErrors = true,
            bool handleErrors = true,
            bool autoDeleteCommand = true);
        bool ExecuteCommandInsideCommand(const char* command, AZStd::string& outCommandResult);
        bool ExecuteCommandInsideCommand(const AZStd::string& command, AZStd::string& outCommandResult);
        bool ExecuteCommandInsideCommand(Command* command, AZStd::string& outCommandResult);

        bool ExecuteCommandOrAddToGroup(const AZStd::string& command, MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);
        bool ExecuteCommandOrAddToGroup(Command* command, MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

        /**
         * Execute a command group.
         * The group will contain a set of commands inside it which are seen as one command.
         * This is useful when you want to for example execute 10 commands, but only show these 10 commands inside the command history as one grouped command.
         * Undoing the group will undo all commands that are part of the group..
         * @param commandGroup The command group to execute.
         * @param outCommandResult The result of the group execution. This generally contains the last successful or failed command status.
         * @param addToHistory Set to true when you want to add this group to the history.
         * @result Returns true when ALL commands inside the group executed successfully, otherwise false is returned.
         */
        bool ExecuteCommandGroup(CommandGroup& commandGroup, AZStd::string& outCommandResult, bool addToHistory = true, bool clearErrors = true, bool handleErrors = true);
        bool ExecuteCommandGroupInsideCommand(CommandGroup& commandGroup, AZStd::string& outCommandResult);

        /**
         * Undo the last executed command in the command history.
         * @param outCommandResult The return/result value of the command.
         * @return True if the undo succeeded, false if not.
         */
        bool Undo(AZStd::string& outCommandResult);

        /**
         * Redo the last command which has been undoed.
         * @param outCommandResult The return/result value of the command.
         * @return True if the redo succeeded, false if not.
         */
        bool Redo(AZStd::string& outCommandResult);

        /**
         * Register a command to the command manager. Each command has to be registered using this function before
         * using it. You can just new a command object when calling this function. The command manager will keep track
         * of the used memory and will destroy it automatically. E.g. MCORE_COMMANDMANAGER.RegisterCommand( new CreateBoxCommand() );.
         * @param command The command to register.
         * @result True if command registration succeeded, false it not.
         */
        bool RegisterCommand(Command* command);

        /**
         * Debug function to log the current command history.
         */
        void LogCommandHistory();

        /**
         * Find the command in the hash table.
         * @param commandName The name of the command.
         * @return A pointer to the command object, nullptr if it hasn't been found.
         */
        Command* FindCommand(const AZStd::string& commandName);

        /**
         * Remove and delete all callbacks from memory.
         * Automatically called by the destructor.
         */
        void RemoveCallbacks();

        /**
         * Register a callback.
         * This increases the number returned by GetNumCallbacks().
         * @param callback The callback to add to the list of registered callbacks.
         */
        void RegisterCallback(CommandManagerCallback* callback);

        /**
         * Remove a given callback from the manager.
         * @param callback The callback to remove.
         * @param delFromMem Set to true when you wish to also delete the callback object from memory.
         */
        void RemoveCallback(CommandManagerCallback* callback, bool delFromMem = true);

        /**
         * Get the number of registered callbacks.
         * @result The number of registered callbacks.
         */
        size_t GetNumCallbacks() const;

        /**
         * Get a given callback.
         * @param index The callback number to get, which must be in range of [0..GetNumCallbacks()-1].
         */
        CommandManagerCallback* GetCallback(size_t index);

        /**
         * Set the maximum number of history items that the manager should remember.
         * On default this value is 100. This means it will remember the last 100 executed commands, which can then be undo-ed and redo-ed.
         * @param maxItems The maximum number of items to remember.
         */
        void SetMaxHistoryItems(size_t maxItems);

        /**
         * Get the maximum number of history items that the manager will remember.
         * On default this value is 100. This means it will remember the last 100 executed commands, which can then be undo-ed and redo-ed.
         * @result The number of maximum history items.
         */
        size_t GetMaxHistoryItems() const;

        /**
         * Get the current history index.
         * This value will be in range of [0..GetMaxHistoryItems()-1].
         * @result The current history index.
         */
        ptrdiff_t GetHistoryIndex() const;

        /**
         * Get the number of history items stored.
         * This is the number of executed commands that are stored in the history right now.
         * @result The number of history items currently stored.
         */
        size_t GetNumHistoryItems() const;

        const CommandHistoryEntry& GetHistoryItem(size_t index) const;

        /**
         * Get a given command from the command history.
         * @param historyIndex The history index number, which must be in range of [0..GetNumHistoryItems()-1].
         * @result A pointer to the command stored at the given history index.
         */
        Command* GetHistoryCommand(size_t historyIndex);

        /**
         * Clear the history.
         */
        void ClearHistory();

        /**
         * Get the command line for a given history item.
         * @param historyIndex The history index number, which must be in range of [0..GetNumHistoryItems()-1].
         * @result A reference to the command line that was used when executing this command.
         */
        const CommandLine& GetHistoryCommandLine(size_t historyIndex) const;

        /**
         * Get the total number of registered commands.
         * @result The number of registered commands.
         */
        size_t GetNumRegisteredCommands() const;

        /**
         * Get a given registered command.
         * @param index The command number, which must be in range of [0..GetNumRegisteredCommands()-1].
         * @result A pointer to the command.
         */
        Command* GetCommand(size_t index);

        /**
         * Remove a given command callback. This automatically finds the command where this callback has been added to and removes it from that.
         * @param callback The callback to remove.
         * @param delFromMem When set to true, the specified callback will be deleted from memory as well (even when not registered at any of the commands).
         */
        void RemoveCommandCallback(Command::Callback* callback, bool delFromMem);

        /**
         * Remove a given command callback from a given command with a given name.
         * @param commandName The non-case-sentive name of the command to remove the callback from.
         * @param callback The callback to remove from the command with the specified name.
         * @param delFromMem When set to true, the specified callback will be deleted from memory as well (even when not registered at any of the commands).
         */
        void RemoveCommandCallback(const char* commandName, Command::Callback* callback, bool delFromMem);

        /**
         * Register (add) a command callback to a given command with a given name.
         * @param commandName The non-case-sensitive name of the command where we should add this callback to.
         * @param callback The callback to add to the given command.
         * @result Returns true when added successfully or false when there is no command with the specified name.
         */
        bool RegisterCommandCallback(const char* commandName, Command::Callback* callback);

        template<typename T, typename... Args>
        bool RegisterCommandCallback(const char* commandName, AZStd::vector<Command::Callback*>& callbacks, Args... args)
        {
            Command::Callback* callback = new T(AZStd::forward<Args>(args)...);
            if (RegisterCommandCallback(commandName, callback))
            {
                callbacks.emplace_back(callback);
                return true;
            }
            delete callback;
            return false;
        }

        /**
         * Add error message to the internal callback based error handling system.
         * @param[in] errorLine The error line to add to the internal error handler.
         */
        MCORE_INLINE void AddError(const char* errorLine)                                       { m_errors.push_back(errorLine); }
        MCORE_INLINE void AddError(const AZStd::string& errorLine)                              { m_errors.push_back(errorLine); }

        /**
         * Checks if an error occurred and calls the error handling callbacks.
         * @result True in case an error occurred, false if not.
         */
        bool ShowErrorReport();

        /**
        * Checks if there are commands currently being executed
        * @result True in case there is at least one command being executed.
        */
        bool IsExecuting() const { return m_commandsInExecution > 0; }

    protected:
        AZStd::unordered_map<AZStd::string, Command*>   m_registeredCommands;    /**< A hash table storing the command objects for fast command object access. */
        AZStd::vector<CommandHistoryEntry>              m_commandHistory;        /**< The command history stack for undo/redo functionality. */
        AZStd::vector<CommandManagerCallback*>          m_callbacks;             /**< The command manager callbacks. */
        AZStd::vector<AZStd::string>                    m_errors;                /**< List of errors that happened during command execution. */
        AZStd::vector<Command*>                         m_commands;              /**< A flat array of registered commands, for easy traversal. */
        size_t                                          m_maxHistoryEntries;     /**< The maximum remembered commands in the command history. */
        ptrdiff_t                                       m_historyIndex;          /**< The command history iterator. The current position in the undo/redo history. */
        size_t                                          m_totalNumHistoryItems; /**< The number of history items since the application start. This number will neither change depending on the size of the history queue nor with undo/redo. */
        int                                             m_commandsInExecution;  /**< The number of commands currently in execution. */

        /**
         * Internal method to execute a command.
         * @param command The registered command object.
         * @param commandLine The commandline object.
         * @param outCommandResult The return/result value of the command.
         * @param[in] callFromCommandGroup True in case the command is called from a command group. False in case the command to be called is not part of a command group and called only by itself.
         * @param addToHistory When set to true it is being added to the command history. This is set to false when redoing a command.
         * @return True if the command succeeded, false if not.
         */
        bool ExecuteCommand(Command* command,
            const CommandLine& commandLine,
            AZStd::string& outCommandResult,
            bool addToHistory,
            bool callFromCommandGroup,
            bool clearErrors,
            bool handleErrors,
            bool autoDeleteCommand);

        bool ShouldDeleteCommand(Command* commandObject, bool commandExecutionResult, bool callFromCommandGroup, bool addToHistory);

        /**
         * Push a command to the command history stack . This method will be automatically called by the system when
         * a command has been called but only if it is undoable.
         * @param command The command instance that has been created at execution time.
         * @param parameters The command arguments.
         */
        void PushCommandHistory(Command* command, const CommandLine& parameters);

        /**
         * Add the given command group to the history.
         * @param commandGroup The command group to add to the history.
         */
        void PushCommandHistory(CommandGroup* commandGroup);

        /**
         * Pop a command history item from the stack. After this the command cannot be un- or redone.
         */
        void PopCommandHistory();

        /**
         * Will be internally called before and after undoing a command.
         * @param command The command which will get or got executed.
         * @param parameters The given command parameters.
         * @param preUndo Execute all pre-undo callbacks in case this parameter is true, execute all post-undo callbacks elsewise.
         */
        void ExecuteUndoCallbacks(Command* command, const CommandLine& parameters, bool preUndo);

        /**
         * Will be internally called before and after executing a command.
         * @param command The command which will get or got executed.
         * @param parameters The given command parameters.
         * @param preCommand Execute all pre-commant callbacks in case this parameter is true, execute all post-command callbacks elsewise.
         */
        void ExecuteCommandCallbacks(Command* command, const CommandLine& parameters, bool preCommand);
    };
} // namespace MCore
