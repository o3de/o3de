/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzToolsFramework;

    class EditorEntitySelectionTest
        : public ToolsApplicationFixture<>
    {
        void SetUpEditorFixtureImpl() override
        {
            m_entity1 = CreateDefaultEditorEntity("Entity1");
            m_entity2 = CreateDefaultEditorEntity("Entity2");
            m_entity3 = CreateDefaultEditorEntity("Entity3");
            m_entity4 = CreateDefaultEditorEntity("Entity4");
        }

    public:
        AZ::EntityId m_entity1;
        AZ::EntityId m_entity2;
        AZ::EntityId m_entity3;
        AZ::EntityId m_entity4;
    };

    TEST_F(EditorEntitySelectionTest, EditorEntitySelectionTests_SetAndGetSelectedEntities)
    {
        // Set entity1 and entity4 as selected
        EntityIdList testEntityIds{ m_entity1, m_entity4 };
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::SetSelectedEntities, testEntityIds);

        EntityIdList selectedEntityIds;
        ToolsApplicationRequestBus::BroadcastResult(
            selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        EXPECT_EQ(selectedEntityIds.size(), testEntityIds.size());
        for (auto& id : testEntityIds)
        {
            EXPECT_TRUE(AZStd::find(selectedEntityIds.begin(), selectedEntityIds.end(), id) != selectedEntityIds.end());
        }

        // Clear all selected entities
        testEntityIds.clear();
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::SetSelectedEntities, testEntityIds);

        ToolsApplicationRequestBus::BroadcastResult(
            selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        EXPECT_TRUE(selectedEntityIds.empty());
    }

    TEST_F(EditorEntitySelectionTest, EditorEntitySelectionTests_MarkEntitySelectedAndDeselected)
    {
        // Mark testEntityId as selected
        AZ::EntityId testEntityId = m_entity1;
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::MarkEntitySelected, testEntityId);

        bool testEntitySelected = false;
        ToolsApplicationRequestBus::BroadcastResult(
            testEntitySelected, &ToolsApplicationRequests::IsSelected, testEntityId);

        bool anyEntitySelected = false;
        ToolsApplicationRequestBus::BroadcastResult(
            anyEntitySelected, &ToolsApplicationRequests::AreAnyEntitiesSelected);

        int selectedEntitiesCount = 0;
        ToolsApplicationRequestBus::BroadcastResult(
            selectedEntitiesCount, &ToolsApplicationRequests::GetSelectedEntitiesCount);

        EntityIdList selectedEntityIds;
        ToolsApplicationRequestBus::BroadcastResult(
            selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        EXPECT_TRUE(testEntitySelected);
        EXPECT_TRUE(anyEntitySelected);
        EXPECT_EQ(selectedEntitiesCount, 1);
        EXPECT_EQ(selectedEntityIds.size(), 1);
        EXPECT_EQ(selectedEntityIds.front(), testEntityId);

        // Mark testEntityId as deselected
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::MarkEntityDeselected, testEntityId);

        ToolsApplicationRequestBus::BroadcastResult(
            testEntitySelected, &ToolsApplicationRequests::IsSelected, testEntityId);

        ToolsApplicationRequestBus::BroadcastResult(
            anyEntitySelected, &ToolsApplicationRequests::AreAnyEntitiesSelected);

        ToolsApplicationRequestBus::BroadcastResult(
            selectedEntitiesCount, &ToolsApplicationRequests::GetSelectedEntitiesCount);

        ToolsApplicationRequestBus::BroadcastResult(
            selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        EXPECT_FALSE(testEntitySelected);
        EXPECT_FALSE(anyEntitySelected);
        EXPECT_EQ(selectedEntitiesCount, 0);
        EXPECT_TRUE(selectedEntityIds.empty());
    }

    TEST_F(EditorEntitySelectionTest, EditorEntitySelectionTests_MarkEntitiesDeselectedAndSelected)
    {
        // Set all entities as selected
        EntityIdList testEntityIds{ m_entity1, m_entity2, m_entity3, m_entity4 };
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::SetSelectedEntities, testEntityIds);

        // Deselect first half of entities
        EntityIdList deselctedEntityIds{ testEntityIds.begin(), testEntityIds.begin() + testEntityIds.size() / 2 };
        EntityIdList expectedSelectedEntityIds{ testEntityIds.begin() + testEntityIds.size() / 2, testEntityIds.end() };
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::MarkEntitiesDeselected, deselctedEntityIds);

        bool targetSelected = false;
        for (auto& id : expectedSelectedEntityIds)
        {
            ToolsApplicationRequestBus::BroadcastResult(
                targetSelected, &ToolsApplicationRequests::IsSelected, id);

            EXPECT_TRUE(targetSelected);
        }
        for (auto& id : deselctedEntityIds)
        {
            ToolsApplicationRequestBus::BroadcastResult(
                targetSelected, &ToolsApplicationRequests::IsSelected, id);

            EXPECT_FALSE(targetSelected);
        }
        
        bool anyEntitySelected = false;
        ToolsApplicationRequestBus::BroadcastResult(
            anyEntitySelected, &ToolsApplicationRequests::AreAnyEntitiesSelected);

        int selectedEntitiesCount = 0;
        ToolsApplicationRequestBus::BroadcastResult(
            selectedEntitiesCount, &ToolsApplicationRequests::GetSelectedEntitiesCount);

        EntityIdList actualSelectedEntityIds;
        ToolsApplicationRequestBus::BroadcastResult(
            actualSelectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        EXPECT_TRUE(anyEntitySelected);
        EXPECT_EQ(selectedEntitiesCount, expectedSelectedEntityIds.size());
        EXPECT_EQ(actualSelectedEntityIds.size(), expectedSelectedEntityIds.size());
        for (auto& id : expectedSelectedEntityIds)
        {
            EXPECT_TRUE(AZStd::find(actualSelectedEntityIds.begin(), actualSelectedEntityIds.end(), id) != actualSelectedEntityIds.end());
        }

        // Re-select first half of entities so that all entities got selected again
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::MarkEntitiesSelected, deselctedEntityIds);

        expectedSelectedEntityIds = testEntityIds;
        ToolsApplicationRequestBus::BroadcastResult(
            anyEntitySelected, &ToolsApplicationRequests::AreAnyEntitiesSelected);

        ToolsApplicationRequestBus::BroadcastResult(
            selectedEntitiesCount, &ToolsApplicationRequests::GetSelectedEntitiesCount);

        ToolsApplicationRequestBus::BroadcastResult(
            actualSelectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        EXPECT_TRUE(anyEntitySelected);
        EXPECT_EQ(selectedEntitiesCount, expectedSelectedEntityIds.size());
        EXPECT_EQ(actualSelectedEntityIds.size(), expectedSelectedEntityIds.size());
        for (auto& id : expectedSelectedEntityIds)
        {
            EXPECT_TRUE(AZStd::find(actualSelectedEntityIds.begin(), actualSelectedEntityIds.end(), id) != actualSelectedEntityIds.end());

            ToolsApplicationRequestBus::BroadcastResult(
                targetSelected, &ToolsApplicationRequests::IsSelected, id);

            EXPECT_TRUE(targetSelected);
        }
    }
}
