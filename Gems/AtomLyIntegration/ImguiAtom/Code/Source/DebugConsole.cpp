/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DebugConsole.h>

#if defined(IMGUI_ENABLED)

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Input/Mappings/InputMappingAnd.h>
#include <AzFramework/Input/Mappings/InputMappingOr.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/string_view.h>

#include <Atom/Feature/ImGui/SystemBus.h>
#include <ImGuiContextScope.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

using namespace AzFramework;

namespace AZ
{
    namespace
    {
        constexpr const char* DebugLogTextLabel = "##DebugLogEntriesText";

        ImGuiWindow* FindChildWindow(ImGuiWindow* parentWindow, ImGuiID childId)
        {
            ImGuiContext& context = *GImGui;
            for (int windowIndex = context.Windows.Size - 1; windowIndex >= 0; --windowIndex)
            {
                ImGuiWindow* window = context.Windows[windowIndex];
                if (window->ParentWindow == parentWindow && window->ChildId == childId)
                {
                    return window;
                }
            }
            return nullptr;
        }
    } // namespace

    AZ_CVAR(bool, bg_showDebugConsole, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Enables or disables the debug console within imGui");
    AZ_CVAR(float, bg_defaultDebugConsoleWidth, 960.f, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The default width for the imGui debug console");
    AZ_CVAR(float, bg_defaultDebugConsoleHeight, 480.f, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The default height for the imGui debug console");

    AZ::Color GetColorForLogLevel(const AZ::LogLevel& logLevel)
    {
        switch (logLevel)
        {
            case AZ::LogLevel::Fatal:
            case AZ::LogLevel::Error:
            {
                return AZ::Colors::Red;
            }
            break;
            case AZ::LogLevel::Warn:
            {
                return AZ::Colors::Yellow;
            }
            break;
            case AZ::LogLevel::Notice:
            case AZ::LogLevel::Info:
            case AZ::LogLevel::Debug:
            case AZ::LogLevel::Trace:
            default:
            {
                return AZ::Colors::White;
            }
            break;
        }
    }

    void ResetTextInputField(ImGuiInputTextCallbackData* data, const AZStd::string& newText)
    {
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, newText.c_str());
    }

    int InputTextCallback(ImGuiInputTextCallbackData* data)
    {
        DebugConsole* debugConsole = static_cast<DebugConsole*>(data->UserData);
        if (!debugConsole)
        {
            return 0;
        }

        switch (data->EventFlag)
        {
        case ImGuiInputTextFlags_CallbackCompletion:
        {
            debugConsole->AutoCompleteCommand(data);
        }
        break;
        case ImGuiInputTextFlags_CallbackHistory:
        {
            debugConsole->BrowseInputHistory(data);
        }
        break;
        }
        return 0;
    }

    DebugConsole::DebugConsole(int maxEntriesToDisplay, int maxInputHistorySize)
        : m_maxEntriesToDisplay(maxEntriesToDisplay)
        , m_maxInputHistorySize(maxInputHistorySize)
    {
        // The debug console is currently only supported when running the standalone launcher.
        // It does function correctly when running the editor if you remove this check, but it
        // conflicts with the legacy debug console that also shows at the bottom of the editor.
        AZ::ApplicationTypeQuery applicationType;
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::QueryApplicationType, applicationType);
        if (!applicationType.IsGame())
        {
            return;
        }

        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    DebugConsole::~DebugConsole()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
    }

    void DebugConsole::OnImGuiMainMenuUpdate()
    {
    }

    void DebugConsole::OnImGuiUpdate()
    {
        if (!bg_showDebugConsole)
        {
            return;
        }

        // Draw the debug console in a closeable, moveable, and resizeable IMGUI window.
        bool continueShowing = bg_showDebugConsole;
        ImGui::SetNextWindowSize(ImVec2(bg_defaultDebugConsoleWidth, bg_defaultDebugConsoleHeight), ImGuiCond_Once);
        if (!ImGui::Begin("Debug Console", &continueShowing))
        {
            ImGui::End();
            return;
        }
        bg_showDebugConsole = continueShowing;

        // Show a scrolling region in which to display all debug log entries.
        const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetStyle().FramePadding.y + ImGui::GetFrameHeightWithSpacing();
        DrawDebugLog(ImVec2(-FLT_MIN, -footerHeightToReserve));

        // Show a text input field.
        ImGui::Separator();
        const ImGuiInputTextFlags inputTextFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
        const bool textWasInput = ImGui::InputText("", m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer), inputTextFlags, &InputTextCallback, (void*)this);
        if (textWasInput)
        {
            OnTextInputEntered(m_inputBuffer);
            azstrncpy(m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer), "", 1);
            ImGui::SetKeyboardFocusHere(-1);
            m_forceScroll = true;
        }

        // Focus on the text input field.
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::SetItemDefaultFocus();

