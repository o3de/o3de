/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzTest/AzTest.h>

#include <AzToolsFramework/API/EntityPropertyEditorRequestsBus.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/ComponentMode/ComponentModeCollection.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/ScriptEditorComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>

#include <QApplication>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzToolsFramework;

    class EntityPropertyEditorTests
        : public UnitTest::LeakDetectionFixture
    {
    };

    TEST_F(EntityPropertyEditorTests, PrioritySort_NonTransformAsFirstItem_TransformMovesToTopRemainderUnchanged)
    {
        ToolsApplication app;

        AZ::Entity::ComponentArrayType unorderedComponents;
        AZ::Entity::ComponentArrayType orderedComponents;

        ToolsApplication::Descriptor desc;
        desc.m_useExistingAllocator = true;

        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        Entity* systemEntity = app.Create(desc, startupParameters);

        // Need to reflect the components so that edit attribute used for sorting, such as FixedComponentListIndex, get set.
        app.RegisterComponentDescriptor(AzToolsFramework::Components::TransformComponent::CreateDescriptor());
        app.RegisterComponentDescriptor(AzToolsFramework::Components::ScriptEditorComponent::CreateDescriptor());
        app.RegisterComponentDescriptor(AZ::AssetManagerComponent::CreateDescriptor());

        // Add more than 31 components, as we are testing the case where the sort fails when there are 32 or more items.
        const int numFillerItems = 32;

        for (int commentIndex = 0; commentIndex < numFillerItems; commentIndex++)
        {
            unorderedComponents.insert(unorderedComponents.begin(), systemEntity->CreateComponent(
                AzToolsFramework::Components::ScriptEditorComponent::RTTI_Type()));
        }

        // Add a TransformComponent at the end which should be sorted to the beginning by the priority sort.
        AZ::Component* transformComponent = systemEntity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        unorderedComponents.push_back(transformComponent);
        
        //add an AssetDatabase component at the beginning which should end up as the second item once the TransformComponent pushes it down
        AZ::Component* secondComponent = systemEntity->CreateComponent(AZ::AssetManagerComponent::RTTI_Type());
        unorderedComponents.insert(unorderedComponents.begin(), secondComponent);

        orderedComponents = unorderedComponents;

        // When this sort happens, the transformComponent should move to the top, the AssetDatabase should move to second, the order of the others should be unaltered, 
        // merely moved to after the AssetDatabase.
        SortComponentsByPriority(orderedComponents);

        // Check the component arrays are intact.
        EXPECT_EQ(orderedComponents.size(), unorderedComponents.size());
        EXPECT_GT(orderedComponents.size(), 2);

        // Check the transform is now the first component.
        EXPECT_EQ(orderedComponents[0], transformComponent);

        // Check the AssetDatabase is now second.
        EXPECT_EQ(orderedComponents[1], secondComponent);

        // Check the order of the remaining items is preserved.
        int firstUnsortedFillerIndex = 1;
        int firstSortedFillerIndex = 2;

        for (int index = 0; index < numFillerItems; index++)
        {
            EXPECT_EQ(orderedComponents[index + firstSortedFillerIndex], unorderedComponents[index + firstUnsortedFillerIndex]);
        }

    }

    void OpenPinnedInspector(const AzToolsFramework::EntityIdList& entities, EntityPropertyEditor* editor)
    {
        if (editor)
        {
            AzToolsFramework::EntityIdSet entitiesSet(entities.begin(), entities.end());
            editor->SetOverrideEntityIds(entitiesSet);
        }
    }

    class EntityPropertyEditorRequestTest
        : public ToolsApplicationFixture<>
    {
        void SetUpEditorFixtureImpl() override
        {
            m_editor = new EntityPropertyEditor();

            m_entity1 = CreateDefaultEditorEntity("Entity1");
            m_entity2 = CreateDefaultEditorEntity("Entity2");
            m_entity3 = CreateDefaultEditorEntity("Entity3");
            m_entity4 = CreateDefaultEditorEntity("Entity4");
        }

        void TearDownEditorFixtureImpl() override
        {
            delete m_editor;
        }

    public:
        EntityPropertyEditor* m_editor;
        EntityIdList m_entityIds;
        AZ::EntityId m_entity1;
        AZ::EntityId m_entity2;
        AZ::EntityId m_entity3;
        AZ::EntityId m_entity4;
    };

    TEST_F(EntityPropertyEditorRequestTest, GetSelectedEntitiesReturnsEitherSelectedEntitiesOrPinnedEntities)
    {
        EntityIdList entityIds;
        entityIds.insert(entityIds.begin(), { m_entity1, m_entity4 });

        // Set entity1 and entity4 as selected
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::SetSelectedEntities, entityIds);

        // Find the entities that are selected
        EntityIdList selectedEntityIds;
        AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
            &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetSelectedEntities, selectedEntityIds);

        // Make sure the correct number of entities are returned
        EXPECT_EQ(selectedEntityIds.size(), 2);

        // Check they are the same entities as selected above
        int found = 0;
        for (auto& id : selectedEntityIds)
        {
            if (id == m_entity1)
            {
                found |= 1;
            }
            if (id == m_entity4)
            {
                found |= 8;
            }
        }
        EXPECT_EQ(found, 9);

        // Clear the selected entities
        entityIds.clear();
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::SetSelectedEntities, entityIds);

        // Open the pinned Inspector with a different set of entities
        entityIds.insert(entityIds.begin(), { m_entity1, m_entity2, m_entity3 });
        OpenPinnedInspector(entityIds, m_editor);

        // Find the entities that are selected
        AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
            &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetSelectedEntities, selectedEntityIds);

        // Make sure the correct number of entities are returned
        EXPECT_EQ(selectedEntityIds.size(), 3);

        // Check they are the same entities as selected above
        found = 0;
        for (auto& id : selectedEntityIds)
        {
            if (id == m_entity1)
            {
                found |= 1;
            }
            if (id == m_entity2)
            {
                found |= 2;
            }
            if (id == m_entity3)
            {
                found |= 4;
            }
        }
        EXPECT_EQ(found, 7);
    }

    class LevelEntityPropertyEditorRequestTest
        : public ToolsApplicationFixture<>
        , public AzToolsFramework::EditorRequestBus::Handler
    {
        void SetUpEditorFixtureImpl() override
        {
            // Create an EntityPropertyEditor initialized to be a Level Inspector
            m_levelEditor = new EntityPropertyEditor(nullptr, {}, true);
            m_levelEntity = CreateDefaultEditorEntity("LevelEntity");

            // Level Inspector expects to have one override entity ID, which would normally be the root slice entity.
            AzToolsFramework::EntityIdSet entities;
            entities.insert(m_levelEntity);
            m_levelEditor->SetOverrideEntityIds(entities);

            // Connect to the EditorRequestBus so that we can intercept calls checking whether or not a level is currently open.
            AzToolsFramework::EditorRequestBus::Handler::BusConnect();
        }

        void TearDownEditorFixtureImpl() override
        {
            AzToolsFramework::EditorRequestBus::Handler::BusDisconnect();

            delete m_levelEditor;
        }

        // Mock out this call so that we can control whether or not the Level Inspector thinks a level is open.
        bool IsLevelDocumentOpen() override { return m_levelOpen; }

        // These are required by implementing the EditorRequestBus
        void BrowseForAssets(AssetBrowser::AssetSelectionModel& /*selection*/) override {}

    public:
        EntityPropertyEditor* m_levelEditor;
        AZ::EntityId m_levelEntity;
        bool m_levelOpen = false;
    };

    TEST_F(LevelEntityPropertyEditorRequestTest, GetSelectedEntitiesForLevelInspectorWhenLevelIsNotLoaded)
    {
        m_levelOpen = false;

        // Find the entities that are selected
        EntityIdList selectedEntityIds;
        AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
            &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetSelectedEntities, selectedEntityIds);

        // Make sure the correct number of entities are returned
        EXPECT_EQ(selectedEntityIds.size(), 0);
    }

    TEST_F(LevelEntityPropertyEditorRequestTest, GetSelectedEntitiesForLevelInspectorWhenLevelIsLoaded)
    {
        m_levelOpen = true;

        // Find the entities that are selected
        EntityIdList selectedEntityIds;

        // Find the entities that are selected
        AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
            &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetSelectedEntities, selectedEntityIds);

        // Make sure the correct number of entities are returned
        EXPECT_EQ(selectedEntityIds.size(), 1);
        EXPECT_EQ(selectedEntityIds[0], m_levelEntity);
    }

}
