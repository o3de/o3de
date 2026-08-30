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
#include <AzCore/std/parallel/scoped_lock.h>
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
        bool stopAssetStatusReporting = false;
        {
            AZStd::scoped_lock lock(m_compileLifecycleMutex);
            if (m_compileInProgress)
            {
                m_cancelRequested = true;
                stopAssetStatusReporting = true;
            }
            else
            {
                switch (m_state.load())
                {
                case State::Idle:
                case State::Failed:
                case State::Complete:
                case State::Canceled:
                    // Reserve synchronously, before GraphDocument dispatches the background job. This closes the window where a second
                    // edit could observe the old terminal state and dispatch another job before the first worker had started.
                    m_compileInProgress = true;
                    m_compileReserved = true;
                    m_cancelRequested = false;
                    return true;
                default:
                    break;
                }
            }
        }

        if (stopAssetStatusReporting)
        {
            AssetStatusReporterSystemRequestBus::Event(
                m_toolId, &AssetStatusReporterSystemRequestBus::Events::StopReporting, m_assetReportRequestId);
        }

        return false;
    }

    void GraphCompiler::Cancel()
    {
        bool stopAssetStatusReporting = false;
        {
            AZStd::scoped_lock lock(m_compileLifecycleMutex);
            if (m_compileInProgress)
            {
                m_cancelRequested = true;
                stopAssetStatusReporting = true;
            }
        }

        if (stopAssetStatusReporting)
        {
            AssetStatusReporterSystemRequestBus::Event(
                m_toolId, &AssetStatusReporterSystemRequestBus::Events::StopReporting, m_assetReportRequestId);
        }
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

            // GraphDocument normally reserves the compiler with Reset before dispatching this worker. Retain support for direct callers
            // by creating and consuming a reservation here only when no job is already active.
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

        if (IsCancelRequested())
        {
            return FinishCompile(State::Canceled);
        }

        m_graph = graph;
        m_graphName = graphName;
        m_graphPath = graphPath;
        m_generatedFiles.clear();

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

    bool GraphCompiler::FinishCompile(State finalState, AZStd::function<void()> completionCallback)
    {
        AZStd::scoped_lock lock(m_compileLifecycleMutex);
        if (!m_compileInProgress)
        {
            return false;
        }

        const State publishedState = m_cancelRequested ? State::Canceled : finalState;
        if (publishedState == State::Complete && completionCallback)
        {
            completionCallback();
        }

        SetState(publishedState);
        m_compileReserved = false;
        m_compileInProgress = false;
        return publishedState == State::Complete;
    }

    bool GraphCompiler::ShouldReportGeneratedFileStatus(const AZStd::string& generatedFile) const
    {
        // Include files have no Asset Processor builder and therefore no jobs to wait for.
        return !generatedFile.ends_with(".azsli");
    }

    bool GraphCompiler::ReportGeneratedFileStatus()
    {
        if (IsCancelRequested())
        {
            return false;
        }

        SetState(State::Processing);

        // Only report on files that the Asset Processor will actually build. Include files like azsli have no builder and therefore no
        // jobs, so querying their status costs a full round trip to the AP that can never return anything but an empty job list. They are
        // deliberately left in m_generatedFiles, which other systems rely on in full, such as the viewport searching it for the generated
        // material to apply.
        AZStd::vector<AZStd::string> filesToReport;
        filesToReport.reserve(m_generatedFiles.size());
        for (const auto& generatedFile : m_generatedFiles)
        {
            if (ShouldReportGeneratedFileStatus(generatedFile))
            {
                filesToReport.push_back(generatedFile);
            }
        }

        // Start monitoring and reporting AP status for any files generated during this compile.
        if (!filesToReport.empty())
        {
            // Begin requesting status from the asset reporting system, which manages a queue of requests from multiple graphs.
            AssetStatusReporterSystemRequestBus::Event(
                m_toolId, &AssetStatusReporterSystemRequestBus::Events::StartReporting, m_assetReportRequestId, filesToReport);

            // Bound the wait. AssetStatusReporter walks its paths with an index that only ever moves forward, so a path it is sitting on
            // has to reach a terminal job state or it waits on that path forever. The Asset Processor can finish all of its work without
            // that happening: a structural change to a graph makes MaterialTypeBuilder delete and regenerate the intermediate shader
            // sources, which retriggers the very jobs being polled, and a duplicated intermediate entry in the Asset Processor database
            // (o3de/o3de#19642, visible in the AP log as "GetTopLevelSourceForProduct found multiple sources") leaves paths that never
            // settle cleanly.
            //
            // Giving up costs nothing. The compile is reported complete and the viewport re-applies the material when the assets actually
            // appear in the asset catalog, which it watches for exactly this reason. Waiting forever, by contrast, strands the preview
            // until something else happens to change the compiler's state.
            const AZ::u64 timeoutMs =
                GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/AssetStatusTimeoutMs", (AZ::u64)15000);
            const auto deadline = AZStd::chrono::steady_clock::now() + AZStd::chrono::milliseconds(timeoutMs);

            while (m_state == State::Processing && !IsCancelRequested())
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

                // A timeout of zero disables the bound, restoring the original behavior of waiting indefinitely.
                if (timeoutMs > 0 && AZStd::chrono::steady_clock::now() >= deadline)
                {
                    AZStd::string statusMessage;
                    AssetStatusReporterSystemRequestBus::EventResult(
                        statusMessage, m_toolId, &AssetStatusReporterSystemRequestBus::Events::GetStatusMessage,
                        m_assetReportRequestId);

                    AZ_Warning(
                        "GraphCompiler",
                        false,
                        "Gave up waiting for the Asset Processor after %llu ms while compiling '%s'. Still waiting on: %s. Reporting the "
                        "compile as complete; the viewport will pick up the generated assets when they reach the asset catalog. Check the "
                        "Asset Processor log if this happens repeatedly.",
                        timeoutMs,
                        GetGraphPath().c_str(),
                        statusMessage.empty() ? "(unknown)" : statusMessage.c_str());

                    AssetStatusReporterSystemRequestBus::Event(
                        m_toolId, &AssetStatusReporterSystemRequestBus::Events::StopReporting, m_assetReportRequestId);
                    return true;
                }

                // Sleep to give other possible threats time to make AssetStatusReporterSystemRequestBus requests
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
            }

            if (IsCancelRequested())
            {
                AssetStatusReporterSystemRequestBus::Event(
                    m_toolId, &AssetStatusReporterSystemRequestBus::Events::StopReporting, m_assetReportRequestId);
                return false;
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
