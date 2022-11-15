/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace AzToolsFramework
{
    struct TransformTestEntityHierarchy
    {
        AZ::EntityId m_parentId;
        AZ::EntityId m_childId;
        AZ::EntityId m_grandchild1Id;
        AZ::EntityId m_grandchild2Id;
    };

    class EditorTransformComponentTest
        : public UnitTest::ToolsApplicationFixture<>
    {
    public:
        static TransformTestEntityHierarchy BuildTestHierarchy()
        {
            TransformTestEntityHierarchy result;

            result.m_parentId = UnitTest::CreateDefaultEditorEntity("Parent");
            result.m_childId = UnitTest::CreateDefaultEditorEntity("Child");
            result.m_grandchild1Id = UnitTest::CreateDefaultEditorEntity("Grandchild1");
            result.m_grandchild2Id = UnitTest::CreateDefaultEditorEntity("Grandchild2");

            // Set parent-child relationships
            AZ::TransformBus::Event(result.m_childId, &AZ::TransformBus::Events::SetParent, result.m_parentId);
            AZ::TransformBus::Event(result.m_grandchild1Id, &AZ::TransformBus::Events::SetParent, result.m_childId);
            AZ::TransformBus::Event(result.m_grandchild2Id, &AZ::TransformBus::Events::SetParent, result.m_childId);

            return result;
        }
    };

    TEST_F(EditorTransformComponentTest, TransformTests_EntityHasParent_WorldScaleInheritsParentScale)
    {
        TransformTestEntityHierarchy hierarchy = BuildTestHierarchy();

        // Set scale to parent entity
        const float parentScale = 2.0f;
        AZ::TransformBus::Event(hierarchy.m_parentId, &AZ::TransformInterface::SetLocalUniformScale, parentScale);

        // Set scale to child entity
        const float childScale = 5.0f;
        AZ::TransformBus::Event(hierarchy.m_childId, &AZ::TransformInterface::SetLocalUniformScale, childScale);

        const float expectedScale = childScale * parentScale;

        float childWorldScale = 1.0f;
        AZ::TransformBus::EventResult(childWorldScale, hierarchy.m_childId, &AZ::TransformBus::Events::GetWorldUniformScale);

        EXPECT_NEAR(childWorldScale, expectedScale, AZ::Constants::Tolerance);
    }

    TEST_F(EditorTransformComponentTest, TransformTests_GetChildren_DirectChildrenMatchHierarchy)
    {
        TransformTestEntityHierarchy hierarchy = BuildTestHierarchy();

        EntityIdList children;
        AZ::TransformBus::EventResult(children, hierarchy.m_parentId, &AZ::TransformBus::Events::GetChildren);

        EXPECT_EQ(children.size(), 1);
        EXPECT_EQ(children[0], hierarchy.m_childId);
    }

    TEST_F(EditorTransformComponentTest, TransformTests_GetAllDescendants_AllDescendantsMatchHierarchy)
    {
        TransformTestEntityHierarchy hierarchy = BuildTestHierarchy();

        EntityIdList descendants;
        AZ::TransformBus::EventResult(descendants, hierarchy.m_parentId, &AZ::TransformBus::Events::GetAllDescendants);

        // Order of descendants here and in other test cases depends on TransformHierarchyInformationBus
        // Sorting it to get predictable order and be able to verify by index
        std::sort(descendants.begin(), descendants.end());

        EXPECT_EQ(descendants.size(), 3);
        EXPECT_EQ(descendants[0], hierarchy.m_childId);
        EXPECT_EQ(descendants[1], hierarchy.m_grandchild1Id);
        EXPECT_EQ(descendants[2], hierarchy.m_grandchild2Id);
    }

    TEST_F(EditorTransformComponentTest, TransformTests_GetEntityAndAllDescendants_AllDescendantsMatchHierarchyAndResultIncludesParentEntity)
    {
        TransformTestEntityHierarchy hierarchy = BuildTestHierarchy();

        EntityIdList entityAndDescendants;
        AZ::TransformBus::EventResult(entityAndDescendants, hierarchy.m_parentId, &AZ::TransformBus::Events::GetEntityAndAllDescendants);

        std::sort(entityAndDescendants.begin(), entityAndDescendants.end());

        EXPECT_EQ(entityAndDescendants.size(), 4);
        EXPECT_EQ(entityAndDescendants[0], hierarchy.m_parentId);
        EXPECT_EQ(entityAndDescendants[1], hierarchy.m_childId);
        EXPECT_EQ(entityAndDescendants[2], hierarchy.m_grandchild1Id);
        EXPECT_EQ(entityAndDescendants[3], hierarchy.m_grandchild2Id);
    }
} // namespace AzToolsFramework
