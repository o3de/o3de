/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>

namespace RuntimeComponentCpp
{
    enum class RuntimeComponentVersion : unsigned int
    {
        DoNotVersionRuntimeComponentBumpTheEditorComponentInstead = 11,
    };
}

namespace ScriptCanvas
{
    RuntimeComponentUserData::RuntimeComponentUserData(RuntimeComponent& component, const AZ::EntityId& entity)
        : component(component)
        , entity(entity)
    {}

    void RuntimeComponent::Activate()
    {
        const auto entityId = GetEntityId();
        AZ::EntityBus::Handler::BusConnect(entityId);
        m_executor.TakeUserData(ExecutionUserData(RuntimeComponentUserData(*this, entityId)));
        m_executor.Initialize();
    }

    void RuntimeComponent::OnEntityActivated(const AZ::EntityId&)
    {
        m_executor.Execute();
    }

    void RuntimeComponent::OnEntityDeactivated(const AZ::EntityId&)
    {
        m_executor.StopAndClearExecutable();
    }

    void RuntimeComponent::Reflect(AZ::ReflectContext* context)
    {
        using namespace RuntimeComponentCpp;

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RuntimeComponent, AZ::Component>()
                ->Version
                    ( static_cast<unsigned int>(RuntimeComponentVersion::DoNotVersionRuntimeComponentBumpTheEditorComponentInstead))
                ->Field("executor", &RuntimeComponent::m_executor)
                ;
        }
    }

    void RuntimeComponent::TakeRuntimeDataOverrides(RuntimeDataOverrides&& overrideData)
    {
        m_executor.TakeRuntimeDataOverrides(AZStd::move(overrideData));
    }

    const RuntimeDataOverrides& RuntimeComponent::GetRuntimeDataOverrides() const
    {
        return m_executor.GetRuntimeOverrides();
    }
}

