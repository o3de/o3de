/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "SimpleStateComponent.h"

namespace
{
    const char * NullStateName = "<None>";
    const char * NewStateName = "New State";
}

namespace LmbrCentral
{
    // BehaviorContext SimpleStateComponentNotificationBus forwarder
    class BehaviorSimpleStateComponentNotificationBusHandler : public SimpleStateComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorSimpleStateComponentNotificationBusHandler, "{F935125C-AE4E-48C1-BB60-24A0559BC4D2}", AZ::SystemAllocator,
            OnStateChanged);

        void OnStateChanged(const char* oldState, const char* newState) override
        {
            Call(FN_OnStateChanged, oldState, newState);
        }
    };

    //=========================================================================
    // ForEachEntity
    //=========================================================================
    template <class Func>
    void ForEachEntity(AZStd::vector<AZ::EntityId>& entitiyIds, Func entityFunction)
    {
        for (const AZ::EntityId& currId : entitiyIds)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, currId);
            if (entity)
            {
                entityFunction(entity);
            }
        }
    }

    //=========================================================================
    // State::State
    //=========================================================================
    State::State()
        : m_name(NewStateName)
    {
    }

    //=========================================================================
    // State::Init
    //=========================================================================
    void State::Init()
    {
        UpdateNameCrc();
        for (const AZ::EntityId& currId : m_entityIds)
        {
            // Listen for the entity's initialization so we can adjust initial activation state.
            AZ::EntityBus::MultiHandler::BusConnect(currId);
        }
    }

    //=========================================================================
    // State::Activate
    //=========================================================================
    void State::Activate()
    {
        ForEachEntity(m_entityIds,
            [](AZ::Entity* entity)
            {
                if (entity->GetState() != AZ::Entity::State::Active)
                {
                    entity->Activate();
                }
            }
        );
    }

    //=========================================================================
    // State::Deactivate
    //=========================================================================
    void State::Deactivate()
    {
        ForEachEntity(m_entityIds,
            [](AZ::Entity* entity)
            {
                if (entity->GetState() == AZ::Entity::State::Active)
                {
                    entity->Deactivate();
                }
            }
        );
    }

    //=========================================================================
    // State::UpdateNameCrc
    //=========================================================================
    void State::UpdateNameCrc()
    {
        m_nameCrc = AZ::Crc32(m_name.c_str());
    }

    //=========================================================================
    // GetStateFromName
    //=========================================================================
    State* State::FindWithName(AZStd::vector<State>& states, const char* stateName)
    {
        if (stateName)
        {
            const AZ::Crc32 stateNameCrc(stateName);
            for (State& currState : states)
            {
                if ((currState.m_nameCrc == stateNameCrc) && (currState.m_name == stateName))
                {
                    return &currState;
                }
            }

            AZ_Error("SimpleStateComponent", false, "StateName '%s' does not map to any existing states", stateName);
        }

        return nullptr;
    }

    //=========================================================================
    // OnEntityExists
    //=========================================================================
    void State::OnEntityExists(const AZ::EntityId& entityId)
    {
        AZ::EntityBus::MultiHandler::BusDisconnect(entityId);

        // Mark the entity to not be activated by default.
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
        if (entity && entity->GetState() <= AZ::Entity::State::Init)
        {
            entity->SetRuntimeActiveByDefault(false);
        }
    }

    //=========================================================================
    // SimpleStateComponent::Reflect
    //=========================================================================
    void State::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<State>()
                ->Version(1)
                ->Field("Name", &State::m_name)
                ->Field("EntityIds", &State::m_entityIds);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<State>("State", "A state includes a name and set of entities that will be activated when the state is entered and deactivated when the state is left.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &State::m_name, "Name", "The name of this state")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshAttributesAndValues"))
                    ->DataElement(0, &State::m_entityIds, "Entities", "The list of entities referenced by this state")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshAttributesAndValues"));
            }
        }
    }

    //=========================================================================
    // SimpleStateComponent::Reflect
    //=========================================================================
    void SimpleStateComponent::Reflect(AZ::ReflectContext* context)
    {
        State::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SimpleStateComponent, AZ::Component>()
                ->Version(1)
                    ->Field("InitialStateName", &SimpleStateComponent::m_initialStateName)
                    ->Field("ResetOnActivate", &SimpleStateComponent::m_resetStateOnActivate)
                    ->Field("States", &SimpleStateComponent::m_states);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<SimpleStateComponent>("Simple State", "The Simple State component provides a simple state machine allowing activation and deactivation of associated entities")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/SimpleState.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/SimpleState.svg")
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/gameplay/simple-state/")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SimpleStateComponent::m_initialStateName, "Initial state", "The initial active state")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshAttributesAndValues"))
                        ->Attribute(AZ::Edit::Attributes::StringList, &SimpleStateComponent::GetStateNames)
                    ->DataElement(0, &SimpleStateComponent::m_resetStateOnActivate, "Reset on activate", "If set, SimpleState will return to the configured initial state when activated, and not the state held prior to being deactivated.")
                    ->DataElement(0, &SimpleStateComponent::m_states, "States", "The list of states")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshAttributesAndValues"));
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<SimpleStateComponentRequestBus>("SimpleStateComponentRequestBus")
                ->Event("SetState", &SimpleStateComponentRequestBus::Events::SetState)
                ->Event("SetStateByIndex", &SimpleStateComponentRequestBus::Events::SetStateByIndex)
                ->Event("SetToNextState", &SimpleStateComponentRequestBus::Events::SetToNextState)
                ->Event("SetToPreviousState", &SimpleStateComponentRequestBus::Events::SetToPreviousState)
                ->Event("SetToFirstState", &SimpleStateComponentRequestBus::Events::SetToFirstState)
                ->Event("SetToLastState", &SimpleStateComponentRequestBus::Events::SetToLastState)
                ->Event("GetNumStates", &SimpleStateComponentRequestBus::Events::GetNumStates)
                ->Event("GetCurrentState", &SimpleStateComponentRequestBus::Events::GetCurrentState)
                ;

            behaviorContext->EBus<SimpleStateComponentNotificationBus>("SimpleStateComponentNotificationBus")
                ->Handler<BehaviorSimpleStateComponentNotificationBusHandler>()
                ;
        }
    }

    //=========================================================================
    // SimpleStateComponent::SimpleStateComponent
    //=========================================================================
    SimpleStateComponent::SimpleStateComponent()
        : m_initialStateName(NullStateName)
    {
    }

    //=========================================================================
    // SimpleStateComponent::Init
    //=========================================================================
    void SimpleStateComponent::Init()
    {
        for (State& currState : m_states)
        {
            currState.Init();
        }

        // Prior revisions used the empty string as null state
        const char * initialStateName = (m_initialStateName.empty() || (m_initialStateName == NullStateName)) ? nullptr : m_initialStateName.c_str();
        m_initialState = State::FindWithName(m_states, initialStateName);
        m_currentState = m_initialState;
    }

    //=========================================================================
    // SimpleStateComponent::Activate
    //=========================================================================
    void SimpleStateComponent::Activate()
    {
        if (m_resetStateOnActivate)
        {
            m_currentState = m_initialState;
        }

        for (State& currState : m_states)
        {
            if (&currState != m_currentState)
            {
                currState.Deactivate();
            }
        }

        if (m_currentState)
        {
            m_currentState->Activate();
        }

        SimpleStateComponentRequestBus::Handler::BusConnect(GetEntityId());

        if (m_currentState)
        {
            // Notify newly activated State
            // Note: Even if !m_resetStateOnActivate, the prior state to Activation is NullState
            SimpleStateComponentNotificationBus::Event(
                GetEntityId(), &SimpleStateComponentNotificationBus::Events::OnStateChanged, nullptr, m_currentState->GetNameCStr());
        }
    }

    //=========================================================================
    // SimpleStateComponent::Deactivate
    //=========================================================================
    void SimpleStateComponent::Deactivate()
    {
        SimpleStateComponentRequestBus::Handler::BusDisconnect();

        if (m_currentState)
        {
            m_currentState->Deactivate();
        }
    }

    //=========================================================================
    // SimpleStateComponent::SetStateInternal
    //=========================================================================
    void SimpleStateComponent::SetStateInternal(State* newState)
    {
        // Out with the old
        const char* oldName = nullptr;
        if (m_currentState)
        {
            oldName = m_currentState->GetNameCStr();
            m_currentState->Deactivate();
        }

        // In with the new
        const char* newName = nullptr;
        if (newState)
        {
            newName = newState->GetNameCStr();
            newState->Activate();
        }

        if (m_currentState != newState)
        {
            m_currentState = newState;

            SimpleStateComponentNotificationBus::Event(
                GetEntityId(), &SimpleStateComponentNotificationBus::Events::OnStateChanged, oldName, newName);
        }
    }

    //=========================================================================
    // SimpleStateComponent::SetState
    //=========================================================================
    void SimpleStateComponent::SetState(const char* stateName)
    {
        State* newState = State::FindWithName(m_states, stateName);
        SetStateInternal(newState);
    }

    //=========================================================================
    // SimpleStateComponent::SetStateByIndex
    //=========================================================================
    void SimpleStateComponent::SetStateByIndex(AZ::u32 stateIndex)
    {
        State* newState;
        if (stateIndex < m_states.size())
        {
            newState = &m_states[stateIndex];
        }
        else
        {
            newState = nullptr;
            AZ_Error("SimpleStateComponent", false, "StateName index '%d' is invalid (currently %d states)", stateIndex, static_cast<AZ::u32>(m_states.size()));
        }
        SetStateInternal(newState);
    }


    //=========================================================================
    // SimpleStateComponent::SetToNextState
    //=========================================================================
    void SimpleStateComponent::SetToNextState()
    {
        if (!m_states.empty())
        {
            SetStateToOffset(1, m_states.front());
        }
    }

    //=========================================================================
    // SimpleStateComponent::SetToPreviousState
    //=========================================================================
    void SimpleStateComponent::SetToPreviousState()
    {
        if (!m_states.empty())
        {
            SetStateToOffset(-1, m_states.back());
        }
    }

    //=========================================================================
    // SimpleStateComponent::SetStateToOffset
    //=========================================================================
    void SimpleStateComponent::SetStateToOffset(AZ::s32 offset, State& fromNullState)
    {
        AZ_Assert(!m_states.empty(), "violated precondition - must be non-empty");

        State* newState;
        if (m_currentState)
        {
            const size_t currentStateIndex = m_currentState - &m_states[0];
            AZ_Assert(currentStateIndex < m_states.size(), "Invalid current state pointer");

            const size_t numStates = m_states.size();
            const size_t newStateIndex = (currentStateIndex + numStates + offset) % numStates;
            AZ_Assert(newStateIndex < numStates, "Invalid negative offset");
            newState = &m_states[newStateIndex];
        }
        else
        {
            // "Advance" to the provided state from NullState
            newState = &fromNullState;
        }

        SetStateInternal(newState);
    }

    //=========================================================================
    // SimpleStateComponent::SetToFirstState
    //=========================================================================
    void SimpleStateComponent::SetToFirstState()
    {
        if (!m_states.empty())
        {
            SetStateInternal(&m_states.front());
        }
    }

    //=========================================================================
    // SimpleStateComponent::SetToLastState
    //=========================================================================
    void SimpleStateComponent::SetToLastState()
    {
        if (!m_states.empty())
        {
            SetStateInternal(&m_states.back());
        }
    }

    //=========================================================================
    // SimpleStateComponent::GetNumStates
    //=========================================================================
    AZ::u32 SimpleStateComponent::GetNumStates()
    {
        return static_cast<AZ::u32>(m_states.size());
    }

    //=========================================================================
    // SimpleStateComponent::GetState
    //=========================================================================
    const char* SimpleStateComponent::GetCurrentState()
    {
        return m_currentState ? m_currentState->GetNameCStr() : nullptr;
    }

    //=========================================================================
    // SimpleStateComponent::GetStateNames
    //=========================================================================
    AZStd::vector<AZStd::string> SimpleStateComponent::GetStateNames() const
    {
        AZStd::vector<AZStd::string> names;

        names.reserve(m_states.size() + 1);

        // Provide "no initial state" as an option
        names.emplace_back(NullStateName);

        for (const State& currState : m_states)
        {
            names.emplace_back(currState.GetName());
        }

        return names;
    }
} // namespace LmbrCentral
