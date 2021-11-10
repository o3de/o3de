/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Uuid.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Entity/EntityContext.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;

    class EntityContextBasicTest
        : public ScopedAllocatorSetupFixture
        , public EntityContextEventBus::Handler
    {
    public:

        EntityContextBasicTest()
        {
        }

        void SetUp() override
        {
            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            Data::AssetManager::Descriptor desc;
            Data::AssetManager::Create(desc);
        }

        ~EntityContextBasicTest() override
        {
        }

        void TearDown() override
        {
            Data::AssetManager::Destroy();

            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorInstance<ThreadPoolAllocator>::Destroy();
        }

        void run()
        {
            ComponentApplication app;
            ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;
            desc.m_enableDrilling = false; // we already created a memory driller for the test (AllocatorsFixture)
            app.Create(desc);

            Data::AssetManager::Instance().RegisterHandler(aznew SliceAssetHandler(app.GetSerializeContext()), AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());

            AZ::Uuid entityContextId = AZ::Uuid::CreateRandom();
            AZStd::unique_ptr<SliceEntityOwnershipService> m_entityOwnershipService =
                AZStd::make_unique<AzFramework::SliceEntityOwnershipService>(entityContextId, app.GetSerializeContext());
            EntityContext context(entityContextId, AZStd::move(m_entityOwnershipService), app.GetSerializeContext());
            context.InitContext();

            EntityContextEventBus::Handler::BusConnect(context.GetContextId());

            AZ::Entity* entity = context.CreateEntity("MyEntity");
            AZ_TEST_ASSERT(entity); // Should have created the entity.
            AZ_TEST_ASSERT(m_createEntityEvents == 1);

            AZ::Uuid contextId = AZ::Uuid::CreateNull();
            EBUS_EVENT_ID_RESULT(contextId, entity->GetId(), EntityIdContextQueryBus, GetOwningContextId);

            AZ_TEST_ASSERT(contextId == context.GetContextId()); // Context properly associated with entity?

            AZ_TEST_ASSERT(context.DestroyEntity(entity));
            AZ_TEST_ASSERT(m_destroyEntityEvents == 1);

            AZ::Entity* sliceEntity = aznew AZ::Entity();
            AZ::SliceComponent* sliceComponent = sliceEntity->CreateComponent<AZ::SliceComponent>();
            sliceComponent->SetSerializeContext(app.GetSerializeContext());
            sliceComponent->AddEntity(aznew AZ::Entity());
            Data::Asset<SliceAsset> sliceAssetHolder = Data::AssetManager::Instance().CreateAsset<SliceAsset>(Data::AssetId(Uuid::CreateRandom()));
            SliceAsset* sliceAsset = sliceAssetHolder.Get();
            sliceAsset->SetData(sliceEntity, sliceComponent);

            EntityContextEventBus::Handler::BusDisconnect(context.GetContextId());

            app.Destroy();
        }

        void OnEntityContextCreateEntity(AZ::Entity& entity) override
        {
            (void)entity;
            ++m_createEntityEvents;
        }

        void OnEntityContextDestroyEntity(const AZ::EntityId& entity) override
        {
            (void)entity;
            ++m_destroyEntityEvents;
        }

        size_t m_createEntityEvents = 0;
        size_t m_destroyEntityEvents = 0;
    };

    TEST_F(EntityContextBasicTest, Test)
    {
        run();
    }
}
