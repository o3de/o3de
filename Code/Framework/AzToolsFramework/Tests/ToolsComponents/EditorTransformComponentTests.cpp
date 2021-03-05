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
        : public UnitTest::ToolsApplicationFixture
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
        const AZ::Vector3 parentScale(2.0f, 1.0f, 3.0f);
        AZ::TransformBus::Event(hierarchy.m_parentId, &AZ::TransformInterface::SetLocalScale, parentScale);

        // Set scale to child entity
        const AZ::Vector3 childScale(5.0f, 6.0f, 10.0f);
        AZ::TransformBus::Event(hierarchy.m_childId, &AZ::TransformInterface::SetLocalScale, childScale);

        const AZ::Vector3 expectedScale = childScale * parentScale;

        AZ::Vector3 childWorldScale = AZ::Vector3::CreateOne();
        AZ::TransformBus::EventResult(childWorldScale, hierarchy.m_childId, &AZ::TransformBus::Events::GetWorldScale);

        EXPECT_THAT(childWorldScale, UnitTest::IsClose(expectedScale));
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
