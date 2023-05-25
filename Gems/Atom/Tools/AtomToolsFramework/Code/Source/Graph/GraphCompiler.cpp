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
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

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

    GraphCompiler::~GraphCompiler()
    {
    }

    bool GraphCompiler::IsCompileLoggingEnabled()
    {
        return GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/EnableLogging", false);
    }

    bool GraphCompiler::Reset()
    {
        if (CanCompileGraph())
        {
            return true;
        }

        SetState(State::Canceled);
        return false;
    }

    void GraphCompiler::SetStateChangeHandler(StateChangeHandler handler)
    {
        m_stateChangeHandler = handler;
    }

    void GraphCompiler::SetState(GraphCompiler::State state)
    {
        m_state = state;

        switch (m_state)
        {
        case State::Idle:
            ReportStatus(AZStd::string::format("%s (Idle)", GetGraphPath().c_str()));
            m_graph.reset();
            m_graphName.clear();
            m_graphPath.clear();
            m_generatedFiles.clear();
            break;
        case State::Compiling:
            ReportStatus(AZStd::string::format("%s (Compiling)", GetGraphPath().c_str()));
            m_generatedFiles.clear();
            break;
        case State::Processing:
            ReportStatus(AZStd::string::format("%s (Processing)", GetGraphPath().c_str()));
            break;
        case State::Complete:
            ReportStatus(AZStd::string::format("%s (Complete)", GetGraphPath().c_str()));
            m_graph.reset();
            break;
        case State::Failed:
            ReportStatus(AZStd::string::format("%s (Failed)", GetGraphPath().c_str()));
            m_graph.reset();
            break;
        case State::Canceled:
            ReportStatus(AZStd::string::format("%s (Cancelled)", GetGraphPath().c_str()));
            m_graph.reset();
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

    bool GraphCompiler::CanCompileGraph() const
    {
        switch (m_state)
        {
        case State::Idle:
        case State::Failed:
        case State::Complete:
            return true;
        }
        return false;
    }

    bool GraphCompiler::CompileGraph(GraphModel::GraphPtr graph, const AZStd::string& graphName, const AZStd::string& graphPath)
    {
        if (!CanCompileGraph())
        {
            return false;
        }

        m_graph = graph;
        m_graphName = graphName;
        m_graphPath = graphPath;
        SetState(State::Compiling);

        // Skip compilation if there is no graph or this is a template.
        if (!m_graph || m_graphName.empty() || GetGraphPath().empty())
        {
            SetState(State::Failed);
            return false;
        }

        return true;
    }

    bool GraphCompiler::ReportGeneratedFileStatus()
    {
        SetState(State::Processing);

        AZStd::vector<AZStd::string> generatedFiles(m_generatedFiles.rbegin(), m_generatedFiles.rend());

        // Check asset processor status of each generated file
        while (!generatedFiles.empty())
        {
            if (m_state != State::Processing)
            {
                return false;
            }

            // Forcing the string to be copied before it's captured and since to the main thread.
            const AZStd::string generatedFile = generatedFiles.back();
            AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
            AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(
                jobOutcome, &AzToolsFramework::AssetSystemJobRequestBus::Events::GetAssetJobsInfo, generatedFile, false);

            bool jobsComplete = true;
            if (jobOutcome.IsSuccess())
            {
                for (const auto& job : jobOutcome.GetValue())
                {
                    if (m_state != State::Processing)
                    {
                        return false;
                    }

                    ReportStatus(AZStd::string::format(
                        "%s (Processing: %s)", generatedFile.c_str(), AzToolsFramework::AssetSystem::JobStatusString(job.m_status)));

                    switch (job.m_status)
                    {
                    case AzToolsFramework::AssetSystem::JobStatus::Queued:
                    case AzToolsFramework::AssetSystem::JobStatus::InProgress:
                        // If any of the asset jobs are still processing then return early instead of allowing the completion
                        // notification to be sent.
                        jobsComplete = false;
                        break;
                    case AzToolsFramework::AssetSystem::JobStatus::Failed:
                    case AzToolsFramework::AssetSystem::JobStatus::Failed_InvalidSourceNameExceedsMaxLimit:
                        // If any of the asset jobs failed, cancel compilation.
                        return false;
                    }
                }
            }

            if (jobsComplete)
            {
                generatedFiles.pop_back();
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
        }

        return true;
    }

    void GraphCompiler::ReportStatus(const AZStd::string& statusMessage)
    {
        AZStd::scoped_lock lock(m_lastStatusMessageMutex);
        if (m_lastStatusMessage != statusMessage)
        {
            m_lastStatusMessage = statusMessage;
            AZ_TracePrintf_IfTrue("GraphCompiler", IsCompileLoggingEnabled(), "%s\n", m_lastStatusMessage.c_str());
            AZ::SystemTickBus::QueueFunction([toolId = m_toolId, statusMessage]() {
                AtomToolsMainWindowRequestBus::Event(toolId, &AtomToolsMainWindowRequestBus::Events::SetStatusMessage, statusMessage);
            });
        }
    }
} // namespace AtomToolsFramework
