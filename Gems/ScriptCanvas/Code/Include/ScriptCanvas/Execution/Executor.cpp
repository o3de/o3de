/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <ScriptCanvas/Execution/Executor.h>

namespace ScriptCanvas
{
    void Executor::Execute()
    {
        m_execution.Execute();
    }

    void Executor::Initialize()
    {
        m_execution.Initialize(m_runtimeOverrides, ExecutionUserData(m_userData));
    }

    void Executor::InitializeAndExecute()
    {
        Initialize();
        Execute();
    }

    bool Executor::IsExecutable() const
    {
        return m_execution.IsExecutable();
    }

    bool Executor::IsPure() const
    {
        return m_execution.IsPure();
    }

    void Executor::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Executor>()
                ->Version(static_cast<unsigned int>(Version::DoNotVersionThisClassButMakeHostSystemsRebuildIt))
                ->Field("runtimeOverrides", &Executor::m_runtimeOverrides)
                ;
        }
    }

    void Executor::SetRuntimeOverrides(const RuntimeDataOverrides& overrideData)
    {
        m_runtimeOverrides = overrideData;
        m_runtimeOverrides.EnforcePreloadBehavior();
    }

    void Executor::SetUserData(const ExecutionUserData& userData)
    {
        m_userData = userData;
    }

    void Executor::StopAndClearExecutable()
    {
        m_execution.StopAndClearExecutable();
    }

    void Executor::StopAndKeepExecutable()
    {
        m_execution.StopAndKeepExecutable();
    }

    void Executor::TakeRuntimeDataOverrides(RuntimeDataOverrides&& overrideData)
    {
        m_runtimeOverrides = overrideData;
        m_runtimeOverrides.EnforcePreloadBehavior();
    }

    void Executor::TakeUserData(ExecutionUserData&& userData)
    {
        m_userData = userData;
    }
}

