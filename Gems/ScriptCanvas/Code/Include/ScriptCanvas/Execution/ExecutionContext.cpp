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

#include <stdarg.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

#include <AzFramework/Entity/EntityContextBus.h>

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>

namespace ScriptCanvas
{
    ExecutionContext::ExecutionContext()
    {
    }

    AZ::Outcome<void, AZStd::string> ExecutionContext::ActivateContext(const ScriptCanvasId& scriptCanvasId)
    {
        SystemRequestBus::BroadcastResult(m_systemComponentConfiguration, &SystemRequests::GetSystemComponentConfiguration);

        m_scriptCanvasId = scriptCanvasId;
        if (IsInIrrecoverableErrorState())
        {
            return AZ::Failure(AZStd::string::format("ExecutionContext with id %s is in an irrecoverable error state. It cannot be activated", m_scriptCanvasId.ToString().data()));
        }

        ErrorReporterBus::Handler::BusConnect(m_scriptCanvasId);
        ExecutionRequestBus::Handler::BusConnect(m_scriptCanvasId);

        ExecutionStack().swap(m_executionStack);
        m_isActive = true;        

        return AZ::Success();
    }

    void ExecutionContext::DeactivateContext()
    {
        ExecutionRequestBus::Handler::BusDisconnect();
        ErrorReporterBus::Handler::BusDisconnect();

        m_isActive = false;

        m_executedNodes.clear();
        m_sourceToErrorHandlerNodes.clear();
    }

    void ExecutionContext::AddToExecutionStack(Node& node, const SlotId& slotId)
    {
        m_executionStack.push(AZStd::make_pair(&node, slotId));
    }

    void ExecutionContext::ErrorIrrecoverably()
    {
        if (!m_isFinalErrorReported)
        {
            m_isInErrorState = true;
            m_isRecoverable = false;
            m_isFinalErrorReported = true;
            AZ_Warning("ScriptCanvas", false, "ERROR! Node: %s, Description: %s\n\n", m_errorReporter ? m_errorReporter->GetNodeName().c_str() : "unknown", m_errorDescription.c_str());
            ExecutionStack().swap(m_executionStack);
            // dump error report(callStackTop, m_errorReporter, m_error, graph name, entity ID)
            DeactivateContext();
        }
    }

    void ExecutionContext::AddErrorHandler(AZ::EntityId errorScopeId, AZ::EntityId errorHandlerNodeId)
    {
        if (errorScopeId == m_scriptCanvasId)
        {
            auto foundGraphScopeHandler = m_sourceToErrorHandlerNodes.find(errorScopeId);
            AZ_Warning("ScriptCanvas", foundGraphScopeHandler == m_sourceToErrorHandlerNodes.end(), "Multiple Graph Scope Error handlers specified");
        }

        m_sourceToErrorHandlerNodes.emplace(errorScopeId, errorHandlerNodeId);
    }

    AZ::EntityId ExecutionContext::GetErrorHandler() const
    {
        if (m_errorReporter)
        {
            auto iter = m_sourceToErrorHandlerNodes.find(m_errorReporter->GetEntityId());
            if (iter != m_sourceToErrorHandlerNodes.end())
            {
                return iter->second;
            }
        }

        auto graphScopeHandlerNodeIter = m_sourceToErrorHandlerNodes.find(m_scriptCanvasId);
        return (graphScopeHandlerNodeIter != m_sourceToErrorHandlerNodes.end()) ? graphScopeHandlerNodeIter->second : AZ::EntityId();
    }

    void ExecutionContext::HandleError(const Node& callStackTop)
    {
        if (!m_isInErrorHandler)
        {
            ErrorIrrecoverably();
        }
        else
        {
            m_errorDescription += "\nMultiple error handling attempted without resolving previous error handling. Last node: ";
            m_errorDescription += callStackTop.GetDebugName();
            ErrorIrrecoverably();
        }
    }

