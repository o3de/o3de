/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ISystem.h>
#include <CryListenerSet.h>
#include <MultiThread_Containers.h>

class CSystemEventDispatcher
    : public ISystemEventDispatcher
{
public:
    CSystemEventDispatcher();
    virtual ~CSystemEventDispatcher(){}

    // ISystemEventDispatcher
    virtual bool RegisterListener(ISystemEventListener* pListener);
    virtual bool RemoveListener(ISystemEventListener* pListener);

    virtual void OnSystemEvent(ESystemEvent event, uintptr_t wparam, uintptr_t lparam);
    virtual void Update();

    // ~ISystemEventDispatcher
private:
    void OnSystemEventAnyThread(ESystemEvent event, uintptr_t wparam, uintptr_t lparam);

    typedef CListenerSet<ISystemEventListener*> TSystemEventListeners;
    TSystemEventListeners   m_listeners;

    // for the events coming from other threads
    struct SEventParams
    {
        ESystemEvent event;
        uintptr_t wparam;
        uintptr_t lparam;
    };

    typedef CryMT::queue<SEventParams> TSystemEventQueue;
    TSystemEventQueue m_systemEventQueue;
    AZStd::recursive_mutex m_listenerRegistrationLock;
};
