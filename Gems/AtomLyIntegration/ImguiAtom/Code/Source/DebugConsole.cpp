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

#include <Atom/Feature/ImGui/SystemBus.h>
#include <ImGuiContextScope.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

using namespace AzFramework;

namespace AZ
{
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

        // Show a scrolling child region in which to display all debug log entires.
        const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetStyle().FramePadding.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("DebugLogEntriesScrollBox", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            const auto& io = ImGui::GetIO();
            const ImGuiID selectionId = ImGui::GetID("LogSelection");
            const ImRect clip = ImGui::GetCurrentWindow()->InnerClipRect;
            const bool clicked = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(clip.Min, clip.Max) && ImGui::IsMouseClicked(0);
            if (clicked)
            {
                ImGui::SetWindowFocus();
                ImGui::SetActiveID(selectionId, ImGui::GetCurrentWindow());
                m_logSelection[1] = {};
            }
            const bool dragging = ImGui::GetActiveID() == selectionId;
            size_t entryIndex = 0;
            // Display each debug log entry individually so they can be colored.
            for (const auto& debugLogEntry : m_debugLogEntires)
            {
                const auto& text = debugLogEntry.first;
                if (dragging ||
                    (AZStd::min(m_logSelection[0], m_logSelection[1]).first <= entryIndex &&
                     entryIndex <= AZStd::max(m_logSelection[0], m_logSelection[1]).first && m_logSelection[0] != m_logSelection[1]))
                {
                    const ImVec2 origin = ImGui::GetCursorScreenPos();
                    const char* line = text.data();
                    const char* end = line + text.size();
                    float y = origin.y;
                    do
                    {
                        const char* newline = AZStd::find(line, end, '\n');
                        const size_t from = static_cast<size_t>(line - text.data());
                        const size_t lineEnd = static_cast<size_t>(newline - text.data());
                        const size_t to = lineEnd +
                            (newline != end || (&debugLogEntry != &m_debugLogEntires.back() && (text.empty() || text.back() != '\n')));
                        const float height = text.empty() ? ImGui::GetStyle().ItemSpacing.y : ImGui::GetTextLineHeight();
                        LogPosition cursor = m_logSelection[1];
                        if (dragging)
                        {
                            const char* hit = newline;
                            if (io.MousePos.y >= y && io.MousePos.y < y + height)
                            {
                                ImGui::GetFont()->CalcTextSizeA(
                                    ImGui::GetFontSize(), AZStd::max(0.0f, io.MousePos.x - origin.x) + 0.01f, 0, line, newline, &hit);
                            }
                            cursor = { entryIndex,
                                       io.MousePos.y < y                 ? from
                                           : io.MousePos.y >= y + height ? to
                                                                         : static_cast<size_t>(hit - text.data()) };
                            if (io.MousePos.y >= y)
                            {
                                m_logSelection[1] = { entryIndex, AZStd::min(cursor.second, text.size()) };
                            }
                        }
                        const LogPosition begin{ entryIndex, from };
                        const LogPosition limit{ entryIndex, to };
                        const size_t a = AZStd::clamp(AZStd::min(m_logSelection[0], cursor), begin, limit).second;
                        const size_t b = AZStd::clamp(AZStd::max(m_logSelection[0], cursor), begin, limit).second;
                        if (!clicked && a < b && y + height >= clip.Min.y && y < clip.Max.y)
                        {
                            const float x1 = ImGui::CalcTextSize(line, text.data() + AZStd::min(a, lineEnd)).x;
                            const float x2 = ImGui::CalcTextSize(line, text.data() + AZStd::min(b, lineEnd)).x +
                                (b > lineEnd ? ImGui::CalcTextSize(" ").x : 0);
                            ImGui::GetWindowDrawList()->AddRectFilled(
                                ImVec2(origin.x + x1, y), ImVec2(origin.x + x2, y + height), ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
                        }
                        y += height;
                        line = newline != end ? newline + 1 : end;
                    } while (line < end);
                }
                const ImVec4 color(debugLogEntry.second.GetR(),
                    debugLogEntry.second.GetG(),
                    debugLogEntry.second.GetB(),
                    debugLogEntry.second.GetA());
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(debugLogEntry.first.c_str());
                ImGui::PopStyleColor();
                ++entryIndex;
            }
            if (clicked)
            {
                m_logSelection[0] = m_logSelection[1];
            }
            if (dragging && !ImGui::IsMouseDown(0))
            {
                ImGui::ClearActiveID();
            }
            if (ImGui::IsWindowFocused() && (io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl) &&
                ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C), false) && m_logSelection[0] != m_logSelection[1])
            {
                AZStd::string selected;
                const auto first = AZStd::min(m_logSelection[0], m_logSelection[1]);
                const auto last = AZStd::max(m_logSelection[0], m_logSelection[1]);
                for (size_t index = first.first; index <= last.first; ++index)
                {
                    const auto& text = m_debugLogEntires[index].first;
                    const size_t a = index == first.first ? first.second : 0;
                    const size_t b = index == last.first ? last.second : text.size();
                    selected.append(text.data() + a, b - a);
                    if (index != last.first && (text.empty() || text.back() != '\n'))
                    {
                        selected.push_back('\n');
                    }
                }
                ImGui::SetClipboardText(selected.c_str());
            }

            // Scroll to the last debug log entry if needed.
            if (m_forceScroll || (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            {
                ImGui::SetScrollHereY(1.0f);
                m_forceScroll = false;
            }
        }
        ImGui::EndChild();

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
        // Add the debug to our display, removing the oldest entry if we exceed the maximum.
        m_debugLogEntires.push_back(AZStd::make_pair(debugLogString, color));
        if (m_debugLogEntires.size() > m_maxEntriesToDisplay)
        {
            for (auto& endpoint : m_logSelection)
            {
                endpoint = endpoint.first ? LogPosition{ endpoint.first - 1, endpoint.second } : LogPosition{};
            }
            m_debugLogEntires.pop_front();
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
        m_debugLogEntires.clear();
        m_logSelection[0] = m_logSelection[1] = {};
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