    void ExecutionContext::ReportError(const Node& reporter, const char* format, ...)
    {
        const size_t k_maxErrorMessageLength = 4096;
        char message[k_maxErrorMessageLength]{ {} };
        va_list args;
        va_start(args, format);
        azvsnprintf(message, k_maxErrorMessageLength, format, args);
        va_end(args);

        if (m_isInErrorState)
        {
            m_errorDescription += "\nMultiple errors reported without allowing for handling. Last node: ";
            m_errorDescription += reporter.GetDebugName();
            m_errorDescription += "\nDescription: ";
            m_errorDescription += message;
            ErrorIrrecoverably();
        }
        else if (m_isInErrorHandler)
        {
            m_errorDescription += "\nFurther error encountered during error handling. Last node: ";
            m_errorDescription += reporter.GetDebugName();
            m_errorDescription += "\nDescription: ";
            m_errorDescription += message;
            ErrorIrrecoverably();
        }
        else
        {
            m_errorReporter = &reporter;
            m_errorDescription = message;
            m_isInErrorState = true;
        }

        m_errorDescription += " Graph: ";
        m_errorDescription += reporter.GetGraphAssetName().c_str();

    }

    bool ExecutionContext::IsExecuting() const
    {
        return m_isExecuting;
    }

    bool ExecutionContext::HasQueuedExecution() const
    {
        return !m_executionStack.empty();
    }

    void ExecutionContext::Execute()
    {
        if (m_isActive && !m_isExecuting && !IsInErrorState())
        {
            m_isExecuting = true;
            AZ::u32 executionCount(0);
            
            while (!m_executionStack.empty() && m_isActive)
            {
                auto nodeAndSlot = m_executionStack.back();
                m_executionStack.pop();
                m_preExecutedStackSize = m_executionStack.size();
                nodeAndSlot.first->SignalInput(nodeAndSlot.second);

                m_executedNodes.insert(nodeAndSlot.first);

                ++executionCount;

                if (executionCount == m_systemComponentConfiguration.m_maxIterationsForInfiniteLoopDetection)
                {
                    ReportError(*nodeAndSlot.first, "Infinite loop detected");
                    ErrorIrrecoverably();

                    executionCount = -1;
                    break;
                }

                if (IsInErrorState())
                {
                    HandleError((*nodeAndSlot.first));

                    if (!m_isRecoverable)
                    {
                        break;
                    }
                }
            }
            
            m_isExecuting = false;
            RefreshInputs();

            SC_EXECUTION_TRACE_THREAD_ENDED(CreateGraphInfo(m_runtimeId));
        }
    }

    void ExecutionContext::ExecuteUntilNodeIsTopOfStack(Node& node)
    {
        if (m_isActive && !IsInErrorState())
        {
            m_isExecuting = true;
            AZ::u32 executionCount(0);

            while (m_isActive && !m_executionStack.empty())
            {
                auto nodeAndSlot = m_executionStack.back();
                m_executionStack.pop();
                
                if (nodeAndSlot.first == &node && !nodeAndSlot.second.IsValid())
                {
                    m_isExecuting = !m_executionStack.empty();
                    //SC_EXECUTION_TRACE_THREAD_ENDED();
                    return;
                }

                m_preExecutedStackSize = m_executionStack.size();
                nodeAndSlot.first->SignalInput(nodeAndSlot.second);

                ++executionCount;

                if (executionCount == SCRIPT_CANVAS_INFINITE_LOOP_DETECTION_COUNT)
                {
                    ReportError(*nodeAndSlot.first, "Infinite loop detected");
                    ErrorIrrecoverably();
                }

                if (IsInErrorState())
                {
                    HandleError((*nodeAndSlot.first));

                    if (!m_isRecoverable)
                    {
                        break;
                    }
                }
            }
            //SC_EXECUTION_TRACE_THREAD_ENDED();
        }
    }

    void ExecutionContext::UnwindStack([[maybe_unused]] const Node& callStackTop)
    {
        while (m_executionStack.size() > m_preExecutedStackSize)
        {
            m_executionStack.pop();
        }
    }

    void ExecutionContext::RefreshInputs()
    {
        if (!IsExecuting() && !IsInIrrecoverableErrorState())
        {
            for (auto node : m_executedNodes)
            {
                node->RefreshInput();
            }

            m_executedNodes.clear();
        }
    }

    AZStd::string_view ExecutionContext::GetLastErrorDescription() const
    {
        return m_errorDescription;
    }
}
