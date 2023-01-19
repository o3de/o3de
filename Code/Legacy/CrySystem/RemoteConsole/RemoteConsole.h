/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYSYSTEM_REMOTECONSOLE_REMOTECONSOLE_H
#define CRYINCLUDE_CRYSYSTEM_REMOTECONSOLE_REMOTECONSOLE_H
#pragma once

#include <CryCommon/IConsole.h>
#include <CryCommon/CryListenerSet.h>

#if (!defined(RELEASE) || defined(RELEASE_LOGGING) || defined(ENABLE_PROFILING_CODE)) && !defined(AZ_LEGACY_CRYSYSTEM_TRAIT_REMOTE_CONSOLE_UNSUPPORTED)
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
        static CRemoteConsole inst;
        return &inst; 
    }

    virtual void RegisterConsoleVariables();
    virtual void UnregisterConsoleVariables();

    virtual void Start();
    virtual void Stop();
    virtual bool IsStarted() const { return m_running; }

    virtual void AddLogMessage(AZStd::string_view log);
    virtual void AddLogWarning(AZStd::string_view log);
    virtual void AddLogError(AZStd::string_view log);

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
#endif
};

#endif // CRYINCLUDE_CRYSYSTEM_REMOTECONSOLE_REMOTECONSOLE_H
