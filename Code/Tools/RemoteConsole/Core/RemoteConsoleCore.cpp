/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformDef.h>
#include <AzCore/Socket/AzSocket.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <platform.h>
#include <IConsole.h>
#include <ILevelSystem.h>
#include <ISystem.h>

#include "RemoteConsoleCore.h"
#include <RemoteConsole_Traits_Platform.h>



const int defaultRemoteConsolePort = 4600; // externed in the header to expose publicly


namespace
{
    static const int kDefaultBufferSize = 32768;

    static const AZ::u16 kMaxBindPorts = 8;                        // max range of ports to bind to for remote console

    static const char* kServerThreadName = "RemoteConsoleServer";
    static const char* kClientThreadName = "RemoteConsoleClient";
}


bool RCON_IsRemoteAllowedToConnect(const AZ::AzSock::AzSocketAddress& connectee)
{
    if ((!gEnv) || (!gEnv->pConsole))
    {
        CryLog("Cannot allow incoming connection for remote console, because we do not yet have a console or an environment.");
        return false;
    }
    ICVar* remoteConsoleAllowedHostList = gEnv->pConsole->GetCVar("log_RemoteConsoleAllowedAddresses");
    if (!remoteConsoleAllowedHostList)
    {
        CryLog("Cannot allow incoming connection for remote console, because there is no registered log_RemoteConsoleAllowedAddresses console variable.");
        return false;
    }

    const char* value = remoteConsoleAllowedHostList->GetString();
    // the default or empty string indicates localhost.
    if (!value)
    {
        value = "";
    }

    AZStd::vector<AZStd::string> addresses;
    AzFramework::StringFunc::Tokenize(value, addresses, ',');

    if (addresses.empty())
    {
        addresses.push_back("127.0.0.1");
    }

    AZ::AzSock::AzSocketAddress testAddress;
    for (const AZStd::string& address : addresses)
    {
        // test the approved addresses with connectee's port to see if we have a match
        testAddress.SetAddress(address.c_str(), connectee.GetAddrPort());

        if (testAddress == connectee)
        {
            // its an exact match.
            if (gEnv->pLog)
            {
                gEnv->pLog->LogToConsole("Remote console connected from ip %s (matches: %s)", connectee.GetAddress().c_str(), address.c_str());
            }
            return true;
        }
    }

    if (gEnv->pLog)
    {
        gEnv->pLog->LogToConsole("An attempt to connect to remote console from ip %s failed because it is not on the ApprovedList.", connectee.GetAddress().c_str());
        gEnv->pLog->LogToConsole("Add to the ApprovedList using the CVAR log_RemoteConsoleAllowedAddresses (comma separated IPs or hostnames)");
        gEnv->pLog->LogToConsole("Example:  log_RemoteConsoleAllowedAddresses localhost,joescomputer");
    }
    return false; // return false by default, so you MUST pass an above check for you to be allowed in.
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteThreadedObject::Start(const char* name)
{
    AZStd::thread_desc desc;
    desc.m_name = name;

    auto function = AZStd::bind(&SRemoteThreadedObject::ThreadFunction, this);
    m_thread = AZStd::thread(desc, function);
}

void SRemoteThreadedObject::WaitForThread()
{
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void SRemoteThreadedObject::ThreadFunction()
{
    Run();
    Terminate();
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::StartServer()
{
    StopServer();
    m_bAcceptClients = true;
    Start(kServerThreadName);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::StopServer()
{
    m_bAcceptClients = false;
    AZ::AzSock::CloseSocket(m_socket);
    m_socket = SOCKET_ERROR;

    AZStd::unique_lock<AZStd::recursive_mutex> lock(m_mutex);
    for (TClients::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        it->pClient->StopClient();
    }
    m_stopCondition.wait(lock, [this] { return m_clients.empty(); });
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::ClientDone(SRemoteClient* pClient)
{
    AZStd::scoped_lock lock(m_mutex);
    for (TClients::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (it->pClient == pClient)
        {
            delete it->pClient;
            delete it->pEvents;
            m_clients.erase(it);
            break;
        }
    }

    if (m_clients.empty())
    {
        m_stopCondition.notify_all();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::Terminate()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::Run()
{
    AZ_TRAIT_REMOTECONSOLE_SET_THREAD_AFFINITY

    AZSOCKET sClient;
    AZ::AzSock::AzSocketAddress local;
    int result = 0;

    if (AZ::AzSock::SocketErrorOccured(AZ::AzSock::Startup()))
    {
        gEnv->pLog->LogError("[RemoteKeyboard] Failed to load Winsock!\n");
        return;
    }

    m_socket = AZ::AzSock::Socket();
    if (!AZ::AzSock::IsAzSocketValid(m_socket))
    {
        CryLog("Remote console FAILED. socket() => SOCKET_ERROR");
        return;
    }

    // this CVAR is optional.
    AZ::u16 remotePort = defaultRemoteConsolePort;
    ICVar* remoteConsolePort = gEnv->pConsole->GetCVar("log_RemoteConsolePort");
    if (remoteConsolePort)
    {
        remotePort = static_cast<AZ::u16>(remoteConsolePort->GetIVal());
    }

    //
    // There may be multiple processes running, and each process will require a unique port for remote console to work. 
    // So we need to be able to bind to ascending ports so that automated tests can connect to each process. QA's Automated
    // tools depend on this behavior for successful testing to occur.
    // Please check with ly-networking, ly-systems or ly-qa before changing this.
    //  Thanks.
    //

    bool bindOk = false;
    for (AZ::u16 port=remotePort; port < (remotePort + kMaxBindPorts); port++)
    {
        local.SetAddrPort(port);

        result = AZ::AzSock::Bind(m_socket, local);
        if (!AZ::AzSock::SocketErrorOccured(result))
        {
            bindOk = true;
            break;
        }
    }

    if ( !bindOk )
    {
        CryLog("Failed to bind Remote Console to ports %hu to %hu", remotePort, static_cast<AZ::u16>(remotePort + kMaxBindPorts - 1) );
        return;
    }

    AZ::AzSock::Listen(m_socket, 8);
    AZ::AzSock::AzSocketAddress sockName;
    result = AZ::AzSock::GetSockName(m_socket, sockName);

    if (!AZ::AzSock::SocketErrorOccured(result))
    {
        CryLog("Remote console listening on: %d\n", sockName.GetAddrPort());
    }
    else
    {
        CryLog("Remote console FAILED to listen on: %d\n", sockName.GetAddrPort());
    }

    while (m_bAcceptClients)
    {
        AZTIMEVAL timeout { 1, 0 };
        if (!AZ::AzSock::IsRecvPending(m_socket, &timeout))
        {
            continue;
        }
        AZ::AzSock::AzSocketAddress clientAddress;
        sClient = AZ::AzSock::Accept(m_socket, clientAddress);
        if (!m_bAcceptClients || !AZ::AzSock::IsAzSocketValid(sClient))
        {
            break;
        }

        if (!RCON_IsRemoteAllowedToConnect(clientAddress))
        {
            AZ::AzSock::CloseSocket(sClient);
            continue;
        }

        AZStd::scoped_lock lock(m_mutex);
        SRemoteClient* pClient = new SRemoteClient(this);
        m_clients.push_back(SRemoteClientInfo(pClient));
        pClient->StartClient(sClient);
    }
    AZ::AzSock::CloseSocket(m_socket);
    CryLog("Remote console terminating.\n");
    //AZ::AzSock::Shutdown();
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::AddEvent(IRemoteEvent* pEvent)
{
    AZStd::scoped_lock lock(m_mutex);
    for (TClients::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        it->pEvents->push_back(pEvent->Clone());
    }
    delete pEvent;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::GetEvents(TEventBuffer& buffer)
{
    AZStd::scoped_lock lock(m_mutex);
    buffer = m_eventBuffer;
    m_eventBuffer.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////
bool SRemoteServer::WriteBuffer(SRemoteClient* pClient,  char* buffer, int& size)
{
    IRemoteEvent* pEvent = nullptr;
    {
        AZStd::scoped_lock lock(m_mutex);
        for (TClients::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
        {
            if (it->pClient == pClient)
            {
                TEventBuffer* pEvents = it->pEvents;
                if (!pEvents->empty())
                {
                    pEvent = pEvents->front();
                    pEvents->pop_front();
                }
                break;
            }
        }
    }
    const bool res = (pEvent != nullptr);
    if (pEvent)
    {
        SRemoteEventFactory::GetInst()->WriteToBuffer(pEvent, buffer, size, kDefaultBufferSize);
        delete pEvent;
    }
    return res;
}

/////////////////////////////////////////////////////////////////////////////////////////////
bool SRemoteServer::ReadBuffer(const char* buffer, int data)
{
    bool result = true;

    // Sometimes multiple events can come in a single buffer, so make sure we look
    // at the entire thing.
    int bytesRemaining = data;
    const char* curBuffer = buffer;
    while (bytesRemaining > 0)
    {
        // Create the event from the current sub string in the buffer.
        IRemoteEvent* event = SRemoteEventFactory::GetInst()->CreateEventFromBuffer(curBuffer, bytesRemaining);

        result &= (event != nullptr);
        if (event)
        {
            if (event->GetType() != eCET_Noop)
            {
                AZStd::scoped_lock lock(m_mutex);
                m_eventBuffer.push_back(event);
            }
            else
            {
                delete event;
            }
        }

        // Advance to the next null terminated string in the buffer
        const int currentSize = static_cast<int>(strnlen(curBuffer, bytesRemaining));
        bytesRemaining -= currentSize + 1;
        curBuffer += currentSize + 1;
    }

    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteClient::StartClient(AZSOCKET socket)
{
    m_socket = socket;
    Start(kClientThreadName);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteClient::StopClient()
{
    AZ::AzSock::CloseSocket(m_socket);
    m_socket = AZ_SOCKET_INVALID;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteClient::Terminate()
{
    m_pServer->ClientDone(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteClient::Run()
{
    AZ_TRAIT_REMOTECONSOLE_SET_THREAD_AFFINITY

    char szBuff[kDefaultBufferSize];
    int size;
    SNoDataEvent<eCET_Req> reqEvt;

    AZStd::vector<AZStd::string> autoCompleteList;
    FillAutoCompleteList(autoCompleteList);

    bool ok = true;
    bool autoCompleteDoneSent = false;
    
    // Send a message that is used to verify that the Remote Console connected
    SNoDataEvent<eCET_ConnectMessage> connectMessage;
    SRemoteEventFactory::GetInst()->WriteToBuffer(&connectMessage, szBuff, size, kDefaultBufferSize);
    ok &= SendPackage(szBuff, size);
    ok &= RecvPackage(szBuff, size);
    ok &= m_pServer->ReadBuffer(szBuff, size);
    while (ok)
    {
        // read data
        SRemoteEventFactory::GetInst()->WriteToBuffer(&reqEvt, szBuff, size, kDefaultBufferSize);
        ok &= SendPackage(szBuff, size);
        ok &= RecvPackage(szBuff, size);
        ok &= m_pServer->ReadBuffer(szBuff, size);

        for (int i = 0; i < 20 && !autoCompleteList.empty(); ++i)
        {
            SStringEvent<eCET_AutoCompleteList> autoCompleteListEvt(autoCompleteList.back().c_str());
            SRemoteEventFactory::GetInst()->WriteToBuffer(&autoCompleteListEvt, szBuff, size, kDefaultBufferSize);
            ok &= SendPackage(szBuff, size);
            ok &= RecvPackage(szBuff, size);
            ok &= m_pServer->ReadBuffer(szBuff, size);
            autoCompleteList.pop_back();
        }
        if (autoCompleteList.empty() && !autoCompleteDoneSent)
        {
            SNoDataEvent<eCET_AutoCompleteListDone> autoCompleteDone;
            SRemoteEventFactory::GetInst()->WriteToBuffer(&autoCompleteDone, szBuff, size, kDefaultBufferSize);

            ok &= SendPackage(szBuff, size);
            ok &= RecvPackage(szBuff, size);
            ok &= m_pServer->ReadBuffer(szBuff, size);
            autoCompleteDoneSent = true;
        }

        // send data
        while (ok && m_pServer->WriteBuffer(this, szBuff, size))
        {
            ok &= SendPackage(szBuff, size);
            ok &= RecvPackage(szBuff, size);
            ok &= m_pServer->ReadBuffer(szBuff, size);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
bool SRemoteClient::RecvPackage(char* buffer, int& size)
{
    size = 0;
    int ret, idx = 0;
    do
    {
        ret = AZ::AzSock::Recv(m_socket, buffer + idx, kDefaultBufferSize - idx, 0);
        if (AZ::AzSock::SocketErrorOccured(ret))
        {
            return false;
        }
        idx += ret;
    } while (buffer[idx - 1] != '\0');
    size = idx;
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
bool SRemoteClient::SendPackage(const char* buffer, int size)
{
    int ret, idx = 0;
    int left = size + 1;
    assert(buffer[size] == '\0');
    while (left > 0)
    {
        ret = AZ::AzSock::Send(m_socket, &buffer[idx], left, 0);
        if (AZ::AzSock::SocketErrorOccured(ret))
        {
            return false;
        }
        left -= ret;
        idx += ret;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteClient::FillAutoCompleteList(AZStd::vector<AZStd::string>& list)
{
    AZStd::vector<AZStd::string_view> cmds;
    size_t count = gEnv->pConsole->GetSortedVars(cmds);
    cmds.resize(count);
    count = gEnv->pConsole->GetSortedVars(cmds);
    for (size_t i = 0; i < count; ++i)
    {
        list.push_back(cmds[i]);
    }

    if (!gEnv->pSystem || !gEnv->pSystem->GetILevelSystem())
    {
        return;
    }

    for (int i = 0, end = gEnv->pSystem->GetILevelSystem()->GetLevelCount(); i < end; ++i)
    {
        ILevelInfo* pLevel = gEnv->pSystem->GetILevelSystem()->GetLevelInfo(i);
        AZStd::string item = "map ";
        const char* levelName = pLevel->GetName();
        int start = 0;
        for (int k = 0, kend = static_cast<int>(strlen(levelName)); k < kend; ++k)
        {
            if ((levelName[k] == '\\' || levelName[k] == '/') && k + 1 < kend)
            {
                start = k + 1;
            }
        }
        item += levelName + start;
        list.push_back(item);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Event factory ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
#define REGISTER_EVENT_NODATA(evt) RegisterEvent(new SNoDataEvent<evt>());
#define REGISTER_EVENT_STRING(evt) RegisterEvent(new SStringEvent<evt>(""));

/////////////////////////////////////////////////////////////////////////////////////////////
SRemoteEventFactory::SRemoteEventFactory()
{
    REGISTER_EVENT_NODATA(eCET_Noop);
    REGISTER_EVENT_NODATA(eCET_Req);
    REGISTER_EVENT_STRING(eCET_LogMessage);
    REGISTER_EVENT_STRING(eCET_LogWarning);
    REGISTER_EVENT_STRING(eCET_LogError);
    REGISTER_EVENT_STRING(eCET_ConsoleCommand);
    REGISTER_EVENT_STRING(eCET_AutoCompleteList);
    REGISTER_EVENT_NODATA(eCET_AutoCompleteListDone);

    REGISTER_EVENT_NODATA(eCET_Strobo_GetThreads);
    REGISTER_EVENT_STRING(eCET_Strobo_ThreadAdd);
    REGISTER_EVENT_NODATA(eCET_Strobo_ThreadDone);
    REGISTER_EVENT_NODATA(eCET_Strobo_GetResult);
    REGISTER_EVENT_NODATA(eCET_Strobo_ResultStart);
    REGISTER_EVENT_NODATA(eCET_Strobo_ResultDone);

    REGISTER_EVENT_NODATA(eCET_Strobo_StatStart);
    REGISTER_EVENT_STRING(eCET_Strobo_StatAdd);
    REGISTER_EVENT_NODATA(eCET_Strobo_ThreadInfoStart);
    REGISTER_EVENT_STRING(eCET_Strobo_ThreadInfoAdd);
    REGISTER_EVENT_NODATA(eCET_Strobo_SymStart);
    REGISTER_EVENT_STRING(eCET_Strobo_SymAdd);
    REGISTER_EVENT_NODATA(eCET_Strobo_CallstackStart);
    REGISTER_EVENT_STRING(eCET_Strobo_CallstackAdd);

    REGISTER_EVENT_STRING(eCET_GameplayEvent);

    REGISTER_EVENT_NODATA(eCET_Strobo_FrameInfoStart);
    REGISTER_EVENT_STRING(eCET_Strobo_FrameInfoAdd);
    REGISTER_EVENT_NODATA(eCET_ConnectMessage);
}

/////////////////////////////////////////////////////////////////////////////////////////////
SRemoteEventFactory::~SRemoteEventFactory()
{
    for (TPrototypes::iterator it = m_prototypes.begin(), end = m_prototypes.end(); it != end; ++it)
    {
        delete it->second;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
IRemoteEvent* SRemoteEventFactory::CreateEventFromBuffer(const char* buffer, int size)
{
    if (size > 1 && buffer[size - 1] == '\0')
    {
        EConsoleEventType type = EConsoleEventType(buffer[0] - '0');
        TPrototypes::const_iterator it = m_prototypes.find(type);
        if (it != m_prototypes.end())
        {
            IRemoteEvent* pEvent = it->second->CreateFromBuffer(buffer + 1, size - 1);
            return pEvent;
        }
    }
    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteEventFactory::WriteToBuffer(IRemoteEvent* pEvent, char* buffer, int& size, int maxsize)
{
    assert(m_prototypes.find(pEvent->GetType()) != m_prototypes.end());
    buffer[0] = '0' + (char)pEvent->GetType();
    pEvent->WriteToBuffer(buffer + 1, size, maxsize - 2);
    buffer[++size] = '\0';
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteEventFactory::RegisterEvent(IRemoteEvent* pEvent)
{
    assert(m_prototypes.find(pEvent->GetType()) == m_prototypes.end());
    m_prototypes[pEvent->GetType()] = pEvent;
}
