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
    //! Manages the transformation of a graph into context-specific data and assets.
    //! Derived classes override CompileGraph to generate their data.
    class GraphCompiler
    {
    public:
        AZ_RTTI(GraphCompiler, "{D79FA3C7-BF5D-4A23-A3AB-1D6733B0C619}");
        AZ_CLASS_ALLOCATOR(GraphCompiler, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(GraphCompiler);

        static void Reflect(AZ::ReflectContext* context);

        GraphCompiler() = default;
        GraphCompiler(const AZ::Crc32& toolId);
        virtual ~GraphCompiler() = default;

        //! Returns the value of a registry setting that enables or disables verbose logging for the compilation process.
        static bool IsCompileLoggingEnabled();

        //! Values representing the state of compiler as it processes the graph data
        enum class State : uint32_t
        {
            Idle = 0,
            Compiling,
            Processing,
            //! Graph-to-source generation completed successfully.
            Complete,
            Canceled,
            Failed
        };

        //! Reserves an idle compiler, or requests cancellation if a compile is already active.
        virtual bool Reset();

        //! Requests cancellation without reserving an idle compiler.
        void RequestCancel();

        //! Assign a callback invoked after each state change.
        using StateChangeHandler = AZStd::function<void(const GraphCompiler*)>;
        virtual void SetStateChangeHandler(StateChangeHandler handler);

        //! Assign the current graph compiler state.
        virtual void SetState(State state);

        //! Get the current graph compiler state.
        virtual State GetState() const;

        //! Returns the path passed to CompileGraph unless overridden. Generated files
        //! will be saved to the same folder as this path.
        virtual AZStd::string GetGraphPath() const;

        //! Returns a list of all the files generated during the last compile.
        virtual const AZStd::vector<AZStd::string>& GetGeneratedFilePaths() const;

        //! Returns the generated files modified during the last compile.
        virtual const AZStd::vector<AZStd::string>& GetModifiedGeneratedFilePaths() const;

        //! Returns true if the compiler can accept a new graph.
        virtual bool CanCompileGraph() const;

        //! Initiates and executes the graph compile, changing states accordingly.
        virtual bool CompileGraph(GraphModel::GraphPtr graph, const AZStd::string& graphName, const AZStd::string& graphPath);

    protected:
        // Helper function to log and report status messages.
        void ReportStatus(const AZStd::string& statusMessage);

        //! Returns true after another graph edit has requested that the active compilation stop.
        bool IsCancelRequested() const;

        //! Publishes one terminal state and releases the compiler reservation. Cancellation takes precedence over finalState.
        bool FinishCompile(State finalState);

        void PublishState(State state);

        const AZ::Crc32 m_toolId = {};

        // The source graph that is being compiled and transformed into generated files
        GraphModel::GraphPtr m_graph;

        // The unique name of the graph
        AZStd::string m_graphName;

        // Target path where generated files will be saved
        AZStd::string m_graphPath;

        // Container of file paths that were affected by the compiler
        AZStd::vector<AZStd::string> m_generatedFiles;

        // Generated files modified during this compile
        AZStd::vector<AZStd::string> m_modifiedGeneratedFiles;

        // Stores the last reported status message that it is not sent repeatedly
        AZStd::mutex m_lastStatusMessageMutex;
        AZStd::string m_lastStatusMessage;

        // Current state of the graph compiler
        AZStd::atomic<State> m_state = State::Idle;

        // Serializes reservation, cancellation, and release.
        mutable AZStd::mutex m_compileLifecycleMutex;
        // Serializes state notifications while allowing handlers to publish a subsequent state.
        mutable AZStd::recursive_mutex m_statePublicationMutex;
        AZStd::atomic_bool m_compileInProgress = false;
        AZStd::atomic_bool m_cancelRequested = false;
        bool m_compileReserved = false;

        // Optional function for handling state changes
        StateChangeHandler m_stateChangeHandler;
    };
} // namespace AtomToolsFramework
