/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Graph/GraphCompiler.h>
#include <AtomToolsFramework/Graph/GraphCompilerNotificationBus.h>
#include <AtomToolsFramework/Graph/GraphDocumentRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
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

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<GraphCompilerRequestBus>("GraphCompilerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Event("GetGraphPath", &GraphCompilerRequests::GetGraphPath)
                ->Event("GetGeneratedFilePaths", &GraphCompilerRequests::GetGeneratedFilePaths)
                ->Event("CompileGraph", &GraphCompilerRequests::CompileGraph)
                ->Event("QueueCompileGraph", &GraphCompilerRequests::QueueCompileGraph)
                ->Event("IsCompileGraphQueued", &GraphCompilerRequests::IsCompileGraphQueued)
                ->Event("ReportGeneratedFileStatus", &GraphCompilerRequests::ReportGeneratedFileStatus)
                ;
        }
    }

    GraphCompiler::GraphCompiler(const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        : m_toolId(toolId)
        , m_documentId(documentId)
    {
        GraphCompilerRequestBus::Handler::BusConnect(documentId);
    }

    GraphCompiler::~GraphCompiler()
    {
        GraphCompilerRequestBus::Handler::BusDisconnect();
    }

    bool GraphCompiler::IsCompileLoggingEnabled()
    {
        return GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileLoggingEnabled", false);
    }

    AZStd::string GraphCompiler::GetGraphPath() const
    {
        AZStd::string absolutePath;
        AtomToolsDocumentRequestBus::EventResult(absolutePath, m_documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);
        return absolutePath;
    }

    const AZStd::vector<AZStd::string>& GraphCompiler::GetGeneratedFilePaths() const
    {
        return m_generatedFiles;
    }

    bool GraphCompiler::CompileGraph()
    {
        CompileGraphStarted();

        GraphModel::GraphPtr graph;
        GraphDocumentRequestBus::EventResult(graph, m_documentId, &GraphDocumentRequestBus::Events::GetGraph);

        AZStd::string graphName;
        GraphDocumentRequestBus::EventResult(graphName, m_documentId, &GraphDocumentRequestBus::Events::GetGraphName);

        // Skip compilation if there is no graph or this is a template.
        if (!graph || graphName.empty())
        {
            CompileGraphFailed();
            return false;
        }

        return true;
    }

    void GraphCompiler::QueueCompileGraph()
    {
        m_compileGraph = true;
    }

    bool GraphCompiler::IsCompileGraphQueued() const
    {
        return m_compileGraph;
    }

    void GraphCompiler::CompileGraphStarted()
    {
        m_compileGraph = false;
        m_generatedFiles.clear();
        m_generatedFileIndexToProcess = 0;
        m_lastStatusMessage.clear();

        ReportStatus(AZStd::string::format("Compiling %s (Started)", GetGraphPath().c_str()));

        GraphCompilerNotificationBus::Event(m_toolId, &GraphCompilerNotificationBus::Events::OnCompileGraphStarted, m_documentId);
    }

    void GraphCompiler::CompileGraphFailed()
    {
        m_generatedFiles.clear();
        m_generatedFileIndexToProcess = 0;
        m_lastStatusMessage.clear();

        ReportStatus(AZStd::string::format("Compiling %s (Failed)", GetGraphPath().c_str()));

        GraphCompilerNotificationBus::Event(m_toolId, &GraphCompilerNotificationBus::Events::OnCompileGraphFailed, m_documentId);
    }

    void GraphCompiler::CompileGraphCompleted()
    {
        ReportStatus(AZStd::string::format("Compiling %s (Completed)", GetGraphPath().c_str()));

        GraphCompilerNotificationBus::Event(m_toolId, &GraphCompilerNotificationBus::Events::OnCompileGraphCompleted, m_documentId);
    }

    bool GraphCompiler::ReportGeneratedFileStatus()
    {
        // Check asset processor status of each generated file
        for (; m_generatedFileIndexToProcess < m_generatedFiles.size(); ++m_generatedFileIndexToProcess)
        {
            // Prevent status requests from blocking the main thread when monitoring several files.
            if (AZStd::chrono::steady_clock::now() < (m_lastStatusRequestTime + AZStd::chrono::milliseconds(10)))
            {
                return false;
            }

            const auto& generatedFile = m_generatedFiles[m_generatedFileIndexToProcess];
            AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
            AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(
                jobOutcome, &AzToolsFramework::AssetSystemJobRequestBus::Events::GetAssetJobsInfo, generatedFile, true);
            m_lastStatusRequestTime = AZStd::chrono::steady_clock::now();

            if (jobOutcome.IsSuccess())
            {
                for (const auto& job : jobOutcome.GetValue())
                {
                    ReportStatus(AZStd::string::format(
                        "Processing %s (%s)", generatedFile.c_str(), AzToolsFramework::AssetSystem::JobStatusString(job.m_status)));

                    switch (job.m_status)
                    {
                    case AzToolsFramework::AssetSystem::JobStatus::Queued:
                    case AzToolsFramework::AssetSystem::JobStatus::InProgress:
                        // If any of the asset jobs are still processing then return early instead of allowing the completion
                        // notification to be sent.
                        return false;
                    case AzToolsFramework::AssetSystem::JobStatus::Failed:
                    case AzToolsFramework::AssetSystem::JobStatus::Failed_InvalidSourceNameExceedsMaxLimit:
                        // If any of the asset jobs failed, cancel compilation.
                        CompileGraphFailed();
                        return true;
                    }
                }
            }
        }

        return true;
    }

    void GraphCompiler::ReportStatus(const AZStd::string& statusMessage)
    {
        if (m_lastStatusMessage != statusMessage)
        {
            m_lastStatusMessage = statusMessage;

            AZ_TracePrintf_IfTrue("GraphCompiler", IsCompileLoggingEnabled(), "%s\n", statusMessage.c_str());

            AtomToolsMainWindowRequestBus::Event(m_toolId, &AtomToolsMainWindowRequestBus::Events::SetStatusMessage, statusMessage);
        }
    }
} // namespace AtomToolsFramework
