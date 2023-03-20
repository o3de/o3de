/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_HIERARCHIAL_STATE_MACHINE_H
#define AZCORE_HIERARCHIAL_STATE_MACHINE_H

#include <AzCore/base.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/delegate/delegate.h>

/**
 * Can be called from the user to automatically stinganize the state name, when a set is set
 * ex. hsm.SetStateHandler(AZ_HSM_STATE_NAME(MyState),&MyHndlr, SuperState);
 */
#define AZ_HSM_STATE_NAME(_STATE) _STATE,#_STATE

namespace AZ
{
    /**
     * Hierarchical state machine.
     * \note This implementation have the standard restriction for HSM:
     * 1. You can not call Transition from HSM::EnterEventId and HSM::ExitEventId! These event are provided
     * to execute your construction/destruction. Use custom events for that.
     * 2. You are not allowed to dispatch an event from within an event dispatch. You should queue events
     * if you want such behavior. This restriction is imposed only to prevent the user from creating
     * extremely complicated to trace state machines (which is what we are trying to avoid).
     */
    class HSM
    {
    public:
        typedef unsigned char StateId;
        enum ReservedEventIds
        {
            EnterEventId    = -1,
            ExitEventId     = -2
        };
        static const unsigned char MaxNumberOfStates = 254;
        static const unsigned char InvalidStateId = 255;
        static const int InvalidEventId = -4;

        struct Event
        {
            Event()
                : id(InvalidEventId)
                , userData(NULL) {}
            Event(int ID)
                : id(ID)
                , userData(NULL) {}
            int     id;
            void*   userData;
        };
        typedef AZStd::delegate<bool (HSM& sm, const Event& e)> StateHandler;

        HSM(const char* name = "HSM no name");

        const char*     GetName() const     { return m_name; }

        /// Starts the state machine.
        void            Start();
        /// Dispatches an event, return true if the event was processed, otherwise false.
        bool            Dispatch(const Event& e);
        /// Return true if the state machine is currently dispatching an event.
        bool            IsDispatching() const       { return m_curSourceState != InvalidStateId; }
        /// Transitions the state machine. This function can not be called from Enter / Exit events in the state handler.
        void            Transition(StateId target);

        StateId         GetCurrentState() const     { return m_curState; }
        StateId         GetSourceState() const      { return m_sourceState; }

        /**
         * Set a handler for a state ID. This function will overwrite the current state handler.
         * \param id state id from 0 to MaxNumberOfStates
         * \param name state debug name.
         * \param handler delegate to the state function.
         * \param superId id of the super state, if InvalidStateId this is a top state. Only one state can be a top state.
         * \param subId if != InvalidStateId this sub state (child state) will be entered after the state Enter event is executed.
         */
        void            SetStateHandler(StateId id, const char* name, const StateHandler& handler, StateId superId = InvalidStateId, StateId subId = InvalidStateId);
        /// Resets the state to invalid mode.
        void            ClearStateHandler(StateId id);
        /// @{ Event handing
        const char*     GetStateName(StateId id) const  { return m_states[id].name;     }
        StateId         GetSuperState(StateId id) const { return m_states[id].superId;  }
        bool            IsValidState(StateId id) const  { return !m_states[id].handler.empty(); }
        bool            IsInState(StateId id) const;
        /// @}

    protected:
        int             StepsToCommonRoot(StateId source, StateId target);
        void            OnEnterSubState();

        const char*     m_name;
        StateId         m_curState;
        StateId         m_nextState;        ///< Pointer to next state. Valid only when a transition is taken.
        StateId         m_sourceState;      ///< Pointer to source state during last transition.
        StateId         m_curSourceState;   ///< Current transition source state. It may not actually complete.
        StateId         m_topState;
        bool            m_disallowTransition;

        struct State
        {
            StateHandler handler;
            StateId      superId = InvalidStateId;   ///< State id of the super state, InvalidStateId if this is a top state InvalidStateId.
            StateId      subId = InvalidStateId;     ///< If != InvalidStateId it will enter the sub ID after the state Enter event is called.
            const char*  name = nullptr;
        };
        AZStd::array<State, MaxNumberOfStates>   m_states;
    };

    /**
    * Default empty handler.
    */
    template<bool handleEvent>
    bool DummyStateHandler(HSM& /*sm*/, const HSM::Event& /*e*/) { return handleEvent; }
}
#endif // AZCORE_HIERARCHIAL_STATE_MACHINE_H
#pragma once
