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
#include <cstring>

#include <Atom/Feature/ImGui/SystemBus.h>
#include <ImGuiContextScope.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

using namespace AzFramework;

namespace AZ
{
    namespace
    {
        size_t FindLogTextOffset(const char* begin, const char* end, float mouseX)
        {
            const ImFont* font = ImGui::GetFont();
            const float scale = ImGui::GetFontSize() / font->FontSize;
            float x = 0;
            for (const char* cursor = begin; cursor < end;)
            {
                unsigned int character = 0;
                const int bytes = ImTextCharFromUtf8(&character, cursor, end);
                if (bytes <= 0)
                {
                    break;
                }
                const float width = character == '\r' ? 0 : font->GetCharAdvance(static_cast<ImWchar>(character)) * scale;
                if (mouseX < x + width * 0.5f)
                {
                    return static_cast<size_t>(cursor - begin);
                }
                x += width;
                cursor += bytes;
            }
            return static_cast<size_t>(end - begin);
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
        DrawDebugLog(ImVec2(0, -footerHeightToReserve));

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
        m_debugLogEntires.push_back(AZStd::make_pair(debugLogString, color));
        if (m_debugLogEntires.size() > static_cast<size_t>(m_maxEntriesToDisplay))
        {
            m_debugLogEntires.pop_front();
            ++m_firstLogEntry;
            const LogPosition first{ m_firstLogEntry, 0 };
            m_logSelectionAnchor = AZStd::max(m_logSelectionAnchor, first);
            m_logSelectionEnd = AZStd::max(m_logSelectionEnd, first);
        }
    }

    AZStd::string DebugConsole::GetSelectedDebugLogText() const
    {
        const auto first = AZStd::min(m_logSelectionAnchor, m_logSelectionEnd);
        const auto last = AZStd::max(m_logSelectionAnchor, m_logSelectionEnd);
        AZStd::string selected;
        if (first == last || m_debugLogEntires.empty())
        {
            return selected;
        }
        const size_t begin = static_cast<size_t>(first.first - m_firstLogEntry);
        const size_t end = static_cast<size_t>(last.first - m_firstLogEntry);
        size_t size = 0;
        for (size_t index = begin; index <= end; ++index)
        {
            const auto& text = m_debugLogEntires[index].first;
            const size_t from = index == begin ? first.second : 0;
            const size_t to = index == end ? last.second : text.size();
            size += to - from;
            size += index != end && (text.empty() || text.back() != '\n');
        }
        // Allocate a contiguous string only for an explicit clipboard request, sized once.
        selected.reserve(size);
        for (size_t index = begin; index <= end; ++index)
        {
            const auto& text = m_debugLogEntires[index].first;
            const size_t from = index == begin ? first.second : 0;
            const size_t to = index == end ? last.second : text.size();
            selected.append(text.data() + from, to - from);
            if (index != end && (text.empty() || text.back() != '\n'))
            {
                selected.push_back('\n');
            }
        }
        return selected;
    }

