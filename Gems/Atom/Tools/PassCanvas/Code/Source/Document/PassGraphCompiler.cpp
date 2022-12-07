/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Graph/GraphDocumentRequestBus.h>
#include <AtomToolsFramework/Graph/GraphUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Document/PassGraphCompiler.h>
#include <Document/PassGraphCompilerNotificationBus.h>
#include <GraphModel/Model/Connection.h>

namespace PassCanvas
{
#if defined(AZ_ENABLE_TRACING)
    namespace
    {
        bool IsCompileLoggingEnabled()
        {
            return AtomToolsFramework::GetSettingsValue("/O3DE/Atom/PassGraphCompiler/CompileLoggingEnabled", false);
        }
    } // namespace
#endif // AZ_ENABLE_TRACING

    void PassGraphCompiler::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PassGraphCompiler>()
                ->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PassGraphCompilerRequestBus>("PassGraphCompilerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "passcanvas")
                ->Event("GetGeneratedFilePaths", &PassGraphCompilerRequests::GetGeneratedFilePaths)
                ->Event("CompileGraph", &PassGraphCompilerRequests::CompileGraph)
                ->Event("QueueCompileGraph", &PassGraphCompilerRequests::QueueCompileGraph)
                ->Event("IsCompileGraphQueued", &PassGraphCompilerRequests::IsCompileGraphQueued);
        }
    }

    PassGraphCompiler::PassGraphCompiler(const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        : m_toolId(toolId)
        , m_documentId(documentId)
    {
        AZ::SystemTickBus::Handler::BusConnect();
        PassGraphCompilerRequestBus::Handler::BusConnect(documentId);
    }

    PassGraphCompiler::~PassGraphCompiler()
    {
        PassGraphCompilerRequestBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    const AZStd::vector<AZStd::string>& PassGraphCompiler::GetGeneratedFilePaths() const
    {
        return m_generatedFiles;
    }

    void PassGraphCompiler::OnSystemTick()
    {
        if (m_compileGraph)
        {
            CompileGraph();
        }

        if (m_compileAssets)
        {
            // Check asset processor status of each generated file
            while (m_compileAssetIndex < m_generatedFiles.size())
            {
                const auto& generatedFile = m_generatedFiles[m_compileAssetIndex];
                AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
                AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(
                    jobOutcome, &AzToolsFramework::AssetSystemJobRequestBus::Events::GetAssetJobsInfo, generatedFile, true);

                if (jobOutcome.IsSuccess())
                {
                    for (const auto& job : jobOutcome.GetValue())
                    {
                        // Generate status messages then trace and send them to the status bar as the messages change
                        const auto& statusMessage = AZStd::string ::format(
                            "Compiling %s (%s)", generatedFile.c_str(), AzToolsFramework::AssetSystem::JobStatusString(job.m_status));

                        if (m_lastAssetStatusMessage != statusMessage)
                        {
                            m_lastAssetStatusMessage = statusMessage;
                            AZ_TracePrintf_IfTrue("PassGraphCompiler", IsCompileLoggingEnabled(), "%s\n", statusMessage.c_str());

                            AtomToolsFramework::AtomToolsMainWindowRequestBus::Event(
                                m_toolId, &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::SetStatusMessage, statusMessage);
                        }

                        switch (job.m_status)
                        {
                        case AzToolsFramework::AssetSystem::JobStatus::Queued:
                        case AzToolsFramework::AssetSystem::JobStatus::InProgress:
                            // If any of the asset jobs are still processing then return early instead of allowing the completion
                            // notification to be sent.
                            return;
                        case AzToolsFramework::AssetSystem::JobStatus::Failed:
                        case AzToolsFramework::AssetSystem::JobStatus::Failed_InvalidSourceNameExceedsMaxLimit:
                            // If any of the asset jobs failed, cancel compilation. 
                            CompileGraphFailed();
                            return;
                        }
                    }
                }

                ++m_compileAssetIndex;
           }

            // All of the jobs completed without failure so send the notification that the generated assets have been compiled.
           CompileGraphCompleted();
        }
    }

    bool PassGraphCompiler::CompileGraph()
    {
        CompileGraphStarted();

        GraphModel::GraphPtr graph;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            graph, m_documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::GetGraph);

        AZStd::string graphName;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            graphName, m_documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::GetGraphName);

        // Skip compilation if there is no graph or this is a template.
        if (!graph || graphName.empty())
        {
            CompileGraphFailed();
            return false;
        }

        m_compileGraph = false;
        m_compileAssets = !m_generatedFiles.empty();
        m_compileAssetIndex = 0;
        return true;
    }

    void PassGraphCompiler::CompileGraphStarted()
    {
        m_compileGraph = false;
        m_compileAssets = false;
        m_compileAssetIndex = 0;
        m_generatedFiles.clear();

        const auto& statusMessage = AZStd::string::format("Compiling %s (Started)", GetGraphPath().c_str());
        AZ_TracePrintf_IfTrue("PassGraphCompiler", IsCompileLoggingEnabled(), "%s.\n", statusMessage.c_str());

        AtomToolsFramework::AtomToolsMainWindowRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::SetStatusMessage, statusMessage);

        PassGraphCompilerNotificationBus::Event(
            m_toolId, &PassGraphCompilerNotificationBus::Events::OnCompileGraphStarted, m_documentId);
    }

    void PassGraphCompiler::CompileGraphFailed()
    {
        m_compileGraph = false;
        m_compileAssets = false;
        m_compileAssetIndex = 0;
        m_generatedFiles.clear();

        const auto& statusMessage = AZStd::string::format("Compiling %s (Failed)", GetGraphPath().c_str());
        AZ_TracePrintf_IfTrue("PassGraphCompiler", IsCompileLoggingEnabled(), "%s.\n", statusMessage.c_str());

        AtomToolsFramework::AtomToolsMainWindowRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::SetStatusError, statusMessage);

        PassGraphCompilerNotificationBus::Event(
            m_toolId, &PassGraphCompilerNotificationBus::Events::OnCompileGraphFailed, m_documentId);
    }

    void PassGraphCompiler::CompileGraphCompleted()
    {
        m_compileGraph = false;
        m_compileAssets = false;
        m_compileAssetIndex = 0;

        const auto& statusMessage = AZStd::string::format("Compiling %s (Completed)", GetGraphPath().c_str());
        AZ_TracePrintf_IfTrue("PassGraphCompiler", IsCompileLoggingEnabled(), "%s.\n", statusMessage.c_str());

        AtomToolsFramework::AtomToolsMainWindowRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::SetStatusMessage, statusMessage);

        PassGraphCompilerNotificationBus::Event(
            m_toolId, &PassGraphCompilerNotificationBus::Events::OnCompileGraphCompleted, m_documentId);
    }

    AZStd::string PassGraphCompiler::GetGraphPath() const
    {
        AZStd::string absolutePath;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            absolutePath, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

        if (absolutePath.ends_with(".passgraph"))
        {
            return absolutePath;
        }

        return AZStd::string::format("%s/Assets/Passes/Generated/untitled.passgraph", AZ::Utils::GetProjectPath().c_str());
    }

    void PassGraphCompiler::QueueCompileGraph()
    {
        m_compileGraph = true;
    }

    bool PassGraphCompiler::IsCompileGraphQueued() const
    {
        return m_compileGraph;
    }
} // namespace PassCanvas
