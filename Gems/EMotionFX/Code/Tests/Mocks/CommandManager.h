/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

namespace MCore
{
    class CommandManager
    {
    public:
        //virtual ~CommandManager();

        //bool ExecuteCommand(const char* command, AZStd::string& outCommandResult, bool addToHistory = true, Command** outExecutedCommand = nullptr, CommandLine* outExecutedParamters = nullptr, bool callFromCommandGroup = false, bool clearErrors = true, bool handleErrors = true);
        //bool ExecuteCommand(const AZStd::string& command, AZStd::string& outCommandResult, bool addToHistory = true, Command** outExecutedCommand = nullptr, CommandLine* outExecutedParamters = nullptr, bool callFromCommandGroup = false, bool clearErrors = true, bool handleErrors = true);
        //bool ExecuteCommand(Command* command, AZStd::string& outCommandResult, bool addToHistory = true, bool clearErrors = true, bool handleErrors = true);
        MOCK_METHOD2(ExecuteCommandInsideCommand, bool(const char* command, AZStd::string& outCommandResult));
        MOCK_METHOD2(ExecuteCommandInsideCommand, bool(const AZStd::string& command, AZStd::string& outCommandResult));
        //bool ExecuteCommandOrAddToGroup(const AZStd::string& command, MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);
        MOCK_METHOD5(ExecuteCommandGroupImpl, bool(CommandGroup& commandGroup, AZStd::string& outCommandResult, bool addToHistory, bool clearErrors, bool handleErrors));
        virtual bool ExecuteCommandGroup(CommandGroup& commandGroup, AZStd::string& outCommandResult, bool addToHistory = true, bool clearErrors = true, bool handleErrors = true) { return ExecuteCommandGroupImpl(commandGroup, outCommandResult, addToHistory, clearErrors, handleErrors); }
        //bool ExecuteCommandGroupInsideCommand(CommandGroup& commandGroup, AZStd::string& outCommandResult);
        //bool Undo(AZStd::string& outCommandResult);
        //bool Redo(AZStd::string& outCommandResult);
        //bool RegisterCommand(Command* command);
        //void LogCommandHistory();
        //Command* FindCommand(const AZStd::string& commandName);
        //void RemoveCallbacks();
        //void RegisterCallback(CommandManagerCallback* callback);
        //void RemoveCallback(CommandManagerCallback* callback, bool delFromMem = true);
        //uint32 GetNumCallbacks() const;
        //CommandManagerCallback* GetCallback(uint32 index);
        //void SetMaxHistoryItems(uint32 maxItems);
        //uint32 GetMaxHistoryItems() const;
        //int32 GetHistoryIndex() const;
        //uint32 GetNumHistoryItems() const;
        //const CommandHistoryEntry& GetHistoryItem(uint32 index) const;
        //Command* GetHistoryCommand(uint32 historyIndex);
        //void ClearHistory();
        //const CommandLine& GetHistoryCommandLine(uint32 historyIndex) const;
        //uint32 GetNumRegisteredCommands() const;
        //Command* GetCommand(uint32 index);
        //void RemoveCommandCallback(const char* commandName, Command::Callback* callback, bool delFromMem);
        //bool RegisterCommandCallback(const char* commandName, Command::Callback* callback);
        //template<typename T, typename... Args>
        //bool RegisterCommandCallback(const char* commandName, AZStd::vector<Command::Callback*>& callbacks, Args... args)
        //void AddError(const char* errorLine);
        //void AddError(const AZStd::string& errorLine);
        //bool ShowErrorReport();
        //bool IsExecuting() const;
    };
} // namespace MCore
