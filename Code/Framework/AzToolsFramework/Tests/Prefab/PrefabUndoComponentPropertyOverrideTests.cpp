/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabUndoComponentPropertyTestFixture.h>

#include <AzToolsFramework/Prefab/Undo/PrefabUndoComponentPropertyOverride.h>

namespace UnitTest
{
    using PrefabUndoComponentPropertyOverrideTests = PrefabUndoComponentPropertyTestFixture;

    TEST_F(PrefabUndoComponentPropertyOverrideTests, EditTransformStaticSucceeds)
    {
        InstanceInfo carInstanceInfo;
        EntityInfo wheelEntityInfo;
        CreateCarPrefabHierarchy(carInstanceInfo, wheelEntityInfo);

        const bool defaultStaticValue = false;
        const bool overriddenStaticValue = true;

        // Modify the IsStatic property in transform component as override
        // Note: Entity needs to be modified manually since we would update cached instance DOM in undo node to avoid reloading
        AZ::TransformBus::Event(wheelEntityInfo.m_entityId, &AZ::TransformInterface::SetIsStaticTransform, overriddenStaticValue);

        PropertyChangePatch changePatch = MakeTransformStaticPropertyChangePatch(wheelEntityInfo.m_entityId, overriddenStaticValue);

        PrefabDom propertyDomValue;
        ConvertToPrefabDomValue(propertyDomValue, changePatch.m_propertyValue);

        AZStd::string wheelEntityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(wheelEntityInfo.m_entityId);
        AZ::Dom::Path pathToPropertyFromOwningPrefab = AZ::Dom::Path(wheelEntityAliasPath) / changePatch.m_pathToPropertyFromOwningEntity;

        // Create an undo node
        PrefabUndoComponentPropertyOverride undoNode("Modify transform static");

        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carInstanceInfo.m_containerEntityId);
        EXPECT_TRUE(carInstance.has_value());

        // Redo
        undoNode.CaptureAndRedo(carInstance->get(), pathToPropertyFromOwningPrefab, propertyDomValue);
        EXPECT_TRUE(undoNode.Changed());
        PropagateAllTemplateChanges();

