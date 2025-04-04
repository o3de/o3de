/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Execution/ExecutionStateHandler.h>

namespace ScriptCanvas
{
    ExecutionStateHandler::~ExecutionStateHandler()
    {
        if (IsExecutable())
        {
            StopAndClearExecutable();
        }
    }

    void ExecutionStateHandler::Execute()
    {
#if defined(SC_RUNTIME_CHECKS_ENABLED)
        if (!m_executionState)
        {
            AZ_Error("ScriptCanvas", false, "ExecutionStateHandler::Execute called without an execution state");
            return;
        }
#else
        AZ_Assert(m_executionState, "ExecutionStateHandler::Execute called without an execution state");
#endif // defined(SC_RUNTIME_CHECKS_ENABLED)
        AZ_PROFILE_SCOPE(ScriptCanvas, "ExecutionStateHandler::Execute (%s)"
            , m_executionState->GetRuntimeDataOverrides().m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        ScriptCanvas::ExecutionNotificationsBus::Broadcast(
            &ScriptCanvas::ExecutionNotifications::GraphActivated, ActivationInfo(GraphInfo(m_executionState)));
        SCRIPT_CANVAS_PERFORMANCE_SCOPE_EXECUTION(m_executionState);
        m_executionState->Execute();
    }

    ExecutionMode ExecutionStateHandler::GetExecutionMode() const
    {
        return m_executionState ? m_executionState->GetExecutionMode() : ExecutionMode::COUNT;
    }

    void ExecutionStateHandler::Initialize(const RuntimeDataOverrides& overrides, ExecutionUserData&& userData)
    {
#if defined(SC_RUNTIME_CHECKS_ENABLED)
        if (auto isPreloaded = IsPreloaded(overrides); isPreloaded != IsPreloadedResult::Yes)
        {
            AZ_Error("ScriptCanvas", false
                , "ExecutionStateHandler::Initialize runtime asset %s-%s loading problem: %s"
                , overrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data()
                , overrides.m_runtimeAsset.GetHint().c_str()
                , ToString(isPreloaded));
            return;
        }
        else if (!overrides.m_runtimeAsset.Get()->m_runtimeData.m_createExecution)
        {
            AZ_Error("ScriptCanvas", false
                , "ExecutionStateHandler::Initialize runtime create execution function not set %s-%s loading problem"
                , overrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data()
                , overrides.m_runtimeAsset.GetHint().c_str());
            return;
        };
#else
        AZ_Assert(IsPreloaded(overrides) == IsPreloadedResult::Yes
            , "ExecutionStateHandler::Intialize runtime asset %s-%s loading problem: %s"
            , overrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data()
            , overrides.m_runtimeAsset.GetHint().c_str()
            , ToString(IsPreloaded(overrides)));

        AZ_Assert(overrides.m_runtimeAsset.Get()->m_runtimeData.m_createExecution
            , "ExecutionStateHandler::Initialize runtime create execution function not set %s-%s loading problem"
            , overrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data()
            , overrides.m_runtimeAsset.GetHint().c_str());
#endif // defined(SC_RUNTIME_CHECKS_ENABLED)

        AZ_PROFILE_SCOPE(ScriptCanvas, "ExecutionStateHandler::Initialize (%s)", overrides.m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());

        ExecutionStateConfig config(overrides, AZStd::move(userData));
        m_executionState = overrides.m_runtimeAsset.Get()->m_runtimeData.m_createExecution(m_executionStateStorage, config);

#if defined(SC_RUNTIME_CHECKS_ENABLED)
        if (!m_executionState)
        {
            AZ_Error("ScriptCanvas", false, "ExecutionStateHandler::m_runtimeAsset AssetId: %s failed to create an execution state, possibly due to missing dependent asset, script will not run"
                , overrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data());
            return;
        }
#else
        AZ_Assert(m_executionState, "ExecutionStateHandler::m_runtimeAsset AssetId: %s failed to create an execution state, possibly due to missing dependent asset, script will not run"
            , overrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data());
#endif // defined(SC_RUNTIME_CHECKS_ENABLED)

        SCRIPT_CANVAS_PERFORMANCE_SCOPE_INITIALIZATION(m_executionState);
        m_executionState->Initialize();
    }

    void ExecutionStateHandler::InitializeAndExecute(const RuntimeDataOverrides& overrides, ExecutionUserData&& userData)
    {
        Initialize(overrides, AZStd::move(userData));
        Execute();
    }

    bool ExecutionStateHandler::IsExecutable() const
    {
        return m_executionState != nullptr;
    }

    bool ExecutionStateHandler::IsPure() const
    {
        return m_executionState != nullptr && m_executionState->IsPure();
    }

    void ExecutionStateHandler::StopAndClearExecutable()
    {
        if (m_executionState)
        {
            m_executionState->StopExecution();
            SCRIPT_CANVAS_PERFORMANCE_FINALIZE_TIMER(m_executionState);
            ScriptCanvas::ExecutionNotificationsBus::Broadcast(
                &ScriptCanvas::ExecutionNotifications::GraphDeactivated, GraphDeactivation(GraphInfo(m_executionState)));
            Execution::Destruct(m_executionStateStorage);
            m_executionState = nullptr;
        }
    }
    void ExecutionStateHandler::StopAndKeepExecutable()
    {
        if (m_executionState)
        {
            m_executionState->StopExecution();
            SCRIPT_CANVAS_PERFORMANCE_FINALIZE_TIMER(m_executionState);
            ScriptCanvas::ExecutionNotificationsBus::Broadcast(
                &ScriptCanvas::ExecutionNotifications::GraphDeactivated, GraphDeactivation(GraphInfo(m_executionState)));
        }
    }
}

