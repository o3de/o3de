/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Console/ILogger.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Debug/TraceMessageBus.h>

#include <AzCore/std/containers/deque.h>
#include <AzCore/std/string/string.h>

#include <Atom/RPI.Public/ViewportContextBus.h>

#ifdef IMGUI_ENABLED
#   include <imgui/imgui.h>
#   include <ImGuiBus.h>
#endif

struct ImGuiInputTextCallbackData;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZ
{
#if !defined(IMGUI_ENABLED)
    class DebugConsole {};
#else
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! A debug console used to enter debug console commands and display debug log messages.
    //!
    //! Toggled using any of the following:
    //! - The '~' key on a keyboard.
    //! - Both the 'L3+R3' buttons on a gamepad.
    //! - The fourth finger press on a touch screen.
    class DebugConsole
        : public AZ::Debug::TraceMessageBus::Handler
        , public ImGui::ImGuiUpdateListenerBus::Handler
    {
        //! The default maximum number of entries to display in the debug log.
        static constexpr int DefaultMaxEntriesToDisplay = 1028;

        //! The default maximum number of input history items to retain.
        static constexpr int DefaultMaxInputHistorySize = 512;

    public:
        //! Allocator
        AZ_CLASS_ALLOCATOR(DebugConsole, AZ::SystemAllocator);

        //! Constructor
        //! \param[in] maxEntriesToDisplay The maximum number of entries to display in the debug log.
        //! \param[in] maxInputHistorySize The maximum number of text input history items to retain.
        DebugConsole(int maxEntriesToDisplay = DefaultMaxEntriesToDisplay,
                     int maxInputHistorySize = DefaultMaxInputHistorySize);

        //! Disable copying
        AZ_DISABLE_COPY_MOVE(DebugConsole);

        //! Destructor
        ~DebugConsole() override;

        //! AZ::Debug::TraceMessageBus
        bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPrintf(const char* window, const char* message) override;

        //! ImGui::ImGuiUpdateListenerBus overrides
        //! @{
        void OnImGuiMainMenuUpdate() override;
        void OnImGuiUpdate() override;
        //! @}

        //! Add a string to the debug log display.
        //! \param[in] debugLogString The string to add to the debug log display.
        //! \param[in] color The color in which to display the above string.
        void AddDebugLog(const AZStd::string& debugLogString, const AZ::Color& color = AZ::Colors::White);

        //! Clears the debug log display.
        void ClearDebugLog();

        //! Attempt to auto complete a command using this input text callback data.
        //! \param[in] data The input text callback data used try and auto complete.
        void AutoCompleteCommand(ImGuiInputTextCallbackData* data);

        //! Attempt to browse the input history using this input text callback data.
        //! \param[in] data The input text callback data used to try and browse history.
        void BrowseInputHistory(ImGuiInputTextCallbackData* data);

        //! Called when the user enters text input.
        //! \param[in] inputText The text input that was entered.
        void OnTextInputEntered(const char* inputText);

    private:
        void AddDebugLog(const char* window, const char* debugLogString, AZ::LogLevel logLevel);

        AZStd::deque<AZStd::pair<AZStd::string, AZ::Color>> m_debugLogEntires; //!< All debug logs.
        AZStd::deque<AZStd::string> m_textInputHistory; //!< History of input that has been entered.
        char m_inputBuffer[1028] = {}; //!< The character buffer used to accept text input.
        int m_currentHistoryIndex = -1; //!< The current index into the input history when browsing.
        int m_maxEntriesToDisplay = DefaultMaxEntriesToDisplay; //!< The maximum entries to display.
        int m_maxInputHistorySize = DefaultMaxInputHistorySize; //!< The maximum input history size.
        bool m_autoScroll = true; //!< Should we auto-scroll as new entries are added?
        bool m_forceScroll = false; //!< Do we need to force scroll after input entered?
    };
#endif // defined(IMGUI_ENABLED)
} // namespace AZ
