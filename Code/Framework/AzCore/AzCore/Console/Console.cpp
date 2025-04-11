/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/Console.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/Serialization/Locale.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <cctype>

namespace AZ
{
    uint32_t CountMatchingPrefixes(const AZStd::string_view& string, const ConsoleCommandContainer& stringSet)
    {
        uint32_t count = 0;

        for (AZStd::string_view iter : stringSet)
        {
            if (StringFunc::StartsWith(iter, string, false))
            {
                ++count;
            }
        }

        return count;
    }

    Console::Console()
        : m_head(nullptr)
    {
    }

    Console::Console(AZ::SettingsRegistryInterface& settingsRegistryInterface)
        : Console()
    {
        RegisterCommandInvokerWithSettingsRegistry(settingsRegistryInterface);
    }

    Console::~Console()
    {
        // on console destruction relink the console functors back to the deferred head
        MoveFunctorsToDeferredHead(AZ::ConsoleFunctorBase::GetDeferredHead());
    }

    PerformCommandResult Console::PerformCommand
    (
        const char* command,
        ConsoleSilentMode silentMode,
        ConsoleInvokedFrom invokedFrom,
        ConsoleFunctorFlags requiredSet,
        ConsoleFunctorFlags requiredClear
    )
    {
        AZStd::string_view commandView;
        ConsoleCommandContainer commandArgsView;
        bool firstIteration = true;
        auto ConvertCommandStringToArray = [&firstIteration, &commandView, &commandArgsView](AZStd::string_view token)
        {
            if (firstIteration)
            {
                commandView = token;
                firstIteration = false;
            }
            else
            {
                commandArgsView.emplace_back(token);
            };
        };
        constexpr AZStd::string_view commandSeparators = " \t\n\r";
        StringFunc::TokenizeVisitor(command, ConvertCommandStringToArray, commandSeparators);

        return PerformCommand(commandView, commandArgsView, silentMode, invokedFrom, requiredSet, requiredClear);
    }

    PerformCommandResult Console::PerformCommand
    (
        const ConsoleCommandContainer& commandAndArgs,
        ConsoleSilentMode silentMode,
        ConsoleInvokedFrom invokedFrom,
        ConsoleFunctorFlags requiredSet,
        ConsoleFunctorFlags requiredClear
    )
    {
        if (commandAndArgs.empty())
        {
            return AZ::Failure("CommandAndArgs is empty.");
        }

        return PerformCommand(commandAndArgs.front(), ConsoleCommandContainer(commandAndArgs.begin() + 1, commandAndArgs.end()), silentMode, invokedFrom, requiredSet, requiredClear);
    }

    PerformCommandResult Console::PerformCommand
    (
        AZStd::string_view command,
        const ConsoleCommandContainer& commandArgs,
        ConsoleSilentMode silentMode,
        ConsoleInvokedFrom invokedFrom,
        ConsoleFunctorFlags requiredSet,
        ConsoleFunctorFlags requiredClear
    )
    {
        if (!DispatchCommand(command, commandArgs, silentMode, invokedFrom, requiredSet, requiredClear))
        {
            // If the command could not be dispatched at this time add it to the deferred commands queue
            DeferredCommand deferredCommand{ AZStd::string_view{ command },
                                             DeferredCommand::DeferredArguments{ commandArgs.begin(), commandArgs.end() },
                                             silentMode,
                                             invokedFrom,
                                             requiredSet,
                                             requiredClear };

            CVarFixedString commandLowercase(command);
            AZStd::to_lower(commandLowercase.begin(), commandLowercase.end());
            m_deferredCommands.emplace_back(AZStd::move(deferredCommand));
            return AZ::Failure(AZStd::string::format(
                "Command \"%s\" is not yet registered. Deferring to attempt execution later.", commandLowercase.c_str()));
        }

        return AZ::Success();
    }

