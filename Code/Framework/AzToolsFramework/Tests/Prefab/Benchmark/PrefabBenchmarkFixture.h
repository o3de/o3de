/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#pragma once
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <Prefab/MockPrefabFileIOActionValidator.h>
#include <Prefab/PrefabSystemComponent.h>
#include <Prefab/PrefabTestData.h>
#include <Prefab/PrefabTestUtils.h>

namespace Benchmark
{
    using namespace UnitTest::PrefabTestUtils;

    class PrefabBenchmarkHarness
    {
        virtual void SetupHarness(const benchmark::State& state) = 0;
        virtual void TeardownHarness(const benchmark::State& state) = 0;
    };

    class BM_Prefab
        : public UnitTest::AllocatorsBenchmarkFixture
        , public PrefabBenchmarkHarness
        , public UnitTest::TraceBusRedirector
    {
    public:
        void SetupHarness(const benchmark::State& state) override;
        void TeardownHarness(const benchmark::State& state) override;

    protected:
        void SetUp(const benchmark::State& state) override
        {
            SetupHarness(state);
        }
        void SetUp(benchmark::State& state) override
        {
            SetupHarness(state);
        }

        void TearDown(const benchmark::State& state) override
        {
            TeardownHarness(state);
        }
        void TearDown(benchmark::State& state) override
        {
            TeardownHarness(state);
        }

        AZ::Entity* CreateEntity(const char* entityName, const AZ::EntityId& parentId = AZ::EntityId());
        void CreateEntities(const unsigned int entityCount, AZStd::vector<AZ::Entity*>& entities);
        void SetEntityParent(const AZ::EntityId& entityId, const AZ::EntityId& parentId);

        void CreateFakePaths(const unsigned int pathCount);

        void SetUpMockValidatorForReadPrefab();

        void DeleteInstances(const AzToolsFramework::Prefab::InstanceList& instances);

        void SetupPrefabSystem();
        void TearDownPrefabSystem();
        void ResetPrefabSystem();

        //prefab specific
        AZStd::unique_ptr<AzToolsFramework::ToolsApplication> m_app;
        AzToolsFramework::Prefab::PrefabSystemComponent* m_prefabSystemComponent = nullptr;
        AzToolsFramework::Prefab::PrefabLoaderInterface* m_prefabLoaderInterface = nullptr;
        AzToolsFramework::Prefab::InstanceUpdateExecutorInterface* m_instanceUpdateExecutorInterface = nullptr;

        const char* m_pathString = "path/to/template";
        AZStd::vector<AZ::IO::Path> m_paths;

        AZStd::unique_ptr <UnitTest::MockPrefabFileIOActionValidator> m_mockIOActionValidator;
    };
}

#endif
