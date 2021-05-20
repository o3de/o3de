/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Prefab/PrefabTestData.h>
#include <Prefab/PrefabTestUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabLoaderInterface;
    }
}

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;
    using namespace PrefabTestUtils;

    class PrefabTestToolsApplication
        : public ToolsTestApplication
    {
    public:
        PrefabTestToolsApplication(AZStd::string appName);

        // Make sure our prefab tests always run with prefabs enabled
        bool IsPrefabSystemEnabled() const override;
    };

    class PrefabTestFixture
        : public ToolsApplicationFixture,
          public UnitTest::TraceBusRedirector
    {
    protected:

        inline static const char* PrefabMockFilePath = "SomePath";
        inline static const char* NestedPrefabMockFilePath = "SomePathToNested";
        inline static const char* WheelPrefabMockFilePath = "SomePathToWheel";
        inline static const char* AxlePrefabMockFilePath = "SomePathToAxle";
        inline static const char* CarPrefabMockFilePath = "SomePathToCar";

        void SetUpEditorFixtureImpl() override;

        AZStd::unique_ptr<ToolsTestApplication> CreateTestApplication() override;

        AZ::Entity* CreateEntity(const char* entityName, const bool shouldActivate = true);

        void CompareInstances(const Instance& instanceA, const Instance& instanceB, bool shouldCompareLinkIds = true,
            bool shouldCompareContainerEntities = true);

        void DeleteInstances(const InstanceList& instancesToDelete);

        //! Validates that all entities within a prefab instance are in 'Active' state.
        void ValidateInstanceEntitiesActive(Instance& instance);

        PrefabSystemComponent* m_prefabSystemComponent = nullptr;
        PrefabLoaderInterface* m_prefabLoaderInterface = nullptr;
        PrefabPublicInterface* m_prefabPublicInterface = nullptr;
        InstanceUpdateExecutorInterface* m_instanceUpdateExecutorInterface = nullptr;
        InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
    };
}
