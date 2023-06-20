/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QString>

#include <AzCore/Component/TickBus.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_set.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationAction.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationModelIds.h>

namespace ScriptCanvas::Developer
{
    /**
        ActionRunner that will manage the editor automation stack, and handle pushing the precondition elements into the queue.
        Will not take ownership of all actions added manually, but any precondition elements will be deleted.
        
        Needs to be externally ticked.
    */
    class EditorAutomationActionRunner
    {
    public:

        EditorAutomationActionRunner() = default;
        ~EditorAutomationActionRunner();

        void Reset();
        bool Tick();

        // Pushes actions onto a stack.
        void AddAction(EditorAutomationAction* actionToRun);

        bool HasActions() const;

        bool HasErrors() const;
        const AZStd::vector< ActionReport >& GetErrors() const;

    private:

        AZStd::vector< ActionReport >                   m_errorReports;
        AZStd::vector< EditorAutomationAction* >        m_executionStack;
        AZStd::unordered_set< EditorAutomationAction* > m_actionsToDelete;

        EditorAutomationAction* m_currentAction = nullptr;
    };

    class StateModel
    {
    public:

        typedef AZStd::string DataKey;

        StateModel() = default;
        virtual ~StateModel() = default;


        const AZStd::any* FindStateData(const DataKey& dataId) const;

        template<typename T>
        const T* GetStateDataAs(const DataKey& dataId) const
        {
            const AZStd::any* stateData = FindStateData(dataId);
            return AZStd::any_cast<T>(stateData);
        }

        template<typename T>
        void SetStateData(const DataKey& dataId, const T& data)
        {
            auto insertResult = m_stateData.insert(dataId);
            auto dataIter = insertResult.first;

            dataIter->second = data;
        }

    protected:

        void ClearModelData();


    private:
        AZStd::unordered_map< DataKey, AZStd::any > m_stateData;
    };
    
    template <AZ::u32 Id>
    struct StateTraits
    {
        static int StateID() { return Id; }
    };

    class EditorAutomationState
    {
    public:
        static constexpr int EXIT_STATE_ID = (-1);

        AZ_TYPE_INFO(EditorAutomationState, "{B18A0531-E3C2-4209-8A9E-1B0195C28443}");
        AZ_CLASS_ALLOCATOR(EditorAutomationState, AZ::SystemAllocator);

        virtual ~EditorAutomationState() = default;

        virtual int GetStateId() const = 0;
        virtual const char* GetStateName() const = 0;

        void SetStateModel(StateModel* stateModel)
        {
            m_stateModel = stateModel;
        }

        template<typename T>
        void SetModelData(StateModel::DataKey dataKey, const T& modelData)
        {
            m_stateModel->SetStateData<T>(dataKey, modelData);
        }

        template<typename T>
        const T* GetModelData(StateModel::DataKey dataKey)
        {
            return m_stateModel->GetStateDataAs<T>(dataKey);
        }

        StateModel* GetStateModel() const
        {
            return m_stateModel;
        }

        void SetupStateActions(EditorAutomationActionRunner& actionRunner)
        {
            m_error = "";

            OnSetupStateActions(actionRunner);
        }

        void StateActionsComplete()
        {
            OnStateActionsComplete();
        }

        bool HasErrors() const
        {
            return !m_error.empty();
        }

        void ReportError(AZStd::string error)
        {
            m_error += AZStd::string::format("%s - %s\n", GetStateName(), error.c_str());
        }

        const AZStd::string& GetError() const
        {
            return m_error;
        }

    protected:

        virtual void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) = 0;
        virtual void OnStateActionsComplete() {};

    private:
        StateModel* m_stateModel = nullptr;

