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
        return GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileLoggingEnabled", false);
    }

    bool GraphCompiler::Reset()
    {
        if (CanCompileGraph())
        {
            return true;
        }

        m_state = GraphCompiler::State::Canceled;
        return false;
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
        case GraphCompiler::State::Idle:
        case GraphCompiler::State::Failed:
        case GraphCompiler::State::Complete:
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
        CompileGraphStarted();

        // Skip compilation if there is no graph or this is a template.
        if (!m_graph || m_graphName.empty() || GetGraphPath().empty())
        {
            CompileGraphFailed();
            return false;
        }

        return true;
    }

    void GraphCompiler::CompileGraphStarted()
    {
        m_state = State::Compiling;
        m_generatedFiles.clear();
        ReportStatus(AZStd::string::format("Compiling %s (Started)", GetGraphPath().c_str()));
    }

    void GraphCompiler::CompileGraphFailed()
    {
        m_state = State::Failed;
        m_generatedFiles.clear();
        ReportStatus(AZStd::string::format("Compiling %s (Failed)", GetGraphPath().c_str()));
    }

    void GraphCompiler::CompileGraphCompleted()
    {
        m_state = State::Complete;
        ReportStatus(AZStd::string::format("Compiling %s (Completed)", GetGraphPath().c_str()));
    }

    bool GraphCompiler::ReportGeneratedFileStatus()
    {
        m_state = State::Processing;
        ReportStatus(AZStd::string::format("Compiling %s (Processing)", GetGraphPath().c_str()));

        AZStd::vector<AZStd::string> generatedFiles = m_generatedFiles;
        AZStd::reverse(generatedFiles.begin(), generatedFiles.end());

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
                        "Processing %s (%s)", generatedFile.c_str(), AzToolsFramework::AssetSystem::JobStatusString(job.m_status)));

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
