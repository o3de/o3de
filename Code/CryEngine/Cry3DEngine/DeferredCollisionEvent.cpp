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

#include "Cry3DEngine_precompiled.h"
#include "DeferredCollisionEvent.h"

CDeferredPhysicsEventManager::CDeferredPhysicsEventManager()
    : m_hThreadPool(-1)
    , m_bEntitySystemReset(false)
{
    if (gEnv->IsDedicated())
    {
        return;
    }

    // Disable MT if Editor.
    if (gEnv->IsEditor())
    {
        return;
    }

    ThreadPoolDesc threadPoolDesc;
    threadPoolDesc.sPoolName = "DeferredPhysicsEvents";
    threadPoolDesc.nThreadStackSizeKB = 24;

    // let the DeferredPhysicsEvents run on Core3
    if (!threadPoolDesc.CreateThread(BIT(3)))
    {
        return;
    }

    IThreadTaskManager* pThreadTaskManager = gEnv->pSystem->GetIThreadTaskManager();
    assert(pThreadTaskManager);

    m_hThreadPool = pThreadTaskManager->CreateThreadsPool(threadPoolDesc);
}


CDeferredPhysicsEventManager::~CDeferredPhysicsEventManager()
{
}


void CDeferredPhysicsEventManager::DispatchDeferredEvent(IDeferredPhysicsEvent* pEvent)
{
    assert(pEvent);
    // execute immediately if we don't use deferred physcis events
    if (GetCVars()->e_DeferredPhysicsEvents == 0 ||  m_hThreadPool < 0)
    {
        pEvent->OnUpdate();
        return;
    }

    // Register the task with the ThreadTask manager
    IThreadTaskManager* pThreadTaskManager = gEnv->pSystem->GetIThreadTaskManager();
    assert(pThreadTaskManager);

    pEvent->GetTaskInfo()->m_params.name = "DeferredPhysicsEvents";
    pEvent->GetTaskInfo()->m_params.nFlags = THREAD_TASK_ASSIGN_TO_POOL;
    pEvent->GetTaskInfo()->m_params.nThreadsGroupId = m_hThreadPool;
    pEvent->GetTaskInfo()->m_pThread = NULL;
    pThreadTaskManager->RegisterTask(pEvent, pEvent->GetTaskInfo()->m_params);
}


void ApplyCollisionImpulse(EventPhysCollision* pCollision)
{
    if (pCollision->normImpulse && pCollision->pEntity[1] &&
        pCollision->pEntity[0] && pCollision->pEntity[0]->GetType() == PE_PARTICLE &&
        pCollision->pEntity[0]->GetForeignData(pCollision->pEntity[0]->GetiForeignData()))      // no foreign data mean it's likely scheduled for deletion
    {
        pe_action_impulse ai;
        ai.point = pCollision->pt;
        ai.partid = pCollision->partid[1];
        ai.impulse = (pCollision->vloc[0] - pCollision->vloc[1]) * pCollision->normImpulse;
        pCollision->pEntity[1]->Action(&ai);
    }
}


int CDeferredPhysicsEventManager::HandleEvent(const EventPhys* pEvent, IDeferredPhysicsEventManager::CreateEventFunc pCreateFunc, [[maybe_unused]] IDeferredPhysicsEvent::DeferredEventType type)
{
    EventPhysCollision* pCollision = (EventPhysCollision*)pEvent;
    assert(pCollision);

    if (pCollision->deferredState == EPC_DEFERRED_FINISHED)
    {
        ApplyCollisionImpulse(pCollision);
        return pCollision->deferredResult;
    }

    // == create new deferred event object, and do some housekeeping(ensuring entities not deleted, remebering event for cleanup) == //
    IDeferredPhysicsEvent* pDeferredEvent = pCreateFunc(pEvent);

    // == start executing == //
    pDeferredEvent->Start();


    // == check if we really needed to deferred this event(early outs, not deferred code paths) == //
    if (GetCVars()->e_DeferredPhysicsEvents == 0 || pDeferredEvent->HasFinished())
    {
        int nResult = pDeferredEvent->Result((EventPhys*)pEvent);
        SAFE_DELETE(pDeferredEvent);
        ApplyCollisionImpulse(pCollision);
        return nResult;
    }

    if (pCollision->pEntity[0])
    {
        pCollision->pEntity[0]->AddRef();
    }
    if (pCollision->pEntity[1])
    {
        pCollision->pEntity[1]->AddRef();
    }

    // == re-queue event for the next frame, to keep the physical entity alive == //
    RegisterDeferredEvent(pDeferredEvent);

    return 0;
}


