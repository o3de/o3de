/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Execution/RuntimeComponent.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>
#include <ScriptCanvas/Execution/ExecutionState.h>

#if !defined(_RELEASE)
#define SCRIPT_CANVAS_RUNTIME_ASSET_CHECK
#endif

AZ_DECLARE_BUDGET(ScriptCanvas);

namespace RuntimeComponentCpp
{
    enum class RuntimeComponentVersion : unsigned int
    {
        ForceAssetPreloads = 5,
        AddRuntimeDataOverrides,
        PrefabSupport,
        RemoveRuntimeAsset,

        // add description above
        Current,
    };
}

namespace ScriptCanvas
{
    GraphInfo CreateGraphInfo(ScriptCanvasId scriptCanvasId, const GraphIdentifier& graphIdentifier)
    {
        return GraphInfo(scriptCanvasId, graphIdentifier);
    }

    DatumValue CreateDatumValue([[maybe_unused]] ScriptCanvasId scriptCanvasId, const GraphVariable& variable)
    {
        return DatumValue::Create(GraphVariable((*variable.GetDatum()), variable.GetVariableId()));
    }

    void RuntimeComponent::Activate()
    {
        InitializeExecution();
    }

    ActivationInfo RuntimeComponent::CreateActivationInfo() const
    {
        return ActivationInfo(GraphInfo(GetEntityId(), GetGraphIdentifier()));
    }

    void RuntimeComponent::Execute()
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "RuntimeComponent::Execute (%s)", m_runtimeOverrides.m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        AZ_Assert(m_executionState, "RuntimeComponent::Execute called without an execution state");
        SC_EXECUTION_TRACE_GRAPH_ACTIVATED(CreateActivationInfo());
        SCRIPT_CANVAS_PERFORMANCE_SCOPE_EXECUTION(m_executionState->GetScriptCanvasId(), m_runtimeOverrides.m_runtimeAsset.GetId());
        m_executionState->Execute();
    }

    const RuntimeData& RuntimeComponent::GetRuntimeAssetData() const
    {
        return m_runtimeOverrides.m_runtimeAsset->GetData();
    }

    ExecutionMode RuntimeComponent::GetExecutionMode() const
    {
        return m_executionState ? m_executionState->GetExecutionMode() : ExecutionMode::Interpreted;
    }

    GraphIdentifier RuntimeComponent::GetGraphIdentifier() const
    {
        return GraphIdentifier(AZ::Data::AssetId(m_runtimeOverrides.m_runtimeAsset.GetId().m_guid, 0), 0);
    }

    AZ::EntityId RuntimeComponent::GetScriptCanvasId() const
    {
        return m_scriptCanvasId;
    }

    const RuntimeDataOverrides& RuntimeComponent::GetRuntimeDataOverrides() const
    {
        return m_runtimeOverrides;
    }

    void RuntimeComponent::TakeRuntimeDataOverrides(RuntimeDataOverrides&& overrideData)
    {
        m_runtimeOverrides = AZStd::move(overrideData);
        m_runtimeOverrides.EnforcePreloadBehavior();
    }

    void RuntimeComponent::Init()
    {
        m_scriptCanvasId = AZ::Entity::MakeId();
        AZ_Assert(RuntimeDataOverrides::IsPreloadBehaviorEnforced(m_runtimeOverrides), "RuntimeComponent::m_runtimeAsset Auto load behavior MUST be set to AZ::Data::AssetLoadBehavior::PreLoad");
    }

    void RuntimeComponent::InitializeExecution()
    {
#if defined(SCRIPT_CANVAS_RUNTIME_ASSET_CHECK)
        if (!m_runtimeOverrides.m_runtimeAsset.Get())
        {
            AZ_Error("ScriptCanvas", false, "RuntimeComponent::m_runtimeAsset AssetId: %s was valid, but the data was not pre-loaded, so this script will not run", m_runtimeOverrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data());
            return;
        }
#else
        AZ_Assert(m_runtimeOverrides.m_runtimeAsset.Get(), "RuntimeComponent::m_runtimeAsset AssetId: %s was valid, but the data was not pre-loaded, so this script will not run", m_runtimeOverrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data());
#endif

        AZ_PROFILE_SCOPE(ScriptCanvas, "RuntimeComponent::InitializeExecution (%s)", m_runtimeOverrides.m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        SCRIPT_CANVAS_PERFORMANCE_SCOPE_INITIALIZATION(m_scriptCanvasId, m_runtimeOverrides.m_runtimeAsset.GetId());
        m_executionState = ExecutionState::Create(ExecutionStateConfig(m_runtimeOverrides.m_runtimeAsset, *this));

#if defined(SCRIPT_CANVAS_RUNTIME_ASSET_CHECK)
        if (!m_executionState)
        {
            AZ_Error("ScriptCanvas", false, "RuntimeComponent::m_runtimeAsset AssetId: %s failed to create an execution state, possibly due to missing dependent asset, script will not run", m_runtimeOverrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data());
            return;
        }
#else
        AZ_Assert(m_executionState, "RuntimeComponent::m_runtimeAsset AssetId: %s failed to create an execution state, possibly due to missing dependent asset, script will not run", m_runtimeOverrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data());
#endif

        AZ::EntityBus::Handler::BusConnect(GetEntityId());
        m_executionState->Initialize();
    }

    void RuntimeComponent::OnEntityActivated(const AZ::EntityId&)
    {
        Execute();
    }

    void RuntimeComponent::OnEntityDeactivated(const AZ::EntityId&)
    {
        StopExecution();
    }

    void RuntimeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RuntimeComponent, AZ::Component>()
                ->Version(static_cast<unsigned int>(RuntimeComponentCpp::RuntimeComponentVersion::Current), &RuntimeComponent::VersionConverter)
                ->Field("runtimeOverrides", &RuntimeComponent::m_runtimeOverrides)
                ;
        }
    }

    void RuntimeComponent::StopExecution()
    {
        if (m_executionState)
        {
            m_executionState->StopExecution();
            SCRIPT_CANVAS_PERFORMANCE_FINALIZE_TIMER(GetScriptCanvasId(), m_runtimeOverrides.m_runtimeAsset.GetId());
            SC_EXECUTION_TRACE_GRAPH_DEACTIVATED(CreateActivationInfo());
        }
    }

    bool RuntimeComponent::VersionConverter(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 3)
        {
            classElement.RemoveElementByName(AZ_CRC("m_uniqueId", 0x52157a7a));
        }

        return true;
    }
}

#undef SCRIPT_CANVAS_RUNTIME_ASSET_CHECK
