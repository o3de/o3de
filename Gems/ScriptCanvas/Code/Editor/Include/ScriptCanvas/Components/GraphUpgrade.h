/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Utils/VersioningUtils.h>
#include <GraphCanvas/Editor/EditorTypes.h>

namespace ScriptCanvas
{
    class Node;
}

namespace ScriptCanvasEditor
{
    class Graph;
    class StateMachine;

    //! StateTraits provides each state the ability to provide its own compile time ID
    template <AZ::u32 Id>
    struct StateTraits
    {
        static int StateID() { return Id; }
    };

    //! State interface, provides the framework for any given state that may run through the state machine
    class IState
    {
    public:

        friend class StateMachine;

        static constexpr int EXIT_STATE_ID = (-1);

        enum class ExitStatus
        {
            Default,
            Skipped,
            Upgraded
        };

    protected:

        virtual int GetStateId() const = 0;

        virtual void Enter() { OnEnter(); }
        virtual ExitStatus Exit() { return OnExit(); }

        virtual void OnEnter() {}
        virtual void Run() {}
        virtual ExitStatus OnExit() { return ExitStatus::Default; }

        virtual StateMachine* GetStateMachine() { return nullptr; }

        virtual int EvaluateTransition()
        {
            return EXIT_STATE_ID;
        }

        virtual const char* GetName() const { return "IState"; }
    };

    //! Base class for all states in the system, users must provide a state ID through StateTraits
    template <typename Traits>
    class State : public IState
    {
    public:

        explicit State(StateMachine* stateMachine)
            : m_stateMachine(stateMachine)
        {}

        template <typename T>
        T* GetStateMachine() { return azrtti_cast<T*>(m_stateMachine); }

        StateMachine* GetStateMachine() override { return m_stateMachine; }

        int GetStateId() const override { return Traits::StateID(); }

        static int StateID() { return Traits::StateID(); }

        void Enter() override
        {
            Log("ENTER >> %s", GetName());
            OnEnter();
        }

        ExitStatus Exit() override
        {
            Log("EXIT  << %s", GetName());
            return OnExit();
        }

        // Simplified function to trace messages
        void Log(const char* format, ...);

    private:

        bool m_verbose = true;

