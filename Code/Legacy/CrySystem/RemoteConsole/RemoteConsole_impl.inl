/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <RemoteConsoleCore.h>

AZ_CVAR(AZ::CVarFixedString, log_RemoteConsoleAllowedAddresses, "127.0.0.1",
    nullptr, AZ::ConsoleFunctorFlags::Null,
    "COMMA separated list of allowed hosts or IP addresses which can connect");

AZ_CVAR(bool, log_EnableRemoteConsole, true,
    nullptr, AZ::ConsoleFunctorFlags::Null,
    "enables/disables the remote console");

AZ_CVAR(uint16_t, log_RemoteConsolePort, static_cast<uint16_t>(defaultRemoteConsolePort),
    nullptr, AZ::ConsoleFunctorFlags::Null,
    "Base port (4600 for example) for remote console to listen on.  It will start there and continue upwards until an unused one is found.");

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
CRemoteConsole::CRemoteConsole()
    : m_listener(1)
    , m_lastPortValue(0)
    , m_running(false)
    , m_pServer(nullptr)
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
    m_lastPortValue = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::UnregisterConsoleVariables()
{
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
void CRemoteConsole::AddLogMessage(AZStd::string_view log)
{
    if (!IsStarted())
    {
        return;
    }

    IRemoteEvent* pEvent = new SStringEvent<eCET_LogMessage>(log);
    m_pServer->AddEvent(pEvent);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::AddLogWarning(AZStd::string_view log)
{
    if (!IsStarted())
    {
        return;
    }

    IRemoteEvent* pEvent = new SStringEvent<eCET_LogWarning>(log);
    m_pServer->AddEvent(pEvent);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::AddLogError(AZStd::string_view log)
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
    auto console = AZ::Interface<AZ::IConsole>::Get();
    if (bool enableRemoteConsole; console != nullptr
        && console->GetCvarValue("log_EnableRemoteConsole", enableRemoteConsole) == AZ::GetValueResult::Success)
    {
        // we disable the remote console in the editor, since there is no reason to remote into it and we don't want it eating up that port
        // number anyway and preventing the game from using it.
        bool isEditor = gEnv && gEnv->IsEditor();
        bool isEnabled = !isEditor && enableRemoteConsole;
        bool isStarted = IsStarted();

        uint16_t newPortValue = static_cast<uint16_t>(defaultRemoteConsolePort);
        console->GetCvarValue("log_RemoteConsolePort", newPortValue);

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
