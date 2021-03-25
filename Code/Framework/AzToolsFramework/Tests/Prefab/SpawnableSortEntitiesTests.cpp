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

#include <Prefab/SpawnableSortEntitiesTestFixture.h>

namespace UnitTest
{
    using SpawnableSortEntitiesTests = SpawnableSortEntitiesTestFixture;


    TEST_F(SpawnableSortEntitiesTests, SpawnableSortEntities_0Entities_IsOk)
    {
        SortAndSanityCheck();
    }

    TEST_F(SpawnableSortEntitiesTests, SpawnableSortEntities_1Entity_IsOk)
    {
        AddEntity(E1);

        SortAndSanityCheck();
    }

    TEST_F(SpawnableSortEntitiesTests, SpawnableSortEntities_ParentAndChild_SortsCorrectly)
    {
        AddEntity(E2, E1);
        AddEntity(E1);

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    TEST_F(SpawnableSortEntitiesTests, SpawnableSortEntities_6EntitiesWith2Roots_SortsCorrectly)
    {
        // Hierarchy looks like:
        // 1
        // + 2
        //   + 3
        // 4
        // + 5
        // + 6
        // The entities are added in "random-ish" order on purpose
        AddEntity(E3, E2);
        AddEntity(E1);
        AddEntity(E6, E4);
        AddEntity(E5, E4);
        AddEntity(E2, E1);
        AddEntity(E4);

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
        EXPECT_TRUE(IsChildAfterParent(E3, E2));
        EXPECT_TRUE(IsChildAfterParent(E5, E4));
        EXPECT_TRUE(IsChildAfterParent(E6, E4));
    }

    TEST_F(SpawnableSortEntitiesTests, SpawnableSortEntities_ParentNotFound_ChildTreatedAsRoot)
    {
        AddEntity(E1);
        AddEntity(E2, E1);
        AddEntity(E3, MissingNo); // E3's parent not found
        AddEntity(E4, E3);

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
        EXPECT_TRUE(IsChildAfterParent(E4, E2));
    }

    TEST_F(SpawnableSortEntitiesTests, SpawnableSortEntities_NullptrEntry_RemovedFromSorted)
    {
        AddEntity(E2, E1);
        AddEntity(nullptr, false);
        AddEntity(E1);

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    TEST_F(SpawnableSortEntitiesTests, SpawnableSortEntities_DuplicateEntityId_RemovedFromSorted)
    {
        AddEntity(E2, E1);
        AddEntity(E1);
        AddEntity(E1, AZ::EntityId(), false); // duplicate EntityId

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    TEST_F(SpawnableSortEntitiesTests, SpawnableSortEntities_LoopingHierarchy_PicksAnyParentAsRoot)
    {
        // loop: E1 -> E2 -> E3 -> E1 -> ...
        AddEntity(E2, E1);
        AddEntity(E3, E2);
        AddEntity(E1, E3);

        SortAndSanityCheck();

        AZ::EntityId first = m_sorted.front()->GetId();

        if (first == E1)
        {
            EXPECT_TRUE(IsChildAfterParent(E2, E1));
            EXPECT_TRUE(IsChildAfterParent(E3, E2));
        }
        else if (first == E2)
        {
            EXPECT_TRUE(IsChildAfterParent(E3, E2));
            EXPECT_TRUE(IsChildAfterParent(E1, E3));
        }
        else // if (first == E3)
        {
            EXPECT_TRUE(IsChildAfterParent(E1, E3));
            EXPECT_TRUE(IsChildAfterParent(E2, E1));
        }
    }

    TEST_F(SpawnableSortEntitiesTests, SpawnableSortEntities_EntityLackingTransformComponent_IsTreatedLikeItHasNoParent)
    {
        AddEntity(E2, E1);
        AddEntity(aznew AZ::Entity(E1));

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    TEST_F(SpawnableSortEntitiesTests, SpawnableSortEntities_EntityParentedToSelf_IsTreatedLikeItHasNoParent)
    {
        AddEntity(E2, E1);
        AddEntity(E1, E1); // parented to self

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    TEST_F(SpawnableSortEntitiesTests, SpawnableSortEntities_EntityWithInvalidId_RemovedFromSorted)
    {
        AddEntity(E2, E1);
        AddEntity(E1);
        AddEntity(AZ::EntityId(), AZ::EntityId(), false); // entity using invalid ID as its own ID

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }
}
