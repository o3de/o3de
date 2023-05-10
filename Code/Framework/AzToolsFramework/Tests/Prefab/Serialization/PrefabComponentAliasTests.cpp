/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestFixture.h>
#include <Prefab/PrefabTestComponent.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace UnitTest
{
    using PrefabComponentAliasTest = PrefabTestFixture;

    static void CreateAndValidateComponentAlias(const AZ::Entity::ComponentArrayType& componentsToAdd, AZStd::string entityAlias, AZ::Dom::Path pathToComponent)
    {
        Instance prefab;
        AZStd::unique_ptr<AZ::Entity> entityInPrefab = AZStd::make_unique<AZ::Entity>();
        
        for (AZ::Component* component: componentsToAdd)
        {
            entityInPrefab->AddComponent(component);
        }

        prefab.AddEntity(AZStd::move(entityInPrefab), AZStd::move(entityAlias));

        PrefabDom prefabDom;
        PrefabDomUtils::StoreInstanceInPrefabDom(prefab, prefabDom);

        PrefabDomPath domPathToComponents(pathToComponent.ToString().c_str());
        const PrefabDomValue* componentsDom = domPathToComponents.Get(prefabDom);
        EXPECT_EQ(componentsDom->IsNull(), false);
    }

    TEST_F(PrefabComponentAliasTest, TypeNameBasedAliasIsCreatedWhenAliasAbsent)
    {
        AZStd::string entityAlias = "EntityAlias";
        AZ::Dom::Path pathToComponent = AZ::Dom::Path(PrefabDomUtils::EntitiesName) / entityAlias.c_str() / PrefabDomUtils::ComponentsName /
            PrefabTestComponent::RTTI_TypeName();

        // Validate that the typename is set as component alias.
        CreateAndValidateComponentAlias(
            AZ::Entity::ComponentArrayType{ aznew PrefabTestComponent() }, AZStd::move(entityAlias), pathToComponent);
    }

    TEST_F(PrefabComponentAliasTest, NumberedAliasesCreatedForMultipleComponentsWithSameType)
    {
        AZStd::string entityAlias = "EntityAlias";
        AZStd::string secondComponentAlias = AZStd::string::format("%s_%u", PrefabTestComponent::RTTI_TypeName(), 2);
        AZ::Dom::Path pathToSecondComponent = AZ::Dom::Path(PrefabDomUtils::EntitiesName) / entityAlias.c_str() /
            PrefabDomUtils::ComponentsName / secondComponentAlias.c_str();

        // Validate that the second component got a number next to the typename as alias.
        CreateAndValidateComponentAlias(
            AZ::Entity::ComponentArrayType{ aznew PrefabTestComponent(), aznew PrefabTestComponent() },
            AZStd::move(entityAlias),
            pathToSecondComponent);
    }

    TEST_F(PrefabComponentAliasTest, AliasNotCreatedWhenAliasAlreadyPresent)
    {
        PrefabTestComponent* prefabTestComponent = aznew PrefabTestComponent();
        AZStd::string customComponentAlias = "CustomSerializedIdentifier";
        AZStd::string entityAlias = "EntityAlias";

        // This is analogous to reading a component from prefab file since we set component aliases when a prefab is loaded from JSON.
        prefabTestComponent->SetSerializedIdentifier(customComponentAlias);
        AZ::Dom::Path pathToComponent = AZ::Dom::Path(PrefabDomUtils::EntitiesName) / entityAlias.c_str() / PrefabDomUtils::ComponentsName /
            customComponentAlias.c_str();

        // Validate that serializing the component again won't make it lose its custom alias. This proves that aliases in existing prefabs
        // will not get changed when the same prefab is saved again.
        CreateAndValidateComponentAlias(AZ::Entity::ComponentArrayType{ prefabTestComponent }, AZStd::move(entityAlias), pathToComponent);
    }

    TEST_F(PrefabComponentAliasTest, UnderlyingTypeNameAliasCreatedForGenericComponentWrapper)
    {
        AZ::EntityId entityId = CreateEditorEntityUnderRoot("entity");

        AZ::Entity* entityInPrefab = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entityInPrefab, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
        ASSERT_TRUE(entityInPrefab);
        entityInPrefab->Deactivate();

        // In order to make make a GenericComponentWrapper wrap against a non-editor component( doesn't derive from EditorComponentBase),
        // we need to add component this way. This is what happens when users try to add a non-editor component in the inspector too
        AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome;
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            addComponentsOutcome,
            &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities,
            AzToolsFramework::EntityIdList{ entityId },
            AZ::ComponentTypeList{ PrefabNonEditorComponent::RTTI_Type() });


        InstanceOptionalReference prefab = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
        EXPECT_TRUE(prefab.has_value());

        PrefabDom prefabDom;
        PrefabDomUtils::StoreInstanceInPrefabDom(prefab->get(), prefabDom);

        AZ::Dom::Path pathToComponent = AZ::Dom::Path(PrefabDomUtils::EntitiesName) / prefab->get().GetEntityAliases().front().c_str() /
            PrefabDomUtils::ComponentsName / PrefabNonEditorComponent::RTTI_TypeName();
        PrefabDomPath domPathToComponents(pathToComponent.ToString().c_str());
        const PrefabDomValue* componentsDom = domPathToComponents.Get(prefabDom);
        ASSERT_EQ(componentsDom->IsNull(), false);
    }
} // namespace UnitTest