    void Console::ExecuteConfigFile(AZStd::string_view configFileName)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        // If the config file is a settings registry file use the SettingsRegistryInterface MergeSettingsFile function
        // otherwise use the SettingsRegistryMergeUtils MergeSettingsToRegistry_ConfigFile function to merge an INI-style
        // file to the settings registry
        AZ::IO::PathView configFile(configFileName);
        if (configFile.Extension() == ".setreg")
        {
            settingsRegistry->MergeSettingsFile(configFile.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        }
        else if (configFile.Extension() == ".setregpatch")
        {
            settingsRegistry->MergeSettingsFile(configFile.Native(), AZ::SettingsRegistryInterface::Format::JsonPatch);
        }
        else
        {
            AZ::SettingsRegistryMergeUtils::ConfigParserSettings configParserSettings;
            configParserSettings.m_registryRootPointerPath = "/Amazon/AzCore/Runtime/ConsoleCommands";
            configParserSettings.m_commandLineSettings.m_delimiterFunc = [](AZStd::string_view line)
            {
                SettingsRegistryInterface::CommandLineArgumentSettings::JsonPathValue pathValue;
                AZStd::string_view parsedLine = line;

                // Splits the line based on the <equal> or <colon>
                if (auto path = AZ::StringFunc::TokenizeNext(parsedLine, "=:"); path.has_value())
                {
                    pathValue.m_path = AZ::StringFunc::StripEnds(*path);
                    pathValue.m_value = AZ::StringFunc::StripEnds(parsedLine);
                }
                // If the value is empty, then the line either contained an equal sign followed only by whitespace or the line was empty
                // 1. line="testInit=", pathValue.m_path="testInit", pathValue.m_value=""
                // 2. line="testInit 1", pathValue.m_path="testInit 1", pathValue.m_value=""
                // Therefore the path is split the path on whitespace in order to retrieve a value
                if (pathValue.m_value.empty())
                {
                    parsedLine = pathValue.m_path;
                    if (auto path = AZ::StringFunc::TokenizeNext(parsedLine, " \t"); path.has_value())
                    {
                        pathValue.m_path = AZ::StringFunc::StripEnds(*path);
                        pathValue.m_value = AZ::StringFunc::StripEnds(parsedLine);
                    }

                }
                return pathValue;
            };
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ConfigFile(*settingsRegistry, configFile.Native(), configParserSettings);
        }
    }

    void Console::ExecuteCommandLine(const AZ::CommandLine& commandLine)
    {
        for (auto&& commandArgument : commandLine)
        {
            const auto& switchKey = commandArgument.m_option;
            const auto& switchValue = commandArgument.m_value;
            ConsoleCommandContainer commandArgs{ switchValue };
            PerformCommand(switchKey, commandArgs, ConsoleSilentMode::NotSilent, ConsoleInvokedFrom::AzConsole, ConsoleFunctorFlags::Null, ConsoleFunctorFlags::Null);
        }
    }

    bool Console::ExecuteDeferredConsoleCommands()
    {
        auto DeferredCommandCallable = [this](const DeferredCommand& deferredCommand)
        {
            return this->DispatchCommand(deferredCommand.m_command,
                ConsoleCommandContainer(AZStd::from_range, deferredCommand.m_arguments), deferredCommand.m_silentMode,
                deferredCommand.m_invokedFrom, deferredCommand.m_requiredSet, deferredCommand.m_requiredClear);
        };
        // Attempt to invoke the deferred command and remove it from the queue if successful
        return AZStd::erase_if(m_deferredCommands, DeferredCommandCallable) != 0;
    }

    void Console::ClearDeferredConsoleCommands()
    {
        m_deferredCommands = {};
    }

    bool Console::HasCommand(AZStd::string_view command, ConsoleFunctorFlags ignoreAnyFlags)
    {
        return FindCommand(command, ignoreAnyFlags) != nullptr;
    }

    ConsoleFunctorBase* Console::FindCommand(AZStd::string_view command, ConsoleFunctorFlags ignoreAnyFlags)
    {
        CVarFixedString lowerName(command);
        AZStd::to_lower(lowerName.begin(), lowerName.end());

        CommandMap::iterator iter = m_commands.find(lowerName);
        if (iter != m_commands.end())
        {
            for (ConsoleFunctorBase* curr : iter->second)
            {
                if ((curr->GetFlags() & ignoreAnyFlags) != ConsoleFunctorFlags::Null)
                {
                    // Filter functors marked as invisible
                    continue;
                }
                return curr;
            }
        }
        return nullptr;
    }

    AZStd::string Console::AutoCompleteCommand(AZStd::string_view command, AZStd::vector<AZStd::string>* matches)
    {
        if (command.empty())
        {
            return command;
        }

        ConsoleCommandContainer commandSubset;

        for (const auto& functor : m_commands)
        {
            if (functor.second.empty())
            {
                continue;
            }

            // Filter functors registered with the same name
            const ConsoleFunctorBase* curr = functor.second.front();

            if ((curr->GetFlags() & ConsoleFunctorFlags::IsInvisible) == ConsoleFunctorFlags::IsInvisible)
            {
                // Filter functors marked as invisible
                continue;
            }

            if (StringFunc::StartsWith(curr->m_name, command, false))
            {
                AZLOG_INFO("- %s : %s", curr->m_name, curr->m_desc);

                if (commandSubset.size() < MaxConsoleCommandPlusArgsLength)
                {
                    commandSubset.push_back(curr->m_name);
                }

                if (matches)
                {
                    matches->push_back(curr->m_name);
                }
            }
        }

        AZStd::string largestSubstring = command;

        if ((!largestSubstring.empty()) && (!commandSubset.empty()))
        {
            const uint32_t totalCount = CountMatchingPrefixes(command, commandSubset);

            for (size_t i = largestSubstring.length(); i < commandSubset.front().length(); ++i)
            {
                const AZStd::string nextSubstring = largestSubstring + commandSubset.front()[i];
                const uint32_t count = CountMatchingPrefixes(nextSubstring, commandSubset);

                if (count < totalCount)
                {
                    break;
                }

                largestSubstring = nextSubstring;
            }
        }

        return largestSubstring;
    }

