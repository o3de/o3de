/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/mutex.h>
#include <Editor/Framework/Configuration.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Execution/Executor.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasEditor
{
    // This is an object that execute ScriptCanvas graphs from source and overrides
    // as safely as possible.
    class Interpreter final
    {
        using Mutex = AZStd::recursive_mutex;
        using MutexLock = AZStd::lock_guard<Mutex>;

    public:
        AZ_TYPE_INFO(Interpreter, "{B77E5BC8-766A-4657-A30F-67797D04D10E}");
        AZ_CLASS_ALLOCATOR(Interpreter, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        Interpreter();

        bool Execute();

        bool IsExecutable() const;

        void ResetUserData();

        void SetUserData(AZStd::any&& runtimeUserData);

        void SetScript(SourceHandle source);

        void Stop();

    private:
        Mutex m_mutex;

        AZ::EventHandler<const Configuration&> m_handlerSourceCompiled;
        AZ::EventHandler<const Configuration&> m_handlerSourceFailed;
        AZStd::any m_userData;
        ScriptCanvas::RuntimeDataOverrides m_runtimeDataOverrides;
        Configuration m_configuration;

        ScriptCanvas::Executor m_executor;

        void OnSourceCompiled();

        void OnSourceFailed();
    };
}
