/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/unordered_set.h>
#include "EMotionFXConfig.h"
#include "EventManager.h"
#include "MotionInstance.h"
#include "EventHandler.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraph.h"
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/LogManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(EventManager, MotionEventManagerAllocator)

    // the constructor
    EventManager::EventManager()
        : MCore::RefCounted()
    {
        m_eventHandlersByEventType.resize(EVENT_TYPE_COUNT);
    }


    // the destructor
    EventManager::~EventManager()
    {
        // Collect all event handlers (avoiding duplicates)
#ifdef AZ_DEBUG_BUILD
        for (const EventHandlerVector& eventHandlers : m_eventHandlersByEventType)
        {
            AZ_Assert(eventHandlers.empty(), "Expected all event handlers to be removed");
        }
#endif
        m_eventHandlersByEventType.clear();
    }


    // create
    EventManager* EventManager::Create()
    {
        return aznew EventManager();
    }


    // lock the event manager
    void EventManager::Lock()
    {
        m_lock.Lock();
    }


    // unlock the event manager
    void EventManager::Unlock()
    {
        m_lock.Unlock();
    }


    // register event handler to the manager
    void EventManager::AddEventHandler(EventHandler* eventHandler)
    {
        MCore::LockGuardRecursive lock(m_lock);

        AZ_Assert(eventHandler, "Expected non-null event handler");
        for (const EventTypes eventType : eventHandler->GetHandledEventTypes())
        {
            AZ_Assert(AZStd::find(m_eventHandlersByEventType[eventType].begin(), m_eventHandlersByEventType[eventType].end(), eventHandler) == m_eventHandlersByEventType[eventType].end(),
                "Event handler already added to manager");
            m_eventHandlersByEventType[eventType].emplace_back(eventHandler);
        }
    }


    // unregister event handler from the manager
    void EventManager::RemoveEventHandler(EventHandler* eventHandler)
    {
        MCore::LockGuardRecursive lock(m_lock);

        for (const EventTypes eventType : eventHandler->GetHandledEventTypes())
        {
            EventHandlerVector& eventHandlers = m_eventHandlersByEventType[eventType];
            eventHandlers.erase(AZStd::remove(eventHandlers.begin(), eventHandlers.end(), eventHandler), eventHandlers.end());
        }
    }


    // trigger an event in all handlers
    void EventManager::OnEvent(const EventInfo& eventInfo)
    {
        if (eventInfo.m_eventState == EventInfo::EventState::ACTIVE)
        {
            return;
        }

        // trigger the event handlers inside the motion instance
        if (eventInfo.m_motionInstance)
        {
            eventInfo.m_motionInstance->OnEvent(eventInfo);
        }

        // Call event handlers
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_EVENT];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnEvent(eventInfo);
        }
    }


    void EventManager::OnPlayMotion(Motion* motion, PlayBackInfo* info)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_PLAY_MOTION];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnPlayMotion(motion, info);
        }
    }


    void EventManager::OnStartMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnStartMotionInstance(info);

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_START_MOTION_INSTANCE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStartMotionInstance(motionInstance, info);
        }
    }


    void EventManager::OnDeleteMotionInstance(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnDeleteMotionInstance();

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_DELETE_MOTION_INSTANCE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnDeleteMotionInstance(motionInstance);
        }
    }


    void EventManager::OnDeleteMotion(Motion* motion)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_DELETE_MOTION];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnDeleteMotion(motion);
        }
    }


    void EventManager::OnStop(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnStop();

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STOP];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStop(motionInstance);
        }
    }


    void EventManager::OnHasLooped(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnHasLooped();

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_HAS_LOOPED];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnHasLooped(motionInstance);
        }
    }


    void EventManager::OnHasReachedMaxNumLoops(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnHasReachedMaxNumLoops();

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_HAS_REACHED_MAX_NUM_LOOPS];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnHasReachedMaxNumLoops(motionInstance);
        }
    }


    void EventManager::OnHasReachedMaxPlayTime(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnHasReachedMaxPlayTime();

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_HAS_REACHED_MAX_PLAY_TIME];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnHasReachedMaxPlayTime(motionInstance);
        }
    }


    void EventManager::OnIsFrozenAtLastFrame(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnIsFrozenAtLastFrame();

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_IS_FROZEN_AT_LAST_FRAME];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnIsFrozenAtLastFrame(motionInstance);
        }
    }


    void EventManager::OnChangedPauseState(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnChangedPauseState();

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CHANGED_PAUSE_STATE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnChangedPauseState(motionInstance);
        }
    }


    void EventManager::OnChangedActiveState(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnChangedActiveState();

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CHANGED_ACTIVE_STATE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnChangedActiveState(motionInstance);
        }
    }


    void EventManager::OnStartBlending(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnStartBlending();

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_START_BLENDING];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStartBlending(motionInstance);
        }
    }


    void EventManager::OnStopBlending(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnStopBlending();

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STOP_BLENDING];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStopBlending(motionInstance);
        }
    }


    void EventManager::OnQueueMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnQueueMotionInstance(info);

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_QUEUE_MOTION_INSTANCE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnQueueMotionInstance(motionInstance, info);
        }
    }


    void EventManager::OnDeleteActor(Actor* actor)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_DELETE_ACTOR];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnDeleteActor(actor);
        }
    }


    // draw a debug triangle
    void EventManager::OnDrawTriangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_DRAW_TRIANGLE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnDrawTriangle(posA, posB, posC, normalA, normalB, normalC, color);
        }
    }


    // draw the triangles that got added using OnDrawTriangles()
    void EventManager::OnDrawTriangles()
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_DRAW_TRIANGLES];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnDrawTriangles();
        }
    }


    // simulate physics
    void EventManager::OnSimulatePhysics(float timeDelta)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_SIMULATE_PHYSICS];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnSimulatePhysics(timeDelta);
        }
    }


    void EventManager::OnCustomEvent(uint32 eventType, void* data)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CUSTOM_EVENT];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnCustomEvent(eventType, data);
        }
    }


    void EventManager::OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_ENTER];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateEnter(animGraphInstance, state);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnStateEnter(state);
    }


    void EventManager::OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_ENTERING];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateEntering(animGraphInstance, state);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnStateEntering(state);
    }


    void EventManager::OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_EXIT];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateExit(animGraphInstance, state);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnStateExit(state);
    }


    void EventManager::OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_END];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateEnd(animGraphInstance, state);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnStateEnd(state);
    }


    void EventManager::OnStartTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_START_TRANSITION];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStartTransition(animGraphInstance, transition);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnStartTransition(transition);
    }


    void EventManager::OnEndTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_END_TRANSITION];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnEndTransition(animGraphInstance, transition);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnEndTransition(transition);
    }


    // call the callbacks for when we renamed a node
    void EventManager::OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const AZStd::string& oldName)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_RENAMED_NODE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnRenamedNode(animGraph, node, oldName);
        }
    }


    // call the callbacks for a node creation
    void EventManager::OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CREATED_NODE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnCreatedNode(animGraph, node);
        }
    }


    // call the callbacks for a node removal
    void EventManager::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        // get the root state machine and call the callbacks recursively
        AnimGraphStateMachine* rootSM = animGraph->GetRootStateMachine();
        rootSM->OnRemoveNode(animGraph, nodeToRemove);

        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_REMOVE_NODE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnRemoveNode(animGraph, nodeToRemove);
        }
    }


    // call the callbacks
    void EventManager::OnRemovedChildNode(AnimGraph* animGraph, AnimGraphNode* parentNode)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_REMOVED_CHILD_NODE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnRemovedChildNode(animGraph, parentNode);
        }
    }


    void EventManager::OnSetVisualManipulatorOffset(AnimGraphInstance* animGraphInstance, uint32 paramIndex, const AZ::Vector3& offset)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_SET_VISUAL_MANIPULATOR_OFFSET];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnSetVisualManipulatorOffset(animGraphInstance, paramIndex, offset);
        }
    }


    void EventManager::OnInputPortsChanged(AnimGraphNode* node, const AZStd::vector<AZStd::string>& newInputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_INPUT_PORTS_CHANGED];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnInputPortsChanged(node, newInputPorts, memberName, memberValue);
        }
    }


    void EventManager::OnOutputPortsChanged(AnimGraphNode* node, const AZStd::vector<AZStd::string>& newOutputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_OUTPUT_PORTS_CHANGED];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnOutputPortsChanged(node, newOutputPorts, memberName, memberValue);
        }
    }


    void EventManager::OnProgressStart()
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_PROGRESS_START];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnProgressStart();
            eventHandler->OnProgressValue(0.0f);
        }
    }


    void EventManager::OnProgressEnd()
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_PROGRESS_END];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnProgressValue(100.0f);
            eventHandler->OnProgressEnd();

        }
    }


    void EventManager::OnProgressText(const char* text)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_PROGRESS_TEXT];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnProgressText(text);
        }
    }


    void EventManager::OnProgressValue(float percentage)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_PROGRESS_VALUE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnProgressValue(percentage);
        }
    }


    void EventManager::OnSubProgressText(const char* text)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_SUB_PROGRESS_TEXT];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnSubProgressText(text);
        }
    }


    void EventManager::OnSubProgressValue(float percentage)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_SUB_PROGRESS_VALUE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnSubProgressValue(percentage);
        }
    }


    // perform a ray intersection test
    bool EventManager::OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, IntersectionInfo* outIntersectInfo)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_RAY_INTERSECTION_TEST];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            const bool result = eventHandler->OnRayIntersectionTest(start, end, outIntersectInfo);
            if (outIntersectInfo->m_isValid)
            {
                return result;
            }
        }
        return false;
    }


    // create an animgraph
    void EventManager::OnCreateAnimGraph(AnimGraph* animGraph)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CREATE_ANIM_GRAPH];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnCreateAnimGraph(animGraph);
        }
    }


    // create an animgraph instance
    void EventManager::OnCreateAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CREATE_ANIM_GRAPH_INSTANCE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnCreateAnimGraphInstance(animGraphInstance);
        }
    }


    // create a motion
    void EventManager::OnCreateMotion(Motion* motion)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CREATE_MOTION];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnCreateMotion(motion);
        }
    }


    // create a motion set
    void EventManager::OnCreateMotionSet(MotionSet* motionSet)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CREATE_MOTION_SET];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnCreateMotionSet(motionSet);
        }
    }


    // create a motion instance
    void EventManager::OnCreateMotionInstance(MotionInstance* motionInstance)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CREATE_MOTION_INSTANCE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnCreateMotionInstance(motionInstance);
        }
    }


    // create a motion system
    void EventManager::OnCreateMotionSystem(MotionSystem* motionSystem)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CREATE_MOTION_SYSTEM];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnCreateMotionSystem(motionSystem);
        }
    }


    // create an actor
    void EventManager::OnCreateActor(Actor* actor)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CREATE_ACTOR];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnCreateActor(actor);
        }
    }


    // on post create actor
    void EventManager::OnPostCreateActor(Actor* actor)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_POST_CREATE_ACTOR];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnPostCreateActor(actor);
        }
    }


    // delete an animgraph
    void EventManager::OnDeleteAnimGraph(AnimGraph* animGraph)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_DELETE_ANIM_GRAPH];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnDeleteAnimGraph(animGraph);
        }
    }


    // delete an animgraph instance
    void EventManager::OnDeleteAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_DELETE_ANIM_GRAPH_INSTANCE];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnDeleteAnimGraphInstance(animGraphInstance);
        }
    }


    // delete motion set
    void EventManager::OnDeleteMotionSet(MotionSet* motionSet)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_DELETE_MOTION_SET];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnDeleteMotionSet(motionSet);
        }
    }


    // delete a motion system
    void EventManager::OnDeleteMotionSystem(MotionSystem* motionSystem)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_DELETE_MOTION_SYSTEM];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnDeleteMotionSystem(motionSystem);
        }
    }


    // scale actor data
    void EventManager::OnScaleActorData(Actor* actor, float scaleFactor)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_SCALE_ACTOR_DATA];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnScaleActorData(actor, scaleFactor);
        }
    }


    // scale motion data
    void EventManager::OnScaleMotionData(Motion* motion, float scaleFactor)
    {
        EventHandlerVector eventHandlers;
        {
            MCore::LockGuardRecursive lock(m_lock);
            eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_SCALE_MOTION_DATA];
        }
        for (EventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnScaleMotionData(motion, scaleFactor);
        }
    }

} // namespace EMotionFX