    void Console::VisitRegisteredFunctors(const FunctorVisitor& visitor)
    {
        for (auto& curr : m_commands)
        {
            if (!curr.second.empty())
            {
                visitor(curr.second.front());
            }
        }
    }

    void Console::RegisterFunctor(ConsoleFunctorBase* functor)
    {
        if (functor->m_console && (functor->m_console != this))
        {
            AZ_Assert(false, "Functor is already registered to a console");
            return;
        }

        CVarFixedString lowerName = functor->GetName();
        AZStd::to_lower(lowerName.begin(), lowerName.end());
        CommandMap::iterator iter = m_commands.find(lowerName);
        if (iter != m_commands.end())
        {
            // Validate we haven't already added this cvar
            AZStd::vector<ConsoleFunctorBase*>::iterator iter2 = AZStd::find(iter->second.begin(), iter->second.end(), functor);
            if (iter2 != iter->second.end())
            {
                AZ_Assert(false, "Duplicate functor registered to the console");
                return;
            }

            // If multiple cvars are registered with the same name, validate that the types and flags match
            if (!iter->second.empty())
            {
                ConsoleFunctorBase* front = iter->second.front();
                if (front->GetFlags() != functor->GetFlags() || front->GetTypeId() != functor->GetTypeId())
                {
                    AZ_Assert(false, "Mismatched console functor types registered under the same name");
                    return;
                }

                // Discard duplicate functors if the 'DontDuplicate' flag has been set
                if ((front->GetFlags() & ConsoleFunctorFlags::DontDuplicate) != ConsoleFunctorFlags::Null)
                {
                    return;
                }
            }
        }
        m_commands[lowerName].emplace_back(functor);
        functor->Link(m_head);
        functor->m_console = this;
    }

    void Console::UnregisterFunctor(ConsoleFunctorBase* functor)
    {
        if (functor->m_console != this)
        {
            AZ_Assert(false, "Unregistering a functor bound to a different console");
            return;
        }

        CVarFixedString lowerName = functor->GetName();
        AZStd::to_lower(lowerName.begin(), lowerName.end());
        CommandMap::iterator iter = m_commands.find(lowerName);
        if (iter != m_commands.end())
        {
            AZStd::vector<ConsoleFunctorBase*>::iterator iter2 = AZStd::find(iter->second.begin(), iter->second.end(), functor);
            if (iter2 != iter->second.end())
            {
                iter->second.erase(iter2);
            }

            if (iter->second.empty())
            {
                m_commands.erase(iter);
            }
        }
        functor->Unlink(m_head);
        functor->m_console = nullptr;
    }

    void Console::LinkDeferredFunctors(ConsoleFunctorBase*& deferredHead)
    {
        ConsoleFunctorBase* curr = deferredHead;

        while (curr != nullptr)
        {
            ConsoleFunctorBase* next = curr->m_next;
            curr->Unlink(deferredHead);
            RegisterFunctor(curr);
            curr->m_isDeferred = false;
            curr = next;
        }

        deferredHead = nullptr;
    }

    void Console::MoveFunctorsToDeferredHead(ConsoleFunctorBase*& deferredHead)
    {
        m_commands.clear();

        // Re-initialize all of the current functors to a deferred state
        for (ConsoleFunctorBase* curr = m_head; curr != nullptr; curr = curr->m_next)
        {
            curr->m_console = nullptr;
            curr->m_isDeferred = true;
        }

        // If the deferred head contains unregistered functors
        // move this AZ::Console functors list to the end of the deferred functors
        if (deferredHead)
        {
            ConsoleFunctorBase* oldDeferred = deferredHead;
            while (oldDeferred->m_next != nullptr)
            {
                oldDeferred = oldDeferred->m_next;
            }
            oldDeferred->Link(m_head);
        }
        else
        {
            deferredHead = m_head;
        }

        ConsoleFunctorBase::s_deferredHeadInvoked = false;

        m_head = nullptr;
    }