void CDeferredPhysicsEventManager::RegisterDeferredEvent(IDeferredPhysicsEvent* pDeferredEvent)
{
    assert(pDeferredEvent);
    m_activeDeferredEvents.push_back(pDeferredEvent);
}


void CDeferredPhysicsEventManager::UnRegisterDeferredEvent(IDeferredPhysicsEvent* pDeferredEvent)
{
    std::vector<IDeferredPhysicsEvent*>::iterator it = std::find(m_activeDeferredEvents.begin(), m_activeDeferredEvents.end(), pDeferredEvent);
    if (it == m_activeDeferredEvents.end())
    {
        return;
    }

    // == remove from active list == //
    m_activeDeferredEvents.erase(it);

    if (m_bEntitySystemReset)
    {
        return;
    }

    // == decrement keep alive counter on entity == //
    EventPhysCollision* pCollision = (EventPhysCollision*)pDeferredEvent->PhysicsEvent();
    if (pCollision->pEntity[0])
    {
        pCollision->pEntity[0]->Release();
    }
    if (pCollision->pEntity[1])
    {
        pCollision->pEntity[1]->Release();
    }
}


void CDeferredPhysicsEventManager::ClearDeferredEvents()
{
    // move content of the active deferred events array to a tmp one to prevent provlems with UnRegisterDeferredEvent called by destructors
    std::vector<IDeferredPhysicsEvent*> tmp = m_activeDeferredEvents;
    m_bEntitySystemReset = true;

    for (std::vector<IDeferredPhysicsEvent*>::iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
        (*it)->Sync();
        delete *it;
    }
    stl::free_container(m_activeDeferredEvents);
    m_bEntitySystemReset = false;
}

void CDeferredPhysicsEventManager::Update()
{
    std::vector<IDeferredPhysicsEvent*> tmp = m_activeDeferredEvents;

    for (std::vector<IDeferredPhysicsEvent*>::iterator it = tmp.begin(), end =  tmp.end(); it != end; ++it)
    {
        IDeferredPhysicsEvent* collisionEvent = *it;
        assert(collisionEvent);
        PREFAST_ASSUME(collisionEvent);
        EventPhysCollision* epc = (EventPhysCollision*) collisionEvent->PhysicsEvent();

        if (collisionEvent->HasFinished() == false)
        {
            continue;
        }

        epc->deferredResult = collisionEvent->Result();

        if (epc->deferredState != EPC_DEFERRED_FINISHED)
        {
            epc->deferredState = EPC_DEFERRED_FINISHED;
        }
        else
        {
            SAFE_DELETE(collisionEvent);
        }
    }
}

IDeferredPhysicsEvent* CDeferredPhysicsEventManager::GetLastCollisionEventForEntity(IPhysicalEntity* pPhysEnt)
{
    EventPhysCollision* pLastPhysEvent;
    for (int i = m_activeDeferredEvents.size() - 1; i >= 0; i--)
    {
        if ((pLastPhysEvent = (EventPhysCollision*)m_activeDeferredEvents[i]->PhysicsEvent()) && pLastPhysEvent->idval == EventPhysCollision::id && pLastPhysEvent->pEntity[0] == pPhysEnt)
        {
            return m_activeDeferredEvents[i];
        }
    }
    return 0;
}
