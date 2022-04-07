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

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasEditor
{
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(InterpreterStatus, AZ::u8,
        Waiting,            // no configuration
        Misconfigured,      // configuration error
        Configured,         // configuration is good
        Pending,            // waiting for asset readiness
        Ready,              // asset ready
        Running,            // running
        Stopped            // (manually) stopped
       );

    // This is an object that execute ScriptCanvas graphs from source and overrides
    // as safely as possible.
    class Interpreter final
        : public ScriptCanvasBuilder::DataSystemAssetNotificationsBus::Handler
    {
        using Mutex = AZStd::recursive_mutex;
        using MutexLock = AZStd::lock_guard<Mutex>;

    public:
        AZ_TYPE_INFO(Interpreter, "{B77E5BC8-766A-4657-A30F-67797D04D10E}");
        AZ_CLASS_ALLOCATOR(Interpreter, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        Interpreter();

        ~Interpreter();

        bool Execute();

        const Configuration& GetConfiguration() const;

        AZ::Event<const Interpreter&>& GetOnStatusChanged() const;

        InterpreterStatus GetStatus() const;

        AZStd::string_view GetStatusString() const;

        bool IsExecutable() const;

        void RefreshConfiguration();

        void ResetUserData();

        void SetUserData(AZStd::any&& runtimeUserData);

        void SetScript(SourceHandle source);

        void Stop();

    private:
        Mutex m_mutex;
        AZ::EventHandler<const Configuration&> m_handlerPropertiesChanged;
        AZ::EventHandler<const Configuration&> m_handlerSourceCompiled;
        AZ::EventHandler<const Configuration&> m_handlerSourceFailed;
        mutable AZ::Event<const Interpreter&> m_onStatusChanged;

        bool m_runtimePropertiesDirty = true;
        InterpreterStatus m_status = InterpreterStatus::Waiting;
        AZStd::any m_userData;
        ScriptCanvas::RuntimeDataOverrides m_runtimeDataOverrides;
        Configuration m_configuration;
        ScriptCanvas::Executor m_executor;

        void ConvertPropertiesToRuntime();

        bool InitializeExecution(ScriptCanvas::RuntimeAssetPtr asset);

        void OnAssetNotReady() override;

        void OnPropertiesChanged();

        void OnReady(ScriptCanvas::RuntimeAssetPtr asset) override;

        void OnSourceCompiled();

        void OnSourceFailed();

        void SetSatus(InterpreterStatus status);
    };
}