    bool Console::DispatchCommand
    (
        AZStd::string_view command,
        const ConsoleCommandContainer& inputs,
        ConsoleSilentMode silentMode,
        ConsoleInvokedFrom invokedFrom,
        ConsoleFunctorFlags requiredSet,
        ConsoleFunctorFlags requiredClear
    )
    {
        // incoming commands are assumed to be in the "C" locale as they might be from portable data files
        AZ::Locale::ScopedSerializationLocale scopedLocale;

        bool result = false;
        ConsoleFunctorFlags flags = ConsoleFunctorFlags::Null;

        CVarFixedString lowerName(command);
        AZStd::to_lower(lowerName.begin(), lowerName.end());

        CommandMap::iterator iter = m_commands.find(lowerName);
        if (iter != m_commands.end())
        {
            for (ConsoleFunctorBase* curr : iter->second)
            {
                if ((curr->GetFlags() & requiredSet) != requiredSet)
                {
                    AZLOG_WARN("%s failed required set flag check", curr->m_name);
                    continue;
                }

                if ((curr->GetFlags() & requiredClear) != ConsoleFunctorFlags::Null)
                {
                    AZLOG_WARN("%s failed required clear flag check", curr->m_name);
                    continue;
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::IsCheat) != ConsoleFunctorFlags::Null)
                {
                    AZLOG_WARN("%s is marked as a cheat", curr->m_name);
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::IsDeprecated) != ConsoleFunctorFlags::Null)
                {
                    AZLOG_WARN("%s is marked as deprecated", curr->m_name);
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::NeedsReload) != ConsoleFunctorFlags::Null)
                {
                    AZLOG_WARN("Changes to %s will only take effect after level reload", curr->m_name);
                }

                // Letting this intentionally fall-through, since in editor we can register common variables multiple times
                (*curr)(inputs);

