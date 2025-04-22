/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/SpawnableRemoveEditorInfoTestFixture.h>

namespace UnitTest
{
    using SpawnableRemoveEditorInfoTests = SpawnableRemoveEditorInfoTestFixture;

    TEST_F(SpawnableRemoveEditorInfoTests, SpawnableRemoveEditorInfo_OnlyRuntimeEntityExported)
    {
        // Create one entity that's flagged as Editor-Only, and one that's enabled for runtime.
        CreateSourceEntity("EditorOnly", true);
        CreateSourceEntity("EditorAndRuntime", false);

        ConvertRuntimePrefab();

        // Only the runtime entity exists in the converted Prefab DOM.
        EXPECT_FALSE(GetRuntimeEntity("EditorOnly"));
        EXPECT_TRUE(GetRuntimeEntity("EditorAndRuntime"));
    }

    TEST_F(SpawnableRemoveEditorInfoTests, SpawnableRemoveEditorInfo_RuntimeComponentExportedSuccessfully)
    {
        // Create a component with RuntimeExportCallback and successfully exports itself.
        CreateSourceTestExportRuntimeEntity("EntityWithRuntimeComponent", true, true);

        ConvertRuntimePrefab();

        // Expected result: processed entity contains the component.
        AZ::Entity* entity = GetRuntimeEntity("EntityWithRuntimeComponent");
        ASSERT_TRUE(entity);
        EXPECT_TRUE(entity->FindComponent<TestExportRuntimeComponentWithCallback>());
    }

    TEST_F(SpawnableRemoveEditorInfoTests, RuntimeExportCallback_RuntimeComponentExportRemoved)
    {
        // Create a component with RuntimeExportCallback and successfully removes itself from exporting.
        CreateSourceTestExportRuntimeEntity("EntityWithRuntimeComponent", false, true);

        ConvertRuntimePrefab();

        // Expected result: processed entity does NOT contain the component.
        AZ::Entity* entity = GetRuntimeEntity("EntityWithRuntimeComponent");
        ASSERT_TRUE(entity);
        EXPECT_FALSE(entity->FindComponent<TestExportRuntimeComponentWithCallback>());
    }

    TEST_F(SpawnableRemoveEditorInfoTests, RuntimeExportCallback_RuntimeComponentExportUnhandled)
    {
        // Create a component with RuntimeExportCallback, returns a pointer to itself, but says it wasn't handled.
        CreateSourceTestExportRuntimeEntity("EntityWithRuntimeComponent", true, false);

        ConvertRuntimePrefab();

        // Expected result:  processed entity contains the component, because the default behavior is "clone/add" for
        // runtime components.
        AZ::Entity* entity = GetRuntimeEntity("EntityWithRuntimeComponent");
        ASSERT_TRUE(entity);
        EXPECT_TRUE(entity->FindComponent<TestExportRuntimeComponentWithCallback>());
    }

    TEST_F(SpawnableRemoveEditorInfoTests, RuntimeExportCallback_RuntimeComponentExportRemovedAndUnhandled)
    {
        // Create a component with RuntimeExportCallback and removes itself from exporting, but says it wasn't handled.
        CreateSourceTestExportRuntimeEntity("EntityWithRuntimeComponent", false, false);

        ConvertRuntimePrefab();

        // Expected result:  processed entity contains the component, because by saying it wasn't handled, it
        // should fall back on the default behavior of "clone/add" for runtime components.
        AZ::Entity* entity = GetRuntimeEntity("EntityWithRuntimeComponent");
        ASSERT_TRUE(entity);
        EXPECT_TRUE(entity->FindComponent<TestExportRuntimeComponentWithCallback>());
    }

