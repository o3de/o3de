/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorSpawnerComponent.h"
#include "SpawnerComponent.h"
#include <QMessageBox>
#include <QApplication>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Slice/SliceEntityBus.h>

namespace LmbrCentral
{
    void EditorSpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorSpawnerComponent, AZ::Component>()
                ->Version(1)
                ->Field("Slice", &EditorSpawnerComponent::m_sliceAsset)
                ->Field("SpawnOnActivate", &EditorSpawnerComponent::m_spawnOnActivate)
                ->Field("DestroyOnDeactivate", &EditorSpawnerComponent::m_destroyOnDeactivate)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorSpawnerComponent>("Spawner", "The Spawner component allows an entity to spawn a design-time or run-time dynamic slice (*.dynamicslice) at the entity's location with an optional offset")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Spawner.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Spawner.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSpawnerComponent::m_sliceAsset, "Dynamic slice", "The slice to spawn")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSpawnerComponent::SliceAssetChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSpawnerComponent::m_spawnOnActivate, "Spawn on activate",
                        "Should the component spawn the selected slice upon activation?")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSpawnerComponent::SpawnOnActivateChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSpawnerComponent::m_destroyOnDeactivate, "Destroy on deactivate",
                        "Upon deactivation, should the component destroy any slices it spawned?")
                    ;
            }
        }
    }

    void EditorSpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        SpawnerComponent::GetProvidedServices(services);
    }

    void EditorSpawnerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        SpawnerComponent::GetRequiredServices(services);
    }

    void EditorSpawnerComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        SpawnerComponent::GetDependentServices(services);
    }

    bool EditorSpawnerComponent::HasInfiniteLoop()
    {
        // If we are set to spawn on activate, then we need to make sure we don't point to ourself or we create an infinite spawn loop
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        AzFramework::SliceEntityRequestBus::EventResult(sliceInstanceAddress, GetEntityId(),
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);
        if (m_spawnOnActivate && sliceInstanceAddress.GetReference())
        {
            // Compare the ids because one is source and the other is going to be the dynamic slice
            return m_sliceAsset.GetId().m_guid == sliceInstanceAddress.GetReference()->GetSliceAsset().GetId().m_guid;
        }

        return false;
    }


    AZ::u32 EditorSpawnerComponent::SliceAssetChanged()
    {
        if (HasInfiniteLoop())
        {
            QMessageBox(QMessageBox::Warning,
                "Input Error",
                "Your spawner is set to Spawn on Activate.  You cannot set the spawner to spawn a dynamic slice that contains this entity or it will spawn infinitely!",
                QMessageBox::Ok, QApplication::activeWindow()).exec();

            m_sliceAsset = AZ::Data::Asset<AZ::DynamicSliceAsset>();

            // We have to refresh entire tree to update the asset control until the bug is fixed.  Just refreshing values does not properly update the UI.
            // Once LY-71192 (and the other variants) are fixed, this can be changed to ::ValuesOnly
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 EditorSpawnerComponent::SpawnOnActivateChanged()
    {
        if (HasInfiniteLoop())
        {
            QMessageBox(QMessageBox::Warning,
                "Input Error",
                "Your spawner is set to spawn a dynamic slice that contains this entity.  You cannot set the spawner to be Spawn on Activate or it will spawn infinitely!",
                QMessageBox::Ok, QApplication::activeWindow()).exec();

            m_spawnOnActivate = false;

            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    bool EditorSpawnerComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SpawnerConfig*>(baseConfig))
        {
            m_sliceAsset = config->m_sliceAsset;
            m_spawnOnActivate = config->m_spawnOnActivate;
            m_destroyOnDeactivate = config->m_destroyOnDeactivate;
            return true;
        }
        return false;
    }

    bool EditorSpawnerComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SpawnerConfig*>(outBaseConfig))
        {
            config->m_sliceAsset = m_sliceAsset;
            config->m_spawnOnActivate = m_spawnOnActivate;
            config->m_destroyOnDeactivate = m_destroyOnDeactivate;
            return true;
        }
        return false;
    }

    void EditorSpawnerComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        // Add corresponding gameComponent to gameEntity.
        auto gameComponent = gameEntity->CreateComponent<SpawnerComponent>();

        SpawnerConfig config;
        config.m_sliceAsset = m_sliceAsset;
        config.m_spawnOnActivate = m_spawnOnActivate;
        config.m_destroyOnDeactivate = m_destroyOnDeactivate;

        gameComponent->SetConfiguration(config);
    }

} // namespace LmbrCentral
