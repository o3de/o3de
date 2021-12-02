/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorRandomTimedSpawnerComponent.h"
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void EditorRandomTimedSpawnerConfiguration::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorRandomTimedSpawnerConfiguration, RandomTimedSpawnerConfiguration>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<RandomTimedSpawnerConfiguration>("RandomTimedSpawner Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RandomTimedSpawnerConfiguration::m_enabled, "Enabled", "")

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &RandomTimedSpawnerConfiguration::m_randomDistribution, "Random Distribution", "")
                    ->EnumAttribute(AZ::RandomDistributionType::Normal, "Normal")
                    ->EnumAttribute(AZ::RandomDistributionType::UniformReal, "Uniform Real")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Timing")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RandomTimedSpawnerConfiguration::m_spawnDelay, "Spawn Delay", "Time in seconds it takes to spawn")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RandomTimedSpawnerConfiguration::m_spawnDelayVariation, "Spawn Delay Variation", "Variation applied to the spawn delay")
                    ;
            }
        }
    }

    void EditorRandomTimedSpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorRandomTimedSpawnerComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("m_config", &EditorRandomTimedSpawnerComponent::m_config)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorRandomTimedSpawnerComponent>("Random Timed Spawner", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/RandomTimedSpawner.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/RandomTimedSpawner.svg")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/random-timed-spawner-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorRandomTimedSpawnerComponent::m_config, "m_config", "No Description")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }

        EditorRandomTimedSpawnerConfiguration::Reflect(context);
    }

    void EditorRandomTimedSpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("RandomTimedSpawnerService", 0x56f2fa36));
    }
    void EditorRandomTimedSpawnerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        //Only compatible with Box and Cylinder shapes 
        incompatible.push_back(AZ_CRC("CapsuleShapeService", 0x9bc1122c));
        incompatible.push_back(AZ_CRC("SphereShapeService", 0x90c8dc80));
        incompatible.push_back(AZ_CRC("CompoundShapeService", 0x4f7c640a));
        incompatible.push_back(AZ_CRC("TubeShapeService", 0x3fe791b4));
        incompatible.push_back(AZ_CRC("PrismShapeService", 0x8dbfb417));
        incompatible.push_back(AZ_CRC("PolygonPrismShapeService", 0x1cbc4ed4));
    }
    void EditorRandomTimedSpawnerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        required.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        required.push_back(AZ_CRC("SpawnerService", 0xd2f1d7a3));
    }

    void EditorRandomTimedSpawnerComponent::Activate()
    {
        RandomTimedSpawnerComponentRequestBus::Handler::BusConnect(GetEntityId());
    }
    
    void EditorRandomTimedSpawnerComponent::Deactivate()
    {
        RandomTimedSpawnerComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void EditorRandomTimedSpawnerComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RandomTimedSpawnerComponent>(&m_config);
    }

} //namespace LmbrCentral
