/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#include <Prefab/Benchmark/PrefabBenchmarkFixture.h>

#include <AzCore/Component/TransformBus.h> //for create entity
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestDataUtils.h>

namespace Benchmark
{
    void BM_Prefab::SetupPrefabSystem()
    {
        m_app = AZStd::make_unique<AzToolsFramework::ToolsApplication>();
        ASSERT_TRUE(m_app != nullptr);

        m_app->Start(AzFramework::Application::Descriptor());

        AZ::Entity* systemEntity = m_app->FindEntity(AZ::SystemEntityId);
        ASSERT_TRUE(systemEntity != nullptr);

        m_prefabSystemComponent = systemEntity->FindComponent<AzToolsFramework::Prefab::PrefabSystemComponent>();
        ASSERT_TRUE(m_prefabSystemComponent != nullptr);

        m_mockIOActionValidator = AZStd::make_unique<UnitTest::MockPrefabFileIOActionValidator>();
        ASSERT_TRUE(m_mockIOActionValidator != nullptr);

        m_instanceUpdateExecutorInterface = AZ::Interface<AzToolsFramework::Prefab::InstanceUpdateExecutorInterface>::Get();
        ASSERT_TRUE(m_instanceUpdateExecutorInterface != nullptr);

        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
    }

    void BM_Prefab::TearDownPrefabSystem()
    {
        m_mockIOActionValidator.reset();

        m_app.reset();
    }

    void BM_Prefab::ResetPrefabSystem()
    {
        TearDownPrefabSystem();
        SetupPrefabSystem();
    }

    void BM_Prefab::internalSetUp(const benchmark::State& state)
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        UnitTest::AllocatorsBenchmarkFixture::SetUp(state);

        SetupPrefabSystem();
    }

    void BM_Prefab::internalTearDown(const benchmark::State& state)
    {
        m_paths = {};

        TearDownPrefabSystem();

        UnitTest::AllocatorsBenchmarkFixture::TearDown(state);

        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    AZ::Entity* BM_Prefab::CreateEntity(const char* entityName, const AZ::EntityId& parentId)
    {
        // Circumvent the EntityContext system and generate a new entity with a transformcomponent
        AZ::Entity* newEntity = aznew AZ::Entity(entityName);
        newEntity->CreateComponent(AZ::TransformComponentTypeId);
        newEntity->Init();
        newEntity->Activate();

        SetEntityParent(newEntity->GetId(), parentId);

        return newEntity;
    }

    void BM_Prefab::CreateEntities(const unsigned int entityCount, AZStd::vector<AZ::Entity*>& entities)
    {
        for (unsigned int entityIndex = 0; entityIndex < entityCount; ++entityIndex)
        {
            AZStd::string entityName = "TestEntity";
            entityName = entityName + AZStd::to_string(entityIndex);
            entities.emplace_back(CreateEntity(entityName.c_str()));
        }
    }

    void BM_Prefab::SetEntityParent(const AZ::EntityId& entityId, const AZ::EntityId& parentId)
    {
        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetParent, parentId);
    }

    void BM_Prefab::CreateFakePaths(const unsigned int pathCount)
    {
        //setup fake paths
        for (unsigned int number = 0; number < pathCount; ++number)
        {
            AZStd::string path = m_pathString;
            m_paths.push_back(path + AZStd::to_string(number) + "_" + AZStd::to_string(pathCount));
        }
    }

    void BM_Prefab::SetUpMockValidatorForReadPrefab()
    {
        const size_t pathCount = m_paths.size();
        for (size_t number = 0; number < pathCount; ++number)
        {
            m_mockIOActionValidator->ReadPrefabDom(
                m_paths[number], UnitTest::PrefabTestDomUtils::CreatePrefabDom());
        }
    }

    void BM_Prefab::DeleteInstances(const AzToolsFramework::Prefab::InstanceList& instancesToDelete)
    {
        for (AzToolsFramework::Prefab::Instance* instanceToDelete : instancesToDelete)
        {
            ASSERT_TRUE(instanceToDelete);
            delete instanceToDelete;
            instanceToDelete = nullptr;
        }
    }
}

#endif