                if (!result)
                {
                    result = true;
                    if ((silentMode == ConsoleSilentMode::NotSilent) && (curr->GetFlags() & ConsoleFunctorFlags::IsInvisible) != ConsoleFunctorFlags::IsInvisible)
                    {
                        // First use the ConsoleFunctorBase::GetValue function
                        // to retrieve the value of the type of the first template parameter to the ConsoleFunctor class template
                        // This is populated for non-void types and is set for Console Variables(CVars) and Console Commands
                        // which are member functions
                        // See `ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::GetValueAsString`
                        CVarFixedString inputStr;
                        if (GetValueResult getCVarValue = curr->GetValue(inputStr);
                            getCVarValue != GetValueResult::Success)
                        {
                            // In this case the ConsoleFunctorBase pointer references a `ConsoleFunctor<void, _REPLICATES_VALUE>` object
                            // which has no type associated with it.
                            // This is used for Console Commands which are free functions
                            // The `ConsoleFunctor<void, _REPLICATES_VALUE>::GetValueAsString` returns NotImplemented
                            // Instead the input arguments to the console command will be logged
                            AZ::StringFunc::Join(inputStr, inputs, ' ');
                        }
                        AZLOG_INFO("> %s : %s", curr->GetName(), inputStr.empty() ? "<no-args>" : inputStr.c_str());
                    }
                    flags = curr->GetFlags();
                }
            }
        }

        if (result)
        {
            m_consoleCommandInvokedEvent.Signal(command, inputs, flags, invokedFrom);
        }
        else
        {
            m_dispatchCommandNotFoundEvent.Signal(command, inputs, invokedFrom);
        }

        return result;
    }

    struct ConsoleCommandKeyNotificationHandler
    {
        ConsoleCommandKeyNotificationHandler(AZ::SettingsRegistryInterface& registry, Console& console)
            : m_settingsRegistry(registry)
            , m_console(console)
        {
        }

        // NotifyEventHandler function for handling when settings underneath the
        // ConsoleAutoexecCommandKey or ConsoleRuntimeCommandKey key paths are modified
        void operator()(const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

            if (notifyEventArgs.m_type == AZ::SettingsRegistryInterface::Type::NoType)
            {
                return;
            }

            constexpr AZ::IO::PathView consoleRuntimeCommandKey{ IConsole::ConsoleRuntimeCommandKey, AZ::IO::PosixPathSeparator };
            constexpr AZ::IO::PathView consoleAutoexecCommandKey{ IConsole::ConsoleAutoexecCommandKey, AZ::IO::PosixPathSeparator };
            AZ::IO::PathView inputKey{ notifyEventArgs.m_jsonKeyPath, AZ::IO::PosixPathSeparator };

            // Abuses the IsRelativeToFuncton function of the path class to extract the console
            // command from the settings registry objects
            FixedValueString command;
            if (inputKey != consoleRuntimeCommandKey && inputKey.IsRelativeTo(consoleRuntimeCommandKey))
            {
                command = inputKey.LexicallyRelative(consoleRuntimeCommandKey).Native();
            }
            else if (inputKey != consoleAutoexecCommandKey && inputKey.IsRelativeTo(consoleAutoexecCommandKey))
            {
                command = inputKey.LexicallyRelative(consoleAutoexecCommandKey).Native();
            }

            if (!command.empty())
            {
                ConsoleCommandContainer commandArgs;
                // Argument string which stores the value from the Settings Registry long enough
                // to pass into the PerformCommand. The ConsoleCommandContainer stores string_views
                // and therefore doesn't own the memory.
                FixedValueString commandArgString;

                if (notifyEventArgs.m_type == SettingsRegistryInterface::Type::String)
                {
                    if (m_settingsRegistry.Get(commandArgString, notifyEventArgs.m_jsonKeyPath))
                    {
                        auto ConvertCommandArgumentToArray = [&commandArgs](AZStd::string_view token)
                        {
                            commandArgs.emplace_back(token);
                        };
                        constexpr AZStd::string_view commandSeparators = " \t\n\r";
                        StringFunc::TokenizeVisitor(commandArgString, ConvertCommandArgumentToArray, commandSeparators);
                    }
                }
                else if (notifyEventArgs.m_type == SettingsRegistryInterface::Type::Boolean)
                {
                    bool commandArgBool{};
                    if (m_settingsRegistry.Get(commandArgBool, notifyEventArgs.m_jsonKeyPath))
                    {
                        commandArgString = commandArgBool ? "true" : "false";
                        commandArgs.emplace_back(commandArgString);
                    }
                }
                else if (notifyEventArgs.m_type == SettingsRegistryInterface::Type::Integer)
                {
                    // Try converting to a signed 64-bit number first and then an unsigned 64-bit number
                    AZ::s64 commandArgInt{};
                    AZ::u64 commandArgUInt{};
                    if (m_settingsRegistry.Get(commandArgInt, notifyEventArgs.m_jsonKeyPath))
                    {
                        AZStd::to_string(commandArgString, commandArgInt);
                        commandArgs.emplace_back(commandArgString);
                    }
                    else if (m_settingsRegistry.Get(commandArgUInt, notifyEventArgs.m_jsonKeyPath))
                    {
                        AZStd::to_string(commandArgString, commandArgUInt);
                        commandArgs.emplace_back(commandArgString);
                    }
                }
                else if (notifyEventArgs.m_type == SettingsRegistryInterface::Type::FloatingPoint)
                {
                    double commandArgFloat{};
                    if (m_settingsRegistry.Get(commandArgFloat, notifyEventArgs.m_jsonKeyPath))
                    {
                        AZStd::to_string(commandArgString, commandArgFloat);
                        commandArgs.emplace_back(commandArgString);
                    }
                }
                CVarFixedString commandTrace(command);
                for (AZStd::string_view commandArg : commandArgs)
                {
                    commandTrace.push_back(' ');
                    commandTrace += commandArg;
                }

                m_console.PerformCommand(command, commandArgs, ConsoleSilentMode::NotSilent,
                    ConsoleInvokedFrom::AzConsole, ConsoleFunctorFlags::Null, ConsoleFunctorFlags::Null);
            }
        }

        AZ::Console& m_console;
        AZ::SettingsRegistryInterface& m_settingsRegistry;
        AZStd::string scratchBuffer;
    };

    void Console::RegisterCommandInvokerWithSettingsRegistry(AZ::SettingsRegistryInterface& settingsRegistry)
    {
        // Make sure the there is a JSON object at the ConsoleRuntimeCommandKey or ConsoleAutoexecKey
        // So that JSON Patch is able to add values underneath that object (JSON Patch doesn't create intermediate objects)
        settingsRegistry.MergeSettings(R"({})", SettingsRegistryInterface::Format::JsonMergePatch,
            IConsole::ConsoleRuntimeCommandKey);
        settingsRegistry.MergeSettings(R"({})", SettingsRegistryInterface::Format::JsonMergePatch,
            IConsole::ConsoleAutoexecCommandKey);
        m_consoleCommandKeyHandler = settingsRegistry.RegisterNotifier(ConsoleCommandKeyNotificationHandler{ settingsRegistry, *this });

        settingsRegistry.SetNotifyForMergeOperations(true);
    }
}
