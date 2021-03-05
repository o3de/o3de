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

#ifndef CRYINCLUDE_CRY3DENGINE_DEFERREDCOLLISIONEVENT_H
#define CRYINCLUDE_CRY3DENGINE_DEFERREDCOLLISIONEVENT_H
#pragma once

#include <IDeferredCollisionEvent.h>

// Implementation class for the DeferredPhysicsEvent Manager
class CDeferredPhysicsEventManager
    : public IDeferredPhysicsEventManager
    ,  public Cry3DEngineBase
{
public:
    CDeferredPhysicsEventManager();
    virtual ~CDeferredPhysicsEventManager();

    virtual void DispatchDeferredEvent(IDeferredPhysicsEvent* pEvent);
    virtual int HandleEvent(const EventPhys* pEvent, IDeferredPhysicsEventManager::CreateEventFunc, IDeferredPhysicsEvent::DeferredEventType);

    virtual void RegisterDeferredEvent(IDeferredPhysicsEvent* pDeferredEvent);
    virtual void UnRegisterDeferredEvent(IDeferredPhysicsEvent* pDeferredEvent);

    virtual void ClearDeferredEvents();

    virtual void Update();

    virtual IDeferredPhysicsEvent* GetLastCollisionEventForEntity(IPhysicalEntity* pPhysEnt);

private:
    ThreadPoolHandle m_hThreadPool;                                                             // thread pool to use for deferred event tasks
    std::vector<IDeferredPhysicsEvent*> m_activeDeferredEvents;     // list of all active deferred events, used for cleanup and statistics
    bool m_bEntitySystemReset;  // means all entity ptrs in events are stale
};

#endif // CRYINCLUDE_CRY3DENGINE_DEFERREDCOLLISIONEVENT_H
