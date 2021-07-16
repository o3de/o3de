/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <RemoteConsoleCore.h>


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
CRemoteConsole::CRemoteConsole()
    : m_listener(1)
    , m_lastPortValue(0)
    , m_running(false)

    , m_pServer(nullptr)
    , m_pLogEnableRemoteConsole(nullptr)
    , m_remoteConsoleAllowedHostList(nullptr)
    , m_remoteConsolePort(nullptr)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////
CRemoteConsole::~CRemoteConsole()
{
    Stop();
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::RegisterConsoleVariables()
{
    m_pLogEnableRemoteConsole = REGISTER_INT("log_EnableRemoteConsole", 1, VF_DUMPTODISK, "enables/disables the remote console");
    m_remoteConsoleAllowedHostList = REGISTER_STRING("log_RemoteConsoleAllowedAddresses", "", VF_DUMPTODISK, "COMMA separated list of allowed hosts or IP addresses which can connect");
    m_remoteConsolePort = REGISTER_INT("log_RemoteConsolePort", defaultRemoteConsolePort, VF_DUMPTODISK, "Base port (4600 for example) for remote console to listen on.  It will start there and continue upwards until an unused one is found.");
    m_lastPortValue = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::UnregisterConsoleVariables()
{
    m_pLogEnableRemoteConsole = nullptr;
    m_remoteConsoleAllowedHostList = nullptr;
    m_remoteConsolePort = nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::Start()
{
    if (!IsStarted())
    {
        m_pServer = new SRemoteServer;

        m_pServer->StartServer();
        m_running = true;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::Stop()
{
    // make sure we don't stop if we never started the remote console in the first place
    if (IsStarted())
    {
        m_running = false;
        m_pServer->StopServer();
        m_pServer->WaitForThread();

        delete m_pServer;
        m_pServer = nullptr;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::AddLogMessage(const char* log)
{
    if (!IsStarted())
    {
        return;
    }

    IRemoteEvent* pEvent = new SStringEvent<eCET_LogMessage>(log);
    m_pServer->AddEvent(pEvent);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::AddLogWarning(const char* log)
{
    if (!IsStarted())
    {
        return;
    }

    IRemoteEvent* pEvent = new SStringEvent<eCET_LogWarning>(log);
    m_pServer->AddEvent(pEvent);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::AddLogError(const char* log)
{
    if (!IsStarted())
    {
        return;
    }

    IRemoteEvent* pEvent = new SStringEvent<eCET_LogError>(log);
    m_pServer->AddEvent(pEvent);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::Update()
{
    if (m_pLogEnableRemoteConsole)
    {
        // we disable the remote console in the editor, since there is no reason to remote into it and we don't want it eating up that port
        // number anyway and preventing the game from using it.
        bool isEditor = gEnv && gEnv->IsEditor();
        bool isEnabled = (!isEditor) && (m_pLogEnableRemoteConsole->GetIVal());
        bool isStarted = IsStarted();

        int newPortValue = m_remoteConsolePort->GetIVal();

        // the editor never allows remote control.
        if ((isEnabled) && (!isStarted))
        {
            Start();
        }
        else if (isStarted)
        {
            if ((!isEnabled) || (newPortValue != m_lastPortValue))
            {
                Stop();
            }
        }

        m_lastPortValue = newPortValue;

    }

    if (m_pServer)
    {
        TEventBuffer events;
        m_pServer->GetEvents(events);
        for (TEventBuffer::iterator it = events.begin(), end = events.end(); it != end; ++it)
        {
            IRemoteEvent* pEvent = *it;
            switch (pEvent->GetType())
            {
            case eCET_ConsoleCommand:
                for (TListener::Notifier notifier(m_listener); notifier.IsValid(); notifier.Next())
                {
                    notifier->OnConsoleCommand(((SStringEvent<eCET_ConsoleCommand>*)pEvent)->GetData());
                }
                break;
            case eCET_GameplayEvent:
                for (TListener::Notifier notifier(m_listener); notifier.IsValid(); notifier.Next())
                {
                    notifier->OnGameplayCommand(((SStringEvent<eCET_GameplayEvent>*)pEvent)->GetData());
                }
                break;
            }
            delete *it;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::RegisterListener(IRemoteConsoleListener* pListener, const char* name)
{
    m_listener.Add(pListener, name);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::UnregisterListener(IRemoteConsoleListener* pListener)
{
    m_listener.Remove(pListener);
}