        StateMachine* m_stateMachine;
    };

#define DefineState(className) class className##Id : public StateTraits<AZ_CRC_CE(#className)> {}; \
                               class className : public State<className##Id>

#define DeclareState(className)     public: \
                                    className(StateMachine* stateMachine) \
                                        : State<className##Id>(stateMachine) {} \
                                    virtual ~className() = default; \
                                    protected: \
                                        const char* GetName() const override { return #className; }

    //! A state machine that operates on the SystemTickBus
    //! Only one state at a time will execute in a given frame
    class StateMachine : private AZ::SystemTickBus::Handler
    {
    public:
        AZ_RTTI(StateMachine, "{A3B08B4F-1E5D-492A-84DA-99AD58BA7AE0}");

        void Run(int startStateID);

        virtual void OnComplete(IState::ExitStatus /*exitStatus*/) {}

        void OnSystemTick() override;

        bool GetVerbose() const;

        const AZStd::string GetError() const { return m_error; }

        void SetVerbose(bool isVerbose);

        const AZStd::string& GetDebugPrefix() const;

        void SetDebugPrefix(AZStd::string_view);

        void MarkError(AZStd::string_view error) { m_error = error; }

        AZStd::shared_ptr<IState> m_currentState = nullptr;
        AZStd::vector<AZStd::shared_ptr<IState>> m_states;

    private:
        bool m_isVerbose = true;
        AZStd::string m_debugPrefix;
        AZStd::string m_error;
    };

    //! This state machine will collect and share a variety of data from the EditorGraph
    //! Each state will operate on the data as needed in order to upgrade different
    //! elements of a graph. It is done with discreet states in order to avoid
    //! blocking the main thread with too many long running operations
    class EditorGraphUpgradeMachine : public StateMachine
    {
    public:
        AZ_RTTI(EditorGraphUpgradeMachine, "{C7EABC22-A3DD-4ABE-8303-418EA3CD1246}", StateMachine);

        EditorGraphUpgradeMachine(Graph* graph);

        AZStd::unordered_set<ScriptCanvas::Node*> m_allNodes;
        AZStd::unordered_set<ScriptCanvas::Node*> m_outOfDateNodes;
        AZStd::unordered_set<ScriptCanvas::Node*> m_deprecatedNodes;
        AZStd::unordered_set<ScriptCanvas::Node*> m_sanityCheckRequiredNodes;

        AZStd::unordered_set< AZ::EntityId > graphCanvasNodesToDelete;

        AZStd::unordered_set<AZ::EntityId> m_deletedNodes;
        AZStd::unordered_set<AZ::EntityId> m_assetSanitizationSet;
        ScriptCanvas::GraphUpdateSlotReport m_updateReport;

        AZStd::unordered_map< AZ::EntityId, AZ::EntityId > m_scriptCanvasToGraphCanvasMapping;

        ScriptCanvas::ScriptCanvasId m_scriptCanvasId;
        GraphCanvas::GraphId m_graphCanvasGraphId;

        AZ::Entity m_scriptCanvasNodeId;

        bool m_graphNeedsDirtying = false;

        Graph* m_graph = nullptr;
        AZ::Data::Asset<AZ::Data::AssetData> m_asset;

        void SetAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

        void OnComplete(IState::ExitStatus exitStatus) override;

    };

    DefineState(ReplaceDeprecatedConnections)
    {
        DeclareState(ReplaceDeprecatedConnections);

        void Run() override;

        int EvaluateTransition() override;
    };

    DefineState(ReplaceDeprecatedNodes)
    {
        DeclareState(ReplaceDeprecatedNodes);

        void Run() override;

        int EvaluateTransition() override;
    };

    DefineState(CollectData)
    {
        DeclareState(CollectData);

        void Run() override;

        ExitStatus OnExit() override;

        int EvaluateTransition() override;
    };

    DefineState(PreRequisites)
    {
        DeclareState(PreRequisites);

        void Run() override;

        int EvaluateTransition() override;
    };

    DefineState(PreventUndo)
    {
        DeclareState(PreventUndo);

        void OnEnter() override;

        int EvaluateTransition() override;
    };

    DefineState(Start)
    {
        DeclareState(Start);

        void OnEnter() override;

        int EvaluateTransition() override;
    };

    DefineState(DisplayReport)
    {
        DeclareState(DisplayReport);

        void Run() override;
        ExitStatus OnExit() override { return IState::ExitStatus::Upgraded; }

        int EvaluateTransition() override;
    };

    DefineState(Finalize)
    {
        DeclareState(Finalize);

        void Run() override;

        int EvaluateTransition() override;
    };

    DefineState(VerifySaveDataVersion)
    {
        DeclareState(VerifySaveDataVersion);

        void Run() override;

        int EvaluateTransition() override;
    };

    DefineState(SanityChecks)
    {
        DeclareState(SanityChecks);

        void Run() override;
        ExitStatus OnExit() override;

        int EvaluateTransition() override;
    };

    DefineState(UpgradeScriptEvents)
    {
        DeclareState(UpgradeScriptEvents);

        void Run() override;

        int EvaluateTransition() override;
    };

    DefineState(FixLeakedData)
    {
        DeclareState(FixLeakedData);

        void Run() override;

        int EvaluateTransition() override;
    };

    DefineState(UpgradeConnections)
    {
        DeclareState(UpgradeConnections);

        void Run() override;

        int EvaluateTransition() override;
    };

    DefineState(UpdateOutOfDateNodes)
    {
        DeclareState(UpdateOutOfDateNodes);

        void Run() override;

        int EvaluateTransition() override;
    };

    DefineState(BuildGraphCanvasMapping)
    {
        DeclareState(BuildGraphCanvasMapping);

        void Run() override;

        int EvaluateTransition() override;
    };

    DefineState(RestoreUndo)
    {
        DeclareState(RestoreUndo);

        void Run() override;

        int EvaluateTransition() override;
    };

    DefineState(Skip)
    {
        DeclareState(Skip);

        void Run() override;
        ExitStatus OnExit() override { return IState::ExitStatus::Skipped; }

        int EvaluateTransition() override;
    };

    DefineState(ParseGraph)
    {
        DeclareState(ParseGraph);

        void Run() override;

        int EvaluateTransition() override;
    };


    template <typename Traits>
    void ScriptCanvasEditor::State<Traits>::Log(const char* format, ...)
    {
        if (m_verbose)
        {
            char sBuffer[2048];
            va_list ArgList;
            va_start(ArgList, format);
            azvsnprintf(sBuffer, sizeof(sBuffer), format, ArgList);
            sBuffer[sizeof(sBuffer) - 1] = '\0';
            va_end(ArgList);
            AZ_TracePrintf(ScriptCanvas::k_VersionExplorerWindow.data()
                , "%s-%s\n"
                , m_stateMachine->GetDebugPrefix().c_str()
                , sBuffer);
        }
    }
}
