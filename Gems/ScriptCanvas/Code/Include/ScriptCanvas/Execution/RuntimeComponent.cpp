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
    RuntimeComponentUserData::RuntimeComponentUserData(RuntimeComponent& component, const AZ::EntityId& entity)
        : component(component)
        , entity(entity)
    {}

    void RuntimeComponent::Activate()
    {
        const auto entityId = GetEntityId();
        AZ::EntityBus::Handler::BusConnect(entityId);
        m_executor.Initialize(m_runtimeOverrides, AZStd::any(RuntimeComponentUserData(*this, entityId)));
    }

    void RuntimeComponent::OnEntityActivated(const AZ::EntityId&)
    {
        m_executor.Execute();
    }

    void RuntimeComponent::OnEntityDeactivated(const AZ::EntityId&)
    {
        m_executor.Stop();
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

    void RuntimeComponent::TakeRuntimeDataOverrides(RuntimeDataOverrides&& overrideData)
    {
        m_runtimeOverrides = AZStd::move(overrideData);
        m_runtimeOverrides.EnforcePreloadBehavior();
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