        AZStd::string m_error;
    };

    template <typename Traits>
    class StaticIdAutomationState : public EditorAutomationState
    {
    public:

        AZ_TYPE_INFO(StaticIdAutomationState, "{7E7450F7-5553-4877-8AC9-365A052F2758}");

        StaticIdAutomationState() = default;

        int GetStateId() const override { return Traits::StateID(); }
        const char* GetStateName() const override { return Traits::StateName(); }

        static int StateID() { return Traits::StateID(); }
    };

    // In order to re-use some states with different constructions setups without needing
    // a bunch of one off states to re-configure the pre-existing states through data model values
    // named state provides a way for states to construct their names at the expense of not
    // being able to statically reference the name/stateid.
    class NamedAutomationState : public EditorAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(NamedAutomationState, AZ::SystemAllocator)

        AZ_TYPE_INFO(NamedAutomationState, "{62DD037C-D80F-4B1B-9F3E-9F05400ABA24}");

        NamedAutomationState() = delete;
        NamedAutomationState(const char* name)
        {
            SetStateName(name);
            m_stateId = AZ::Crc32(name);
        }

        NamedAutomationState(const AZStd::string& stateName)
        {
            SetStateName(stateName);
        }

        int GetStateId() const override { return m_stateId; }

    protected:
        
        void SetStateName(const AZStd::string& stateName)
        {
            m_name = stateName;
            m_stateId = AZ_CRC(stateName.c_str());
        }

        const char* GetStateName() const override { return m_name.c_str(); }

    private:

        AZStd::string m_name;
        int m_stateId = EXIT_STATE_ID;
    };

    class CustomActionState
        : public NamedAutomationState
    {
    public:
        AZ_TYPE_INFO(CustomActionState, "{DA39829B-08F8-47BF-890D-9DA695BAD5A0}");

        CustomActionState(const char* name)
            : NamedAutomationState(name)
        {
        }

        void OnSetupStateActions(EditorAutomationActionRunner&) override final
        {
        }

        void OnStateActionsComplete() override final
        {
            OnCustomAction();
        }

        virtual void OnCustomAction() = 0;
    };

#define DefineStateId(className) class className##Id : public StateTraits<AZ_CRC_CE(#className)> {\
 public:\
        static const char* StateName()\
        {\
            return #className;\
        }\
};

    /**
        Generic test base class that will handle incrementing test steps, running the action runner, and interface with the test dialog to report back errors.
    */
    class EditorAutomationTest
        : public AZ::SystemTickBus::Handler
        , public StateModel
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorAutomationTest, AZ::SystemAllocator);
        AZ_RTTI(EditorAutomationTest, "{C46081C5-E7E9-47C7-96D4-29E7D1C4D403}");

        ~EditorAutomationTest();

        void StartTest();

        // AZ::SystemTick::Handler
        void OnSystemTick();
        ////

        void AddState(EditorAutomationState* newState);

        void SetHasCustomTransitions(bool hasCustomTransition);

        template <typename IdClass>
        void SetInitialStateId()
        {
            m_initialStateId = IdClass::StateID();
            AZ_TracePrintf("Script Canvas", "Initial State: %s\n", IdClass::StateName());
        }

        virtual void OnTestStarting() {}
        virtual void OnStateComplete(int /*stateId*/) {}
        virtual void OnTestComplete() {}
        

        bool HasRun() const;
        bool IsRunning() const;

        void SetTestName(QString testName) { m_testName = testName; }
        QString GetTestName() { return m_testName; }
        bool HasErrors() const { return !m_testErrors.empty(); }

        AZStd::vector< AZStd::string > GetErrors() const { return m_testErrors; }

    protected:

        bool SetupState(int stateId);

        int FindNextState(int stateId);

        virtual int EvaluateTransition(int)
        {
            return EditorAutomationState::EXIT_STATE_ID;
        }

        EditorAutomationTest(QString testName);

        void AddError(AZStd::string error);
        
        EditorAutomationActionRunner m_actionRunner;
        AZStd::vector< AZStd::string > m_testErrors;

        bool m_hasCustomTransitions = false;

        AZStd::vector< int > m_registrationOrder;
        AZStd::unordered_map< int, EditorAutomationState* > m_states;

    private:
        
        QString m_testName;

        int m_initialStateId = EditorAutomationState::EXIT_STATE_ID;

        int m_stateId = EditorAutomationState::EXIT_STATE_ID;
        EditorAutomationState* m_currentState = nullptr;
        //int m_state = 0;

        bool m_hasRun = false;
    };
}
