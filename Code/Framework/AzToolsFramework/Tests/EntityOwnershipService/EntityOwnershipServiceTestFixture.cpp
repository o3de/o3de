/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EntityOwnershipServiceTestFixture.h"
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Components/AzFrameworkConfigurationSystemComponent.h>

#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>

namespace UnitTest
{
    void EntityOwnershipServiceTestFixture::SetUpEntityOwnershipServiceTest()
    {
        LeakDetectionFixture::SetUp();
        AZ::ComponentApplication::Descriptor componentApplicationDescriptor;
        componentApplicationDescriptor.m_useExistingAllocator = true;
        m_app = AZStd::make_unique<EntityOwnershipServiceApplication>();
        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        m_app->Start(componentApplicationDescriptor, startupParameters);

        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
    }

    void EntityOwnershipServiceTestFixture::TearDownEntityOwnershipServiceTest()
    {
        m_app.reset();
        LeakDetectionFixture::TearDown();
    }

    AZ::ComponentTypeList EntityOwnershipServiceTestFixture::EntityOwnershipServiceApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList defaultRequiredComponents = AzFramework::Application::GetRequiredSystemComponents();

        defaultRequiredComponents.emplace_back(azrtti_typeid<AzToolsFramework::Prefab::PrefabSystemComponent>());

        auto findComponentIterator = AZStd::find(defaultRequiredComponents.begin(), defaultRequiredComponents.end(),
            azrtti_typeid<AzFramework::GameEntityContextComponent>());
        if (findComponentIterator != defaultRequiredComponents.end())
        {
            defaultRequiredComponents.erase(findComponentIterator);
        }
        findComponentIterator = AZStd::find(defaultRequiredComponents.begin(), defaultRequiredComponents.end(),
            azrtti_typeid<AzFramework::AzFrameworkConfigurationSystemComponent>());
        if (findComponentIterator != defaultRequiredComponents.end())
        {
            defaultRequiredComponents.erase(findComponentIterator);
        }
        return defaultRequiredComponents;
    }

    AzFramework::RootSliceAsset EntityOwnershipServiceTestFixture::GetRootSliceAsset()
    {
        AzFramework::RootSliceAsset rootSliceAsset;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(rootSliceAsset,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootAsset);
        return rootSliceAsset;
    }

    void EntityOwnershipServiceTestFixture::HandleEntitiesAdded(const AzFramework::EntityList& entityList)
    {
        m_entitiesAddedCallbackTriggered = true;

        for (AZ::Entity* entity : entityList)
        {
            // If the entities are not initialized, they won't be removed from ComponentApplication during Entity destruction.
            if (entity->GetState() != AZ::Entity::State::Init)
            {
                entity->Init();
            }
        }
    }

    void EntityOwnershipServiceTestFixture::HandleEntitiesRemoved(const AzFramework::EntityIdList&)
    {
        m_entityRemovedCallbackTriggered = true;
    }

    bool EntityOwnershipServiceTestFixture::ValidateEntities(const AzFramework::EntityList&)
    {
        m_validateEntitiesCallbackTriggered = true;
        return m_areEntitiesValidForContext;
    }

    AzFramework::SliceInstantiationTicket EntityOwnershipServiceTestFixture::AddSlice(const EntityList& entityList,
        const bool isAsynchronous)
    {
        AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
        sliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), false);

        return AddSlice(entityList, isAsynchronous, sliceAsset);
    }

    AzFramework::SliceInstantiationTicket EntityOwnershipServiceTestFixture::AddSlice(const EntityList& entityList,
        const bool isAsynchronous, AZ::Data::Asset<AZ::SliceAsset>& sliceAsset)
    {
        AddSliceComponentToAsset(sliceAsset, entityList);

        AzFramework::SliceInstantiationTicket sliceInstantiationTicket;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(sliceInstantiationTicket,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::InstantiateSlice, sliceAsset, nullptr, nullptr);
        if (!isAsynchronous)
        {
            AZ::TickBus::ExecuteQueuedEvents();
        }
        return sliceInstantiationTicket;
    }

    void EntityOwnershipServiceTestFixture::AddSliceComponentToAsset(AZ::Data::Asset<AZ::SliceAsset>& sliceAsset,
        const EntityList& entityList)
    {
        AZ::Entity* sliceEntity = aznew AZ::Entity();
        AZ::SliceComponent* sliceComponent = sliceEntity->CreateComponent<AZ::SliceComponent>();
        sliceComponent->SetSerializeContext(m_app->GetSerializeContext());

        for (AZ::Entity* entity : entityList)
        {
            sliceComponent->AddEntity(entity);
        }

        sliceAsset->SetData(sliceEntity, sliceComponent);
    }

}
