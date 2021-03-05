/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or , if provided, by the license below or the license accompanying this file.Do not
*remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    class BM_Prefab
        : public UnitTest::AllocatorsBenchmarkFixture
        , public UnitTest::TraceBusRedirector
    {
    protected:
        using ::benchmark::Fixture::SetUp;
        using ::benchmark::Fixture::TearDown;

        void SetUp(::benchmark::State& state) override;
        void TearDown(::benchmark::State& state) override;

        AZ::Entity* CreateEntity(
            const char* entityName,
            const AZ::EntityId& parentId = AZ::EntityId());
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
        AZStd::vector<AZStd::string> m_paths;

        AZStd::unique_ptr <UnitTest::MockPrefabFileIOActionValidator> m_mockIOActionValidator;
    };
}

#endif
