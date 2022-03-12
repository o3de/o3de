/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "StartupTraceHandler.h"

// Qt
#include <QMessageBox>

// AzCore
#include <AzCore/Component/TickBus.h>

// AzToolsFramework
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

// Editor
#include "ErrorDialog.h"


namespace SandboxEditor
{
    StartupTraceHandler::StartupTraceHandler()
    {
        ConnectToMessageBus();
    }

    StartupTraceHandler::~StartupTraceHandler()
    {
        EndCollectionAndShowCollectedErrors();
        DisconnectFromMessageBus();
    }

    bool StartupTraceHandler::OnPreAssert(const char* fileName, int line, const char* func, const char* message)
    {
        // Asserts are more fatal than errors, and need to be displayed right away.
        // After the assert occurs, nothing else may be functional enough to collect and display messages.

        // Only use Cry message boxes if we aren't using native dialog boxes
#ifndef USE_AZ_ASSERT
        if (message == nullptr || message[0] == 0)
        {
            AZStd::string emptyText = AZStd::string::format("Assertion failed in %s %s:%i", func, fileName, line);
            OnMessage(emptyText.c_str(), nullptr, MessageDisplayBehavior::AlwaysShow);
        }
        else
        {
            OnMessage(message, nullptr, MessageDisplayBehavior::AlwaysShow);
        }
#else
        AZ_UNUSED(fileName);
        AZ_UNUSED(line);
        AZ_UNUSED(func);
        AZ_UNUSED(message);
#endif // !USE_AZ_ASSERT

        // Return false so other listeners can handle this. The StartupTraceHandler won't report messages
        // will probably crash before that occurs, because this is an assert.
        return false;
    }

    bool StartupTraceHandler::OnException(const char* message)
    {
        // Exceptions are more fatal than errors, and need to be displayed right away.
        // After the exception occurs, nothing else may be functional enough to collect and display messages.
        OnMessage(message, nullptr, MessageDisplayBehavior::AlwaysShow);
        // Return false so other listeners can handle this. The StartupTraceHandler won't report messages 
        // until the next time the main thread updates the system tick bus function queue. The editor 
        // will probably crash before that occurs, because this is an exception.
        return false;
    }

    bool StartupTraceHandler::OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        // If a collection is not active, then show this error. Otherwise, collect it 
        // and show it with other occurs that occured in the operation.
        return OnMessage(message, &m_errors, MessageDisplayBehavior::ShowWhenNotCollecting);
    }

    bool StartupTraceHandler::OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        // Only track warnings if messages are being collected.
        return OnMessage(message, &m_warnings, MessageDisplayBehavior::OnlyCollect);
    }

    bool StartupTraceHandler::OnMessage(
        const char *message,
        AZStd::list<QString>* messageList,
        MessageDisplayBehavior messageDisplayBehavior)
    {
        if (m_isCollecting && messageList)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            messageList->push_back(QString(message));
            return true;
        }

        if ((!m_isCollecting && messageDisplayBehavior == MessageDisplayBehavior::ShowWhenNotCollecting) ||
            messageDisplayBehavior == MessageDisplayBehavior::AlwaysShow)
        {
            ShowMessageBox(message);
            return true;
        }

        return false;
    }

    void StartupTraceHandler::StartCollection()
    {
        ConnectToMessageBus();
        if (m_isCollecting)
        {
            EndCollectionAndShowCollectedErrors();
        }
        m_isCollecting = true;
    }

    void StartupTraceHandler::EndCollectionAndShowCollectedErrors()
    {
        AZStd::list<QString> cachedWarnings;
        AZStd::list<QString> cachedErrors;
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            m_isCollecting = false;
            if (m_warnings.size() == 0 && m_errors.size() == 0)
            {
                return;
            }
            if (m_warnings.size() > 0)
            {
                cachedWarnings.splice(cachedWarnings.end(), m_warnings);
            }
            if (m_errors.size() > 0)
            {
                cachedErrors.splice(cachedErrors.end(), m_errors);
            }
        }
        if (m_showWindow)
        {
            AZ::SystemTickBus::QueueFunction([cachedWarnings, cachedErrors]() 
            {
                // Parent to the main window, so that the error dialog doesn't
                // show up as a separate window when alt-tabbing.
                QWidget* mainWindow = nullptr;
                AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                    mainWindow,
                    &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

                ErrorDialog* errorDialog = new ErrorDialog(mainWindow);
                errorDialog->AddMessages(
                    SandboxEditor::ErrorDialog::MessageType::Warning,
                    cachedWarnings);
                errorDialog->AddMessages(
                    SandboxEditor::ErrorDialog::MessageType::Error,
                    cachedErrors);

                // Use open() instead of exec() here so that we still achieve the modal dialog functionality without
                // blocking the event queue
                errorDialog->setAttribute(Qt::WA_DeleteOnClose, true);
                errorDialog->open();
            });
        }
    }

    void StartupTraceHandler::ShowMessageBox(const QString& message)
    {
        AZ::SystemTickBus::QueueFunction([message]()
        {
            // Parent to the main window, so that the error dialog doesn't
            // show up as a separate window when alt-tabbing.
            QWidget* mainWindow = nullptr;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                mainWindow,
                &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

            QMessageBox* msg = new QMessageBox(
                QMessageBox::Critical, 
                QObject::tr("Error"), 
                message, 
                QMessageBox::Ok,
                mainWindow);
            msg->setTextInteractionFlags(Qt::TextSelectableByMouse);

            // Use open() instead of exec() here so that we still achieve the modal dialog functionality without
            // blocking the event queue
            msg->setAttribute(Qt::WA_DeleteOnClose, true);
            msg->open();
        });
    }

    void StartupTraceHandler::ConnectToMessageBus()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    void StartupTraceHandler::DisconnectFromMessageBus()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    bool StartupTraceHandler::IsConnectedToMessageBus() const
    {
        return AZ::Debug::TraceMessageBus::Handler::BusIsConnected();
    }
}