        bool currentStaticValue = defaultStaticValue; // default to opposite value
        AZ::TransformBus::EventResult(currentStaticValue, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::IsStaticTransform);
        EXPECT_EQ(currentStaticValue, overriddenStaticValue);
        EXPECT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));

        // Undo
        undoNode.Undo();
        PropagateAllTemplateChanges();

        currentStaticValue = overriddenStaticValue; // reset to opposite value
        AZ::TransformBus::EventResult(currentStaticValue, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::IsStaticTransform);
        EXPECT_EQ(currentStaticValue, defaultStaticValue);
        EXPECT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));

        // Redo
        undoNode.Redo();
        PropagateAllTemplateChanges();

        currentStaticValue = defaultStaticValue; // reset to opposite value
        AZ::TransformBus::EventResult(currentStaticValue, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::IsStaticTransform);
        EXPECT_EQ(currentStaticValue, overriddenStaticValue);
        EXPECT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));
    }

    TEST_F(PrefabUndoComponentPropertyOverrideTests, EditTransformStaticNoOverrideForUnchangedDefaultValue)
    {
        // This test validates that changing from default value 'false' to 'false' would not incur an override edit.

        InstanceInfo carInstanceInfo;
        EntityInfo wheelEntityInfo;
        CreateCarPrefabHierarchy(carInstanceInfo, wheelEntityInfo);

        const bool defaultStaticValue = false;

        // Modify the IsStatic property in transform component as override
        AZ::TransformBus::Event(wheelEntityInfo.m_entityId, &AZ::TransformInterface::SetIsStaticTransform, defaultStaticValue);

        PropertyChangePatch changePatch = MakeTransformStaticPropertyChangePatch(wheelEntityInfo.m_entityId, defaultStaticValue);

        PrefabDom propertyDomValue;
        ConvertToPrefabDomValue(propertyDomValue, changePatch.m_propertyValue);

        AZStd::string wheelEntityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(wheelEntityInfo.m_entityId);
        AZ::Dom::Path pathToPropertyFromOwningPrefab = AZ::Dom::Path(wheelEntityAliasPath) / changePatch.m_pathToPropertyFromOwningEntity;

        // Create an undo node
        PrefabUndoComponentPropertyOverride undoNode("Modify transform static to default value");

        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carInstanceInfo.m_containerEntityId);
        EXPECT_TRUE(carInstance.has_value());

        // Redo
        undoNode.CaptureAndRedo(carInstance->get(), pathToPropertyFromOwningPrefab, propertyDomValue);
        EXPECT_FALSE(undoNode.Changed());
        PropagateAllTemplateChanges();

        bool currentStaticValue = true; // default to opposite value
        AZ::TransformBus::EventResult(currentStaticValue, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::IsStaticTransform);
        EXPECT_EQ(currentStaticValue, defaultStaticValue);

        // Validate that there is no override created.
        EXPECT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));

        // Undo
        undoNode.Undo();
        PropagateAllTemplateChanges();

        currentStaticValue = true; // reset to opposite value
        AZ::TransformBus::EventResult(currentStaticValue, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::IsStaticTransform);
        EXPECT_EQ(currentStaticValue, defaultStaticValue);

        // Validate that there is no override created.
        EXPECT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));

        // Redo
        undoNode.Redo();
        PropagateAllTemplateChanges();

        currentStaticValue = true; // reset to opposite value
        AZ::TransformBus::EventResult(currentStaticValue, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::IsStaticTransform);
        EXPECT_EQ(currentStaticValue, defaultStaticValue);

        // Validate that there is no override created.
        EXPECT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));
    }

    TEST_F(PrefabUndoComponentPropertyOverrideTests, EditTranslationSucceeds)
    {
        InstanceInfo carInstanceInfo;
        EntityInfo wheelEntityInfo;
        CreateCarPrefabHierarchy(carInstanceInfo, wheelEntityInfo);

        const AZ::Vector3 defaultTranslation(0.0f, 0.0f, 0.0f);
        const AZ::Vector3 overriddenTranslation(10.0f, 20.0f, 0.0f);

        // Modify the local translation property in transform component as override
        // Note: Entity needs to be modified manually since we would update cached instance DOM in undo node to avoid reloading
        AZ::TransformBus::Event(wheelEntityInfo.m_entityId, &AZ::TransformInterface::SetLocalTranslation, overriddenTranslation);

        PropertyChangePatch changePatch = MakeTransformTranslationPropertyChangePatch(wheelEntityInfo.m_entityId, overriddenTranslation);

        PrefabDom propertyDomValue;
        ConvertToPrefabDomValue(propertyDomValue, changePatch.m_propertyValue);

        AZStd::string wheelEntityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(wheelEntityInfo.m_entityId);
        AZ::Dom::Path pathToPropertyFromOwningPrefab = AZ::Dom::Path(wheelEntityAliasPath) / changePatch.m_pathToPropertyFromOwningEntity;

        // Create an undo node
        PrefabUndoComponentPropertyOverride undoNode("Modify transform translation");
        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carInstanceInfo.m_containerEntityId);
        EXPECT_TRUE(carInstance.has_value());

        // Redo
        undoNode.CaptureAndRedo(carInstance->get(), pathToPropertyFromOwningPrefab, propertyDomValue);
        EXPECT_TRUE(undoNode.Changed());
        PropagateAllTemplateChanges();

        AZ::Vector3 currentWheelTranslate(-1.0f); // init to other value.
        AZ::TransformBus::EventResult(currentWheelTranslate, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::GetLocalTranslation);
        EXPECT_EQ(currentWheelTranslate, overriddenTranslation);
        EXPECT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));

        // Undo
        undoNode.Undo();
        PropagateAllTemplateChanges();

        currentWheelTranslate = AZ::Vector3(-1.0f); // reset
        AZ::TransformBus::EventResult(currentWheelTranslate, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::GetLocalTranslation);
        EXPECT_EQ(currentWheelTranslate, defaultTranslation);
        EXPECT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));

        // Redo
        undoNode.Redo();
        PropagateAllTemplateChanges();

        currentWheelTranslate = AZ::Vector3(-1.0f); // reset
        AZ::TransformBus::EventResult(currentWheelTranslate, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::GetLocalTranslation);
        EXPECT_EQ(currentWheelTranslate, overriddenTranslation);
        EXPECT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));
    }

    TEST_F(PrefabUndoComponentPropertyOverrideTests, EditTranslationOnEntityInNestedPrefabSucceeds)
    {
        const AZ::Vector3 defaultTranslation(0.0f, 0.0f, 0.0f);
        const AZ::Vector3 overriddenTranslation(5.0f, 10.0f, 15.0f);

        InstanceInfo superCarInstanceInfo;
        InstanceInfo carInstanceInfo;
        EntityInfo wheelEntityInfo;
        CreateSuperCarPrefabHierarchy(superCarInstanceInfo, carInstanceInfo, wheelEntityInfo);

        // Modify the transform component as override
        AZ::TransformBus::Event(wheelEntityInfo.m_entityId, &AZ::TransformInterface::SetLocalTranslation, overriddenTranslation);
        PropertyChangePatch changePatch = MakeTransformTranslationPropertyChangePatch(wheelEntityInfo.m_entityId, overriddenTranslation);

        PrefabDom propertyDomValue;
        ConvertToPrefabDomValue(propertyDomValue, changePatch.m_propertyValue);

        AZStd::string wheelEntityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(wheelEntityInfo.m_entityId);
        AZ::Dom::Path pathToPropertyFromOwningPrefab = AZ::Dom::Path(wheelEntityAliasPath) / changePatch.m_pathToPropertyFromOwningEntity;

        // Create an undo node
        PrefabUndoComponentPropertyOverride undoNode("Modify transform translation");
        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carInstanceInfo.m_containerEntityId);
        EXPECT_TRUE(carInstance.has_value());

        // Redo
        undoNode.CaptureAndRedo(carInstance->get(), pathToPropertyFromOwningPrefab, propertyDomValue);
        EXPECT_TRUE(undoNode.Changed());
        PropagateAllTemplateChanges();

        AZ::Vector3 currentWheelTranslate(-1.0f); // init to other value
        AZ::TransformBus::EventResult(currentWheelTranslate, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::GetLocalTranslation);
        EXPECT_EQ(currentWheelTranslate, overriddenTranslation);
        EXPECT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));

        // Undo
        undoNode.Undo();
        PropagateAllTemplateChanges();

        currentWheelTranslate = AZ::Vector3(-1.0f); // reset
        AZ::TransformBus::EventResult(currentWheelTranslate, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::GetLocalTranslation);
        EXPECT_EQ(currentWheelTranslate, defaultTranslation);
        EXPECT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));

        // Redo
        undoNode.Redo();
        PropagateAllTemplateChanges();

        currentWheelTranslate = AZ::Vector3(-1.0f); // reset
        AZ::TransformBus::EventResult(currentWheelTranslate, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::GetLocalTranslation);
        EXPECT_EQ(currentWheelTranslate, overriddenTranslation);
        EXPECT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));
    }

    TEST_F(PrefabUndoComponentPropertyOverrideTests, EditTranslationOverridePersistsAfterChangingBackToDefault)
    {
        // This test validates that override edit should persist when changing an overridden value '[10, 20, 0]' back
        // to its default value '[0, 0, 0]'.

        InstanceInfo carInstanceInfo;
        EntityInfo wheelEntityInfo;
        CreateCarPrefabHierarchy(carInstanceInfo, wheelEntityInfo);

        const AZ::Vector3 defaultTranslation(0.0f, 0.0f, 0.0f);
        const AZ::Vector3 overriddenTranslation(10.0f, 20.0f, 0.0f);

        // Modify the transform component as override
        AZ::TransformBus::Event(wheelEntityInfo.m_entityId, &AZ::TransformInterface::SetLocalTranslation, overriddenTranslation);

        PropertyChangePatch changePatch = MakeTransformTranslationPropertyChangePatch(wheelEntityInfo.m_entityId, overriddenTranslation);

        PrefabDom propertyDomValue;
        ConvertToPrefabDomValue(propertyDomValue, changePatch.m_propertyValue);

        AZStd::string wheelEntityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(wheelEntityInfo.m_entityId);
        AZ::Dom::Path pathToPropertyFromOwningPrefab = AZ::Dom::Path(wheelEntityAliasPath) / changePatch.m_pathToPropertyFromOwningEntity;

        // Create an undo node
        PrefabUndoComponentPropertyOverride undoNode("Modify transform translation to non-default value");
        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carInstanceInfo.m_containerEntityId);
        EXPECT_TRUE(carInstance.has_value());

        // Redo
        undoNode.CaptureAndRedo(carInstance->get(), pathToPropertyFromOwningPrefab, propertyDomValue);
        EXPECT_TRUE(undoNode.Changed());
        PropagateAllTemplateChanges();

        AZ::Vector3 currentWheelTranslate(-1.0f); // init to other value
        AZ::TransformBus::EventResult(currentWheelTranslate, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::GetLocalTranslation);
        EXPECT_EQ(currentWheelTranslate, overriddenTranslation);
        EXPECT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));

        // Change back to default value
        AZ::TransformBus::Event(wheelEntityInfo.m_entityId, &AZ::TransformInterface::SetLocalTranslation, defaultTranslation);
        PropertyChangePatch changePatchToDefault =
            MakeTransformTranslationPropertyChangePatch(wheelEntityInfo.m_entityId, defaultTranslation);

        PrefabDom defaultPropertyDomValue;
        ConvertToPrefabDomValue(defaultPropertyDomValue, changePatchToDefault.m_propertyValue);

        PrefabUndoComponentPropertyOverride undoNodeToDefault("Modify transform translation to default value");
        undoNodeToDefault.CaptureAndRedo(
            carInstance->get(), changePatchToDefault.m_pathToPropertyFromOwningEntity, defaultPropertyDomValue);
        PropagateAllTemplateChanges();

        currentWheelTranslate = AZ::Vector3(-1.0f); // reset
        AZ::TransformBus::EventResult(currentWheelTranslate, wheelEntityInfo.m_entityId, &AZ::TransformBus::Events::GetLocalTranslation);

        // Validate the value changes back to default and the override patch still persists.
        EXPECT_EQ(currentWheelTranslate, defaultTranslation);
        EXPECT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(
            wheelEntityInfo.m_entityId, changePatch.m_pathToPropertyFromOwningEntity.ToString()));
    }

} // namespace UnitTest
