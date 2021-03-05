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

#include <AzCore/Console/Console.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Settings/CommandLine.h>
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

    Console::~Console()
    {
        // on console destruction relink the console functors back to the deferred head
        MoveFunctorsToDeferredHead(AZ::ConsoleFunctorBase::GetDeferredHead());
    }

    bool Console::PerformCommand
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

    bool Console::PerformCommand
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
            return false;
        }

        return PerformCommand(commandAndArgs.front(), ConsoleCommandContainer(commandAndArgs.begin() + 1, commandAndArgs.end()), silentMode, invokedFrom, requiredSet, requiredClear);
    }

    bool Console::PerformCommand
    (
        AZStd::string_view command,
        const ConsoleCommandContainer& commandArgs,
        ConsoleSilentMode silentMode,
        ConsoleInvokedFrom invokedFrom,
        ConsoleFunctorFlags requiredSet,
        ConsoleFunctorFlags requiredClear
    )
    {
        return DispatchCommand(command, commandArgs, silentMode, invokedFrom, requiredSet, requiredClear);
    }

    void Console::ExecuteConfigFile(AZStd::string_view configFileName)
    {
        IO::FixedMaxPath filePathFixed = configFileName;
        if (AZ::IO::FileIOBase* fileIOBase = AZ::IO::FileIOBase::GetInstance())
        {
            fileIOBase->ResolvePath(filePathFixed, configFileName);
        }

        IO::SystemFile file;
        if (!file.Open(filePathFixed.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
        {
            AZLOG_ERROR("Failed to load '%s'. File could not be opened.", filePathFixed.c_str());
            return;
        }
        const IO::SizeType length = file.Length();
        if (length == 0)
        {
            AZLOG_ERROR("Failed to load '%s'. File is empty.", filePathFixed.c_str());
            return;
        }
        file.Seek(0, IO::SystemFile::SF_SEEK_BEGIN);
        AZStd::string fileBuffer;
        fileBuffer.resize(length);
        IO::SizeType bytesRead = file.Read(length, fileBuffer.data());
        file.Close();
        // Resize again just in case bytesRead is less than length for some reason
        fileBuffer.resize(bytesRead);

        AZLOG_INFO("Loading config file %s", filePathFixed.c_str());

        AZStd::vector<AZStd::string_view> separatedCommands;
        auto BreakCommandsByLine = [&separatedCommands](AZStd::string_view token)
        {
            separatedCommands.emplace_back(token);
        };
        StringFunc::TokenizeVisitor(fileBuffer, BreakCommandsByLine, "\n\r");

        for (const auto& commandView : separatedCommands)
        {
            ConsoleCommandContainer commandArgsView;
            auto ConvertCommandStringToArray = [&commandArgsView](AZStd::string_view token)
            {
                commandArgsView.emplace_back(token);
            };
            constexpr AZStd::string_view commandSeparators = " =";
            StringFunc::TokenizeVisitor(commandView, ConvertCommandStringToArray, commandSeparators);
            PerformCommand(commandArgsView, ConsoleSilentMode::NotSilent, ConsoleInvokedFrom::AzConsole, ConsoleFunctorFlags::Null, ConsoleFunctorFlags::Null);
        }
    }

    void Console::ExecuteCommandLine(const AZ::CommandLine& commandLine)
    {
        for (const auto& [switchKey, switchValues] : commandLine.GetSwitchList())
        {
            ConsoleCommandContainer commandArgs(switchValues.begin(), switchValues.end());
            PerformCommand(switchKey, commandArgs, ConsoleSilentMode::NotSilent, ConsoleInvokedFrom::AzConsole, ConsoleFunctorFlags::Null, ConsoleFunctorFlags::Null);
        }
    }

    bool Console::HasCommand(const char* command)
    {
        return FindCommand(command) != nullptr;
    }

    ConsoleFunctorBase* Console::FindCommand(const char* command)
    {
        CVarFixedString lowerName(command);
        AZStd::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](char value) { return std::tolower(value); });

        CommandMap::iterator iter = m_commands.find(lowerName);
        if (iter != m_commands.end())
        {
            for (ConsoleFunctorBase* curr : iter->second)
            {
                if ((curr->GetFlags() & ConsoleFunctorFlags::IsInvisible) == ConsoleFunctorFlags::IsInvisible)
                {
                    // Filter functors marked as invisible
                    continue;
                }
                return curr;
            }
        }
        return nullptr;
    }

    AZStd::string Console::AutoCompleteCommand(const char* command)
    {
        const size_t commandLength = strlen(command);

        if (commandLength <= 0)
        {
            return command;
        }

        ConsoleCommandContainer commandSubset;

        for (ConsoleFunctorBase* curr = m_head; curr != nullptr; curr = curr->m_next)
        {
            if ((curr->GetFlags() & ConsoleFunctorFlags::IsInvisible) == ConsoleFunctorFlags::IsInvisible)
            {
                // Filter functors marked as invisible
                continue;
            }

            if (StringFunc::Equal(curr->m_name, command, false, commandLength))
            {
                AZLOG_INFO("- %s : %s\n", curr->m_name, curr->m_desc);
                commandSubset.push_back(curr->m_name);
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
            visitor(curr.second.front());
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
        AZStd::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](char value) { return std::tolower(value); });
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
        AZStd::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](char value) { return std::tolower(value); });
        CommandMap::iterator iter = m_commands.find(lowerName);
        if (iter != m_commands.end())
        {
            AZStd::vector<ConsoleFunctorBase*>::iterator iter2 = AZStd::find(iter->second.begin(), iter->second.end(), functor);
            if (iter2 != iter->second.end())
            {
                iter->second.erase(iter2);
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
        bool result = false;
        ConsoleFunctorFlags flags = ConsoleFunctorFlags::Null;

        CVarFixedString lowerName(command);
        AZStd::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](char value) { return std::tolower(value); });

        CommandMap::iterator iter = m_commands.find(lowerName);
        if (iter != m_commands.end())
        {
            for (ConsoleFunctorBase* curr : iter->second)
            {
                if ((curr->GetFlags() & requiredSet) != requiredSet)
                {
                    AZLOG_WARN("%s failed required set flag check\n", curr->m_name);
                    continue;
                }

                if ((curr->GetFlags() & requiredClear) != ConsoleFunctorFlags::Null)
                {
                    AZLOG_WARN("%s failed required clear flag check\n", curr->m_name);
                    continue;
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::IsCheat) != ConsoleFunctorFlags::Null)
                {
                    AZLOG_WARN("%s is marked as a cheat\n", curr->m_name);
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::IsDeprecated) != ConsoleFunctorFlags::Null)
                {
                    AZLOG_WARN("%s is marked as deprecated\n", curr->m_name);
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::NeedsReload) != ConsoleFunctorFlags::Null)
                {
                    AZLOG_WARN("Changes to %s will only take effect after level reload\n", curr->m_name);
                }

                // Letting this intentionally fall-through, since in editor we can register common variables multiple times
                (*curr)(inputs);

                if (!result)
                {
                    result = true;
                    if ((silentMode == ConsoleSilentMode::NotSilent) && (curr->GetFlags() & ConsoleFunctorFlags::IsInvisible) != ConsoleFunctorFlags::IsInvisible)
                    {
                        CVarFixedString value;
                        curr->GetValue(value);
                        AZLOG_INFO("> %s : %s\n", curr->GetName(), value.empty() ? "<empty>" : value.c_str());
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
}
