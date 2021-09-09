/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/State/HSM.h>

#include <AzCore/std/containers/fixed_vector.h>
using namespace AZ;

#define AZ_HSM_MAX_NEST_DEPTH   10

//=========================================================================
// HSM
// [1/18/2011]
//=========================================================================
HSM::HSM(const char* name)
    : m_name(name)
    , m_curState(InvalidStateId)
    , m_nextState(InvalidStateId)
    , m_sourceState(InvalidStateId)
    , m_curSourceState(InvalidStateId)
    , m_topState(InvalidStateId)
    , m_disallowTransition(false)
{}


//=========================================================================
// Start
// [1/18/2011]
//=========================================================================
void
HSM::Start()
{
    AZ_Assert(m_topState != InvalidStateId, "You must provide a valid top state (state without super state) to start a HSM!");
    m_curState = m_topState;
    m_nextState = InvalidStateId;

    m_states[m_curState].handler(*this, Event(EnterEventId));
    while ((m_nextState = m_states[m_curState].subId) != InvalidStateId)
    {
        OnEnterSubState();
    }
}

//=========================================================================
// Dispatch
// [1/18/2011]
//=========================================================================
bool
HSM::Dispatch(const Event& e)
{
    AZ_Assert(e.id >= 0, "Events <0 are reserved for internal oprations!");
    bool isProcessed = false;
    AZ_Assert(!IsDispatching(), "You can't dispatch an event from within dispatch (make the machine really complicated), make sure you quque the events");
    for (StateId sid = m_curState; sid != InvalidStateId; )
    {
        State& state = m_states[sid];
        m_curSourceState = sid;
        if (state.handler(*this, e)) // processed ?
        {
            if (m_nextState != InvalidStateId)  // if we made a transition, execute it.
            {
                do
                {
                    OnEnterSubState();
                } while ((m_nextState = m_states[m_curState].subId) != InvalidStateId);
            }
            isProcessed = true;
            break;
        }

        sid = state.superId;
    }
    m_curSourceState = InvalidStateId;
    return isProcessed;
}

//=========================================================================
// OnEnterSubState
// [1/18/2011]
//=========================================================================
void
HSM::OnEnterSubState()
{
    AZ_Assert(m_nextState != InvalidStateId, "You must has set a valid transition state id!");

    AZStd::fixed_vector<State*, AZ_HSM_MAX_NEST_DEPTH> entryPath;
    for (StateId sid = m_nextState; sid != m_curState; )
    {
        State& state = m_states[sid];
        entryPath.push_back(&state);
        sid = state.superId;
        AZ_Assert(IsValidState(sid), "State id points to an invalid state!");
        AZ_Assert(sid != InvalidStateId, "Next state is not a sub/child state of the current state!");
    }

    while (!entryPath.empty())
    {
        m_disallowTransition = true;
        entryPath.back()->handler(*this, Event(EnterEventId));
        m_disallowTransition = false;
        entryPath.pop_back();
    }

    m_curState = m_nextState;
    m_nextState = InvalidStateId;
}

//=========================================================================
// Transition
// [1/18/2011]
//=========================================================================
void
HSM::Transition(StateId target)
{
    AZ_Assert(target < MaxNumberOfStates, "Invalid state");
    AZ_Assert(m_disallowTransition == false, "You are NOT allowed to execute state transition in the Enter/Exit events");
    AZ_Assert(IsDispatching(), "You can call Transition only from a session event!");

    if (m_disallowTransition)
    {
        return;
    }

    if (m_curSourceState != InvalidStateId)  // if we transition from within event CB
    {
        m_sourceState = m_curSourceState;
    }

    m_disallowTransition = true; // we can't execute transition while exiting
    // exit to the source state
    StateId sid;
    for (sid = m_curState; sid != m_sourceState; )
    {
        State& state = m_states[sid];
        state.handler(*this, Event(ExitEventId));
        sid = state.superId;
    }
    // stepsToCommonRoot can be cached in a quick look up table ([source][target]) if states don't change dynamically.
    int stepsToCommonRoot = StepsToCommonRoot(m_sourceState, target);
    AZ_Assert(stepsToCommonRoot >= 0, "Cannot find common root between %d and %d state!", m_sourceState, target);

    while (stepsToCommonRoot--)
    {
        State& state = m_states[sid];
        state.handler(*this, Event(ExitEventId));
        sid = state.superId;
    }
    m_disallowTransition = false;

    // set up the transition that will be executed after we leave the current event/block.
    m_curState = sid;
    m_nextState = target;
}

//=========================================================================
// SetStateHandler
// [1/18/2011]
//=========================================================================
void
HSM::SetStateHandler(StateId id, const char* name, const StateHandler& handler, StateId superId, StateId subId)
{
    if (superId == InvalidStateId)
    {
        AZ_Assert(m_topState == InvalidStateId, "A state without super state (top state) has been already set!");
        m_topState = id;
    }
    State state;
    state.superId = superId;
    state.subId = subId;
    state.handler = handler;
    state.name = name;
    m_states[id] = state;

    AZ_Error("HSM", superId == InvalidStateId || m_states[superId].subId != InvalidStateId, "The super state doesn't have defaule subId! It must have one if he has sub states. This is ok if set the super state sub Id later!");
}

//=========================================================================
// ClearStateHandler
// [1/18/2011]
//=========================================================================
void
HSM::ClearStateHandler(StateId id)
{
    m_states[id].handler.clear();
    m_states[id].name = nullptr;
    m_states[id].superId = InvalidStateId;
}

//=========================================================================
// IsInState
// [1/27/2011]
//=========================================================================
bool
HSM::IsInState(StateId id) const
{
    StateId sid = m_curState;
    while (sid != InvalidStateId)
    {
        if (sid == id)
        {
            return true;
        }

        sid = m_states[sid].superId;
    }
    return false;
}

//=========================================================================
// StepsToCommonRoot
// [1/18/2011]
//=========================================================================
int
HSM::StepsToCommonRoot(StateId source, StateId target)
{
    if (source == target)
    {
        return 1;
    }

    int toLca = 0;
    for (StateId s = source; s != InvalidStateId; ++toLca)
    {
        for (StateId t = target; t != InvalidStateId; )
        {
            if (s == t)
            {
                return toLca;
            }

            t = m_states[t].superId;
        }

        s = m_states[s].superId;
    }

    return -1;
}
