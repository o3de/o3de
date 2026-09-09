/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <GraphModel/Model/Graph.h>

namespace AtomToolsFramework
{
    //! GraphCompiler is a base class for setting up and managing the transformation of a graph model graph into
    //! context specific data and assets. Derived classes will override the CompileGgraph function to traverse the graph and generate their
    //! specific data.
    class GraphCompiler
    {
    public:
        AZ_RTTI(GraphCompiler, "{D79FA3C7-BF5D-4A23-A3AB-1D6733B0C619}");
        AZ_CLASS_ALLOCATOR(GraphCompiler, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(GraphCompiler);

        static void Reflect(AZ::ReflectContext* context);

        GraphCompiler() = default;
        GraphCompiler(const AZ::Crc32& toolId);
        virtual ~GraphCompiler();

        //! Returns the value of a registry setting that enables or disables verbose logging for the compilation process.
        static bool IsCompileLoggingEnabled();

        //! Values representing the state of compiler as it processes the graph data
        enum class State : uint32_t
        {
            Idle = 0,
            Compiling,
            Processing,
            Complete,
            Canceled,
            Failed
        };

        //! Reserves the compiler for a new job when idle. If a job is already active, requests cancellation and returns false so the
        //! caller can leave the replacement queued until the active job acknowledges cancellation and releases the compiler.
        virtual bool Reset();

        //! Requests cooperative cancellation of the active compilation without reserving a new one.
        virtual void Cancel();

        //! Assign the current graph compiler state.
        using StateChangeHandler = AZStd::function<void(const GraphCompiler*)>;
        virtual void SetStateChangeHandler(StateChangeHandler handler);

        //! Assign the current graph compiler state.
        virtual void SetState(State state);

        //! Get the current graph compiler state.
        virtual State GetState() const;

        //! Returns the path that was passed into the compiled graph function unless overridden to provided different value. Generated files
        //! will be saved to the same folder as this path.
        virtual AZStd::string GetGraphPath() const;

        //! Returns a list of all the files generated during the last compile.
        virtual const AZStd::vector<AZStd::string>& GetGeneratedFilePaths() const;

        //! Returns true if the graph is in a state that can be aborted or restarted to reinitiate a new compile.
        virtual bool CanCompileGraph() const;

        //! Dysfunction initiates and executes the graph compile, changing states accordingly.
        virtual bool CompileGraph(GraphModel::GraphPtr graph, const AZStd::string& graphName, const AZStd::string& graphPath);

        //! Records whether the compile about to run should produce the derived compiler's full production output in addition to whatever
        //! reduced output it maintains for its own preview. Set by the document immediately before the compile job is started, so it is
        //! stable for the duration of that compile. A compiler that draws no such distinction can ignore this entirely.
        void SetProductionOutputRequested(bool requested)
        {
            m_productionOutputRequested = requested;
        }

        //! See SetProductionOutputRequested. Read from the compile worker thread.
        bool IsProductionOutputRequested() const
        {
            return m_productionOutputRequested;
        }

        //! Returns true when this compiler keeps a production output distinct from the reduced one it maintains for a preview, and that
        //! production output is behind the graph as of the last compile. A compiler that produces a single output has nothing to
        //! publish and always reports false.
        virtual bool IsProductionOutputStale() const
        {
            return false;
        }

    protected:
        // Helper function to log and report status messages.
        void ReportStatus(const AZStd::string& statusMessage);

        // Requests and reports job status of generated files from the AP
        // Return true if generation and processing is complete. Otherwise, return falss
        bool ReportGeneratedFileStatus();

        //! Returns whether a generated source file should block graph completion while its Asset Processor jobs settle.
        //! Derived compilers can exclude files whose readiness is handled asynchronously by their consumers.
        virtual bool ShouldReportGeneratedFileStatus(const AZStd::string& generatedFile) const;

        //! Returns true after another graph edit has requested that the active compilation stop.
        bool IsCancelRequested() const;

        //! Publishes one terminal state and releases the compiler reservation. A cancellation request always wins over the requested
        //! state. The optional callback is invoked only for a successful completion while cancellation is excluded by the lifecycle lock.
        bool FinishCompile(State finalState, AZStd::function<void()> completionCallback = {});

        const AZ::Crc32 m_toolId = {};

        // The source graph that is being compiled and transformed into generated files
        GraphModel::GraphPtr m_graph;

        // The unique name of the graph
        AZStd::string m_graphName;

        // Target path where generated files will be saved
        AZStd::string m_graphPath;

        // Container of file paths that were affected by the compiler
        AZStd::vector<AZStd::string> m_generatedFiles;

        // Stores the last reported status message that it is not sent repeatedly
        AZStd::mutex m_lastStatusMessageMutex;
        AZStd::string m_lastStatusMessage;

        // Current state of the graph compiler
        AZStd::atomic<State> m_state = State::Idle;

        // Serializes compile reservation, cancellation, terminal publication, and release. The atomics are read by the compile worker at
        // cancellation checkpoints without taking this lock.
        mutable AZStd::mutex m_compileLifecycleMutex;
        AZStd::atomic_bool m_compileInProgress = false;
        AZStd::atomic_bool m_cancelRequested = false;

        // True when this compile was asked for the full production output. See SetProductionOutputRequested.
        AZStd::atomic_bool m_productionOutputRequested = false;
        bool m_compileReserved = false;

        // Optional function for handling state changes
        StateChangeHandler m_stateChangeHandler;

        // Asset status report request ID
        const AZ::Uuid m_assetReportRequestId = AZ::Uuid::CreateRandom();
    };
} // namespace AtomToolsFramework
