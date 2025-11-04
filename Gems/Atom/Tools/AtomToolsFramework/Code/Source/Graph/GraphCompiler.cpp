/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/AssetStatusReporterSystemRequestBus.h>
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
        // Stop monitoring assets from prior requests since the graph compiler is being destroyed.
        AssetStatusReporterSystemRequestBus::Event(
            m_toolId, &AssetStatusReporterSystemRequestBus::Events::StopReporting, m_assetReportRequestId);
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

        AssetStatusReporterSystemRequestBus::Event(
            m_toolId, &AssetStatusReporterSystemRequestBus::Events::StopReporting, m_assetReportRequestId);

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
        m_generatedFiles.clear();

        // Skip compilation if there is no graph or this is a template.
        if (!m_graph || m_graphName.empty() || GetGraphPath().empty())
        {
            SetState(State::Failed);
            return false;
        }

        SetState(State::Compiling);
        return true;
    }

    bool GraphCompiler::ReportGeneratedFileStatus()
    {
        SetState(State::Processing);

        // Start monitoring and reporting AP status for any files generated during this compile.
        if (!m_generatedFiles.empty())
        {
            // Begin requesting status from the asset reporting system, which manages a queue of requests from multiple graphs.
            AssetStatusReporterSystemRequestBus::Event(
                m_toolId, &AssetStatusReporterSystemRequestBus::Events::StartReporting, m_assetReportRequestId, m_generatedFiles);

            while (m_state == State::Processing)
            {
                AssetStatusReporterState status = AssetStatusReporterState::Failed;
                AssetStatusReporterSystemRequestBus::EventResult(
                    status, m_toolId, &AssetStatusReporterSystemRequestBus::Events::GetStatus, m_assetReportRequestId);

                if (status != AssetStatusReporterState::Processing)
                {
                    AssetStatusReporterSystemRequestBus::Event(
                        m_toolId, &AssetStatusReporterSystemRequestBus::Events::StopReporting, m_assetReportRequestId);
                    return status == AssetStatusReporterState::Succeeded;
                }

                // Sleep to give other possible threats time to make AssetStatusReporterSystemRequestBus requests
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
            }

            AssetStatusReporterSystemRequestBus::Event(
                m_toolId, &AssetStatusReporterSystemRequestBus::Events::StopReporting, m_assetReportRequestId);
        }

        return true;
    }

    void GraphCompiler::ReportStatus(const AZStd::string& statusMessage)
    {
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
