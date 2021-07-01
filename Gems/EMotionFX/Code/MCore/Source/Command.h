/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "StandardHeaders.h"
#include <AzCore/std/containers/vector.h>
#include "CommandLine.h"
#include "CommandSyntax.h"

#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class ReflectContext;
}


namespace MCore
{
    // define empty dll APIs in case they have not been defined yet
#ifndef DEFINECOMMAND_API
    #define DEFINECOMMAND_API
#endif
#ifndef DEFINECOMMANDCALLBACK_API
    #define DEFINECOMMANDCALLBACK_API
#endif


    // define for easy command class creation
#define MCORE_DEFINECOMMAND(CLASSNAME, COMMANDSTRING, HISTORYNAME, ISUNDOABLE)                                         \
    class DEFINECOMMAND_API CLASSNAME                                                                                  \
        : public MCore::Command                                                                                        \
    {                                                                                                                  \
        MCORE_MEMORYOBJECTCATEGORY(CLASSNAME, MCore::MCORE_DEFAULT_ALIGNMENT, MCore::MCORE_MEMCATEGORY_COMMANDSYSTEM); \
    public:                                                                                                            \
        CLASSNAME(MCore::Command * orgCommand = nullptr)                                                               \
            : MCore::Command(COMMANDSTRING, orgCommand) {}                                                             \
        virtual ~CLASSNAME() {}                                                                                        \
        bool Execute(const MCore::CommandLine & parameters, AZStd::string & outResult);                                \
        bool Undo(const MCore::CommandLine & parameters, AZStd::string & outResult);                                   \
        void InitSyntax();                                                                                             \
        bool GetIsUndoable() const { return ISUNDOABLE; }                                                              \
        const char* GetHistoryName() const { return HISTORYNAME; }                                                     \
        const char* GetDescription() const;                                                                            \
        MCore::Command* Create() { return new CLASSNAME(this); }                                                       \
    };


