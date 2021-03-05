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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/Execution/ErrorBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>

namespace ScriptCanvas
{
    class Node;

    //! Execution Context  responsible for executing a ScriptCanvas Graph
    //! It maintains a stack of currently executing nodes as well a unique id
    //! for identifying the execution context
    class ExecutionContext
        : protected ExecutionRequestBus::Handler
        , protected ErrorReporterBus::Handler
    {
    public:
        
        AZ_TYPE_INFO(ExecutionContext, "{2C137581-19F4-42EB-8BF3-14DBFBC02D8D}");

        ExecutionContext();
        ~ExecutionContext() override = default;

        AZ::Outcome<void, AZStd::string> ActivateContext(const ScriptCanvasId& scriptCanvasId);
        void DeactivateContext();

        ScriptCanvasId GetScriptCanvasId() const { return m_scriptCanvasId; };

        //// ErrorReporterBus::Handler
        AZStd::string_view GetLastErrorDescription() const override;
        void HandleError(const Node& callStackTop) override;
        bool IsInErrorState() const override{ return m_isInErrorState; }
        bool IsInIrrecoverableErrorState() const override { return m_isInErrorState && !m_isRecoverable; }
        void ReportError(const Node& reporter, const char* format, ...) override;

        ////

        void AddErrorHandler(AZ::EntityId errorScopeId, AZ::EntityId errorHandlerNodeId);

        /**
        ** Use with caution, or better, not at all. The slot is the input execution slot, and empty slot is
        ** acceptable for nodes that only have one execution input.
        **/
        void AddToExecutionStack(Node& node, const SlotId& slotId) override;

        bool IsExecuting() const override;
        bool HasQueuedExecution() const;
        /**
        ** Use with caution, or better, not at all. This is only currently public to all for BehaviorContext ebus handlers with return values
        ** to properly function.
        **/
        void Execute() override;
        void ExecuteUntilNodeIsTopOfStack(Node& node) override;
        ////

    protected:
        void ErrorIrrecoverably();
        AZ::EntityId GetErrorHandler() const;
        //! Searches for the node ids that from both endpoints of the connection inside of the supplied node set container
        void UnwindStack(const Node& callStackTop);

        void RefreshInputs();

    private:
        ScriptCanvasId m_scriptCanvasId;

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_sourceToErrorHandlerNodes;

        bool m_isActive = false;
        bool m_isInErrorState = false;
        bool m_isExecuting = false;
        bool m_isFinalErrorReported = false;
        bool m_isRecoverable = true;
        bool m_isInErrorHandler = false;

        const Node* m_errorReporter{};
        AZStd::string m_errorDescription;

        using StackEntry = AZStd::pair<Node*, SlotId>;
        using ExecutionStack = AZStd::stack<StackEntry, AZStd::vector<StackEntry>>;
        ExecutionStack m_executionStack;
        size_t m_preExecutedStackSize{};

        AZStd::unordered_set<Node*> m_executedNodes;

        SystemComponentConfiguration m_systemComponentConfiguration;
    };

}
