/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzFramework/Input/Contexts/InputContext.h>
#include <AzFramework/Input/Events/InputChannelEventFilter.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Math/Color.h>

#include <AzCore/std/containers/deque.h>
#include <AzCore/std/string/string.h>

struct ImGuiInputTextCallbackData;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZ
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! The default maximum number of entries to display in the debug log.
    constexpr int DefaultMaxEntriesToDisplay = 1028;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! The default maximum number of input history items to retain.
    constexpr int DefaultMaxInputHistorySize = 512;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! A debug console used to enter debug console commands and display debug log messages.
    //!
    //! Toggled using any of the following:
    //! - The '~' key on a keyboard.
    //! - Both the 'L3+R3' buttons on a gamepad.
    //! - The fourth finger press on a touch screen.
    class DebugConsole : public AZ::TickBus::Handler
                       , public AzFramework::InputChannelEventListener
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(DebugConsole, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] maxEntriesToDisplay The maximum number of entries to display in the debug log.
        //! \param[in] maxInputHistorySize The maximum number of text input history items to retain.
        DebugConsole(int maxEntriesToDisplay = DefaultMaxEntriesToDisplay,
                     int maxInputHistorySize = DefaultMaxInputHistorySize);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(DebugConsole);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~DebugConsole() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler
        int GetTickOrder() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::TickEvents::OnTick
        void OnTick(float deltaTime, AZ::ScriptTimePoint scriptTimePoint) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelEventListener::OnInputChannelEventFiltered
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add a string to the debug log display.
        //! \param[in] debugLogString The string to add to the debug log display.
        //! \param[in] color The color in which to display the above string.
        void AddDebugLog(const AZStd::string& debugLogString, const AZ::Color& color = AZ::Colors::White);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Clears the debug log display.
        void ClearDebugLog();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Attempt to auto complete a command using this input text callback data.
        //! \param[in] data The input text callback data used try and auto complete.
        void AutoCompleteCommand(ImGuiInputTextCallbackData* data);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Attempt to browse the input history using this input text callback data.
        //! \param[in] data The input text callback data used to try and browse history.
        void BrowseInputHistory(ImGuiInputTextCallbackData* data);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when the user enters text input.
        //! \param[in] inputText The text input that was entered.
        void OnTextInputEntered(const char* inputText);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Draw the debug console.
        void DrawDebugConsole();

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::deque<AZStd::pair<AZStd::string, AZ::Color>> m_debugLogEntires; //!< All debug logs.
        AZStd::deque<AZStd::string> m_textInputHistory; //!< History of input that has been entered.
        AZ::ILogger::LogEvent::Handler m_logHandler; //!< Handler that receives log events to display.
        AzFramework::InputContext m_inputContext; //!< Input context used to open/close the console.
        char m_inputBuffer[1028] = {}; //!< The character buffer used to accept text input.
        int m_currentHistoryIndex = -1; //!< The current index into the input history when browsing.
        int m_maxEntriesToDisplay = DefaultMaxEntriesToDisplay; //!< The maximum entries to display.
        int m_maxInputHistorySize = DefaultMaxInputHistorySize; //!< The maximum input history size.
        int m_logLevelToSet = 0; //!< The minimum log level to set (see AZ::LogLevel).
        bool m_isShowing = false; //!< Is the debug console currently being displayed?
        bool m_autoScroll = true; //!< Should we auto-scroll as new entries are added?
        bool m_forceScroll = false; //!< Do we need to force scroll after input entered?
    };
} // namespace AZ
