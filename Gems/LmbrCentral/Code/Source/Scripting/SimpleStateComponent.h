/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <LmbrCentral/Scripting/SimpleStateComponentBus.h>

namespace LmbrCentral
{
    /**
    * State
    *
    * Structure describing a single state
    */
    class State
        : private AZ::EntityBus::MultiHandler
    {
    public:
        AZ_TYPE_INFO(State, "{97BCF9D8-A76D-456F-A4B8-98EFF6897CE7}");

        State();

        void Init();
        void Activate();
        void Deactivate();

        const char* GetNameCStr() const
        {
            return m_name.c_str();
        }

        const AZStd::string& GetName() const
        {
            return m_name;
        }

        static State* FindWithName(AZStd::vector<State>& states, const char* stateName);
        static void Reflect(AZ::ReflectContext* context);

    private:

        //////////////////////////////////////////////////////////////////////////
        // EntityBus::Handler
        void OnEntityExists(const AZ::EntityId& entityId) override;
        //////////////////////////////////////////////////////////////////////////

        AZ::Crc32 OnStateNameChanged();
        void UpdateNameCrc();

        // runtime value, not serialized
        AZ::Crc32 m_nameCrc;

        // serialized values
        AZStd::string m_name;
        AZStd::vector<AZ::EntityId> m_entityIds;
    };


    /**
     * SimpleStateComponent
     *
     * SimpleState provides a simple state machine.
     *
     * Each state is represented by a name and zero or more entities to activate when entered and deactivate when the state is left.
     */
    class SimpleStateComponent
        : public AZ::Component
        , public SimpleStateComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(SimpleStateComponent, "{242D4707-BC72-4245-AC96-BCEE38BBC1B7}");

        SimpleStateComponent();
        ~SimpleStateComponent() override {}

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SimpleStateComponentRequestBus::Handler
        //////////////////////////////////////////////////////////////////////////
        void SetState(const char* stateName) override;
        void SetStateByIndex(AZ::u32 stateIndex) override;
        void SetToNextState() override;
        void SetToPreviousState() override;
        void SetToFirstState()  override;
        void SetToLastState() override;
        AZ::u32 GetNumStates()  override;
        const char* GetCurrentState() override;

    private:

        //////////////////////////////////////////////////////////////////////////
        // Component descriptor
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("SimpleStateService"));
        }

        //////////////////////////////////////////////////////////////////////////
        // Helpers
        AZStd::vector<AZStd::string> GetStateNames() const;
        void SetStateInternal(State * newState);
        void SetStateToOffset(AZ::s32 offset, State& fromNullState);

        //////////////////////////////////////////////////////////////////////////
        // Runtime state, not serialized
        State* m_initialState = nullptr;
        State* m_currentState = nullptr;

        // Serialized
        AZStd::string m_initialStateName;
        AZStd::vector<State> m_states;
        bool m_resetStateOnActivate = true;
    };
} // namespace LmbrCentral
