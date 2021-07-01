/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include "SystemEventDispatcher.h"

CSystemEventDispatcher::CSystemEventDispatcher()
    : m_listeners(0)
{
}

bool CSystemEventDispatcher::RegisterListener(ISystemEventListener* pListener)
{
    m_listenerRegistrationLock.Lock();
    bool ret = m_listeners.Add(pListener);
    m_listenerRegistrationLock.Unlock();
    return ret;
}

bool CSystemEventDispatcher::RemoveListener(ISystemEventListener* pListener)
{
    m_listenerRegistrationLock.Lock();
    m_listeners.Remove(pListener);
    m_listenerRegistrationLock.Unlock();
    return true;
}


//////////////////////////////////////////////////////////////////////////
void CSystemEventDispatcher::OnSystemEventAnyThread(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    m_listenerRegistrationLock.Lock();
    for (TSystemEventListeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnSystemEventAnyThread(event, wparam, lparam);
    }
    m_listenerRegistrationLock.Unlock();
}


//////////////////////////////////////////////////////////////////////////
void CSystemEventDispatcher::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    if (gEnv && gEnv->mMainThreadId == CryGetCurrentThreadId())
    {
        for (TSystemEventListeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
        {
            notifier->OnSystemEvent(event, wparam, lparam);
        }
    }
    else
    {
        SEventParams params;
        params.event = event;
        params.wparam = wparam;
        params.lparam = lparam;
        m_systemEventQueue.push(params);
    }

    // Also dispatch the event on this thread. This technically means the event
    //  will be sent twice (thru different OnSystemEventXX functions), therefore it is up to listeners which one they react to.
    OnSystemEventAnyThread(event, wparam, lparam);
}

//////////////////////////////////////////////////////////////////////////
void CSystemEventDispatcher::Update()
{
    AZ_TRACE_METHOD();
    assert(gEnv && gEnv->mMainThreadId == CryGetCurrentThreadId());

    SEventParams params;
    while (m_systemEventQueue.try_pop(params))
    {
        OnSystemEvent(params.event, params.wparam, params.lparam);
    }
}




