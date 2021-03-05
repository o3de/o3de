/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYSYSTEM_SYSTEMEVENTDISPATCHER_H
#define CRYINCLUDE_CRYSYSTEM_SYSTEMEVENTDISPATCHER_H
#pragma once


#include <ISystem.h>
#include <CryListenerSet.h>

class CSystemEventDispatcher
    : public ISystemEventDispatcher
{
public:
    CSystemEventDispatcher();
    virtual ~CSystemEventDispatcher(){}

    // ISystemEventDispatcher
    virtual bool RegisterListener(ISystemEventListener* pListener);
    virtual bool RemoveListener(ISystemEventListener* pListener);

    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    virtual void Update();

    // ~ISystemEventDispatcher
private:
    void OnSystemEventAnyThread(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);

    typedef CListenerSet<ISystemEventListener*> TSystemEventListeners;
    TSystemEventListeners   m_listeners;

    // for the events coming from other threads
    struct SEventParams
    {
        ESystemEvent event;
        UINT_PTR wparam;
        UINT_PTR lparam;
    };

    typedef CryMT::queue<SEventParams> TSystemEventQueue;
    TSystemEventQueue m_systemEventQueue;
    CryCriticalSection m_listenerRegistrationLock;
};

#endif // CRYINCLUDE_CRYSYSTEM_SYSTEMEVENTDISPATCHER_H
