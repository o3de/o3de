/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Console/IConsole.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    //! @class Console
    //! A simple console class for providing text based variable and process interaction.
    class Console final
        : public IConsole
    {
    public:
        AZ_RTTI(Console, "{CF6DCDE7-1A66-442C-BA87-01A432C13E7D}", IConsole);
        AZ_CLASS_ALLOCATOR(Console, AZ::OSAllocator);

        Console();
        //! Constructor overload which registers a notifier with the Settings Registry that will execute
        //! a console command whenever a key is set under the AZ::IConsole::ConsoleCommandRootKey JSON object
        explicit Console(AZ::SettingsRegistryInterface& settingsRegistry);
        ~Console() override;

        //! IConsole interface
        //! @{
        PerformCommandResult PerformCommand
        (
            const char* command,
            ConsoleSilentMode silentMode = ConsoleSilentMode::NotSilent,
            ConsoleInvokedFrom invokedFrom = ConsoleInvokedFrom::AzConsole,
            ConsoleFunctorFlags requiredSet = ConsoleFunctorFlags::Null,
            ConsoleFunctorFlags requiredClear = ConsoleFunctorFlags::ReadOnly
        ) override;
        PerformCommandResult PerformCommand
        (
            const ConsoleCommandContainer& commandAndArgs,
            ConsoleSilentMode silentMode = ConsoleSilentMode::NotSilent,
            ConsoleInvokedFrom invokedFrom = ConsoleInvokedFrom::AzConsole,
            ConsoleFunctorFlags requiredSet = ConsoleFunctorFlags::Null,
            ConsoleFunctorFlags requiredClear = ConsoleFunctorFlags::ReadOnly
        ) override;
        PerformCommandResult PerformCommand
        (
            AZStd::string_view command,
            const ConsoleCommandContainer& commandArgs,
            ConsoleSilentMode silentMode = ConsoleSilentMode::NotSilent,
            ConsoleInvokedFrom invokedFrom = ConsoleInvokedFrom::AzConsole,
            ConsoleFunctorFlags requiredSet = ConsoleFunctorFlags::Null,
            ConsoleFunctorFlags requiredClear = ConsoleFunctorFlags::ReadOnly
        ) override;
        void ExecuteConfigFile(AZStd::string_view configFileName) override;
        void ExecuteCommandLine(const AZ::CommandLine& commandLine) override;
        bool ExecuteDeferredConsoleCommands() override;

        void ClearDeferredConsoleCommands() override;

        bool HasCommand(AZStd::string_view command, ConsoleFunctorFlags ignoreAnyFlags = ConsoleFunctorFlags::IsInvisible) override;
        ConsoleFunctorBase* FindCommand(AZStd::string_view command, ConsoleFunctorFlags ignoreAnyFlags = ConsoleFunctorFlags::IsInvisible) override;
        AZStd::string AutoCompleteCommand(AZStd::string_view command, AZStd::vector<AZStd::string>* matches = nullptr) override;
        void VisitRegisteredFunctors(const FunctorVisitor& visitor) override;
        void RegisterFunctor(ConsoleFunctorBase* functor) override;
        void UnregisterFunctor(ConsoleFunctorBase* functor) override;
        void LinkDeferredFunctors(ConsoleFunctorBase*& deferredHead) override;
        void RegisterCommandInvokerWithSettingsRegistry(AZ::SettingsRegistryInterface& settingsRegistry) override;
        //! @}

    private:

        void MoveFunctorsToDeferredHead(ConsoleFunctorBase*& deferredHead);

        //! Invokes a single console command, optionally returning the command output.
        //! @param command       the function to invoke
        //! @param inputs        the set of inputs to provide the function
        //! @param silentMode    if true, logs will be suppressed during command execution
        //! @param invokedFrom   the source point that initiated console invocation
        //! @param requiredSet   a set of flags that must be set on the functor for it to execute
        //! @param requiredClear a set of flags that must *NOT* be set on the functor for it to execute
        //! @return boolean true on success, false otherwise
        bool DispatchCommand
        (
            AZStd::string_view command,
            const ConsoleCommandContainer& inputs,
            ConsoleSilentMode silentMode,
            ConsoleInvokedFrom invokedFrom,
            ConsoleFunctorFlags requiredSet,
            ConsoleFunctorFlags requiredClear
        );

        AZ_DISABLE_COPY_MOVE(Console);

        ConsoleFunctorBase* m_head;
        using CommandMap = AZStd::unordered_map<CVarFixedString, AZStd::vector<ConsoleFunctorBase*>>;
        CommandMap m_commands;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_consoleCommandKeyHandler;
        struct DeferredCommand
        {
            using DeferredArguments = AZStd::vector<AZStd::string>;
            AZStd::string m_command;
            DeferredArguments m_arguments;
            ConsoleSilentMode m_silentMode;
            ConsoleInvokedFrom m_invokedFrom;
            ConsoleFunctorFlags m_requiredSet;
            ConsoleFunctorFlags m_requiredClear;
        };
        using DeferredCommandQueue = AZStd::deque<DeferredCommand>;
        DeferredCommandQueue m_deferredCommands;

        friend struct ConsoleCommandKeyNotificationHandler;
        friend class ConsoleFunctorBase;
    };
}
