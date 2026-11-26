/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/GraphCompiler.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AtomToolsFramework
{
    void GraphCompiler::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GraphCompiler>()
                ->Version(0)
                ;
        }
    }

    GraphCompiler::GraphCompiler(const AZ::Crc32& toolId)
        : m_toolId(toolId)
    {
    }

    bool GraphCompiler::IsCompileLoggingEnabled()
    {
        return GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/EnableLogging", false);
    }

    bool GraphCompiler::Reset()
    {
        AZStd::scoped_lock lock(m_compileLifecycleMutex);
        if (m_compileInProgress)
        {
            m_cancelRequested = true;
            return false;
        }

        switch (m_state.load())
        {
        case State::Idle:
        case State::Failed:
        case State::Complete:
        case State::Canceled:
            // Reserve before GraphDocument dispatches the background job.
            m_compileInProgress = true;
            m_compileReserved = true;
            m_cancelRequested = false;
            return true;
        default:
            break;
        }

        return false;
    }

    void GraphCompiler::RequestCancel()
    {
        AZStd::scoped_lock lock(m_compileLifecycleMutex);
        if (m_compileInProgress)
        {
            m_cancelRequested = true;
        }
    }

    void GraphCompiler::SetStateChangeHandler(StateChangeHandler handler)
    {
        m_stateChangeHandler = handler;
    }

    void GraphCompiler::SetState(GraphCompiler::State state)
    {
        switch (state)
        {
        case State::Complete:
        case State::Canceled:
        case State::Failed:
            FinishCompile(state);
            return;
        default:
            break;
        }

        AZStd::scoped_lock lock(m_statePublicationMutex);
        PublishState(state);
    }

    void GraphCompiler::PublishState(GraphCompiler::State state)
    {
        m_state = state;

        switch (m_state)
        {
        case State::Idle:
            ReportStatus(AZStd::string::format("%s (Idle)", GetGraphPath().c_str()));
            break;
        case State::Compiling:
            ReportStatus(AZStd::string::format("%s (Compiling)", GetGraphPath().c_str()));
            break;
        case State::Processing:
            ReportStatus(AZStd::string::format("%s (Processing)", GetGraphPath().c_str()));
            break;
        case State::Complete:
            ReportStatus(AZStd::string::format("%s (Complete)", GetGraphPath().c_str()));
            break;
        case State::Failed:
            ReportStatus(AZStd::string::format("%s (Failed)", GetGraphPath().c_str()));
            break;
        case State::Canceled:
            ReportStatus(AZStd::string::format("%s (Cancelled)", GetGraphPath().c_str()));
            break;
        }

        // Invoke the optional state change handler function if provided
        if (m_stateChangeHandler)
        {
            m_stateChangeHandler(this);
        }
    }

    GraphCompiler::State GraphCompiler::GetState() const
    {
        return m_state;
    }

    AZStd::string GraphCompiler::GetGraphPath() const
    {
        return m_graphPath;
    }

    const AZStd::vector<AZStd::string>& GraphCompiler::GetGeneratedFilePaths() const
    {
        return m_generatedFiles;
    }

    const AZStd::vector<AZStd::string>& GraphCompiler::GetModifiedGeneratedFilePaths() const
    {
        return m_modifiedGeneratedFiles;
    }

    bool GraphCompiler::CanCompileGraph() const
    {
        if (m_compileInProgress)
        {
            return false;
        }

        switch (m_state.load())
        {
        case State::Idle:
        case State::Failed:
        case State::Complete:
        case State::Canceled:
            return true;
        }
        return false;
    }

    bool GraphCompiler::CompileGraph(GraphModel::GraphPtr graph, const AZStd::string& graphName, const AZStd::string& graphPath)
    {
        {
            AZStd::scoped_lock lock(m_compileLifecycleMutex);

            // Direct callers do not reserve the compiler through Reset.
            if (!m_compileInProgress)
            {
                switch (m_state.load())
                {
                case State::Idle:
                case State::Failed:
                case State::Complete:
                case State::Canceled:
                    m_compileInProgress = true;
                    m_compileReserved = true;
                    m_cancelRequested = false;
                    break;
                default:
                    return false;
                }
            }

            if (!m_compileReserved)
            {
                return false;
            }
            m_compileReserved = false;
        }

        {
            AZStd::scoped_lock lock(m_statePublicationMutex);
            m_generatedFiles.clear();
            m_modifiedGeneratedFiles.clear();
        }
        if (IsCancelRequested())
        {
            return FinishCompile(State::Canceled);
        }

        m_graph = graph;
        m_graphName = graphName;
        m_graphPath = graphPath;

        // Skip compilation if there is no graph or this is a template.
        if (!m_graph || m_graphName.empty() || GetGraphPath().empty())
        {
            return FinishCompile(State::Failed);
        }

        SetState(State::Compiling);
        if (IsCancelRequested())
        {
            return FinishCompile(State::Canceled);
        }
        return true;
    }

    bool GraphCompiler::IsCancelRequested() const
    {
        return m_cancelRequested;
    }

    bool GraphCompiler::FinishCompile(State finalState)
    {
        AZStd::unique_lock stateLock(m_statePublicationMutex);
        AZStd::unique_lock lifecycleLock(m_compileLifecycleMutex);
        if (!m_compileInProgress)
        {
            lifecycleLock.unlock();
            PublishState(finalState);
            return finalState == State::Complete;
        }

        State publishedState = finalState;
        if (m_cancelRequested)
        {
            publishedState = State::Canceled;
        }
        m_state = publishedState;
        m_compileReserved = false;
        m_compileInProgress = false;
        m_cancelRequested = false;
        lifecycleLock.unlock();
        PublishState(publishedState);
        return publishedState == State::Complete;
    }

    void GraphCompiler::ReportStatus(const AZStd::string& statusMessage)
    {
        if (m_toolId == AZ::Crc32{})
        {
            return;
        }

        AZStd::scoped_lock lock(m_lastStatusMessageMutex);
        if (m_lastStatusMessage != statusMessage)
        {
            m_lastStatusMessage = statusMessage;
            AZ::SystemTickBus::QueueFunction([toolId = m_toolId, statusMessage]() {
                AtomToolsMainWindowRequestBus::Event(toolId, &AtomToolsMainWindowRequestBus::Events::SetStatusMessage, statusMessage);
            });
        }
    }
} // namespace AtomToolsFramework
