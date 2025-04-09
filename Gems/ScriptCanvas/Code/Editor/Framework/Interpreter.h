/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/std/parallel/mutex.h>
#include <Builder/ScriptCanvasBuilderDataSystemBus.h>
#include <Editor/Framework/Configuration.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Execution/Executor.h>
#include <ScriptCanvas/Execution/ExecutionStateDeclarations.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasEditor
{
    using SourceHandle = ScriptCanvas::SourceHandle;

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(InterpreterStatus, AZ::u8,
        Waiting,            // no configuration
        Misconfigured,      // configuration error
        Incompatible,       // source is incompatible with with interpreter settings
        Configured,         // configuration is good
        Pending,            // waiting for asset readiness
        Ready,              // asset ready
        Running,            // running
        Stopped             // (manually) stopped
       );

    /// This class defines provides source and property configuration for ScriptCanvas graphs, and executes them
    /// as safely as possible. This can be used while the graph is being actively edited, whether in the O3DE provided editor or in
    /// another editor. When the graph properties are updated, the interpreter will always present and (attempt to) run the latest version.
    class Interpreter final
        : public ScriptCanvasBuilder::DataSystemAssetNotificationsBus::Handler
    {
        using Mutex = AZStd::recursive_mutex;
        using MutexLock = AZStd::lock_guard<Mutex>;

    public:
        AZ_TYPE_INFO(Interpreter, "{B77E5BC8-766A-4657-A30F-67797D04D10E}");
        AZ_CLASS_ALLOCATOR(Interpreter, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        Interpreter();

        ~Interpreter();

        AZ::EventHandler<const Interpreter&> ConnectOnStatusChanged(AZStd::function<void(const Interpreter&)>&& function) const;

        /// <summary
        /// Executes the selected Script if possible, and returns true if it did so. 
        /// </summary>
        /// <returns>true iff the script was Executable</returns>
        bool Execute();

        const Configuration& GetConfiguration() const;

        InterpreterStatus GetStatus() const;

        AZStd::string_view GetStatusString() const;

        bool IsExecutable() const;

        Configuration& ModConfiguration();

        /// <summary>
        /// Allows a manual refresh of the configuration to update Editor properties.
        /// </summary>
        void RefreshConfiguration();

        /// <summary>
        /// Sets the default user data in the Executable to a pointer to this Interpreter object.
        /// \note It will not be used until the next execution.
        /// </summary>
        void ResetUserData();

        void SetScript(SourceHandle source);

        /// <summary>
        /// Stops the execution of the script if it is executable and stoppable. If the script does not require being stopped, does nothing.
        /// </summary>
        void Stop();

        /// <summary>
        /// Sets the user data in the Executable to the input runtimeUserData.
        /// \note It will not be used until the next execution.
        /// </summary>
        void TakeUserData(ScriptCanvas::ExecutionUserData&& runtimeUserData);

    private:
        Mutex m_mutex;
        AZ::EventHandler<const Configuration&> m_handlerPropertiesChanged;
        AZ::EventHandler<const Configuration&> m_handlerSourceCompiled;
        AZ::EventHandler<const Configuration&> m_handlerSourceFailed;
        // #scriptcanvas_component_extension...
        AZ::EventHandler<const Configuration&> m_handlerUnacceptedComponentScript;

        mutable AZ::Event<const Interpreter&> m_onStatusChanged;

        bool m_runtimePropertiesDirty = true;
        InterpreterStatus m_status = InterpreterStatus::Waiting;
        Configuration m_configuration;
        ScriptCanvas::Executor m_executor;

        bool InitializeExecution(ScriptCanvas::RuntimeAssetPtr asset);

        void OnAssetNotReady() override;

        void OnPropertiesChanged();

        void OnReady(ScriptCanvas::RuntimeAssetPtr asset) override;

        void OnSourceCompiled();

        void OnSourceFailed();

        void OnSourceIncompatible();

        void SetSatus(InterpreterStatus status);
    };
}
