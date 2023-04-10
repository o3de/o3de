/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/${Name}ComponentController.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

namespace ${Name}
{
    void ${Name}ComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${Name}ComponentConfig>()
                ;
        }
    }

    void ${Name}ComponentController::Reflect(AZ::ReflectContext* context)
    {
        ${Name}ComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${Name}ComponentController>()
                ->Version(0)
                ->Field("Configuration", &${Name}ComponentController::m_configuration);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<${Name}ComponentController>(
                    "${Name}ComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &${Name}ComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void ${Name}ComponentController::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("TransformService"));
    }

    void ${Name}ComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${Name}Service"));
    }

    void ${Name}ComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("${Name}Service"));
    }

    void ${Name}ComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    ${Name}ComponentController::${Name}ComponentController(const ${Name}ComponentConfig& config)
        : m_configuration(config)
    {
    }

    void ${Name}ComponentController::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);

        m_featureProcessor = AZ::RPI::Scene::GetFeatureProcessorForEntity<${Name}FeatureProcessorInterface>(entityId);
        AZ_Assert(m_featureProcessor, "${Name}ComponentController was unable to find a ${Name}FeatureProcessor on the EntityContext provided.");

    }

    void ${Name}ComponentController::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect();
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
