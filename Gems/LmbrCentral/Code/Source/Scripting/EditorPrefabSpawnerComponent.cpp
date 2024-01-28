/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(CARBONATED)

#include "EditorPrefabSpawnerComponent.h"
#include "PrefabSpawnerComponent.h"
#include <QMessageBox>
#include <QApplication>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace LmbrCentral
{
    void EditorPrefabSpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorPrefabSpawnerComponent, AZ::Component>()
                ->Version(1)
                ->Field("Prefab", &EditorPrefabSpawnerComponent::m_prefabAsset)
                ->Field("SpawnOnActivate", &EditorPrefabSpawnerComponent::m_spawnOnActivate)
                ->Field("DestroyOnDeactivate", &EditorPrefabSpawnerComponent::m_destroyOnDeactivate)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorPrefabSpawnerComponent>("Prefab Spawner", "The Spawner component allows an entity to spawn a design-time or run-time dynamic prefab (*.spawnable) at the entity's location with an optional offset")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Spawner.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Spawner.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPrefabSpawnerComponent::m_prefabAsset, "Dynamic prefab", "The prefab to spawn")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorPrefabSpawnerComponent::PrefabAssetChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPrefabSpawnerComponent::m_spawnOnActivate, "Spawn on activate",
                        "Should the component spawn the selected prefab upon activation?")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorPrefabSpawnerComponent::SpawnOnActivateChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPrefabSpawnerComponent::m_destroyOnDeactivate, "Destroy on deactivate",
                        "Upon deactivation, should the component destroy any prefabs it spawned?")
                    ;
            }
        }
    }

    void EditorPrefabSpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        PrefabSpawnerComponent::GetProvidedServices(services);
    }

    void EditorPrefabSpawnerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        PrefabSpawnerComponent::GetRequiredServices(services);
    }

    void EditorPrefabSpawnerComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        PrefabSpawnerComponent::GetDependentServices(services);
    }

    bool EditorPrefabSpawnerComponent::HasInfiniteLoop()
    {
        // If we are set to spawn on activate, then we need to make sure we don't point to ourself or we create an infinite spawn loop
        AZStd::shared_ptr<AzFramework::SpawnableInstanceDescriptor> spawnableInfo =
            AzFramework::SpawnableEntitiesInterface::Get()->GetOwningSpawnable(GetEntityId());

        if (m_spawnOnActivate && spawnableInfo && spawnableInfo->IsValid())
        {
            // Compare the ids because one is source and the other is going to be the dynamic prefab
            return m_prefabAsset.GetId().m_guid == spawnableInfo->GetAssetId().m_guid;
        }

        return false;
    }


    AZ::u32 EditorPrefabSpawnerComponent::PrefabAssetChanged()
    {
        if (HasInfiniteLoop())
        {
            QMessageBox(QMessageBox::Warning,
                "Input Error",
                "Your spawner is set to Spawn on Activate.  You cannot set the spawner to spawn a dynamic prefab that contains this entity or it will spawn infinitely!",
                QMessageBox::Ok, QApplication::activeWindow()).exec();

            m_prefabAsset = AZ::Data::Asset<AzFramework::Spawnable>();

            // We have to refresh entire tree to update the asset control until the bug is fixed.  Just refreshing values does not properly update the UI.
            // Once LY-71192 (and the other variants) are fixed, this can be changed to ::ValuesOnly
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 EditorPrefabSpawnerComponent::SpawnOnActivateChanged()
    {
        if (HasInfiniteLoop())
        {
            QMessageBox(QMessageBox::Warning,
                "Input Error",
                "Your spawner is set to spawn a dynamic prefab that contains this entity.  You cannot set the spawner to be Spawn on Activate or it will spawn infinitely!",
                QMessageBox::Ok, QApplication::activeWindow()).exec();

            m_spawnOnActivate = false;

            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    bool EditorPrefabSpawnerComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const PrefabSpawnerConfig*>(baseConfig))
        {
            m_prefabAsset = config->m_prefabAsset;
            m_spawnOnActivate = config->m_spawnOnActivate;
            m_destroyOnDeactivate = config->m_destroyOnDeactivate;
            return true;
        }
        return false;
    }

    bool EditorPrefabSpawnerComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<PrefabSpawnerConfig*>(outBaseConfig))
        {
            config->m_prefabAsset = m_prefabAsset;
            config->m_spawnOnActivate = m_spawnOnActivate;
            config->m_destroyOnDeactivate = m_destroyOnDeactivate;
            return true;
        }
        return false;
    }

    void EditorPrefabSpawnerComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        // Add corresponding gameComponent to gameEntity.
        auto gameComponent = gameEntity->CreateComponent<PrefabSpawnerComponent>();

        PrefabSpawnerConfig config;
        config.m_prefabAsset = m_prefabAsset;
        config.m_spawnOnActivate = m_spawnOnActivate;
        config.m_destroyOnDeactivate = m_destroyOnDeactivate;

        gameComponent->SetConfiguration(config);
    }

} // namespace LmbrCentral

#endif