    TEST_F(SpawnableRemoveEditorInfoTests, RuntimeExportCallback_EditorComponentExportedSuccessfully)
    {
        // Create an editor component that has a RuntimeExportCallback and successfully exports itself.
        CreateSourceTestExportEditorEntity(
            "EntityWithEditorComponent",
            TestExportEditorComponent::ExportComponentType::ExportRuntimeComponentWithoutCallBack,
            true);

        ConvertRuntimePrefab();

        // Expected result:  processed entity contains the runtime component, exported from RuntimeExportCallback.
        AZ::Entity* entity = GetRuntimeEntity("EntityWithEditorComponent");
        ASSERT_TRUE(entity);
        EXPECT_FALSE(entity->FindComponent<TestExportEditorComponent>());
        EXPECT_FALSE(entity->FindComponent<TestExportRuntimeComponentWithCallback>());
        EXPECT_TRUE(entity->FindComponent<TestExportRuntimeComponentWithoutCallback>());
    }

    TEST_F(SpawnableRemoveEditorInfoTests, RuntimeExportCallback_EditorComponentExportRemoved)
    {
        // Create an editor component that has a RuntimeExportCallback and successfully removes itself from exporting.
        CreateSourceTestExportEditorEntity(
            "EntityWithEditorComponent",
            TestExportEditorComponent::ExportComponentType::ExportNullComponent,
            true);

        ConvertRuntimePrefab();

        // Expected result: processed entity does NOT contain either component.
        AZ::Entity* entity = GetRuntimeEntity("EntityWithEditorComponent");
        ASSERT_TRUE(entity);
        EXPECT_FALSE(entity->FindComponent<TestExportEditorComponent>());
        EXPECT_FALSE(entity->FindComponent<TestExportRuntimeComponentWithCallback>());
        EXPECT_FALSE(entity->FindComponent<TestExportRuntimeComponentWithoutCallback>());
    }

    TEST_F(SpawnableRemoveEditorInfoTests, RuntimeExportCallback_EditorComponentExportUnhandledFallBackToBuildGameEntity)
    {
        // Create an editor component that has a RuntimeExportCallback, returns a pointer to itself, but says it wasn't handled.
        CreateSourceTestExportEditorEntity(
            "EntityWithEditorComponent",
            TestExportEditorComponent::ExportComponentType::ExportEditorComponent,
            false);

        ConvertRuntimePrefab();

        // Expected result:  processed entity contains the runtime component, because the fallback to BuildGameEntity()
        // produced a runtime component.
        AZ::Entity* entity = GetRuntimeEntity("EntityWithEditorComponent");
        ASSERT_TRUE(entity);
        EXPECT_FALSE(entity->FindComponent<TestExportEditorComponent>());
        EXPECT_TRUE(entity->FindComponent<TestExportRuntimeComponentWithCallback>());
        EXPECT_FALSE(entity->FindComponent<TestExportRuntimeComponentWithoutCallback>());
    }

    TEST_F(SpawnableRemoveEditorInfoTests, RuntimeExportCallback_EditorComponentExportRemovedAndUnhandledFallBackToBuildGameEntity)
    {
        // Create an editor component that has a RuntimeExportCallback and removes itself from exporting, but says it wasn't handled.
        CreateSourceTestExportEditorEntity(
            "EntityWithEditorComponent",
            TestExportEditorComponent::ExportComponentType::ExportNullComponent,
            false);

        ConvertRuntimePrefab();

        // Expected result:  processed entity contains the runtime component, because the fallback to BuildGameEntity()
        // produced a runtime component.
        AZ::Entity* entity = GetRuntimeEntity("EntityWithEditorComponent");
        ASSERT_TRUE(entity);
        EXPECT_FALSE(entity->FindComponent<TestExportEditorComponent>());
        EXPECT_TRUE(entity->FindComponent<TestExportRuntimeComponentWithCallback>());
        EXPECT_FALSE(entity->FindComponent<TestExportRuntimeComponentWithoutCallback>());
    }

    TEST_F(SpawnableRemoveEditorInfoTests, RuntimeExportCallback_EditorComponentFailsToExportItself)
    {
        // Create an editor component that has a RuntimeExportCallback and removes itself from exporting, but says it wasn't handled.
        CreateSourceTestExportEditorEntity(
            "EntityWithEditorComponent",
            TestExportEditorComponent::ExportComponentType::ExportEditorComponent,
            true);

        // We expect the exporting to fail, since an editor component is being exported as a game component.
        ConvertRuntimePrefab(false);
    }
}
