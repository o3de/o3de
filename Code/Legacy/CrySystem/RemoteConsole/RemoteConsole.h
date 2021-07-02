/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYSYSTEM_REMOTECONSOLE_REMOTECONSOLE_H
#define CRYINCLUDE_CRYSYSTEM_REMOTECONSOLE_REMOTECONSOLE_H
#pragma once

#include <IConsole.h>
#include <CryListenerSet.h>

#if !defined(RELEASE) || defined(RELEASE_LOGGING) || defined(ENABLE_PROFILING_CODE)
    #define USE_REMOTE_CONSOLE

    struct SRemoteServer;
#endif


/////////////////////////////////////////////////////////////////////////////////////////////
// CRemoteConsole
//
// IRemoteConsole implementation
//
/////////////////////////////////////////////////////////////////////////////////////////////
class CRemoteConsole
    : public IRemoteConsole
{
public:
    static CRemoteConsole* GetInst() 
    { 
        static StaticInstance<CRemoteConsole, AZStd::no_destruct<CRemoteConsole>> inst;
        return &inst; 
    }

    virtual void RegisterConsoleVariables();
    virtual void UnregisterConsoleVariables();

    virtual void Start();
    virtual void Stop();
    virtual bool IsStarted() const { return m_running; }

    virtual void AddLogMessage(const char* log);
    virtual void AddLogWarning(const char* log);
    virtual void AddLogError(const char* log);

    virtual void Update();

    virtual void RegisterListener(IRemoteConsoleListener* pListener, const char* name);
    virtual void UnregisterListener(IRemoteConsoleListener* pListener);

    typedef CListenerSet<IRemoteConsoleListener*> TListener;


    CRemoteConsole();
    virtual ~CRemoteConsole();

    TListener m_listener;
    int m_lastPortValue = 0;
    volatile bool m_running;

#if defined(USE_REMOTE_CONSOLE)
    SRemoteServer* m_pServer;
    ICVar* m_pLogEnableRemoteConsole = nullptr;
    ICVar* m_remoteConsoleAllowedHostList = nullptr;
    ICVar* m_remoteConsolePort = nullptr;
#endif
};

#endif // CRYINCLUDE_CRYSYSTEM_REMOTECONSOLE_REMOTECONSOLE_H
