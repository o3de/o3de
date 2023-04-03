/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/GraphicsGem_AR_TestComponentController.h>

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

namespace GraphicsGem_AR_Test
{
    void GraphicsGem_AR_TestComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphicsGem_AR_TestComponentConfig>()
                ;
        }
    }

    void GraphicsGem_AR_TestComponentController::Reflect(AZ::ReflectContext* context)
    {
        GraphicsGem_AR_TestComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphicsGem_AR_TestComponentController>()
                ->Version(0)
                ->Field("Configuration", &GraphicsGem_AR_TestComponentController::m_configuration);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GraphicsGem_AR_TestComponentController>(
                    "GraphicsGem_AR_TestComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphicsGem_AR_TestComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void GraphicsGem_AR_TestComponentController::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("TransformService"));
    }

    void GraphicsGem_AR_TestComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GraphicsGem_AR_TestService"));
    }

    void GraphicsGem_AR_TestComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GraphicsGem_AR_TestService"));
    }

    void GraphicsGem_AR_TestComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    GraphicsGem_AR_TestComponentController::GraphicsGem_AR_TestComponentController(const GraphicsGem_AR_TestComponentConfig& config)
        : m_configuration(config)
    {
    }

    void GraphicsGem_AR_TestComponentController::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);

        m_featureProcessor = AZ::RPI::Scene::GetFeatureProcessorForEntity<GraphicsGem_AR_TestFeatureProcessorInterface>(entityId);
        AZ_Assert(m_featureProcessor, "GraphicsGem_AR_TestComponentController was unable to find a GraphicsGem_AR_TestFeatureProcessor on the EntityContext provided.");

    }

    void GraphicsGem_AR_TestComponentController::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void GraphicsGem_AR_TestComponentController::SetConfiguration(const GraphicsGem_AR_TestComponentConfig& config)
    {
        m_configuration = config;
    }

    const GraphicsGem_AR_TestComponentConfig& GraphicsGem_AR_TestComponentController::GetConfiguration() const
    {
        return m_configuration;
    }

    void GraphicsGem_AR_TestComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        if (!m_featureProcessor)
        {
            return;
        }
    }
}
