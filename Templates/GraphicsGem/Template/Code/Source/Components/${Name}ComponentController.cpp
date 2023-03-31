/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/${Name}ComponentController.h>
#include <Components/${Name}ComponentConstants.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Scene.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ::Render
{
    void ${Name}ComponentConfig::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<${Name}ComponentConfig>()
                ;
        }
    }

    void ${Name}ComponentController::Reflect(ReflectContext* context)
    {
        ${Name}ComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<${Name}ComponentController>()
                ->Version(0)
                ->Field("Configuration", &${Name}ComponentController::m_configuration);
        }
    }

    void ${Name}ComponentController::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void ${Name}ComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("${Name}Service", 0x63d32042));
    }

    void ${Name}ComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("${Name}Service", 0x63d32042));
    }

    void ${Name}ComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    ${Name}ComponentController::${Name}ComponentController(const ${Name}ComponentConfig& config)
        : m_configuration(config)
    {
    }

    void ${Name}ComponentController::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;

        TransformNotificationBus::Handler::BusConnect(m_entityId);

        m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<${Name}FeatureProcessorInterface>(entityId);
        AZ_Assert(m_featureProcessor, "${Name}ComponentController was unable to find a ${Name}FeatureProcessor on the EntityContext provided.");

    }

    void ${Name}ComponentController::Deactivate()
    {
        TransformNotificationBus::Handler::BusDisconnect();
    }

    void ${Name}ComponentController::SetConfiguration(const ${Name}ComponentConfig& config)
    {
        m_configuration = config;
    }

    const ${Name}ComponentConfig& ${Name}ComponentController::GetConfiguration() const
    {
        return m_configuration;
    }

    void ${Name}ComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        if (!m_featureProcessor)
        {
            return;
        }
    }
}
