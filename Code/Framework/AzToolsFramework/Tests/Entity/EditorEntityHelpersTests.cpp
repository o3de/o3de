/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    class EditorEntityHelpersTest
        : public ToolsApplicationFixture<>
    {
        void SetUpEditorFixtureImpl() override
        {
            m_parent1 = CreateDefaultEditorEntity("Parent1");
            m_child1 = CreateDefaultEditorEntity("Child1");
            m_child2 = CreateDefaultEditorEntity("Child2");
            m_grandChild1 = CreateDefaultEditorEntity("GrandChild1");
            m_parent2 = CreateDefaultEditorEntity("Parent2");

            AZ::TransformBus::Event(m_child1, &AZ::TransformBus::Events::SetParent, m_parent1);
            AZ::TransformBus::Event(m_child2, &AZ::TransformBus::Events::SetParent, m_parent1);
            AZ::TransformBus::Event(m_grandChild1, &AZ::TransformBus::Events::SetParent, m_child1);
        }

    public:
        AZ::EntityId m_parent1;
        AZ::EntityId m_child1;
        AZ::EntityId m_child2;
        AZ::EntityId m_grandChild1;
        AZ::EntityId m_parent2;
    };

    TEST_F(EditorEntityHelpersTest, EditorEntityHelpersTests_GetCulledEntityHierarchy)
    {
        AzToolsFramework::EntityIdList testEntityIds{ m_parent1, m_child1, m_child2, m_grandChild1, m_parent2 };

        AzToolsFramework::EntityIdSet culledSet = AzToolsFramework::GetCulledEntityHierarchy(testEntityIds);

        // There should only be two EntityIds returned (m_parent1, and m_parent2),
        // since all the others should be culled out since they have a common ancestor
        // in the list already
        using ::testing::UnorderedElementsAre;
        EXPECT_THAT(culledSet, UnorderedElementsAre(m_parent1, m_parent2));
    }
}
