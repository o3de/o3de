/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /*!
     * SimpleStateComponentRequests
     * Messages serviced by the Simple State component.
     */
    class SimpleStateComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~SimpleStateComponentRequests() {}

        //! Set the active state
        virtual void SetState(const char* stateName) = 0;

        //! Set the active state by 0 based index
        virtual void SetStateByIndex(AZ::u32 stateIndex) = 0;

        //! Advance to the next state, NullState "advances" to the first state
        virtual void SetToNextState() = 0;

        //! Go to the previous state, NullState "advances" to the end state
        virtual void SetToPreviousState() = 0;

        //! Go to the first state
        virtual void SetToFirstState() = 0;

        //! Go to the last state
        virtual void SetToLastState() = 0;

        // Get the number of states
        virtual AZ::u32 GetNumStates() = 0;
        
        //! Get the current state
        virtual const char* GetCurrentState() = 0;
    };

    using SimpleStateComponentRequestBus = AZ::EBus<SimpleStateComponentRequests>;

    /*!
     * SimpleStateComponentNotificationBus
     * Events dispatched by the Simple State component.
     */
    class SimpleStateComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual ~SimpleStateComponentNotifications() {}

        //! Notify that the state has changed from oldState to newState
        virtual void OnStateChanged(const char* oldState, const char* newState) = 0;
    };

    using SimpleStateComponentNotificationBus = AZ::EBus<SimpleStateComponentNotifications>;
} // namespace LmbrCentral
