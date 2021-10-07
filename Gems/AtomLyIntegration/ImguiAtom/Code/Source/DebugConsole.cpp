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

#include <Atom/Feature/ImGui/SystemBus.h>
#include <ImGuiContextScope.h>
#include <ImGui/ImGuiPass.h>
#include <imgui/imgui.h>

using namespace AzFramework;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZ
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    constexpr const char* DebugConsoleInputContext = "DebugConsoleInputContext";
    const InputChannelId ToggleDebugConsoleInputChannelId("ToggleDebugConsole");
    const InputChannelId ThumbstickL3AndR3InputChannelId("ThumbstickL3AndR3");

    ////////////////////////////////////////////////////////////////////////////////////////////////
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

    ////////////////////////////////////////////////////////////////////////////////////////////////
    DebugConsole::DebugConsole(int maxEntriesToDisplay, int maxInputHistorySize)
        : InputChannelEventListener(InputChannelEventListener::GetPriorityFirst(), true)
        , m_logHandler
        (
            [this](AZ::LogLevel logLevel,
                   const char* message,
                   [[maybe_unused]]const char* file,
                   [[maybe_unused]]const char* function,
                   [[maybe_unused]]int32_t line)
            {
                AddDebugLog(message, GetColorForLogLevel(logLevel));
            }
        )
        , m_inputContext(DebugConsoleInputContext, {nullptr, InputChannelEventListener::GetPriorityFirst(), true, true})
        , m_maxEntriesToDisplay(maxEntriesToDisplay)
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

        // Create an input mapping so that the debug console can be toggled by 'L3+R3' on a gamepad.
        AZStd::shared_ptr<InputMappingAnd> inputMappingL3AndR3 = AZStd::make_shared<InputMappingAnd>(ThumbstickL3AndR3InputChannelId, m_inputContext);
        inputMappingL3AndR3->AddSourceInput(InputDeviceGamepad::Button::L3);
        inputMappingL3AndR3->AddSourceInput(InputDeviceGamepad::Button::R3);
        m_inputContext.AddInputMapping(inputMappingL3AndR3);

        // Create an input mapping so that the debug console can be toggled by any of:
        // - The '~' key on a keyboard.
        // - Both the 'L3+R3' buttons on a gamepad.
        // - The fourth finger press on a touch screen.
        AZStd::shared_ptr<InputMappingOr> inputMappingToggleConsole = AZStd::make_shared<InputMappingOr>(ToggleDebugConsoleInputChannelId, m_inputContext);
        inputMappingToggleConsole->AddSourceInput(InputDeviceKeyboard::Key::PunctuationTilde);
        inputMappingToggleConsole->AddSourceInput(inputMappingL3AndR3->GetInputChannelId());
        inputMappingToggleConsole->AddSourceInput(InputDeviceTouch::Touch::Index4);
        m_inputContext.AddInputMapping(inputMappingToggleConsole);

        // Filter so the input context only listens for input events that can map to
        // 'ToggleDebugConsoleInputChannelId', and so the debug console only listens
        // for the custom 'ToggleDebugConsoleInputChannelId' id (optimization only).
        AZStd::shared_ptr<InputChannelEventFilterInclusionList> inputFilter = AZStd::make_shared<InputChannelEventFilterInclusionList>();
        inputFilter->IncludeChannelName(InputDeviceGamepad::Button::L3.GetNameCrc32());
        inputFilter->IncludeChannelName(InputDeviceGamepad::Button::R3.GetNameCrc32());
        inputFilter->IncludeChannelName(inputMappingL3AndR3->GetInputChannelId().GetNameCrc32());
        inputFilter->IncludeChannelName(InputDeviceKeyboard::Key::PunctuationTilde.GetNameCrc32());
        inputFilter->IncludeChannelName(InputDeviceTouch::Touch::Index4.GetNameCrc32());
        m_inputContext.SetFilter(inputFilter);
        inputFilter = AZStd::make_shared<InputChannelEventFilterInclusionList>();
        inputFilter->IncludeChannelName(inputMappingToggleConsole->GetInputChannelId().GetNameCrc32());
        SetFilter(inputFilter);

        // Bind our custom log handler.
        AZ::ILogger* loggerInstance = AZ::Interface<AZ::ILogger>::Get();
        AZ_Assert(loggerInstance, "Failed to get ILogger instance. Log handler not bound.")
        if (loggerInstance)
        {
            loggerInstance->BindLogHandler(m_logHandler);
        }

        // Connect to receive render tick events.
        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        const AZ::Name contextName = atomViewportRequests->GetDefaultViewportContextName();
        AZ::RPI::ViewportContextNotificationBus::Handler::BusConnect(contextName);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    DebugConsole::~DebugConsole()
    {
        // Disconnect to stop receiving render tick events.
        AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();

        // Disconnect our custom log handler.
        m_logHandler.Disconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void DebugConsole::OnRenderTick()
    {
        if (!m_isShowing)
        {
            return;
        }

        const bool continueShowing = DrawDebugConsole();
        if (!continueShowing)
        {
            ToggleIsShowing();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool DebugConsole::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        if (inputChannel.GetInputChannelId() == ToggleDebugConsoleInputChannelId &&
            inputChannel.IsStateBegan())
        {
            ToggleIsShowing();
            return true;
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void DebugConsole::AddDebugLog(const AZStd::string& debugLogString, const AZ::Color& color)
    {
        // Add the debug to our display, removing the oldest entry if we exceed the maximum.
        m_debugLogEntires.push_back(AZStd::make_pair<AZStd::string, AZ::Color>(debugLogString, color));
        if (m_debugLogEntires.size() > m_maxEntriesToDisplay)
        {
            m_debugLogEntires.pop_front();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void DebugConsole::ClearDebugLog()
    {
        m_debugLogEntires.clear();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ResetTextInputField(ImGuiInputTextCallbackData* data, const AZStd::string& newText)
    {
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, newText.c_str());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
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

    ////////////////////////////////////////////////////////////////////////////////////////////////
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

    ////////////////////////////////////////////////////////////////////////////////////////////////
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

    ////////////////////////////////////////////////////////////////////////////////////////////////
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

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool DebugConsole::DrawDebugConsole()
    {
        // Get the default ImGui pass.
        AZ::Render::ImGuiPass* defaultImGuiPass = nullptr;
        AZ::Render::ImGuiSystemRequestBus::BroadcastResult(defaultImGuiPass, &AZ::Render::ImGuiSystemRequestBus::Events::GetDefaultImGuiPass);
        if (!defaultImGuiPass)
        {
            return false;
        }

        // Create an ImGui context scope using the default ImGui pass context.
        ImGui::ImGuiContextScope contextScope(defaultImGuiPass->GetContext());

        // Draw the debug console in a closeable, moveable, and resizeable IMGUI window.
        bool continueShowing = true;
        ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_Once);
        if (!ImGui::Begin("Debug Console", &continueShowing, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return false;
        }

        // Show a scrolling child region in which to display all debug log entires.
        const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetStyle().FramePadding.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("DebugLogEntriesScrollBox", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            // Display each debug log entry individually so they can be colored.
            for (const auto& debugLogEntry : m_debugLogEntires)
            {
                const ImVec4 color(debugLogEntry.second.GetR(),
                                   debugLogEntry.second.GetG(),
                                   debugLogEntry.second.GetB(),
                                   debugLogEntry.second.GetA());
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(debugLogEntry.first.c_str());
                ImGui::PopStyleColor();
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

        return continueShowing;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AzFramework::SystemCursorState GetDesiredSystemCursorState()
    {
        AZ::ApplicationTypeQuery applicationType;
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::QueryApplicationType, applicationType);
        return applicationType.IsEditor() ?
               AzFramework::SystemCursorState::ConstrainedAndVisible :
               AzFramework::SystemCursorState::UnconstrainedAndVisible;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void DebugConsole::ToggleIsShowing()
    {
        m_isShowing = !m_isShowing;
        if (m_isShowing)
        {
            AzFramework::InputSystemCursorRequestBus::EventResult(m_previousSystemCursorState,
                                                                  AzFramework::InputDeviceMouse::Id,
                                                                  &AzFramework::InputSystemCursorRequests::GetSystemCursorState);
            AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                            &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                            GetDesiredSystemCursorState());
        }
        else
        {
            AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                            &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                            m_previousSystemCursorState);
        }
    }
}

#endif // defined(IMGUI_ENABLED)
