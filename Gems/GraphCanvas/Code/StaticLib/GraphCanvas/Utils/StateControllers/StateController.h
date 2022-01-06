/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_set.h>

// A configurable queue that allows for multiple sources to try to control a single value in a configurable way
// such that each object can control the object independently of the other systems, while still maintaining a reasonable state.
//
// Helps avoid situations where objects want to temporary modify a state and then return it to whatever previous state was necessary.
// Also can be used to handle prioritized states.
namespace GraphCanvas
{
    template<class T>
    class StateController;

    template<class T>
    class StateSetter;

    template<class T>
    class StateControllerNotificationInterface
        : public AZ::EBusTraits
    {
    public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef StateController<T>* BusIdType;

            virtual void OnStateChanged(const T& state) = 0;
    };

    template<class T>
    class StateController
    {
        friend class StateSetter<T>;
        
    public:
        using Notifications = AZ::EBus< StateControllerNotificationInterface<T> >;

        explicit StateController(const T& defaultState)
            : m_defaultState(defaultState)
        {
        }
        
        virtual ~StateController() = default;        
        
        const T& GetState() const
        {
            if (HasState())
            {
                return GetCalculatedState();
            }
            else
            {
                return GetDefaultState();
            }
        }

        virtual bool HasState() const = 0;
        
        bool operator==(const T& comparisonValue) const
        {
            return GetState() == comparisonValue;
        }

        bool operator!=(const T& comparisonValue) const
        {
            return GetState() != comparisonValue;
        }

    protected:
    
        bool PushState(StateSetter<T>* stateSetter, const T& state)
        {
            bool pushedState = false;

            T oldState = GetState();

            if (OnPushState(stateSetter, state))
            {
                pushedState = true;

                if (oldState != GetState())
                {
                    Notifications::Event(this, &Notifications::Events::OnStateChanged,GetState());
                }
            }

            return pushedState;
        }

        bool ReleaseState(StateSetter<T>* stateSetter)
        {
            bool releasedState = false;
            T oldState = GetState();

            if (OnReleaseState(stateSetter))
            {
                releasedState = true;

                if (oldState != GetState())
                {
                    Notifications::Event(this, &Notifications::Events::OnStateChanged,GetState());
                }
            }

            return releasedState;
        }

        virtual const T& GetCalculatedState() const = 0;
    
        const T& GetDefaultState() const
        {
            return m_defaultState;
        }

        virtual bool OnPushState(StateSetter<T>* stateSetter, const T& state) = 0;
        virtual bool OnReleaseState(StateSetter<T>* stateSetter) = 0;        
    
    private:
    
        T m_defaultState;
    };

    template<typename T>
    bool operator!=(const T& comparisonValue, const StateController<T>& controller)
    {
        return controller != comparisonValue;
    }

    template<typename T>
    bool operator==(const T& comparisonValue, const StateController<T>& controller)
    {
        return controller == comparisonValue;
    }
    
    template<class T>
    class StateSetter
    {
    private:
        StateSetter(const StateSetter<T>& stateSetter) = delete;

    public:
        StateSetter()
            : m_hasPushedState(false)
        {
        
        }        

        explicit StateSetter(StateController<T>* stateController)
            : StateSetter()
        {
            AddStateController(stateController);
        }
        
        StateSetter(StateController<T>* stateController, const T& state)
            : StateSetter()
        {
            AddStateController(stateController);
            SetState(state);
        }
        
        virtual ~StateSetter()
        {
            ResetStateSetter();
        }
        
        void SetState(const T& state)
        {
            ReleaseState();

            m_hasPushedState = true;
            m_pushedState = state;

            for (StateController<T>* stateController : m_stateControllers)
            {
                stateController->PushState(this, m_pushedState);
            }
        }
        
        void ReleaseState()
        {
            if (m_hasPushedState)
            {
                for (StateController<T>* stateController : m_stateControllers)
                {
                    stateController->ReleaseState(this);
                }

                m_hasPushedState = false;
            }
        }
        
        void AddStateController(StateController<T>* stateController)
        {
            if (stateController)
            {
                auto insertResult = m_stateControllers.insert(stateController);

                if (m_hasPushedState && insertResult.second)
                {                    
                    stateController->PushState(this, m_pushedState);
                }
            }
        }

        bool RemoveStateController(StateController<T>* stateController)
        {
            bool releasedState = false;

            if (stateController)
            {
                size_t elementsRemoved = m_stateControllers.erase(stateController);

                if (m_hasPushedState && elementsRemoved != 0)
                {
                    releasedState = stateController->ReleaseState(this);
                }
            }

            return releasedState;
        }

        void ResetStateSetter()
        {
            ReleaseState();
            m_stateControllers.clear();
        }

        bool HasTargets() const
        {
            return !m_stateControllers.empty();
        }

        bool HasState() const
        {
            return m_hasPushedState;
        }
        
    private:
        T                   m_pushedState;
        bool                m_hasPushedState;
        AZStd::unordered_set< StateController<T>* > m_stateControllers;
    };
}