    // define for easy command class creation remembering one old value
#define MCORE_DEFINECOMMAND_1(CLASSNAME, COMMANDSTRING, HISTORYNAME, ISUNDOABLE, OLDVALUETYPE)                         \
    class DEFINECOMMAND_API CLASSNAME                                                                                  \
        : public MCore::Command                                                                                        \
    {                                                                                                                  \
        MCORE_MEMORYOBJECTCATEGORY(CLASSNAME, MCore::MCORE_DEFAULT_ALIGNMENT, MCore::MCORE_MEMCATEGORY_COMMANDSYSTEM); \
    public:                                                                                                            \
        CLASSNAME(MCore::Command * orgCommand = nullptr);                                                              \
        virtual ~CLASSNAME();                                                                                          \
        bool Execute(const MCore::CommandLine & parameters, AZStd::string & outResult);                                \
        bool Undo(const MCore::CommandLine & parameters, AZStd::string & outResult);                                   \
        void InitSyntax();                                                                                             \
        bool GetIsUndoable() const  { return ISUNDOABLE; }                                                             \
        const char* GetHistoryName() const { return HISTORYNAME; }                                                     \
        const char* GetDescription() const;                                                                            \
        MCore::Command* Create() { return new CLASSNAME(this); }                                                       \
        OLDVALUETYPE& GetData() { return mData; }                                                                      \
    protected:                                                                                                         \
        OLDVALUETYPE mData;                                                                                            \

#define MCORE_DEFINECOMMAND_1_END };

    // define for easy command class creation in two steps
#define MCORE_DEFINECOMMAND_START_BASE(CLASSNAME, HISTORYNAME, ISUNDOABLE, BASECLASS)                                  \
    class DEFINECOMMAND_API CLASSNAME                                                                                  \
        : public BASECLASS                                                                                             \
    {                                                                                                                  \
        MCORE_MEMORYOBJECTCATEGORY(CLASSNAME, MCore::MCORE_DEFAULT_ALIGNMENT, MCore::MCORE_MEMCATEGORY_COMMANDSYSTEM); \
    public:                                                                                                            \
        CLASSNAME(MCore::Command * orgCommand = nullptr);                                                              \
        ~CLASSNAME() override;                                                                                         \
        bool Execute(const MCore::CommandLine & parameters, AZStd::string & outResult) override;                       \
        bool Undo(const MCore::CommandLine & parameters, AZStd::string & outResult) override;                          \
        void InitSyntax() override;                                                                                    \
        bool GetIsUndoable() const override { return ISUNDOABLE; }                                                     \
        const char* GetHistoryName() const override { return HISTORYNAME; }                                            \
        const char* GetDescription() const override;                                                                   \
        MCore::Command* Create() override { return new CLASSNAME(this); }                                              \
    protected:
#define MCORE_DEFINECOMMAND_END };

#define MCORE_DEFINECOMMAND_START(CLASSNAME, HISTORYNAME, ISUNDOABLE)                                                  \
    MCORE_DEFINECOMMAND_START_BASE(CLASSNAME, HISTORYNAME, ISUNDOABLE, MCore::Command)


    // define a command callback
#define MCORE_DEFINECOMMANDCALLBACK(CLASSNAME)                                                                         \
    class DEFINECOMMANDCALLBACK_API CLASSNAME                                                                          \
        : public MCore::Command::Callback                                                                              \
    {                                                                                                                  \
        MCORE_MEMORYOBJECTCATEGORY(CLASSNAME, MCore::MCORE_DEFAULT_ALIGNMENT, MCore::MCORE_MEMCATEGORY_COMMANDSYSTEM); \
    public:                                                                                                            \
        CLASSNAME(bool executePreUndo, bool executePreCommand = false)                                                 \
            : MCore::Command::Callback(executePreUndo, executePreCommand) {}                                           \
        bool Execute(MCore::Command * command, const MCore::CommandLine & commandLine);                                \
        bool Undo(MCore::Command * command, const MCore::CommandLine & commandLine);                                   \
    };


    /**
     * If a command changes the state of any data in an application, it should implement undo and redo methods. The command manager
     * checks if a command is undoable when executing it and if that method returns true the command will be remained and saved in
     * the command history. If it is not undoable, the command instance is destroyed right away.
     */
    class MCORE_API Command
    {
        MCORE_MEMORYOBJECTCATEGORY(Command, MCore::MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_COMMANDSYSTEM);

    public:
        AZ_RTTI(MCore::Command, "{49C636CE-7C0E-408A-A0F7-F7D12647EFBA}")
        /**
         * The command callback base class.
         * The callbacks get executed when executing a command, or when undoing a command.
         * You could for example link your 3D engine to loading of some 3D mesh command (outside of your engine), where the
         * callback would convert this data into an object inside your 3D engine.
         */
        class MCORE_API Callback
        {
            MCORE_MEMORYOBJECTCATEGORY(Callback, MCore::MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_COMMANDSYSTEM);

        public:
            /**
             * The destructor.
             * @param executePreUndo Flag which controls if the callback gets executed before the undo (true) or after it (false).
             * @param executePreCommand Flag which controls if the callback gets executed before the command (true) or after it (false).
             */
            Callback(bool executePreUndo, bool executePreCommand = false);

            /**
             * The destructor.
             */
            virtual ~Callback();

            /**
             * Execute the callback.
             * Command callbacks are executed before/after executing the real command where this callback is linked to.
             * @param command The command where this callback belongs to.
             * @param commandLine The command line used when executing the command.
             * @result Returns true when the command callback executed without problems, otherwise false is returned.
             */
            virtual bool Execute(MCore::Command* command, const MCore::CommandLine& commandLine) = 0;

            /**
             * Execute the undo of this callback.
             * The undo is executed before/after executing the undo of the actual command where this callback is linked to.
             * @param command The command where this callback belongs to.
             * @param commandLine The command line used when executing the command.
             * @result Returns true when the command callback undo executed without problems, otherwise false is returned.
             */
            virtual bool Undo(MCore::Command* command, const MCore::CommandLine& commandLine) = 0;

            /**
             * Get the flag which controls if the callback gets executed before the command or after it.
             * @return True in case the callback gets called before the command, false when it gets called afterwards.
             */
            MCORE_INLINE bool GetExecutePreCommand() const                      { return mPreCommandExecute; }

            /**
             * Get the flag which controls if the callback gets executed before undo or after it.
             * @return True in case the callback gets called before undo, false when it gets called afterwards.
             */
            MCORE_INLINE bool GetExecutePreUndo() const                         { return mPreUndoExecute; }

        private:
            bool mPreCommandExecute;        /**< Flag which controls if the callback gets executed before the command (true) or after it (false). */
            bool mPreUndoExecute;           /**< Flag which controls if the callback gets executed before the undo (true) or after it (false). */
        };

        /**
         * Default constructor.
         * @param commandName The unique identifier for the command.
         * @param originalCommand The original command, or nullptr when this is the original command.
         */
        Command(const char* commandName, Command* originalCommand);

        /**
         * Destructor.
         */
        virtual ~Command();

        static void Reflect(AZ::ReflectContext* context);

        /**
         * The do it method should call redo it to make the command happen. The redo it method should do the
         * actual work.
         * This is a pure virtual method, and must be overridden in derived commands.
         * @param parameters A list of the passed command arguments.
         * @param outResult The result/return value of the command.
         * @return True if the command execution succeeded, false if not.
         */
        virtual bool Execute(const CommandLine& parameters, AZStd::string& outResult) = 0;

        /**
         * This method should undo the work done be the redo it method.
         * This is a pure virtual method, and must be overridden in derived commands.
         * @param parameters A list of the passed command arguments.
         * @param outResult The result/return value of the command.
         * @return True if the command undo succeeded, false if not.
         */
        virtual bool Undo(const CommandLine& parameters, AZStd::string& outResult)         { MCORE_UNUSED(parameters); MCORE_UNUSED(outResult); return false; }

        /**
         * This will be called by the CommandManager when the command is executed.
         * The function will return an instance of the command implementation which will be stored in the command history.
         * An instance of the called command is needed to be able to store information for the undo.
         * @result The instance of the command.
         */
        virtual Command* Create() = 0;

        /**
         * This method is used to specify whether or not the command is undoable. On default, when not overloaded, it always
         * returns false. If you are writing a command that might be eligible for undo, you should override this method.
         * The is undoable method will be called and used to indicate whether or not the command will be saved in the
         * command history in the command manager.
         * @return True if the command is undoable and should be saved in the command history, false if not.
         */
        virtual bool GetIsUndoable() const;

        /**
         * Initialize the command syntax.
         * This is automatically called by the command constructor.
         * On default the syntax will be empty, which means the command would have no parameters.
         */
        virtual void InitSyntax();

        virtual bool SetCommandParameters(const CommandLine& parameters) { MCORE_UNUSED(parameters); return false; }

        /**
         * Get the command optional description.
         * This can contain additional detailed information about the command.
         * @result A string containing the description of the command, which is optional. On default it is an empty string.
         */
        virtual const char* GetDescription() const                              { return ""; }

        /**
         * Get the name of the command as it can be shown in the command history.
         * This can be a nicer name than the actual command name. For example some "MyAPILoadActor" command could be shown in the command history to the user with
         * a more friendly name like "Load Actor". The "Load Actor" string could be returned by this function.
         * On default the command name is returned.
         * @result The user-friendly name of the command.
         */
        virtual const char* GetHistoryName() const                              { return GetName(); }

        /**
         * Get the command string that is associated with this command.
         * @return The unique command name used to identify the command.
         */
        const char* GetName() const;

        /**
         * Get the command string that is associated with this command.
         * @return The unique command name used to identify the command.
         */
        const AZStd::string& GetNameString() const;

        /**
         * Get the command syntax.
         * The syntax describes the possible parameters that can be passed to this command.
         * Also it can verify and show info about these parameters.
         * @result The syntax object.
         */
        MCORE_INLINE CommandSyntax& GetSyntax()                                 { return mSyntax; }

        /**
         * Get the number of registered/added command callbacks.
         * @result The number of command callbacks that have been added.
         */
        uint32 GetNumCallbacks() const;

        /**
         * Calculate the number of registered pre-execute callbacks.
         * @result The number of registered pre-execute callbacks.
         */
        uint32 CalcNumPreCommandCallbacks() const;

        /**
         * Calculate the number of registered post-execute callbacks.
         * @result The number of registered post-execute callbacks.
         */
        uint32 CalcNumPostCommandCallbacks() const;

        /**
         * Get a given command callback.
         * @param index The callback number, which must be in range of [0..GetNumCallbacks()-1].
         * @result A pointer to the command callback object.
         */
        MCORE_INLINE Command::Callback* GetCallback(uint32 index)               { return mCallbacks[index]; }

        /**
         * Add (register) a command callback.
         * @param callBack The command callback to register to this command.
         */
        void AddCallback(Command::Callback* callback);

        /**
         * Check if we already registered a given command callback.
         * @param callback The callback object to check for.
         * @result Returns true when the specified callback has already been added to this command, otherwise false is returned.
         */
        bool CheckIfHasCallback(Command::Callback* callback) const;

        /**
         * Remove a given callback.
         * @param callBack The callback to remove.
         * @param delFromMem Set to true when you want the callback to be automatically removed from memory as well (even if its not found and removed).
         */
        void RemoveCallback(Command::Callback* callback, bool delFromMem = true);

        /**
         * Remove all the registered command callbacks.
         */
        void RemoveAllCallbacks();

        void SetOriginalCommand(Command* orgCommand) { mOrgCommand = orgCommand; }

        /**
         * Get the original command where this command has been cloned from.
         * The original command contains the syntax.
         * In case this is the original command, it returns a pointer to itself.
         * @result A pointer to the original command.
         */
        MCORE_INLINE Command* GetOriginalCommand()
        {
            if (mOrgCommand)
            {
                return mOrgCommand;
            }
            else
            {
                return this;
            }
        }

        template<class T>
        void ExecuteParameter(AZStd::optional<T>& oldParameter, AZStd::optional<T>& parameter, T& value)
        {
            if (parameter.has_value())
            {
                if (!oldParameter.has_value())
                {
                    oldParameter = value;
                }
                value = parameter.value();
            }
        }

    private:
        Command*                            mOrgCommand;    /**< The original command, or nullptr when this is the original. */
        AZStd::string                       mCommandName;   /**< The unique command name used to identify the command. */
        CommandSyntax                       mSyntax;        /**< The command syntax, which contains info about the possible parameters etc. */
        AZStd::vector<Command::Callback*>   mCallbacks;     /**< The command callbacks. */
    };
} // namespace MCore
