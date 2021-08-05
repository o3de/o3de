/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/TraceMessageBus.h>

namespace SandboxEditor
{
    /// Class to display errors during startup.
    /// This is a system lifted from CryEdit.cpp and made thread safe.
    /// It existed to handle errors that occur during editor startup, before
    /// the regular error handler is loaded and available.
    class StartupTraceHandler
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        StartupTraceHandler();
        ~StartupTraceHandler();

        //////////////////////////////////////////////////////////////////////
        // AZ::Debug::TraceMessageBus::Handler overrides
        //////////////////////////////////////////////////////////////////////
        bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override;
        bool OnException(const char* message) override;
        bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        
        //////////////////////////////////////////////////////////////////////
        // Public interface
        //////////////////////////////////////////////////////////////////////
        //! Tells the trace handler to start collecting messages, instead of displaying them as they occur.
        //! Connects to the message bus to make sure collection can occur.
        void StartCollection();
        //! Ends collection, and displays all collected messages in one dialog.
        void EndCollectionAndShowCollectedErrors();
        //! @return if any errors occured during level load.
        bool HasAnyErrors() const { return m_errors.size() > 0; }

        //! Connects the trace handler to the trace message bus.
        void ConnectToMessageBus();
        //! Disconnects the trace handler from the trace message bus.
        void DisconnectFromMessageBus();

        //! @return if the trace handler is connected to the message bus.
        bool IsConnectedToMessageBus() const;

        //! Sets whether to display the error window or not
        void SetShowWindow(bool showWindow) { m_showWindow = showWindow; }

    private:

        /// The display behavior for messages.
        /// Some, like warnings, are only showed in a shared message dialog.
        /// Others, like Exceptions, are likely fatal and need to be displayed immediately. The program
        /// state after a fatal error may not be recoverable, so these messages can't be collected
        /// to be displayed later.
        enum class MessageDisplayBehavior
        {
            AlwaysShow,             ///< Always show this message, when collecting or not collecting messages.
            ShowWhenNotCollecting,  ///< Only show this message when collecting is not active, otherwise 
                                    ///< the collection system will show this later.
            OnlyCollect,            ///< Only collect this message, don't show it if not collecting.
        };

        // Returns whether or not the message was handled
        bool OnMessage(
            const char *message,
            AZStd::list<QString>* messageList,
            MessageDisplayBehavior messageDisplayBehavior);

        void ShowMessageBox(const QString& message);

        mutable AZStd::recursive_mutex m_mutex; ///< Messages can come in from multiple threads, so use a mutex to lock things when necessary.

        AZStd::list<QString> m_errors; ///< The list of errors that occured while collecting.
        AZStd::list<QString> m_warnings; ///< The list of warnings that occured while collecting.
        bool m_isCollecting = false; ///< Tracks if the trace handler is collecting messages or displaying them as they occur.
        bool m_showWindow = true; ///< Whether it should display the window

        ///< The list of messages that occured on a thread that can't display a Qt popup.
        ///< These are intended for 1 at a time popups.
        AZStd::list<QString> m_mainThreadMessages;

    };
}