    void DebugConsole::DrawDebugLog(const ImVec2& size)
    {
        ImGui::BeginChild("DebugLogEntriesScrollBox", size, false, ImGuiWindowFlags_HorizontalScrollbar);
        const auto& io = ImGui::GetIO();
        const ImGuiID selectionId = ImGui::GetID("DebugLogSelection");
        const ImRect content = ImGui::GetCurrentWindow()->InnerClipRect;
        const bool clicked = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(content.Min, content.Max) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        const bool resetAnchor = clicked && !io.KeyShift;
        if (clicked)
        {
            ImGui::SetWindowFocus();
            ImGui::SetActiveID(selectionId, ImGui::GetCurrentWindow());
            m_draggingLogSelection = true;
        }
        if (m_draggingLogSelection && ImGui::GetActiveID() != selectionId)
        {
            m_draggingLogSelection = false;
        }
        if (m_draggingLogSelection)
        {
            ImGui::KeepAliveID(selectionId);
        }
        const bool selecting = m_draggingLogSelection;
        const bool highlight = selecting || m_logSelectionAnchor != m_logSelectionEnd;
        const bool wasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
        LogPosition mousePosition{ m_firstLogEntry, 0 };
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        uint64_t entryId = m_firstLogEntry;
        for (const auto& entry : m_debugLogEntires)
        {
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            if (highlight)
            {
                const char* text = entry.first.data();
                const char* end = text + entry.first.size();
                const char* line = text;
                float y = origin.y;
                do
                {
                    const auto* newline = static_cast<const char*>(memchr(line, '\n', static_cast<size_t>(end - line)));
                    const char* lineEnd = newline ? newline : end;
                    const size_t from = static_cast<size_t>(line - text);
                    const size_t textEnd = static_cast<size_t>(lineEnd - text);
                    const size_t to = textEnd + (newline || entryId + 1 < m_firstLogEntry + m_debugLogEntires.size());
                    const float height = entry.first.empty() ? ImGui::GetStyle().ItemSpacing.y : ImGui::GetTextLineHeight();
                    const auto project = [&](LogPosition position)
                    {
                        return position.first < entryId ? from : position.first > entryId ? to : AZStd::clamp(position.second, from, to);
                    };
                    size_t cursor = project(m_logSelectionEnd);
                    if (selecting)
                    {
                        cursor = io.MousePos.y < y        ? from
                            : io.MousePos.y >= y + height ? to
                                                          : from + FindLogTextOffset(line, lineEnd, io.MousePos.x - origin.x);
                        if (io.MousePos.y >= y)
                        {
                            mousePosition = { entryId, AZStd::min(cursor, entry.first.size()) };
                        }
                    }
                    if (!resetAnchor)
                    {
                        const size_t anchor = project(m_logSelectionAnchor);
                        const size_t a = AZStd::min(anchor, cursor);
                        const size_t b = AZStd::max(anchor, cursor);
                        if (a != b && y + height >= ImGui::GetWindowPos().y && y <= ImGui::GetWindowPos().y + ImGui::GetWindowHeight())
                        {
                            const float x1 = ImGui::CalcTextSize(line, text + AZStd::min(a, textEnd)).x;
                            float x2 = ImGui::CalcTextSize(line, text + AZStd::min(b, textEnd)).x;
                            if (b > textEnd)
                            {
                                x2 += ImGui::CalcTextSize(" ").x;
                            }
                            drawList->AddRectFilled(
                                ImVec2(origin.x + x1, y), ImVec2(origin.x + x2, y + height), ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
                        }
                    }
                    y += ImGui::GetTextLineHeight();
                    line = newline ? newline + 1 : end;
                } while (line < end);
            }
            const auto& color = entry.second;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.GetR(), color.GetG(), color.GetB(), color.GetA()));
            ImGui::TextUnformatted(entry.first.data(), entry.first.data() + entry.first.size());
            ImGui::PopStyleColor();
            ++entryId;
        }
        if (selecting)
        {
            m_logSelectionEnd = mousePosition;
            if (resetAnchor)
            {
                m_logSelectionAnchor = mousePosition;
            }
            if (io.MouseDown[ImGuiMouseButton_Left])
            {
                const float top = ImGui::GetWindowPos().y;
                const float bottom = top + ImGui::GetWindowHeight();
                const float outside = io.MousePos.y < top ? io.MousePos.y - top : io.MousePos.y > bottom ? io.MousePos.y - bottom : 0;
                ImGui::SetScrollY(ImGui::GetScrollY() + outside * io.DeltaTime * 10);
            }
            else
            {
                m_draggingLogSelection = false;
                ImGui::ClearActiveID();
            }
        }
        const auto selectAll = [&]()
        {
            if (!m_debugLogEntires.empty())
            {
                m_logSelectionAnchor = { m_firstLogEntry, 0 };
                m_logSelectionEnd = { m_firstLogEntry + m_debugLogEntires.size() - 1, m_debugLogEntires.back().first.size() };
            }
        };
        const bool shortcut = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
        if (ImGui::IsWindowFocused() && shortcut)
        {
            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A), false))
            {
                selectAll();
            }
            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C), false) && m_logSelectionAnchor != m_logSelectionEnd)
            {
                ImGui::SetClipboardText(GetSelectedDebugLogText().c_str());
            }
        }
        if (ImGui::BeginPopupContextWindow("DebugLogContextMenu", ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Copy", nullptr, false, m_logSelectionAnchor != m_logSelectionEnd))
            {
                ImGui::SetClipboardText(GetSelectedDebugLogText().c_str());
            }
            if (ImGui::MenuItem("Select All"))
            {
                selectAll();
            }
            ImGui::EndPopup();
        }
        if (m_forceScroll || (m_autoScroll && wasAtBottom && !selecting))
        {
            ImGui::SetScrollHereY(1.0f);
            m_forceScroll = false;
        }
        ImGui::EndChild();
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
        m_firstLogEntry += m_debugLogEntires.size();
        m_debugLogEntires.clear();
        m_logSelectionAnchor = m_logSelectionEnd = { m_firstLogEntry, 0 };
        m_draggingLogSelection = false;
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