        // Show a button to clear the debug log.
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            ClearDebugLog();
        }

        // Show an options menu.
        if (ImGui::BeginPopup("Options"))
        {
            // Show a combo box that controls the minimum log level (options correspond to AZ::LogLevel).
            ImGui::SetNextItemWidth((ImGui::CalcTextSize("WWWWWW").x + ImGui::GetStyle().FramePadding.x) * 2.0f);
            int logLevel = static_cast<int>(AZ::Interface<AZ::ILogger>::Get()->GetLogLevel());
            if (ImGui::Combo("Minimum Log Level", &logLevel, "All\0Trace\0Debug\0Info\0Notice\0Warn\0Error\0Fatal\0\0"))
            {
                logLevel = AZStd::clamp(logLevel, static_cast<int>(AZ::LogLevel::Trace), static_cast<int>(AZ::LogLevel::Fatal));
                AZ::Interface<AZ::ILogger>::Get()->SetLogLevel(static_cast<AZ::LogLevel>(logLevel));
            }

            // Show a checkbox that controls whether to auto scroll when new debug log entires are added.
            ImGui::Checkbox("Auto Scroll New Log Entries", &m_autoScroll);

            ImGui::EndPopup();
        }

        // Show a button to open the options menu.
        ImGui::SameLine();
        if (ImGui::Button("Options"))
        {
            ImGui::OpenPopup("Options");
        }

        ImGui::End();
    }

    void DebugConsole::AddDebugLog(const AZStd::string& debugLogString, const AZ::Color& color)
    {
        if (m_maxEntriesToDisplay <= 0)
        {
            return;
        }
        // Separate adjacent entries unless the previous entry already ended with a newline.
        if (!m_debugLogEntries.empty())
        {
            const auto& previousEntry = m_debugLogEntries.back();
            if (previousEntry.m_begin == previousEntry.m_end || m_debugLogText.back() != '\n')
            {
                m_debugLogText.push_back('\n');
            }
        }

        const size_t begin = m_debugLogText.size();
        m_debugLogText += debugLogString;
        m_debugLogEntries.push_back({ begin, m_debugLogText.size(), color });
        if (m_debugLogEntries.size() > static_cast<size_t>(m_maxEntriesToDisplay))
        {
            m_debugLogEntries.pop_front();
        }

        // Advance the visible range without moving text on each trim. Compact only after reclaiming at least half.
        const size_t discarded = m_debugLogEntries.front().m_begin;
        if (discarded >= 4096 && discarded >= m_debugLogText.size() - discarded)
        {
            m_debugLogText.erase(0, discarded);
            for (auto& entry : m_debugLogEntries)
            {
                entry.m_begin -= discarded;
                entry.m_end -= discarded;
            }
        }
    }

    void DebugConsole::DrawDebugLog(const ImVec2& size)
    {
        ImGuiWindow* parentWindow = ImGui::GetCurrentWindow();
        const ImGuiID debugLogId = parentWindow->GetID(DebugLogTextLabel);

        // InputTextMultiline supplies text selection, drag handling, and platform-native copy
        // shortcuts. Its own text is transparent because the colored entries are redrawn below.
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        const size_t visibleBegin = m_debugLogEntries.empty() ? 0 : m_debugLogEntries.front().m_begin;
        ImGui::InputTextMultiline(
            DebugLogTextLabel,
            const_cast<char*>(m_debugLogText.c_str() + visibleBegin),
            m_debugLogText.size() - visibleBegin + 1,
            size,
            ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        ImGuiWindow* debugLogWindow = FindChildWindow(parentWindow, debugLogId);
        if (!debugLogWindow)
        {
            return;
        }

        const ImGuiInputTextState* inputState = ImGui::GetInputTextState(debugLogId);
        const float horizontalScroll = inputState ? inputState->ScrollX : 0.0f;
        const ImGuiStyle& style = ImGui::GetStyle();
        const ImVec2 textOrigin(
            debugLogWindow->DC.CursorStartPos.x + style.FramePadding.x - horizontalScroll,
            debugLogWindow->DC.CursorStartPos.y + style.FramePadding.y);

        ImDrawList* drawList = debugLogWindow->DrawList;
        drawList->PushClipRect(debugLogWindow->InnerClipRect.Min, debugLogWindow->InnerClipRect.Max, true);

        size_t lineIndex = 0;
        for (auto entryIterator = m_debugLogEntries.begin(); entryIterator != m_debugLogEntries.end(); ++entryIterator)
        {
            const AZStd::string_view entryText(
                m_debugLogText.data() + entryIterator->m_begin, entryIterator->m_end - entryIterator->m_begin);
            if (!entryText.empty())
            {
                const AZ::Color& entryColor = entryIterator->m_color;
                const ImVec4 color(entryColor.GetR(), entryColor.GetG(), entryColor.GetB(), entryColor.GetA());
                drawList->AddText(
                    ImGui::GetFont(),
                    ImGui::GetFontSize(),
                    ImVec2(textOrigin.x, textOrigin.y + lineIndex * ImGui::GetFontSize()),
                    ImGui::GetColorU32(color),
                    entryText.data(),
                    entryText.data() + entryText.size());
            }

            for (char character : entryText)
            {
                lineIndex += character == '\n' ? 1 : 0;
            }
            auto nextEntryIterator = entryIterator;
            const bool hasAnotherEntry = ++nextEntryIterator != m_debugLogEntries.end();
            if (hasAnotherEntry && (entryText.empty() || entryText.back() != '\n'))
            {
                ++lineIndex;
            }
        }

        drawList->PopClipRect();

        if (m_forceScroll || (m_autoScroll && debugLogWindow->Scroll.y >= debugLogWindow->ScrollMax.y))
        {
            ImGui::SetScrollFromPosY(debugLogWindow, debugLogWindow->DC.CursorMaxPos.y - debugLogWindow->Pos.y, 1.0f);
            m_forceScroll = false;
        }
    }

    void DebugConsole::AddDebugLog(const char* window, const char* debugLogString, AZ::LogLevel logLevel)
    {
        AZ::ILogger* logger = AZ::Interface<AZ::ILogger>::Get();
        if (logger == nullptr || logLevel < logger->GetLogLevel())
        {
            return;
        }

        AZ::Color color = GetColorForLogLevel(logLevel);
        if (strcmp(window, AZ::Debug::Trace::GetDefaultSystemWindow()) == 0)
        {
            AddDebugLog(debugLogString, color);
        }
        else
        {
            AddDebugLog(AZStd::string::format("(%s) - %s", window, debugLogString), color);
        }
    }

    void DebugConsole::ClearDebugLog()
    {
        m_debugLogEntries.clear();
        m_debugLogText.clear();
    }

    bool DebugConsole::OnPreError(const char* window, [[maybe_unused]] const char* fileName, [[maybe_unused]] int line, [[maybe_unused]] const char* func, const char* message)
    {
        AddDebugLog(window, message, AZ::LogLevel::Error);
        return false;
    }

    bool DebugConsole::OnPreWarning(const char* window, [[maybe_unused]] const char* fileName, [[maybe_unused]] int line, [[maybe_unused]] const char* func, const char* message)
    {
        AddDebugLog(window, message, AZ::LogLevel::Warn);
        return false;
    }

    bool DebugConsole::OnPrintf(const char* window, const char* message)
    {
        AddDebugLog(window, message, AZ::LogLevel::Notice); // Notice is one level below warning
        return false;
    }

    void DebugConsole::AutoCompleteCommand(ImGuiInputTextCallbackData* data)
    {
        AZStd::vector<AZStd::string> matchingCommands;
        const AZStd::string longestMatchingSubstring = AZ::Interface<AZ::IConsole>::Get()->AutoCompleteCommand(data->Buf, &matchingCommands);
        ResetTextInputField(data, longestMatchingSubstring);

        // Auto complete options are logged in AutoCompleteCommand using AZLOG_INFO,
        // so if the log level is set higher we display auto complete options here.
        if (AZ::Interface<AZ::ILogger>::Get()->GetLogLevel() > AZ::LogLevel::Info)
        {
            if (matchingCommands.empty())
            {
                AZStd::string noAutoCompletionResults("No auto completion options: ");
                noAutoCompletionResults += data->Buf;
                AddDebugLog(noAutoCompletionResults, AZ::Colors::Gray);
            }
            else if (matchingCommands.size() > 1)
            {
                AZStd::string autoCompletionResults("Auto completion options: ");
                autoCompletionResults += data->Buf;
                AddDebugLog(autoCompletionResults, AZ::Colors::Green);
                for (const AZStd::string& matchingCommand : matchingCommands)
                {
                    AddDebugLog(matchingCommand, AZ::Colors::Green);
                }
            }
        }
    }

    void DebugConsole::BrowseInputHistory(ImGuiInputTextCallbackData* data)
    {
        const int previousHistoryIndex = m_currentHistoryIndex;
        const int maxHistoryIndex = static_cast<int>(m_textInputHistory.size() - 1);
        switch (data->EventKey)
        {
            // Browse backwards through the history.
            case ImGuiKey_UpArrow:
            {
                if (m_currentHistoryIndex < 0)
                {
                    // Go to the last history entry.
                    m_currentHistoryIndex = maxHistoryIndex;
                }
                else if (m_currentHistoryIndex > 0)
                {
                    // Go to the previous history entry.
                    --m_currentHistoryIndex;
                }
            }
            break;

            // Browse forwards through the history.
            case ImGuiKey_DownArrow:
            {
                if (m_currentHistoryIndex >= 0 && m_currentHistoryIndex < maxHistoryIndex)
                {
                    ++m_currentHistoryIndex;
                }
            }
            break;
        }

        if (previousHistoryIndex != m_currentHistoryIndex)
        {
            ResetTextInputField(data, m_textInputHistory[m_currentHistoryIndex]);
        }
    }

    void DebugConsole::OnTextInputEntered(const char* inputText)
    {
        // Add the input text to our history, removing the oldest entry if we exceed the maximum.
        const AZStd::string inputTextString(inputText);
        m_textInputHistory.push_back(inputTextString);
        if (m_textInputHistory.size() > m_maxInputHistorySize)
        {
            m_textInputHistory.pop_front();
        }

        // Clear the current history index;
        m_currentHistoryIndex = -1;

        // Attempt to perform a console command.
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(inputTextString.c_str());
    }
}

#endif // defined(IMGUI_ENABLED)
