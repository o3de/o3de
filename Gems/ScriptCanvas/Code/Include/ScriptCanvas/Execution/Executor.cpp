/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Execution/Executor.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>
#include <ScriptCanvas/Execution/ExecutionState.h>

namespace ScriptCanvas
{
    Executor::~Executor()
    {
        Stop();
    }

    ActivationInfo Executor::CreateActivationInfo() const
    {
        return ActivationInfo(GraphInfo(m_executionState.get()));
    }

    void Executor::Execute()
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "Executor::Execute (%s)"
            , m_overrides->m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        AZ_Assert(m_executionState, "Executor::Execute called without an execution state");
        SC_EXECUTION_TRACE_GRAPH_ACTIVATED(CreateActivationInfo());
        SCRIPT_CANVAS_PERFORMANCE_SCOPE_EXECUTION(m_executionState.get());
        m_executionState->Execute();
    }

    const RuntimeData& Executor::GetRuntimeAssetData() const
    {
        return m_overrides->m_runtimeAsset->m_runtimeData;
    }

    ExecutionMode Executor::GetExecutionMode() const
    {
        return m_executionState ? m_executionState->GetExecutionMode() : ExecutionMode::Interpreted;
    }

    const RuntimeDataOverrides& Executor::GetRuntimeDataOverrides() const
    {
        return *m_overrides;
    }

    void Executor::Initialize(const RuntimeDataOverrides& overrides, AZStd::any&& userData)
    {
#if defined(SCRIPT_CANVAS_RUNTIME_ASSET_CHECK)
        if (!m_overrides->m_runtimeAsset.Get())
        {
            AZ_Error("ScriptCanvas", false
                , "Executor::m_runtimeAsset AssetId: %s was valid, but the data was not pre-loaded, so this script will not run"
                , m_overrides->m_runtimeAsset.GetId().ToString<AZStd::string>().data());
            return;
        }
#else
        AZ_Assert(m_overrides->m_runtimeAsset.Get()
            , "Executor::m_runtimeAsset AssetId: %s was valid, but the data was not pre-loaded, so this script will not run"
            , m_overrides->m_runtimeAsset.GetId().ToString<AZStd::string>().data());
#endif

        AZ_PROFILE_SCOPE(ScriptCanvas, "Executor::Initialize (%s)"
            , m_overrides->m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());

        ExecutionStateConfig config(*m_overrides, AZStd::move(userData));
        m_executionState = ExecutionState::Create(config);

#if defined(SCRIPT_CANVAS_RUNTIME_ASSET_CHECK)
        if (!m_executionState)
        {
            AZ_Error("ScriptCanvas", false, "Executor::m_runtimeAsset AssetId: %s failed to create an execution state, possibly due to missing dependent asset, script will not run"
                , m_overrides->m_runtimeAsset.GetId().ToString<AZStd::string>().data());
            return;
        }
#else
        AZ_Assert(m_executionState, "Executor::m_runtimeAsset AssetId: %s failed to create an execution state, possibly due to missing dependent asset, script will not run"
            , m_overrides->m_runtimeAsset.GetId().ToString<AZStd::string>().data());
#endif

        SCRIPT_CANVAS_PERFORMANCE_SCOPE_INITIALIZATION(m_executionState.get());
        m_executionState->Initialize();
    }

    void Executor::InitializeAndExecute(const RuntimeDataOverrides& overrides, AZStd::any&& userData)
    {
        Initialize(overrides, AZStd::move(userData));
        Execute();
    }

    void Executor::Stop()
    {
        if (m_executionState)
        {
            m_executionState->StopExecution();
            SCRIPT_CANVAS_PERFORMANCE_FINALIZE_TIMER(m_executionState.get());
            SC_EXECUTION_TRACE_GRAPH_DEACTIVATED(CreateActivationInfo());
        }
    }
}

