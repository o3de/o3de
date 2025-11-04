/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabUndoComponentPropertyTestFixture.h>

#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace UnitTest
{
    void PrefabUndoComponentPropertyTestFixture::SetUpEditorFixtureImpl()
    {
        PrefabTestFixture::SetUpEditorFixtureImpl();

        m_prefabOverridePublicInterface = AZ::Interface<PrefabOverridePublicInterface>::Get();
        ASSERT_TRUE(m_prefabOverridePublicInterface);
    }

    void PrefabUndoComponentPropertyTestFixture::CreateWheelEntityHierarchy(EntityInfo& wheelEntityInfo)
    {
        AZ::EntityId wheelEntityId = CreateEditorEntityUnderRoot(WheelEntityName);
        ASSERT_TRUE(wheelEntityId.IsValid());

        EntityAlias wheelEntityAlias = FindEntityAliasInInstance(GetRootContainerEntityId(), WheelEntityName);
        ASSERT_FALSE(wheelEntityAlias.empty());

        wheelEntityInfo = { wheelEntityId, wheelEntityAlias };
    }

    void PrefabUndoComponentPropertyTestFixture::CreateCarPrefabHierarchy(
        InstanceInfo& carInstanceInfo, EntityInfo& wheelEntityInfo)
    {
        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::IO::Path carPrefabFilepath = engineRootPath / CarPrefabName;

        // Create the Car prefab
        AZ::EntityId wheelEntityId = CreateEditorEntityUnderRoot(WheelEntityName);
        ASSERT_TRUE(wheelEntityId.IsValid());

        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { wheelEntityId });
        ASSERT_TRUE(carContainerId.IsValid());

        InstanceAlias carInstanceAlias = FindNestedInstanceAliasInInstance(GetRootContainerEntityId(), CarPrefabName);
        ASSERT_FALSE(carInstanceAlias.empty());

        EntityAlias wheelEntityAlias = FindEntityAliasInInstance(carContainerId, WheelEntityName);
        ASSERT_FALSE(wheelEntityAlias.empty());

        // Retrieve the Wheel entity id after adding it to Car instance
        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        ASSERT_TRUE(carInstance.has_value());
        EntityOptionalReference wheelEntity = carInstance->get().GetEntity(wheelEntityAlias);
        ASSERT_TRUE(wheelEntity.has_value());
        wheelEntityId = wheelEntity->get().GetId();
        ASSERT_TRUE(wheelEntityId.IsValid());

        carInstanceInfo = { carContainerId, carInstanceAlias };
        wheelEntityInfo = { wheelEntityId, wheelEntityAlias };
    }

    void PrefabUndoComponentPropertyTestFixture::CreateSuperCarPrefabHierarchy(
        InstanceInfo& superCarInstanceInfo, InstanceInfo& carInstanceInfo, EntityInfo& wheelEntityInfo)
    {
        // Create the Car prefab
        CreateCarPrefabHierarchy(carInstanceInfo, wheelEntityInfo);

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::IO::Path superCarPrefabFilepath = engineRootPath / SuperCarPrefabName;

        // Create the SuperCar prefab
        AZ::EntityId superCarContainerId = CreateEditorPrefab(superCarPrefabFilepath, { carInstanceInfo.m_containerEntityId });
        ASSERT_TRUE(superCarContainerId.IsValid());

        InstanceAlias superCarInstanceAlias = FindNestedInstanceAliasInInstance(GetRootContainerEntityId(), SuperCarPrefabName);
        ASSERT_FALSE(superCarInstanceAlias.empty());

        superCarInstanceInfo = { superCarContainerId, superCarInstanceAlias };

        // Retrieve the Car instance info after adding it to SuperCar instance
        InstanceAlias newCarInstanceAlias = FindNestedInstanceAliasInInstance(superCarContainerId, CarPrefabName);
        ASSERT_FALSE(newCarInstanceAlias.empty());

        InstanceOptionalReference superCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(superCarContainerId);
        ASSERT_TRUE(superCarInstance.has_value());

        InstanceOptionalReference carInstance = superCarInstance->get().FindNestedInstance(newCarInstanceAlias);
        ASSERT_TRUE(carInstance.has_value());

        carInstanceInfo = { carInstance->get().GetContainerEntityId(), newCarInstanceAlias };

        // Retrieve the Wheel entity id after adding the Car instance to SuperCar instance
        EntityOptionalReference wheelEntity = carInstance->get().GetEntity(wheelEntityInfo.m_entityAlias);
        ASSERT_TRUE(wheelEntity.has_value());

        wheelEntityInfo.m_entityId = wheelEntity->get().GetId();
        ASSERT_TRUE(wheelEntityInfo.m_entityId.IsValid());
    }
    
    auto PrefabUndoComponentPropertyTestFixture::MakeTransformTranslationPropertyChangePatch(
        const AZ::EntityId entityId, const AZ::Vector3& translation) -> PropertyChangePatch
    {
        AZ::Entity* entity = AzToolsFramework::GetEntity(entityId);
        EXPECT_TRUE(entity) << "MakeTransformTranslationPropertyChangePatch - Cannot retrieve the entity that is provided.";

        const auto transformComponent = entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
        EXPECT_TRUE(transformComponent) << "MakeTransformTranslationPropertyChangePatch - Cannot get the transform component.";

        AZ::Dom::Value propertyValue;
        propertyValue.SetArray();
        propertyValue.ArrayPushBack(AZ::Dom::Value(translation.GetX()));
        propertyValue.ArrayPushBack(AZ::Dom::Value(translation.GetY()));
        propertyValue.ArrayPushBack(AZ::Dom::Value(translation.GetZ()));

        const AZStd::string componentAlias = transformComponent->GetSerializedIdentifier();
        EXPECT_FALSE(componentAlias.empty()) << "MakeTransformTranslationPropertyChangePatch - Component alias is empty.";

        AZ::Dom::Path pathToProperty;
        pathToProperty /= PrefabDomUtils::ComponentsName;
        pathToProperty /= componentAlias;
        pathToProperty /= AZ::Dom::Path("/Transform Data/Translate");

        return { pathToProperty, propertyValue };
    }

    auto PrefabUndoComponentPropertyTestFixture::MakeTransformStaticPropertyChangePatch(
        const AZ::EntityId entityId, const float isStatic) -> PropertyChangePatch
    {
        AZ::Entity* entity = AzToolsFramework::GetEntity(entityId);
        EXPECT_TRUE(entity) << "MakeTransformStaticPropertyChangePatch - Cannot retrieve the entity that is provided.";

        const auto transformComponent = entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
        EXPECT_TRUE(transformComponent) << "MakeTransformStaticPropertyChangePatch - Cannot get the transform component.";

        AZ::Dom::Value propertyValue;
        propertyValue.SetBool(isStatic);

        const AZStd::string componentAlias = transformComponent->GetSerializedIdentifier();
        EXPECT_FALSE(componentAlias.empty()) << "MakeTransformStaticPropertyChangePatch - Component alias is empty.";

        AZ::Dom::Path pathToProperty;
        pathToProperty /= PrefabDomUtils::ComponentsName;
        pathToProperty /= componentAlias;
        pathToProperty /= AZ::Dom::Path("/IsStatic");

        return { pathToProperty, propertyValue };
    }

    void PrefabUndoComponentPropertyTestFixture::ConvertToPrefabDomValue(
        PrefabDom& outputDom, const AZ::Dom::Value& domValue)
    {
        auto convertToRapidJsonOutcome = AZ::Dom::Json::WriteToRapidJsonDocument(
            [&domValue](AZ::Dom::Visitor& visitor)
            {
                return domValue.Accept(visitor, false);
            });
        EXPECT_TRUE(convertToRapidJsonOutcome.IsSuccess());

        outputDom = convertToRapidJsonOutcome.TakeValue();
    }

} // namespace UnitTest
